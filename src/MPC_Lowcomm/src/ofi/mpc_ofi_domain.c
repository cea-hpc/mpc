#include "mpc_ofi_domain.h"
#include "mpc_common_spinlock.h"
#include "mpc_ofi_dns.h"
#include "mpc_ofi_helpers.h"

#include <mpc_common_debug.h>
#include <sctk_alloc.h>

#include "mpc_ofi_request.h"
#include "rdma/fabric.h"
#include "rdma/fi_cm.h"
#include "rdma/fi_domain.h"
#include "rdma/fi_eq.h"
#include "rdma/fi_errno.h"
#include <rdma/fi_endpoint.h>
#include <rdma/fi_rma.h>

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int mpc_ofi_domain_buffer_init(struct mpc_ofi_domain_buffer_t * buff, struct mpc_ofi_domain_t * domain, unsigned int eager_size, unsigned int eager_per_buff, short has_multi_recv)
{
   buff->domain = domain;
   buff->size = (size_t)eager_per_buff * eager_size;
   buff->has_multi_recv = has_multi_recv;
   buff->aligned_mem = mpc_ofi_alloc_aligned(buff->size);

   buff->buffer = buff->aligned_mem.ret;

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
                              FI_RECV,
                              0,
                              0,
                              0,
                              &buff->mr,
                              NULL));

   return 0;
}

void mpc_ofi_domain_buffer_acquire(struct mpc_ofi_domain_buffer_t * buff)
{
   assert(buff);
   if(buff->has_multi_recv)
   {
      mpc_common_spinlock_lock(&buff->lock);
      buff->pending_operations++;
      mpc_common_spinlock_unlock(&buff->lock);
   }
}

int mpc_ofi_domain_buffer_relax(struct mpc_ofi_domain_buffer_t * buff)
{
   int ret = 0;

   if(buff->has_multi_recv)
   {
      mpc_common_spinlock_lock(&buff->lock);

      assert(buff->pending_operations > 0);
      buff->pending_operations--;

      if(!buff->pending_operations && !buff->is_posted)
      {
         ret = mpc_ofi_domain_buffer_post(buff);
      }

      mpc_common_spinlock_unlock(&buff->lock);
   }
   else
   {
      buff->is_posted = 0;
      ret = mpc_ofi_domain_buffer_post(buff);
   }

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

   MPC_OFI_CHECK_RET(fi_recvmsg(buff->domain->ep, &msg, buff->has_multi_recv?FI_MULTI_RECV:0));

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

   mpc_ofi_free_aligned(&buff->aligned_mem);

   return 0;
}


int mpc_ofi_domain_buffer_manager_init(struct mpc_ofi_domain_buffer_manager_t * buffs, struct mpc_ofi_domain_t * domain,
                                        unsigned int eager_size, unsigned int eager_per_buff, unsigned int num_recv_buff,
                                        int has_multi_recv)
{
   buffs->domain = domain;

   buffs->eager_size= eager_size;

   if(has_multi_recv)
   {
      buffs->buffer_count = num_recv_buff;
      buffs->eager_per_buff = eager_per_buff;
   }
   else
   {
      buffs->eager_per_buff = 1;
      buffs->buffer_count = eager_per_buff;
   }

   buffs->rx_buffers = sctk_malloc(sizeof(struct mpc_ofi_domain_buffer_t) * buffs->buffer_count);
   assume(buffs->rx_buffers);

   buffs->pending_repost_count = buffs->buffer_count;

   mpc_common_spinlock_init(&buffs->pending_repost_count_lock, 0);

   unsigned i = 0;

   for(i = 0 ; i < buffs->buffer_count ; i++)
   {
      if(mpc_ofi_domain_buffer_init(&buffs->rx_buffers[i], domain, buffs->eager_size, buffs->eager_per_buff, has_multi_recv))
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
   unsigned int i = 0;

   for(i = 0 ; i < buffs->buffer_count ; i++)
   {
      if(mpc_ofi_domain_buffer_release(&buffs->rx_buffers[i]))
      {
         mpc_common_errorpoint("Error releasing a RX buff");
         return 1;
      }
   }

   sctk_free(buffs->rx_buffers);

   return 0;
}


/**************
 * THE DOMAIN *
 **************/


int mpc_ofi_domain_init(struct mpc_ofi_domain_t * domain, struct mpc_ofi_context_t *ctx,    struct _mpc_lowcomm_config_struct_net_driver_ofi * config)
{
   mpc_common_spinlock_init(&domain->lock, 0);
   domain->being_polled = 0;
   domain->ctx = ctx;
   domain->config = config;

   /* Initialize request cache */
   if(mpc_ofi_request_cache_init(&domain->rcache, domain))
   {
      mpc_common_errorpoint("Failed to initialize request cache");
      return -1;
   }

   /* Initialize domain */
   MPC_OFI_CHECK_RET(fi_domain(ctx->fabric, ctx->config, &domain->domain, NULL));


   /* Now create per domain eqs */
   struct fi_eq_attr eq_attrs = {0};

   eq_attrs.wait_obj = FI_WAIT_UNSPEC;

#ifdef EQ_ENABLED
   MPC_OFI_CHECK_RET(fi_eq_open(ctx->fabric, &eq_attrs, &domain->eq, domain));
#endif

   /* Now create two CQs per domain */
   struct fi_cq_attr cq_attr = {0};

   cq_attr.format = FI_CQ_FORMAT_DATA;
   cq_attr.wait_obj = FI_WAIT_NONE;
   cq_attr.wait_cond = FI_CQ_COND_NONE;

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
#ifdef EQ_ENABLED
   MPC_OFI_CHECK_RET(fi_ep_bind(domain->ep, &domain->eq->fid, 0));
#endif
   MPC_OFI_CHECK_RET(fi_ep_bind(domain->ep, &domain->tx_cq->fid, FI_SEND));
   MPC_OFI_CHECK_RET(fi_ep_bind(domain->ep, &domain->rx_cq->fid, FI_RECV));

   int has_multi_recv = !!(ctx->config->caps & FI_MULTI_RECV) && config->enable_multi_recv;

   if(has_multi_recv)
   {
      size_t max_eager_size = config->eager_size;
      MPC_OFI_CHECK_RET(fi_setopt(&domain->ep->fid, FI_OPT_ENDPOINT, FI_OPT_MIN_MULTI_RECV, &max_eager_size, sizeof(size_t)));
   }

   /* Finally enable the EP */
   MPC_OFI_CHECK_RET(fi_enable(domain->ep));

   /* Retrieve the local address */
   domain->address_len = MPC_OFI_ADDRESS_LEN;
	MPC_OFI_CHECK_RET(fi_getname(&domain->ep->fid, domain->address, &domain->address_len) );


   /* Eventually Prepare the Buffers*/
   if(mpc_ofi_domain_buffer_manager_init(&domain->buffers,
                                         domain,
                                         config->eager_size,
                                         config->eager_per_buff,
                                         config->number_of_multi_recv_buff,
                                         has_multi_recv))
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
#ifdef EQ_ENABLED
   MPC_OFI_CHECK_RET(fi_close(&domain->eq->fid));
#endif

   TODO("Understand why we get -EBUSY");
   fi_close(&domain->domain->fid);


   /* release request cache */
   if(mpc_ofi_request_cache_release(&domain->rcache))
   {
      mpc_common_errorpoint("Failed to release request cache");
      return -1;
   }


   return 0;
}

static int _ofi_domain_per_req_recv_completion_callback(struct mpc_ofi_request_t *req, void * arg)
{
   struct mpc_ofi_domain_buffer_t * buff = (struct mpc_ofi_domain_buffer_t *)arg;
   return mpc_ofi_domain_buffer_relax(buff);
}

static struct mpc_ofi_request_t * _ofi_domain_poll_for_recv_request(struct mpc_ofi_domain_t * domain,
                                                                   struct mpc_ofi_domain_buffer_t * buff)
{
      struct mpc_ofi_request_t *req = mpc_ofi_request_acquire(&domain->rcache,
                                                            _ofi_domain_per_req_recv_completion_callback,
                                                            buff,
                                                            NULL,
                                                            NULL);

      while(!req)
      {
         req = mpc_ofi_request_acquire(&domain->rcache,
                                      _ofi_domain_per_req_recv_completion_callback,
                                      buff,
                                      NULL,
                                      NULL);
         if(req)
         {
            break;
         }

#if 0
         if(mpc_ofi_domain_poll(domain, 0))
         {
            mpc_common_errorpoint("Error polling for requests");
            return NULL;
         }
#endif
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

      if((domain->ctx->recv_callback)(addr, size, req, domain->ctx->callback_arg))
      {
         mpc_common_errorpoint("Non zero status from global recv handler");
         return -1;
      }

      return 0;
   }

   if(evt->flags & FI_RMA)
   {
      struct mpc_ofi_request_t *request = (struct mpc_ofi_request_t *)evt->op_context;
      /* This is a Send */
      mpc_ofi_request_done(request);

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
   int return_code = 0;

   ssize_t rd = 0;
	struct fi_cq_data_entry comp[MPC_OFI_DOMAIN_NUM_CQ_REQ_TO_POLL];

   if( mpc_common_spinlock_trylock(&domain->lock))
   {
      return 0;
   }

   rd = fi_cq_read(cq, &comp, MPC_OFI_DOMAIN_NUM_CQ_REQ_TO_POLL);

   if (rd == -FI_EAGAIN)
   {
      goto unlock_cq_poll;
   }

   if(rd < 0)
   {
      MPC_OFI_DUMP_ERROR(rd);
      return_code = (int)rd;
      goto unlock_cq_poll;
   }

   int i = 0;

   {
      /* We process events unlocked */
      mpc_common_spinlock_unlock(&domain->lock);


      for( i = 0 ; i < rd; i++)
      {
         if(_ofi_domain_cq_process_event(domain, &comp[i]))
         {
            mpc_common_errorpoint("Error processing a CQ event");
            return -1;
         }
      }

      return 0;
   }

unlock_cq_poll:
   mpc_common_spinlock_unlock(&domain->lock);
	return return_code;
}

#ifdef EQ_ENABLED

static inline int _mpc_ofi_domain_eq_poll(struct mpc_ofi_domain_t * domain)
{
   int return_code = 0;

	struct fi_eq_entry entry;

	uint32_t event = 0;
	ssize_t rd = 0;

   if( mpc_common_spinlock_trylock(&domain->lock) )
   {
      return 0;
   }


   rd = fi_eq_read(domain->eq, &event, &entry, sizeof(entry), 0);

	if(rd == -FI_EAVAIL)
	{
		struct fi_eq_err_entry err;
      long ret = fi_eq_readerr(domain->eq, &err, 0);
      if(ret < 0)
      {
		   MPC_OFI_DUMP_ERROR(ret);
         return_code = (int)ret;
         goto unlock_eq_poll;
      }
		char err_buff[1024];
		err_buff[0] = '\0';
		fi_eq_strerror(domain->eq, err.prov_errno, err.err_data, err_buff, 1024);
		mpc_common_errorpoint_fmt("%d : Got eq error on %p %s == %s\n", err.err,  err.fid,
				fi_strerror(err.err),
				err_buff);
      return_code = -1;
		goto unlock_eq_poll;
	}

   if(rd == -FI_EAGAIN)
   {
      /* No events */
      goto unlock_eq_poll;
   }

   (void)fprintf(stderr, "EQ event %s\n", fi_tostr(&event, FI_TYPE_EQ_EVENT));

unlock_eq_poll:
   mpc_common_spinlock_unlock(&domain->lock);
   return return_code;
}
#endif


int mpc_ofi_domain_poll(struct mpc_ofi_domain_t * domain, int type)
{
   if(domain->being_polled)
   {
      return 0;
   }

   domain->being_polled = 1;

#ifdef EQ_ENABLED
   if( _mpc_ofi_domain_eq_poll(domain) )
   {
      domain->being_polled = 0;
      mpc_common_errorpoint("CQ reported an error");
      return 1;
   }
#endif

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

int mpc_ofi_domain_memory_register_no_lock(struct mpc_ofi_domain_t * domain,
                                           void *buff,
                                           size_t size,
                                           uint64_t acs,
                                           struct fid_mr **mr)
{
   int ret = fi_mr_reg(domain->domain,
                              buff,
                              size,
                              acs, 0,
                              1337,
                              0,
                              mr,
                              NULL);

   MPC_OFI_CHECK_RET(ret);

   return ret;

}


int mpc_ofi_domain_memory_register(struct mpc_ofi_domain_t * domain,
                                  void *buff,
                                  size_t size,
                                  uint64_t acs,
                                  struct fid_mr **mr)
{

   mpc_common_spinlock_lock(&domain->lock);

   int ret = mpc_ofi_domain_memory_register_no_lock(domain, buff, size, acs, mr);

   mpc_common_spinlock_unlock(&domain->lock);

   return ret;
}

int mpc_ofi_domain_memory_unregister_no_lock(struct fid_mr *mr)
{
   MPC_OFI_CHECK_RET(fi_close(&mr->fid));

   return 0;
}

int mpc_ofi_domain_memory_unregister(struct mpc_ofi_domain_t * domain, struct fid_mr *mr)
{
   mpc_common_spinlock_lock(&domain->lock);

   int ret = mpc_ofi_domain_memory_unregister_no_lock(mr);

   mpc_common_spinlock_unlock(&domain->lock);

   return ret;
}


static inline int __free_mr(struct mpc_ofi_domain_t * domain, struct fid_mr **mr)
{
   if(!*mr)
   {
      return 0;
   }

   if( mpc_ofi_domain_memory_unregister(domain, *mr) )
   {
      mpc_common_errorpoint("Failed to unregister send buffer");
      return -1;
   }

   *mr = NULL;

   return 0;
}


int _mpc_ofi_domain_request_complete(struct mpc_ofi_request_t * req, void *dummy)
{
   unsigned int i = 0;

   for( i = 0 ; i < req->mr_count; i++)
   {
      if( __free_mr(req->domain, &req->mr[i]) )
         return -1;
   }

   return 0;
}


static struct mpc_ofi_request_t * __acquire_request(struct mpc_ofi_domain_t * domain,
                                                    int (*comptetion_cb)(struct mpc_ofi_request_t *, void *),
                                                    void *arg,
                                                    int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                                                    void *arg_ext)
{
   struct mpc_ofi_request_t * ret = NULL;

   do
   {
      ret = mpc_ofi_request_acquire(&domain->rcache, comptetion_cb, arg, comptetion_cb_ext, arg_ext );

      if(ret)
         break;

#if 0
      if(mpc_ofi_domain_poll(domain, 0))
      {
         mpc_common_errorpoint("Error raised when polling for request");
      }
#endif

   }while(!ret);

   return ret;
}


int mpc_ofi_domain_send(struct mpc_ofi_domain_t * domain,
                       uint64_t dest,
                       void *buff,
                       size_t size,
                       struct mpc_ofi_request_t **preq,
                       int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                       void *arg_ext)
{
   struct mpc_ofi_request_t ** req = preq;
   struct mpc_ofi_request_t * __req = NULL;
   int retcode = 0;

   if(!req)
   {
      req = &__req;
   }

   mpc_common_spinlock_lock(&domain->lock);

   *req = __acquire_request(domain, _mpc_ofi_domain_request_complete, NULL, comptetion_cb_ext, arg_ext);

   if( mpc_ofi_domain_memory_register_no_lock(domain, buff, size, FI_SEND, &(*req)->mr[0]) )
   {
      mpc_common_errorpoint("Failed to register send buffer");
      retcode = -1;
      goto send_unlock;
   }

   (*req)->mr_count = 1;

   fi_addr_t addr = 0;

   if(mpc_ofi_domain_dns_resolve(&domain->ddns, dest, &addr))
   {
      mpc_common_errorpoint("Failed to resolve address");
      retcode = -1;
      goto send_unlock;
   }

   while(1)
   {
      ssize_t ret = fi_send(domain->ep, buff, size, fi_mr_desc((*req)->mr[0]), addr, (void *)*req);

      if(ret == -FI_EAGAIN)
      {
         continue;
      }

      if(ret < 0)
      {
         MPC_OFI_DUMP_ERROR(ret);
         retcode = (int)ret;
         goto send_unlock;
      }

      break;
   }

send_unlock:
   mpc_common_spinlock_unlock(&domain->lock);

   return retcode;
}

int mpc_ofi_domain_sendv(struct mpc_ofi_domain_t * domain,
                        uint64_t dest,
                        const struct iovec *iov,
                        size_t iovcnt,
                        struct mpc_ofi_request_t **preq,
                        int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                        void *arg_ext)
{
   struct mpc_ofi_request_t ** req = preq;
   struct mpc_ofi_request_t * __req = NULL;
   int retcode = 0;

   if(!req)
   {
      req = &__req;
   }

   assume(iovcnt<MPC_OFI_IOVEC_SIZE);
   mpc_common_spinlock_lock(&domain->lock);


   *req = __acquire_request(domain, _mpc_ofi_domain_request_complete, NULL, comptetion_cb_ext, arg_ext);

   unsigned int i = 0;

   for(i=0 ; i < iovcnt; i++)
   {
      if( mpc_ofi_domain_memory_register_no_lock(domain, iov[i].iov_base, iov[i].iov_len, FI_SEND, &(*req)->mr[i]) )
      {
         mpc_common_errorpoint("Failed to register send buffer");
         retcode = -1;
         goto sendv_unlock;
      }
      (*req)->mr_count++;
   }

   fi_addr_t addr = 0;

   if(mpc_ofi_domain_dns_resolve(&domain->ddns, dest, &addr))
   {
      mpc_common_errorpoint("Failed to resolve address");
      retcode = -1;
      goto sendv_unlock;
   }


   void * descs[MPC_OFI_IOVEC_SIZE];

   for(i = 0 ; i < iovcnt; i++)
   {
      descs[i] = fi_mr_desc((*req)->mr[i]);
   }

   while(1)
   {
      ssize_t ret = fi_sendv(domain->ep, iov, descs, iovcnt, addr, (void *)*req);

      if(ret == -FI_EAGAIN)
      {
         continue;
      }

      if(ret < 0)
      {
         MPC_OFI_DUMP_ERROR(ret);
         retcode = (int)ret;
         goto sendv_unlock;
      }

      break;
   }

sendv_unlock:
   mpc_common_spinlock_unlock(&domain->lock);
   return 0;
}



int mpc_ofi_domain_get(struct mpc_ofi_domain_t *domain,
                       void *buf,
                       size_t len,
                       uint64_t dest,
                       uint64_t remote_addr,
                       uint64_t key,
                       struct mpc_ofi_request_t **preq,
                       int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                       void *arg_ext)
{
   struct mpc_ofi_request_t ** req = preq;
   struct mpc_ofi_request_t * __req = NULL;
   int retcode = 0;

   if(!req)
   {
      req = &__req;
   }

   mpc_common_spinlock_lock(&domain->lock);

   *req = __acquire_request(domain, _mpc_ofi_domain_request_complete, NULL, comptetion_cb_ext, arg_ext);

   if( mpc_ofi_domain_memory_register_no_lock(domain, buf, len, FI_READ, &(*req)->mr[0]) )
   {
      mpc_common_errorpoint("Failed to register send buffer");
      retcode = -1;
      goto get_unlock;
   }

   (*req)->mr_count = 1;

   fi_addr_t target_addr = 0;

   if(mpc_ofi_domain_dns_resolve(&domain->ddns, dest, &target_addr))
   {
      mpc_common_errorpoint("Failed to resolve address");
      retcode = -1;
      goto get_unlock;
   }
#if 1
   struct iovec iov;
   iov.iov_base = buf;
   iov.iov_len = len;

   struct fi_rma_iov riov;
   riov.addr = remote_addr;
   riov.len = len;
   riov.key = key;

   struct fi_msg_rma rma_msg = { .msg_iov = &iov,
                                 .desc = fi_mr_desc((*req)->mr[0]),
                                 .iov_count = 1,
                                 .addr = target_addr,
                                 .rma_iov = &riov,
                                 .rma_iov_count = 1,
                                 .context = *req
   };


   while(1)
   {
      ssize_t ret = fi_readmsg(domain->ep, &rma_msg, FI_COMPLETION);

      if(ret == -FI_EAGAIN)
      {
         continue;
      }

      if(ret < 0)
      {
         MPC_OFI_DUMP_ERROR(ret);
         retcode = (int)ret;
         goto get_unlock;
      }
      break;
   }
#else
   while(1)
   {
      ssize_t ret = fi_read(domain->ep, buf, len, fi_mr_desc((*req)->mr[0]), 0, remote_addr, key, *req);

      if(ret == -FI_EAGAIN)
      {
         continue;
      }

      if(ret < 0)
      {
         MPC_OFI_DUMP_ERROR(ret);
         retcode = (int)ret;
         goto get_unlock;
      }
      break;
   }
#endif

get_unlock:
   mpc_common_spinlock_unlock(&domain->lock);

   return retcode;
}


int mpc_ofi_domain_put(struct mpc_ofi_domain_t *domain,
                       void *buf,
                       size_t len,
                       uint64_t dest,
                       uint64_t remote_addr,
                       uint64_t key,
                       struct mpc_ofi_request_t **preq,
                       int (*comptetion_cb_ext)(struct mpc_ofi_request_t *, void *),
                       void *arg_ext)
{
   struct mpc_ofi_request_t ** req = preq;
   struct mpc_ofi_request_t * __req = NULL;
   int retcode = 0;

   if(!req)
   {
      req = &__req;
   }

   mpc_common_spinlock_lock(&domain->lock);

   *req = __acquire_request(domain, _mpc_ofi_domain_request_complete, NULL, comptetion_cb_ext, arg_ext);

   if( mpc_ofi_domain_memory_register_no_lock(domain, buf, len, FI_READ, &(*req)->mr[0]) )
   {
      mpc_common_errorpoint("Failed to register send buffer");
      retcode = -1;
      goto get_unlock;
   }

   (*req)->mr_count = 1;

   fi_addr_t target_addr = 0;

   if(mpc_ofi_domain_dns_resolve(&domain->ddns, dest, &target_addr))
   {
      mpc_common_errorpoint("Failed to resolve address");
      retcode = -1;
      goto get_unlock;
   }

   struct iovec iov;
   iov.iov_base = buf;
   iov.iov_len = len;

   struct fi_rma_iov riov;
   riov.addr = remote_addr;
   riov.len = len;
   riov.key = key;

   struct fi_msg_rma rma_msg = { .msg_iov = &iov,
                                 .desc = fi_mr_desc((*req)->mr[0]),
                                 .iov_count = 1,
                                 .addr = target_addr,
                                 .rma_iov = &riov,
                                 .rma_iov_count = 1,
                                 .context = *req
   };


   while(1)
   {
      ssize_t ret = fi_writemsg(domain->ep, &rma_msg, FI_COMPLETION);

      if(ret == -FI_EAGAIN)
      {
         continue;
      }

      if(ret < 0)
      {
         MPC_OFI_DUMP_ERROR(ret);
         retcode = (int)ret;
         goto get_unlock;
      }
      break;
   }

get_unlock:
   mpc_common_spinlock_unlock(&domain->lock);

   return retcode;
}