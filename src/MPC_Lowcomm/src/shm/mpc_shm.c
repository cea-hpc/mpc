#include <mpc_config.h>
#ifdef MPC_USE_CMA
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#endif


#include "mpc_shm.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#include <sctk_alloc.h>

#include <mpc_common_datastructure.h>
#include <mpc_common_debug.h>
#include <mpc_common_rank.h>
#include <mpc_common_spinlock.h>
#include <mpc_launch_pmi.h>
#include <mpc_launch_shm.h>
#include <mpc_lowcomm_monitor.h>
#include <mpc_lowcomm_types.h>



#include "rail.h"
#include <lcr/lcr_component.h>

/*************************
 * FRAGMENTED OPERATIONS *
 *************************/

#define SHM_FRAG_HT_SIZE 64
#define SHM_MEMPOOL_BASE_SIZE 32

/**
 * @brief This initializes a fragment factory. This allows the copy of the incoming fragments
 * and manages the total size to call a callback on completion.
 * 
 * @param factory the factory to initialize.
 * @return int 0 on success
 */
int _mpc_shm_fragment_factory_init(struct _mpc_shm_fragment_factory *factory)
{
   mpc_common_spinlock_init(&factory->lock, 0);
   factory->fragment_id = 1;
   factory->segment_id = 1;
   mpc_common_hashtable_init(&factory->frags, SHM_FRAG_HT_SIZE);
   mpc_common_hashtable_init(&factory->regs, SHM_FRAG_HT_SIZE);

   mpc_mempool_init(&factory->frag_pool, 1, SHM_MEMPOOL_BASE_SIZE, sizeof(struct _mpc_shm_fragment), sctk_malloc, sctk_free);
   mpc_mempool_init(&factory->reg_pool, 1, SHM_MEMPOOL_BASE_SIZE, sizeof(struct _mpc_shm_region), sctk_malloc, sctk_free);

   return 0;
}

/**
 * @brief This registers a new segment for tracking and returns a segment ID
 * 
 * @param factory the factory to register in
 * @param addr the base address of the segment
 * @param size the size of the segment
 * @return uint64_t the segment ID which uniquely describes the segment
 */
uint64_t _mpc_shm_fragment_factory_register_segment(struct _mpc_shm_fragment_factory *factory, void * addr, size_t size)
{
   uint64_t reg_id = 0;
   mpc_common_spinlock_lock(&factory->lock);
   reg_id = factory->segment_id++;
   mpc_common_spinlock_unlock(&factory->lock);

   struct _mpc_shm_region * reg = (struct _mpc_shm_region*)mpc_mempool_alloc(&factory->reg_pool);
   assume(reg != NULL);

   reg->addr = addr;
   reg->size = size;
   reg->id = reg_id;

   mpc_common_hashtable_set(&factory->regs, reg_id, reg);

   return reg_id;
}

/**
 * @brief Remove a previously registered segment from the factory
 * 
 * @param factory the factory to manage
 * @param regid the registration ID as returned from @ref _mpc_shm_fragment_factory_register_segment
 * @return int 0 on success 1 if the segment is not known
 */
int _mpc_shm_fragment_factory_unregister_segment(struct _mpc_shm_fragment_factory *factory, uint64_t regid)
{
   struct _mpc_shm_region * reg = mpc_common_hashtable_get(&factory->regs, regid);

   if(!reg)
   {
      return 1;
   }

   mpc_common_hashtable_delete(&factory->regs, regid);

   mpc_mempool_free(NULL, reg);

   return 0;
}

/**
 * @brief Get the segment matching a registration ID (see @ref _mpc_shm_fragment_factory_register_segment)
 * 
 * @param factory the factory to get from
 * @param id the registration id
 * @return struct _mpc_shm_region* the segment NULL if not found 
 */
struct _mpc_shm_region * _mpc_shm_fragment_factory_get_segment(struct _mpc_shm_fragment_factory *factory, uint64_t id)
{
   return mpc_common_hashtable_get(&factory->regs, id);
}

/**
 * @brief This initializes a fragmented operation over a factory. A fragment is a write to a segment relying on multiple copies
 * 
 * @param factory the factory to manipulate
 * @param segment_id the id of the segment to reference (@ref _mpc_shm_fragment_factory_register_segment)
 * @param base_addr the base address of where to copy the data
 * @param size the size of the whole local segement
 * @param completion the callback to call when completed
 * @param arg the argument to pass to the callback to call
 * @return uint64_t registration ID of the fragmented operation
 */
uint64_t _mpc_shm_fragment_init(struct _mpc_shm_fragment_factory *factory, uint64_t segment_id, void * base_addr, size_t size, void (*completion)(void * arg), void *arg)
{
   uint64_t reg_id = 0;
   mpc_common_spinlock_lock(&factory->lock);
   reg_id = factory->fragment_id++;
   mpc_common_spinlock_unlock(&factory->lock);

   struct _mpc_shm_fragment * frag = (struct _mpc_shm_fragment*)mpc_mempool_alloc(&factory->frag_pool);
   assume(frag != NULL);

   mpc_common_spinlock_init(&frag->lock, 0);
   frag->id = reg_id;
   frag->segment_id = segment_id;
   frag->arg = arg;
   frag->completion = completion;
   frag->sizeleft = size;
   frag->base_addr = base_addr;

   mpc_common_hashtable_set(&factory->frags, reg_id, frag);

   return reg_id;
}

/**
 * @brief Notify incoming data on a fragment. This copies the data in the target buffers and tracks completion (calling the callback)
 * 
 * @param factory the factory to manipulate
 * @param frag_id the fragment ID
 * @param buffer the buffer to inject in the fragment
 * @param offset the offset of the buffer
 * @param size the size of the fragment
 * @return int 0 on success
 */
int _mpc_shm_fragment_notify(struct _mpc_shm_fragment_factory *factory, uint64_t frag_id, void * buffer, size_t offset, size_t size)
{
   struct _mpc_shm_fragment * frag = mpc_common_hashtable_get(&factory->frags, frag_id);

   if(!frag)
   {
      mpc_common_debug_warning("No such segment %lld", frag_id);
      return 1;
   }

   /* Region not overlaping no need to lock */
   memcpy(frag->base_addr + offset, buffer, size);

   size_t sizeleft = 0;

   mpc_common_spinlock_lock(&frag->lock);
   sizeleft = frag->sizeleft = frag->sizeleft - size;
   mpc_common_spinlock_unlock(&frag->lock);

   if(sizeleft == 0)
   {
      (frag->completion)(frag->arg);
      mpc_common_hashtable_delete(&factory->frags, frag->id);
      mpc_mempool_free(NULL, frag);
   }

   return 0;
}

/*********************
 * LIST MANIPULATION *
 *********************/

/**
 * @brief This initializes an SHM list
 * 
 * @param head the list to initialize
 */
void _mpc_shm_list_head_init(struct _mpc_shm_list_head *head)
{
   mpc_common_spinlock_init(&head->lock, 0);
   head->head = NULL;
   head->tail = NULL;
}


/**
 * @brief Add a segment in the SHM list using segment-local offsets only
 * @warning call with no locks
 * 
 * @param list the target list object
 * @param cell the cell to push in (must be in the segment)
 * @param base the base address of the SHM segment
 */
static inline void ___mpc_shm_list_head_push_no_lock(struct _mpc_shm_list_head * list, struct _mpc_shm_cell *cell, void * base)
{
   assert(cell->next == NULL);
   assert(cell->prev == NULL);

   /* Work with relative addresses */
   void * relative_cell_addr = (void *)cell - (uint64_t)base;

   if(!list->head)
   {
      list->tail = relative_cell_addr;
   }
   else
   {
      /* Need the true address to modify the target cell */
      struct _mpc_shm_cell * phead = (struct _mpc_shm_cell *)((void *)list->head + (uint64_t)base); 
      phead->prev = relative_cell_addr;
   }

   cell->prev = NULL;
   cell->next = list->head;
   list->head = relative_cell_addr;
}

/**
 * @brief Add a segment in the SHM list (threadsafe)
 * 
 * @param list the target list object
 * @param cell the cell to push in (must be in the segment)
 * @param base the base address of the SHM segment
 */
static inline void _mpc_shm_list_head_push(struct _mpc_shm_list_head * list, struct _mpc_shm_cell *cell, void *base)
{
   mpc_common_spinlock_lock(&list->lock);

   ___mpc_shm_list_head_push_no_lock(list, cell, base);

   mpc_common_spinlock_unlock(&list->lock);
}

/**
 * @brief Try to add a segment in the SHM list using segment-local offsets only
 * @warning call with no locks
 * 
 * @param list the target list object
 * @param cell the cell to push in (must be in the segment)
 * @param base the base address of the SHM segment
 * @return 1 if the push succeeded 0 otherwise
 */
static inline int _mpc_shm_list_head_try_push(struct _mpc_shm_list_head * list, struct _mpc_shm_cell *cell, void *base)
{
   if( mpc_common_spinlock_trylock(&list->lock) == 0)
   {

      ___mpc_shm_list_head_push_no_lock(list, cell, base);

      mpc_common_spinlock_unlock(&list->lock);

      return 0;
   }

   return 1;
}

/**
 * @brief Try to get a cell from a list
 * 
 * @param list the list to query
 * @param base the base address of the SHM segment
 * @return struct _mpc_shm_cell* the cell if queried (NULL otherwise either list empty already locked)
 */
static inline struct _mpc_shm_cell * _mpc_shm_list_head_try_get(struct _mpc_shm_list_head * list, void * base)
{
   struct _mpc_shm_cell * ret = NULL;

   if( mpc_common_spinlock_trylock(&list->lock) == 0)
   {

      if(list->tail)
      {
      	 ret = (struct _mpc_shm_cell*)((char*)list->tail + (uint64_t)base);
         list->tail = ret->prev;
         ret->next = NULL;
         ret->prev = NULL;
         if(!list->tail)
         {
            list->head = NULL;
         }
      }

      mpc_common_spinlock_unlock(&list->lock);
   }

   return ret;
}

/***********************
 * CROSS MEMORY ATTACH *
 ***********************/

 #if MPC_USE_CMA

int __do_cma_read(pid_t pid, void * src, void *dest, size_t size)
{
   struct iovec local;
   struct iovec remote;
   
   local.iov_base=dest;
   local.iov_len = size;

   remote.iov_base = src;
   remote.iov_len = size;

   ssize_t ret = process_vm_readv(pid,
                    &local,
                    1,
                    &remote,
                    1,
                    0);


   if(ret != (ssize_t)size)
   {
      return 1;
   }

   return 0;
}

int __do_cma_write(pid_t pid, void * src, void *dest, size_t size)
{
   struct iovec local;
   struct iovec remote;
   
   local.iov_base=dest;
   local.iov_len = size;

   remote.iov_base = src;
   remote.iov_len = size;

   ssize_t ret = process_vm_writev(pid,
                    &local,
                    1,
                    &remote,
                    1,
                    0);


   if(ret != (ssize_t)size)
   {
      return 1;
   }

   return 0;
}


int mpc_shm_has_cma_support(struct _mpc_shm_storage * storage)
{
   if(storage->cma_state == MPC_SHM_CMA_OK)
   {
      return 1;
   }

   if(storage->cma_state == MPC_SHM_CMA_NOK)
   {
      return 0;
   }

   /* We are unknown */
   int i = 0;
   int local_pcount = mpc_common_get_local_process_count();
   int my_rank = mpc_common_get_local_process_rank();

   for(i = 0 ; i < local_pcount; i++)
   {
      if(i == my_rank)
      {
         continue;
      }

      pid_t remote_pid = storage->pids[i];
      mpc_lowcomm_peer_uid_t *remote_uid_addresses = storage->remote_uid_addresses[i];
      mpc_lowcomm_peer_uid_t remote_uid_cma = 0;
      mpc_lowcomm_peer_uid_t remote_uid_value = mpc_lowcomm_monitor_local_uid_of(i);

      if( __do_cma_read(remote_pid, remote_uid_addresses, &remote_uid_cma, sizeof(mpc_lowcomm_peer_uid_t)) )
      {
         goto error_cma;
      }

      if(remote_uid_value != remote_uid_cma)
      {
         goto error_cma;
      }

   }

   storage->cma_state = MPC_SHM_CMA_OK;
   return 1;

error_cma:
   storage->cma_state = MPC_SHM_CMA_NOK;
   return 0;
}

#endif


int _mpc_shm_get_local_rank(struct _mpc_shm_storage *storage, mpc_lowcomm_peer_uid_t uid)
{
   int i;
   for(i = 0 ; i < storage->process_count; i++)
   {
      if(storage->uids[i] == uid)
      {
         return i;
      }
   }

   return -1;
}



struct _mpc_shm_cell * _mpc_shm_storage_get_free_cell(struct _mpc_shm_storage *storage)
{
   int my_rank = mpc_common_get_local_process_rank();

   struct _mpc_shm_cell * ret = NULL;

   unsigned int target_list = (my_rank * SHM_FREELIST_PER_PROC) % storage->freelist_count;

   while( !(ret = _mpc_shm_list_head_try_get(&storage->free_lists[target_list], storage->shm_buffer)))
   {
      target_list = (target_list + 1) % storage->freelist_count;
   }

   return ret;
}


void _mpc_shm_storage_send_cell_local_rank(struct _mpc_shm_storage * storage,
                                           int local_rank,
                                           struct _mpc_shm_cell * cell)
{
   int my_rank = mpc_common_get_local_process_rank();

   unsigned int target_rank = local_rank;
   unsigned int target_list = (size_t)target_rank * storage->process_arity +  (my_rank % storage->process_arity);

   /* Pick one of the ARIRY incoming list */
   struct _mpc_shm_list_head * list = &storage->per_process[target_list];
   unsigned int cnt = 0;

   assert( (storage->shm_buffer < (void*)cell) && ((void*)cell < (storage->shm_buffer + storage->segment_size)));

   while(1)
   {
      if(mpc_common_spinlock_trylock(&list->lock) == 0)
      {
         ___mpc_shm_list_head_push_no_lock(list, cell, storage->shm_buffer);
         mpc_common_spinlock_unlock(&list->lock);
         break;
      }

      cnt++;
      target_list = (size_t)target_rank * storage->process_arity +  ((my_rank + cnt) % storage->process_arity);
   }
}


void _mpc_shm_storage_send_cell(struct _mpc_shm_storage * storage,
                               _mpc_lowcomm_endpoint_t * endpoint,
                               struct _mpc_shm_cell * cell)
{
   unsigned int target_rank = endpoint->data.shm.local_rank;
   _mpc_shm_storage_send_cell_local_rank(storage,
                                         target_rank,
                                         cell);
}

void _mpc_shm_storage_free_cell(struct _mpc_shm_storage * storage,
                                struct _mpc_shm_cell * cell)
{
   unsigned char dest = cell->free_list;

   int did_push = 0;

   do
   {
      if(_mpc_shm_list_head_try_push(&storage->free_lists[dest], cell, storage->shm_buffer) == 0)
      {
         did_push = 1;
      }
      dest = (dest + 1) % storage->freelist_count;
   }while(!did_push);
}

/*********************
 * PINNING FUNCTIONS *
 *********************/

void  mpc_shm_pin(struct sctk_rail_info_s *rail, struct sctk_rail_pin_ctx_list *list, void *addr, size_t size)
{
   list->pin.shm.addr = addr;
   list->pin.shm.id = _mpc_shm_fragment_factory_register_segment(&rail->network.shm.frag_factory, addr, size);
}

void mpc_shm_unpin(struct sctk_rail_info_s *rail, struct sctk_rail_pin_ctx_list *list)
{
   _mpc_shm_fragment_factory_unregister_segment(&rail->network.shm.frag_factory, list->pin.shm.id);
}


int mpc_shm_pack_rkey(sctk_rail_info_t *rail,
                      lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        memcpy(dest, &memp->pin.shm, sizeof(_mpc_lowcomm_shm_pinning_ctx_t));
        return sizeof(_mpc_lowcomm_shm_pinning_ctx_t);
}

int mpc_shm_unpack_rkey(sctk_rail_info_t *rail,
                        lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        memcpy( &memp->pin.shm, dest, sizeof(_mpc_lowcomm_shm_pinning_ctx_t));
        return sizeof(_mpc_lowcomm_shm_pinning_ctx_t);
}



/***********************
 * MESSAGING FUNCTIONS *
 ***********************/

int mpc_shm_send_am_zcopy(__UNUSED__ _mpc_lowcomm_endpoint_t *ep,
                          __UNUSED__ uint8_t id,
                          __UNUSED__ const void *header,
                          __UNUSED__ unsigned header_length,
                          __UNUSED__ const struct iovec *iov,
                          __UNUSED__ size_t iovcnt,
                          __UNUSED__ unsigned flags,
                          __UNUSED__ lcr_completion_t *comp)
{
   not_implemented();
}


ssize_t mpc_shm_send_am_bcopy(_mpc_lowcomm_endpoint_t *ep,
                              uint8_t id,
                              lcr_pack_callback_t pack,
                              void *arg,
                              __UNUSED__ unsigned flags)
{
   sctk_rail_info_t *rail = ep->rail;
   uint32_t payload_length = 0;

   struct _mpc_shm_cell * cell = _mpc_shm_storage_get_free_cell(&rail->network.shm.storage);

   assume(cell);

   _mpc_shm_am_hdr_t *hdr = (_mpc_shm_am_hdr_t *)cell->data;

   hdr->op = MPC_SHM_AM_EAGER;
   hdr->am_id = id;
   hdr->length = payload_length = pack(hdr->data, arg);
   payload_length = hdr->length;

   _mpc_shm_storage_send_cell(&rail->network.shm.storage,
                              ep,
                              cell);

   return payload_length;
}

static inline void __deffered_completion_cb(void *pcomp)
{
   lcr_completion_t *comp = (lcr_completion_t *)pcomp;
   comp->comp_cb(comp);
}


int __get_zcopy_over_frag(_mpc_lowcomm_endpoint_t *ep,
                           uint64_t local_addr,
                           uint64_t remote_offset,
                           lcr_memp_t *remote_key,
                           size_t size,
                           lcr_completion_t *comp) 
{
   sctk_rail_info_t *rail = ep->rail;

   comp->sent = size;

   uint64_t fragment_id = _mpc_shm_fragment_init(&rail->network.shm.frag_factory, 
                                              remote_key->pin.shm.id,
                                              (void*)local_addr,
                                              size,
                                              __deffered_completion_cb,
                                              comp);

   struct _mpc_shm_cell * cell = _mpc_shm_storage_get_free_cell(&rail->network.shm.storage);
   assume(cell);

   _mpc_shm_am_hdr_t *hdr = (_mpc_shm_am_hdr_t *)cell->data;

   hdr->op = MPC_SHM_AM_GET;
   hdr->am_id = 0;
   hdr->length = sizeof(_mpc_shm_am_get);

   _mpc_shm_am_get * get = (_mpc_shm_am_get*)hdr->data;

   get->source = mpc_lowcomm_monitor_get_uid();
   get->frag_id = fragment_id;
   get->segment_id = remote_key->pin.shm.id;
   get->offset = remote_offset;
   get->size = size;

   _mpc_shm_storage_send_cell(&rail->network.shm.storage,
                              ep,
                              cell);

   return MPC_LOWCOMM_SUCCESS;
}

#ifdef MPC_USE_CMA
int __get_zcopy_over_cma(_mpc_lowcomm_endpoint_t *ep,
                           uint64_t local_addr,
                           uint64_t remote_offset,
                           lcr_memp_t *remote_key,
                           size_t size,
                           lcr_completion_t *comp)
{
   pid_t target_pid = ep->data.shm.cma_pid;
   assume(0 <= target_pid);
   int ret = __do_cma_read(target_pid, remote_key->pin.shm.addr + remote_offset, (void*)local_addr, size);
   assume(ret == 0);

   comp->sent = size;
   comp->comp_cb(comp);

   return MPC_LOWCOMM_SUCCESS;
}

int __put_zcopy_over_cma(_mpc_lowcomm_endpoint_t *ep,
                           uint64_t local_addr,
                           uint64_t remote_offset,
                           lcr_memp_t *remote_key,
                           size_t size,
                           lcr_completion_t *comp)
{
   pid_t target_pid = ep->data.shm.cma_pid;
   assume(0 <= target_pid);
   int ret = __do_cma_write(target_pid, remote_key->pin.shm.addr + remote_offset, (void*)local_addr, size);
   assume(ret == 0);

   comp->sent = size;
   comp->comp_cb(comp);

   return MPC_LOWCOMM_SUCCESS;
}

#endif


int mpc_shm_get_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           uint64_t local_addr,
                           uint64_t remote_offset,
                           lcr_memp_t *remote_key,
                           size_t size,
                           lcr_completion_t *comp) 
{

 #if MPC_USE_CMA

   int cma = mpc_shm_has_cma_support(&ep->rail->network.shm.storage);

   if(cma)
   {
         return __get_zcopy_over_cma(ep, local_addr, remote_offset, remote_key, size, comp);
   }

#endif

   return __get_zcopy_over_frag(ep, local_addr, remote_offset, remote_key, size, comp);
}




int mpc_shm_put_zcopy(_mpc_lowcomm_endpoint_t *ep,
                        uint64_t local_addr,
                        uint64_t remote_offset,
                        lcr_memp_t *remote_key,
                        size_t size,
                        lcr_completion_t *comp)
{
 #if MPC_USE_CMA

   int cma = mpc_shm_has_cma_support(&ep->rail->network.shm.storage);

   if(cma)
   {
         return __put_zcopy_over_cma(ep, local_addr, remote_offset, remote_key, size, comp);
   }

#endif
   not_implemented();
}




/*****************************
 * INCOMING MESSAGE HANDLING *
 *****************************/


int ____handle_incoming_eager(__UNUSED__ struct _mpc_shm_storage * storage, sctk_rail_info_t *rail, _mpc_shm_am_hdr_t *hdr )
{
   //mpc_common_debug_error("IN CB %d LEN %lld", hdr->am_id, hdr->length);

	lcr_am_handler_t handler = rail->am[hdr->am_id];
	if (handler.cb == NULL) {
		mpc_common_debug_fatal("LCP: handler id %d not supported.", hdr->am_id);
	}

	int rc = handler.cb(handler.arg, hdr->data, hdr->length, 0);

	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: handler id %d failed.", hdr->am_id);
	}

   return rc;
}


int ____handle_incoming_get(__UNUSED__ struct _mpc_shm_storage * storage, sctk_rail_info_t *rail, _mpc_shm_am_hdr_t *hdr)
{
   _mpc_shm_am_get * get = (_mpc_shm_am_get*)hdr->data;

   struct _mpc_shm_region * region = _mpc_shm_fragment_factory_get_segment(&rail->network.shm.frag_factory, get->segment_id);

   if(!region)
   {
      mpc_common_debug_fatal("No such SHM region %lld registered", get->segment_id);
   }

   size_t block_size = MPC_SHM_EAGER_SIZE - sizeof(_mpc_shm_am_hdr_t) - sizeof(_mpc_shm_frag);
   size_t sent = 0;

   assume(region->size <= (get->offset + get->size));


   while(sent < get->size)
   {
      size_t sizeleft = (get->size - sent);
      size_t to_send = (sizeleft < block_size)?sizeleft:block_size;

      struct _mpc_shm_cell * cell = _mpc_shm_storage_get_free_cell(&rail->network.shm.storage);
      assume(cell);

      _mpc_shm_am_hdr_t *hdr = (_mpc_shm_am_hdr_t *)cell->data;

      hdr->op = MPC_SHM_AM_FRAG;
      hdr->length = sizeof(_mpc_shm_frag);

      _mpc_shm_frag * frag = (_mpc_shm_frag *)hdr->data;

      frag->frag_id = get->frag_id;
      frag->segment_id = get->segment_id;
      frag->size = to_send;
      frag->offset = sent;
      memcpy(frag->data, region->addr + get->offset + sent, to_send);

      _mpc_shm_storage_send_cell_local_rank(&rail->network.shm.storage,
                                 _mpc_shm_get_local_rank(storage, get->source),
                                          cell);


      sent += to_send;
   }

   return MPC_LOWCOMM_SUCCESS;
}

int ____handle_incoming_frag(__UNUSED__ struct _mpc_shm_storage * storage, sctk_rail_info_t *rail, _mpc_shm_am_hdr_t *hdr)
{
   _mpc_shm_frag * frag = (_mpc_shm_frag*)hdr->data;
   return _mpc_shm_fragment_notify(&rail->network.shm.frag_factory, frag->frag_id, frag->data, frag->offset, frag->size);
}


int __handle_incoming_message(struct _mpc_shm_storage * storage, sctk_rail_info_t *rail, struct _mpc_shm_cell * cell)
{
   _mpc_shm_am_hdr_t *hdr = (_mpc_shm_am_hdr_t*)cell->data;

   int rc = MPC_LOWCOMM_SUCCESS;

   switch (hdr->op) {
      case MPC_SHM_AM_EAGER:
         rc = ____handle_incoming_eager(storage, rail, hdr);
      break;
      case MPC_SHM_AM_GET:
         rc = ____handle_incoming_get(storage, rail, hdr);
      break;
      case MPC_SHM_AM_FRAG:
         rc = ____handle_incoming_frag(storage, rail, hdr);
      break;
      default:
         not_implemented();
   }

   _mpc_shm_storage_free_cell(storage, cell);

   return rc;
}


int mpc_shm_progress(sctk_rail_info_t *rail)
{
   int my_rank = mpc_common_get_local_process_rank();

   assume(0 <= my_rank);

   int rc = MPC_LOWCOMM_SUCCESS;

   unsigned int i = 0;

   for( i = my_rank * SHM_PROCESS_ARITY; i < (my_rank + 1) * SHM_PROCESS_ARITY; i++)
   {
      struct _mpc_shm_cell * cell = _mpc_shm_list_head_try_get(&rail->network.shm.storage.per_process[i], rail->network.shm.storage.shm_buffer);

      if(cell)
      {
         rc = __handle_incoming_message(&rail->network.shm.storage, rail, cell);
      }
   }

   return rc;
}



/***************
 * SHM STORAGE *
 ***************/

/**
 * @brief This internal function computes the size of the SHM segment
 * 
 * @param process_count Number of processes on the node
 * @param task_count Number of tasks on the node
 * @param freelist_count 
 * @return size_t 
 */
static size_t _mpc_shm_storage_get_size(unsigned int process_count,
                                        unsigned int freelist_count)
{
   size_t ret = process_count * SHM_PROCESS_ARITY * sizeof(struct _mpc_shm_list_head);
   ret += freelist_count * sizeof(struct _mpc_shm_list_head);
   /* Begin CMA context */
   ret += process_count * sizeof(mpc_lowcomm_peer_uid_t *);
   ret += process_count * sizeof(pid_t);
   ret += process_count * sizeof(mpc_lowcomm_peer_uid_t);
   /* End CMA context */
   ret += SHM_CELL_COUNT_PER_PROC * process_count * sizeof(struct _mpc_shm_cell);

   mpc_common_debug_info("SHM: segment is %g MB", (double)ret / (1024 * 1024));

   return ret;
}

/*
SEGMENT Layout:

List heads

[SHM_PROCESS_ARITY  x (LIST HEAD)] x LOCAL PROCESS COUNT
[SHM_FREELIST_PER_PROC x (LIST HEAD)] x LOCAL PROCESS COUNT

For CMA checks, we read from local memory and use
the tables of UIDs and PID to convert to PID

[address of UID in local mem] x LOCAL PROCESS COUNT
[PID of each process] x LOCAL PROCESS COUNT
[UID for each process] x LOCAL PROCESS COUNT

Cells used for transfer

[SHM_CELL_COUNT_PER_PROC shm cell] x LOCAL PROCESS COUNT
*/

/**
 * @brief This function is central in SHM it defines the layout of the SHM segment
 * all operations on the SHM segment are done using structures located in this segment
 * 
 * @param storage the storage to initialize
 * @return int 0 on success
 */
int _mpc_shm_storage_init(struct _mpc_shm_storage * storage)
{
   unsigned int process_count = mpc_common_get_local_process_count();

   storage->cma_state = MPC_SHM_CMA_UNCHECKED;

   storage->process_count = process_count;
   storage->process_arity = SHM_PROCESS_ARITY;

   assume(0 < process_count);

   storage->cell_count = SHM_CELL_COUNT_PER_PROC * process_count;

   storage->freelist_count = process_count * SHM_FREELIST_PER_PROC;
   size_t segment_size = _mpc_shm_storage_get_size(process_count, storage->freelist_count);
   storage->segment_size = segment_size;
   storage->shm_buffer = mpc_launch_shm_map(segment_size, MPC_LAUNCH_SHM_USE_PMI, NULL);

   assume(storage != NULL);
   assume(0 < storage->freelist_count);

   storage->per_process = storage->shm_buffer;
   assume((void*)storage->per_process < ((void*)storage->shm_buffer + segment_size));

   storage->free_lists = storage->per_process + SHM_PROCESS_ARITY * storage->process_count;

   assume((void*)storage->per_process < (void*)storage->free_lists);
   assume((void*)storage->free_lists < ((void*)storage->shm_buffer + segment_size));

   /* Begin CMA context */

   storage->remote_uid_addresses = (mpc_lowcomm_peer_uid_t **)(storage->free_lists + storage->freelist_count);
   assume(((void*)storage->free_lists < (void*)storage->remote_uid_addresses) && ((void*)storage->remote_uid_addresses < ((void*)storage->shm_buffer + segment_size)));

   storage->pids = (pid_t *)(storage->remote_uid_addresses + process_count);
   assume(((void*)storage->remote_uid_addresses < (void*)storage->pids) && ((void*)storage->pids < ((void*)storage->shm_buffer + segment_size)));

   storage->uids = (mpc_lowcomm_peer_uid_t*)(storage->pids + process_count);
   assume(((void*)storage->pids < (void*)storage->uids) && ((void*)storage->uids < ((void*)storage->shm_buffer + segment_size)));


   unsigned int proc_rank = mpc_common_get_local_process_rank();

   /* Set my address and my value and my pid */
   storage->my_uid = mpc_lowcomm_monitor_get_uid();
   storage->remote_uid_addresses[proc_rank] = &storage->my_uid;
   storage->pids[proc_rank] = getpid();
   storage->uids[proc_rank] = mpc_lowcomm_monitor_get_uid();

   /* End CMA context */

   /* Per process head do init */
   unsigned int i = 0;

   for( i = proc_rank * SHM_PROCESS_ARITY; i < (proc_rank + 1) * SHM_PROCESS_ARITY; i++)
   {
	   _mpc_shm_list_head_init(&storage->per_process[i]);
   }

   struct _mpc_shm_cell * cells = (struct _mpc_shm_cell*)(storage->uids + process_count);
   assume(((void*)storage->free_lists < (void*)cells) && ((void*)cells < ((void*)storage->shm_buffer + segment_size)));

      int rr_counter = 0;

      for( i = proc_rank * SHM_CELL_COUNT_PER_PROC ; i < (proc_rank + 1) * SHM_CELL_COUNT_PER_PROC; i++)
      {

         assume(i < storage->cell_count);
         cells[i].next = NULL;
         cells[i].prev = NULL;
         _mpc_shm_list_head_push(&storage->free_lists[proc_rank * SHM_FREELIST_PER_PROC + rr_counter], &cells[i],  storage->shm_buffer);
         cells[i].free_list = (char)proc_rank;
         rr_counter = (rr_counter + 1)% (int)SHM_FREELIST_PER_PROC;
      }

 #if 0
   /* This leads to deadlocks in the monitor */
   mpc_launch_pmi_barrier();
   mpc_shm_has_cma_support(storage);
#endif

   return 0;
}

int _mpc_shm_storage_release(struct _mpc_shm_storage *storage)
{
   not_implemented();
}



/********************
 * INIT AND RELEASE *
 ********************/


void mpc_shm_connect_on_demand(__UNUSED__ struct sctk_rail_info_s *rail, mpc_lowcomm_peer_uid_t dest)
{
        UNUSED(rail);
        //NOTE: nothing to be done since all transport endpoints were created at
        //      init.
        mpc_common_debug("On demand to %llu", dest);
}

void _mpc_lowcomm_shm_endpoint_info_init(_mpc_lowcomm_shm_endpoint_info_t * infos, mpc_lowcomm_peer_uid_t uid, struct _mpc_shm_storage *storage)
{
   int i;
   int local_proc_count = mpc_common_get_local_process_count();

   for(i = 0 ; i < local_proc_count; i++)
   {
      if(storage->uids[i] == uid)
      {
         infos->cma_pid = storage->pids[i];
         infos->local_rank = i;
         return;
      }
   }

   mpc_common_debug_fatal("Route UID not found in SHM segment");
}


void __add_route(mpc_lowcomm_peer_uid_t dest_uid, sctk_rail_info_t *rail)
{
	_mpc_lowcomm_endpoint_t *new_route = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_t) );
	assume(new_route != NULL);
	_mpc_lowcomm_endpoint_init(new_route, dest_uid, rail, _MPC_LOWCOMM_ENDPOINT_DYNAMIC);

   _mpc_lowcomm_shm_endpoint_info_init(&new_route->data.shm, dest_uid, &rail->network.shm.storage);

	/* Make sure the route is flagged connected */
	_mpc_lowcomm_endpoint_set_state(new_route, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
   sctk_rail_add_dynamic_route(rail, new_route);
}

void __add_node_local_routes(sctk_rail_info_t *rail)
{
   struct mpc_launch_pmi_process_layout *layout = NULL;
   mpc_launch_pmi_get_process_layout( &layout );

   assume(layout != NULL);

   int node_rank = mpc_common_get_node_rank();

   struct mpc_launch_pmi_process_layout *my_node = NULL;

   HASH_FIND_INT(layout, &node_rank, my_node);

   assume(my_node != NULL);

   int i = 0;

   uint64_t my_uid = mpc_lowcomm_monitor_get_uid();
   for(i = 0 ; i < my_node->nb_process; i++)
   {
           uint64_t dest_uid = mpc_lowcomm_monitor_local_uid_of(my_node->process_list[i]);
           if (!(dest_uid == my_uid)) {
                   __add_route(dest_uid, rail);
           }
   }

}

int mpc_shm_iface_is_reachable(sctk_rail_info_t *rail, uint64_t uid) {
        /* If the transport endpoint hase been created, it means the UID is
         * reachable */
        return sctk_rail_get_any_route_to_process(rail, uid) != NULL;
}

int mpc_shm_get_attr(__UNUSED__ sctk_rail_info_t *rail,
                     lcr_rail_attr_t *attr)
{
#if MPC_USE_CMA
	attr->iface.cap.am.max_iovecs = sysconf(_SC_IOV_MAX);
#else
	attr->iface.cap.am.max_iovecs = 1;
#endif
	attr->iface.cap.am.max_bcopy  = MPC_SHM_BCOPY_SIZE - sizeof(_mpc_shm_am_hdr_t);
   /* We only have bcopy */
	attr->iface.cap.am.max_zcopy  = 0;

	attr->iface.cap.tag.max_bcopy = 0;
	attr->iface.cap.tag.max_zcopy = 0;

	attr->iface.cap.rndv.max_put_zcopy = INT_MAX;
	attr->iface.cap.rndv.max_get_zcopy = INT_MAX;

	return MPC_LOWCOMM_SUCCESS;
}




int mpc_shm_query_devices(__UNUSED__ lcr_component_t *component,
                          lcr_device_t **devices_p,
                          unsigned int *num_devices_p)
{
#if 0
   if(mpc_common_get_local_process_count() == 1)
   {
      *num_devices_p = 0;
      return MPC_LOWCOMM_SUCCESS;
   }
#endif

   *devices_p = sctk_malloc(sizeof(lcr_device_t));
   (void)snprintf((*devices_p)->name, LCR_DEVICE_NAME_MAX, "shmsegment");
   *num_devices_p = 1;

   return MPC_LOWCOMM_SUCCESS;
}



int mpc_shm_iface_open(char *device_name, int id,
                       lcr_rail_config_t *rail_config,
                       lcr_driver_config_t *driver_config,
                       sctk_rail_info_t **iface_p)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	sctk_rail_info_t *rail = NULL;

	UNUSED(device_name);

	lcr_rail_init(rail_config, driver_config, &rail);
	if(rail == NULL)
	{
		mpc_common_debug_error("LCR: could not allocate SHM rail");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	strcpy(rail->device_name, "default");
	rail->rail_number = id;


	rail->iface_get_attr = mpc_shm_get_attr;



   if( _mpc_shm_storage_init(&rail->network.shm.storage) )
   {
      mpc_common_debug_error("Failed to initialize SHM segment");
      return MPC_LOWCOMM_ERROR;
   }

   if( _mpc_shm_fragment_factory_init(&rail->network.shm.frag_factory))
   {
      mpc_common_debug_error("Failed to initalize fragment factory");
      return MPC_LOWCOMM_ERROR;
   }

   rail->connect_on_demand = mpc_shm_connect_on_demand;
   rail->send_am_bcopy = mpc_shm_send_am_bcopy;
   rail->iface_progress = mpc_shm_progress;
   rail->iface_is_reachable = mpc_shm_iface_is_reachable;
   rail->rail_pin_region = mpc_shm_pin;
   rail->rail_unpin_region = mpc_shm_unpin;
   rail->iface_pack_memp = mpc_shm_pack_rkey;
   rail->iface_unpack_memp = mpc_shm_unpack_rkey;
   rail->get_zcopy = mpc_shm_get_zcopy;
   rail->send_am_zcopy = mpc_shm_send_am_zcopy;
   rail->put_zcopy = mpc_shm_put_zcopy;

#if 0
   rail->driver_finalize = mpc_ofi_release;
#endif

   /* Init capabilities */
   rail->cap = 0;
   rail->cap |= LCR_IFACE_CAP_RMA;
   rail->cap |= LCR_IFACE_CAP_REMOTE;


   __add_node_local_routes(rail);

	*iface_p = rail;
err:
	return rc;
}

lcr_component_t shm_component =
{
	.name          = { "shm"    },
	.rail_name     = { "shmmpi" },
	.query_devices = mpc_shm_query_devices,
	.iface_open    = mpc_shm_iface_open,
	.devices       = NULL,
	.num_devices   = 0,
	.flags         = 0,
	.next          = NULL
};

LCR_COMPONENT_REGISTER(&shm_component)
