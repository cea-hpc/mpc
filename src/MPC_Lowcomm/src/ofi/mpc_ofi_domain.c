#include "mpc_ofi_domain.h"
#include "mpc_common_spinlock.h"
#include "mpc_ofi_dns.h"
#include "mpc_ofi_helpers.h"

#include <mpc_common_debug.h>

#include "mpc_ofi_request.h"
#include "rdma/fabric.h"
#include "rdma/fi_cm.h"
#include "rdma/fi_domain.h"
#include "rdma/fi_eq.h"
#include "rdma/fi_errno.h"
#include <rdma/fi_endpoint.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int mpc_ofi_domain_buffer_init(struct mpc_ofi_domain_buffer_t * buff, struct mpc_ofi_domain_t * domain)
{
   buff->domain = domain;
   buff->size = MPC_OFI_DOMAIN_EAGER_PER_BUFF * MPC_OFI_DOMAIN_EAGER_SIZE;
   buff->buffer = malloc(buff->size);

   if(!buff->buffer)
   {
      perror("malloc");
      return 1;
   }

   /* Make sure to generate all page faults */
   memset(buff->buffer, 0, buff->size);

   mpc_common_spinlock_init(&buff->lock, 0);

   buff->pending_operations = 0;
   buff->is_posted = 0;

   MPC_OFI_CHECK_RET(fi_mr_reg(buff->domain->domain,
                              buff->buffer,
                              buff->size,
                              FI_RECV, 0,
                              0,
                              0,
                              &buff->mr,
                              NULL));

   return 0;
}

void mpc_ofi_domain_buffer_acquire(struct mpc_ofi_domain_buffer_t * buff)
{
   assert(buff);
   mpc_common_spinlock_lock(&buff->lock);
   buff->pending_operations++;
   mpc_common_spinlock_unlock(&buff->lock);

}

int mpc_ofi_domain_buffer_relax(struct mpc_ofi_domain_buffer_t * buff)
{
   int ret = 0;
   mpc_common_spinlock_lock(&buff->lock);

   assert(buff->pending_operations > 0);
   buff->pending_operations--;

   if(!buff->pending_operations && !buff->is_posted)
   {
      ret = mpc_ofi_domain_buffer_post(buff);
   }

   mpc_common_spinlock_unlock(&buff->lock);

   return ret;
}

int mpc_ofi_domain_buffer_post(struct mpc_ofi_domain_buffer_t * buff)
{
   assert(!buff->is_posted);

   struct fi_msg msg;
   struct iovec msg_iov;

   msg_iov.iov_base = buff->buffer;
   msg_iov.iov_len = buff->size;

   msg.msg_iov = &msg_iov;
   msg.desc = fi_mr_desc(buff->mr);
   msg.iov_count = 1;
   msg.addr = 0;
   msg.data = 0;
   msg.context = buff;

   buff->is_posted = 1;

   MPC_OFI_CHECK_RET(fi_recvmsg(buff->domain->ep, &msg, FI_MULTI_RECV));

   return 0;
}


void mpc_ofi_domain_buffer_set_unposted(struct mpc_ofi_domain_buffer_t * buff)
{
   assert(buff);
   mpc_common_spinlock_lock(&buff->lock);

   buff->is_posted = 0;

   mpc_common_spinlock_unlock(&buff->lock);
}


int mpc_ofi_domain_buffer_release(struct mpc_ofi_domain_buffer_t * buff)
{
   MPC_OFI_CHECK_RET(fi_close(&buff->mr->fid));

   free(buff->buffer);

   return 0;
}


int mpc_ofi_domain_buffer_manager_init(struct mpc_ofi_domain_buffer_manager_t * buffs, struct mpc_ofi_domain_t * domain)
{
   buffs->domain = domain;

   buffs->pending_repost_count = MPC_OFI_NUM_MULTIRECV_BUFF;

   mpc_common_spinlock_init(&buffs->pending_repost_count_lock, 0);

   int i = 0;

   for(i = 0 ; i < MPC_OFI_NUM_MULTIRECV_BUFF; i++)
   {
      if(mpc_ofi_domain_buffer_init(&buffs->rx_buffers[i], domain))
      {
         mpc_common_errorpoint("Error initializing a RX buff");
         return 1;
      }

      if(mpc_ofi_domain_buffer_post(&buffs->rx_buffers[i]))
      {
         mpc_common_errorpoint("Error posting recv buffer");
         return 1;
      }
   }


   return 0;
}


int mpc_ofi_domain_buffer_manager_release(struct mpc_ofi_domain_buffer_manager_t * buffs)
{
   int i = 0;

   for(i = 0 ; i < MPC_OFI_NUM_MULTIRECV_BUFF; i++)
   {
      if(mpc_ofi_domain_buffer_release(&buffs->rx_buffers[i]))
      {
         mpc_common_errorpoint("Error releasing a RX buff");
         return 1;
      }
   }

   return 0;
}


/**************
 * THE DOMAIN *
 **************/


int mpc_ofi_domain_init(struct mpc_ofi_domain_t * domain, struct mpc_ofi_context_t *ctx)
{
   mpc_common_spinlock_init(&domain->lock, 0);
   domain->being_polled = 0;
   domain->ctx = ctx;

   /* Initialize request cache */
   if(mpc_ofi_request_cache_init(&domain->rcache))
   {
      mpc_common_errorpoint("Failed to initialize request cache");
      return -1;
   }

   /* Initialize domain */
   MPC_OFI_CHECK_RET(fi_domain(ctx->fabric, ctx->config, &domain->domain, NULL));


   /* Now create per domain eqs */
   struct fi_eq_attr eq_attrs = {0};

   eq_attrs.wait_obj = FI_WAIT_UNSPEC;

   MPC_OFI_CHECK_RET(fi_eq_open(ctx->fabric, &eq_attrs, &domain->eq, domain));

   /* Now create two CQs per domain */
   struct fi_cq_attr cq_attr = {0};

   cq_attr.format = FI_CQ_FORMAT_DATA;
   cq_attr.wait_obj = FI_WAIT_UNSPEC;

   MPC_OFI_CHECK_RET(fi_cq_open(domain->domain, &cq_attr, &domain->rx_cq, domain));
   MPC_OFI_CHECK_RET(fi_cq_open(domain->domain, &cq_attr, &domain->tx_cq, domain));


   /* And the local DNS */
   if( mpc_ofi_domain_dns_init(&domain->ddns,
                              &ctx->dns,
                              domain->domain) < 0)
   {
      mpc_common_errorpoint("Failed to start per domain DNS");
      return -1;
   }

   /* And the endpoint */
   MPC_OFI_CHECK_RET(fi_endpoint(domain->domain, ctx->config, &domain->ep, domain));

   /* Now bind all cqs , av and eqs */
   MPC_OFI_CHECK_RET(fi_ep_bind(domain->ep,
                    mpc_ofi_domain_dns_av(&domain->ddns),
                    0));
   MPC_OFI_CHECK_RET(fi_ep_bind(domain->ep, &domain->eq->fid, 0));
   MPC_OFI_CHECK_RET(fi_ep_bind(domain->ep, &domain->tx_cq->fid, FI_SEND));
   MPC_OFI_CHECK_RET(fi_ep_bind(domain->ep, &domain->tx_cq->fid, FI_RECV));

   /* Finally enable the EP */
   MPC_OFI_CHECK_RET(fi_enable(domain->ep));

   /* Retrieve the local address */
   domain->address_len = MPC_OFI_ADDRESS_LEN;
	MPC_OFI_CHECK_RET(fi_getname(&domain->ep->fid, domain->address, &domain->address_len) );


   /* Eventually Prepare the Buffers*/
   if(mpc_ofi_domain_buffer_manager_init(&domain->buffers, domain))
   {
      mpc_common_errorpoint("Failed to allocate buffers");
      return 1;
   }


   return 0;
}

int mpc_ofi_domain_release(struct mpc_ofi_domain_t * domain)
{
   domain->ctx = NULL;

   if(mpc_ofi_domain_buffer_manager_release(&domain->buffers))
   {
      mpc_common_errorpoint("Failed to release per domain buffers");
      return 1;
   }

   MPC_OFI_CHECK_RET(fi_close(&domain->ep->fid));

   if( mpc_ofi_domain_dns_release(&domain->ddns) < 0)
   {
      mpc_common_errorpoint("Failed to free domain local DNS");
      return -1;
   }

   MPC_OFI_CHECK_RET(fi_close(&domain->rx_cq->fid));
   MPC_OFI_CHECK_RET(fi_close(&domain->tx_cq->fid));
   MPC_OFI_CHECK_RET(fi_close(&domain->eq->fid));
   MPC_OFI_CHECK_RET(fi_close(&domain->domain->fid));


   /* release request cache */
   if(mpc_ofi_request_cache_release(&domain->rcache))
   {
      mpc_common_errorpoint("Failed to release request cache");
      return -1;
   }


   return 0;
}

static int _ofi_domain_per_req_recv_completion_callback(void * arg)
{
   struct mpc_ofi_domain_buffer_t * buff = (struct mpc_ofi_domain_buffer_t *)arg;
   return mpc_ofi_domain_buffer_relax(buff);
}

static struct mpc_ofi_request_t * _ofi_domain_poll_for_recv_request(struct mpc_ofi_domain_t * domain,
                                                                   struct mpc_ofi_domain_buffer_t * buff)
{
      struct mpc_ofi_request_t *req = mpc_ofi_request_acquire(&domain->rcache,
                                                            _ofi_domain_per_req_recv_completion_callback,
                                                            buff);

      while(!req)
      {
         req = mpc_ofi_request_acquire(&domain->rcache,
                                      _ofi_domain_per_req_recv_completion_callback,
                                      buff);
         if(req)
         {
            break;
         }

         if(mpc_ofi_domain_poll(domain, 0))
         {
            mpc_common_errorpoint("Error polling for requests");
            return NULL;
         }
      }

      return req;
}


static inline int _ofi_domain_cq_process_event(struct mpc_ofi_domain_t * domain, struct fi_cq_data_entry*evt)
{

   //mpc_ofi_decode_cq_flags(evt->flags);

   if(evt->flags & FI_MULTI_RECV)
   {
      struct mpc_ofi_domain_buffer_t * buff = (struct mpc_ofi_domain_buffer_t *)evt->op_context;

      /* The buffer is about to be exausted it is then not posted anymore */
      mpc_ofi_domain_buffer_set_unposted(buff);
   }

   if((evt->flags & FI_RECV) && (evt->flags & FI_MSG))
   {
      struct mpc_ofi_domain_buffer_t * buff = (struct mpc_ofi_domain_buffer_t *)evt->op_context;

      /* This is a RECV */
      size_t size = evt->len;
      void * addr = evt->buf;


      /* Flag the buffer as in flight
         it will be relaxed when the associated request will be completed */
      mpc_ofi_domain_buffer_acquire(buff);

      struct mpc_ofi_request_t * req = _ofi_domain_poll_for_recv_request(domain,  buff);

      if(!req)
      {
         mpc_common_errorpoint("Could not acquire request");
         return -1;
      }

      if((domain->ctx->recv_callback)(addr, size, req))
      {
         mpc_common_errorpoint("Non zero status from global recv handler");
         return -1;
      }

      return 0;
   }

   if( (evt->flags & FI_SEND) && (evt->flags & FI_MSG))
   {
      struct mpc_ofi_request_t *request = (struct mpc_ofi_request_t *)evt->op_context;
      /* This is a Send */
      mpc_ofi_request_done(request);

      return 0;
   }

   return 0;
}


static inline int _ofi_domain_cq_poll(struct mpc_ofi_domain_t * domain, struct fid_cq * cq)
{
   ssize_t ret = 0;
	struct fi_cq_data_entry comp[MPC_OFI_DOMAIN_NUM_CQ_REQ_TO_POLL];


   ret = fi_cq_read(cq, &comp, MPC_OFI_DOMAIN_NUM_CQ_REQ_TO_POLL);

   if (ret == -FI_EAGAIN)
   {
        return 0;
   }

   MPC_OFI_CHECK_RET(ret);


   int i = 0;

   for( i = 0 ; i < ret; i++)
   {
      if(_ofi_domain_cq_process_event(domain, &comp[i]))
      {
         mpc_common_errorpoint("Error processing a CQ event");
         return 1;
      }
   }
#if 0
   if (comp.flags & FI_RECV) {
      if (comp.len != opts.transfer_size) {
         FT_ERR("completion length %lu, expected %lu",
            comp.len, opts.transfer_size);
         return -FI_EIO;
      }

      if (comp.buf < (void *) rx_buf ||
            comp.buf >=  (void *) (rx_buf + rx_size) ||
            comp.buf == last_rx_buf) {
         FT_ERR("returned completion buffer %p out of range",
            comp.buf);
         return -FI_EIO;
      }

      if (ft_check_opts(FT_OPT_VERIFY_DATA | FT_OPT_ACTIVE) &&
            ft_check_buf(comp.buf, opts.transfer_size))
         return -FI_EIO;

      per_buf_cnt++;
      num_completions--;
      last_rx_buf = comp.buf;
   }

   if (comp.flags & FI_MULTI_RECV) {
      if (per_buf_cnt != comp_per_buf) {
         FT_ERR("Received %d completions per buffer, expected %d",
            per_buf_cnt, comp_per_buf);
         return -FI_EIO;
      }
      per_buf_cnt = 0;
      i = comp.op_context == &ctx_multi_recv[1];

      ret = repost_recv(i);
      if (ret)
         return ret;
   }
#endif
	return 0;
}


static inline int _mpc_ofi_domain_eq_poll(struct mpc_ofi_domain_t * domain)
{
	struct fi_eq_entry entry;

	uint32_t event = 0;
	ssize_t rd = 0;

   rd = fi_eq_read(domain->eq, &event, &entry, sizeof(entry), 0);

	if(rd == -FI_EAVAIL)
	{
		struct fi_eq_err_entry err;
		MPC_OFI_CHECK_RET(fi_eq_readerr(domain->eq, &err, 0));
		char err_buff[1024];
		err_buff[0] = '\0';
		fi_eq_strerror(domain->eq, err.prov_errno, err.err_data, err_buff, 1024);
		printf("%d : Got eq error on %p %s == %s\n", err.err,  err.fid,
				fi_strerror(err.err),
				err_buff);
		return 1;
	}

   if(rd == -FI_EAGAIN)
   {
      /* No events */
      return 0;
   }

   (void)fprintf(stderr, "EQ event %s\n", fi_tostr(&event, FI_TYPE_EQ_EVENT));

   return 0;
}



int mpc_ofi_domain_poll(struct mpc_ofi_domain_t * domain, int type)
{
   if(domain->being_polled)
   {
      return 0;
   }

   domain->being_polled = 1;

   if( _mpc_ofi_domain_eq_poll(domain) )
   {
      domain->being_polled = 0;
      mpc_common_errorpoint("CQ reported an error");
      return 1;
   }

   if( (!type) | (type & FI_RECV) )
   {

      if( _ofi_domain_cq_poll(domain, domain->rx_cq))
      {
         domain->being_polled = 0;
         mpc_common_errorpoint("RECV cq reported an error");
         return 1;
      }
   }

   if( (!type) | (type & FI_SEND) )
   {

      if( _ofi_domain_cq_poll(domain, domain->tx_cq))
      {
         domain->being_polled = 0;
         mpc_common_errorpoint("TX cq reported an error");
         return 1;
      }
   }

   domain->being_polled = 0;

   return 0;
}

int mpc_ofi_domain_memory_register(struct mpc_ofi_domain_t * domain,
                                  void *buff,
                                  size_t size,
                                  uint64_t acs,
                                  struct fid_mr **mr)
{
   MPC_OFI_CHECK_RET(fi_mr_reg(domain->domain,
                              buff,
                              size,
                              acs, 0,
                              0xC0DE,
                              0,
                              mr,
                              NULL));
   return 0;
}

int mpc_ofi_domain_memory_unregister(struct fid_mr *mr)
{
   MPC_OFI_CHECK_RET(fi_close(&mr->fid));
   return 0;
}



int _mpc_ofi_domain_request_complete(void *pmr)
{
   struct fid_mr *mr = (struct fid_mr *)pmr;

   if( mpc_ofi_domain_memory_unregister(mr) )
   {
      mpc_common_errorpoint("Failed to unregister send buffer");
      return -1;
   }

   return 0;
}



int mpc_ofi_domain_send(struct mpc_ofi_domain_t * domain,
                       uint64_t dest,
                       void *buff,
                       size_t size,
                       struct mpc_ofi_request_t **req)
{
   struct fid_mr *mr = NULL;

   if( mpc_ofi_domain_memory_register(domain, buff, size, FI_SEND, &mr) )
   {
      mpc_common_errorpoint("Failed to register send buffer");
      return -1;
   }

   fi_addr_t addr = 0;

   if(mpc_ofi_domain_dns_resolve(&domain->ddns, dest, &addr))
   {
      mpc_common_errorpoint("Failed to resolve address");
      return -1;
   }

   *req = mpc_ofi_request_acquire(&domain->rcache, _mpc_ofi_domain_request_complete, (void*)mr);

   while(1)
   {
      ssize_t ret = fi_send(domain->ep, buff, size, fi_mr_desc(mr), addr, (void *)*req);

      if(ret == -FI_EAGAIN)
      {
         continue;
      }

      MPC_OFI_CHECK_RET(ret);
      break;
   }

   return 0;
}