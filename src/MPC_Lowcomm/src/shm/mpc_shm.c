#include "mpc_shm.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <lcr/lcr_component.h>
#include "mpc_common_datastructure.h"
#include "mpc_common_debug.h"
#include <sctk_alloc.h>

#include <mpc_lowcomm_monitor.h>
#include <mpc_launch_shm.h>
#include <mpc_launch_pmi.h>
#include <string.h>

#include "mpc_common_rank.h"
#include "mpc_common_spinlock.h"
#include "mpc_lowcomm_types.h"
#include "rail.h"
#include "uthash.h"

/*************************
 * FRAGMENTED OPERATIONS *
 *************************/

int _mpc_shm_fragment_factory_init(struct _mpc_shm_fragment_factory *factory)
{
   mpc_common_spinlock_init(&factory->lock, 0);
   factory->fragment_id = 1;
   factory->segment_id = 1;
   mpc_common_hashtable_init(&factory->frags, 64);
   mpc_common_hashtable_init(&factory->regs, 64);

   mpc_mempool_init(&factory->frag_pool, 1, 32, sizeof(struct _mpc_shm_fragment), sctk_malloc, sctk_free);
   mpc_mempool_init(&factory->reg_pool, 1, 32, sizeof(struct _mpc_shm_region), sctk_malloc, sctk_free);

   return 0;
}

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

struct _mpc_shm_region * _mpc_shm_fragment_factory_get_segment(struct _mpc_shm_fragment_factory *factory, uint64_t id)
{
   return mpc_common_hashtable_get(&factory->regs, id);
}

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

   assume(0 <= sizeleft);

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

static inline void ___mpc_shm_list_head_push_no_lock(struct _mpc_shm_list_head * list, struct _mpc_shm_cell *cell)
{
   assert(cell->next == NULL);
   assert(cell->prev == NULL);

   if(!list->head)
   {
      list->tail = cell;
   }
   else
   {
      list->head->prev = cell;
   }

   cell->prev = NULL;
   cell->next = list->head;
   list->head = cell;
}


static inline void _mpc_shm_list_head_push(struct _mpc_shm_list_head * list, struct _mpc_shm_cell *cell)
{
   mpc_common_spinlock_lock(&list->lock);

   ___mpc_shm_list_head_push_no_lock(list, cell);

   mpc_common_spinlock_unlock(&list->lock);
}

static inline int _mpc_shm_list_head_try_push(struct _mpc_shm_list_head * list, struct _mpc_shm_cell *cell)
{
   if( mpc_common_spinlock_trylock(&list->lock) == 0)
   {

      ___mpc_shm_list_head_push_no_lock(list, cell);

      mpc_common_spinlock_unlock(&list->lock);

      return 0;
   }

   return 1;
}


void _mpc_shm_list_head_init(struct _mpc_shm_list_head *head)
{
   mpc_common_spinlock_init(&head->lock, 0);
   head->head = NULL;
   head->tail = NULL;
}

static inline struct _mpc_shm_cell * _mpc_shm_list_head_try_get(struct _mpc_shm_list_head * list)
{
   struct _mpc_shm_cell * ret = NULL;

   if( mpc_common_spinlock_trylock(&list->lock) == 0)
   {
      ret = list->tail;

      if(ret)
      {
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


/***************
 * SHM STORAGE *
 ***************/


static size_t _mpc_shm_storage_get_size(unsigned int process_count, unsigned int freelist_count)
{
   size_t ret = process_count * SHM_PROCESS_ARITY * sizeof(struct _mpc_shm_list_head);
   ret += freelist_count * sizeof(struct _mpc_shm_list_head);
   ret += SHM_CELL_COUNT_PER_PROC * process_count * sizeof(struct _mpc_shm_cell);

   mpc_common_debug_error("SHM segment is %g MB", (double)ret / (1024 * 1024));

   return ret;
}

int _mpc_shm_storage_init(struct _mpc_shm_storage * storage)
{
   unsigned int process_count = mpc_common_get_local_process_count();

   if(process_count == 1)
   {
      return 1;
   }

   storage->process_count = process_count;
   storage->cell_count = SHM_CELL_COUNT_PER_PROC * process_count;
   storage->process_arity = SHM_PROCESS_ARITY;

   assume(0 < process_count);

   storage->freelist_count = process_count * SHM_FREELIST_PER_PROC;

   size_t segment_size = _mpc_shm_storage_get_size(process_count, storage->freelist_count);

   storage->shm_buffer = mpc_launch_shm_map(segment_size, MPC_LAUNCH_SHM_USE_PMI, NULL);

   assume(storage != NULL);


   assume(0 < storage->freelist_count);
   assume(storage->freelist_count < 255);

   storage->per_process = storage->shm_buffer;
   assume((void*)storage->per_process < ((void*)storage->shm_buffer + segment_size));

   storage->free_lists = storage->per_process + SHM_PROCESS_ARITY * storage->process_count;

   assume((void*)storage->per_process < (void*)storage->free_lists);
   assume((void*)storage->free_lists < ((void*)storage->shm_buffer + segment_size));


   unsigned int proc_rank = mpc_common_get_local_process_rank();

   /* Per process head do init */
   unsigned int i = 0;

   for( i = proc_rank * SHM_PROCESS_ARITY; i < (proc_rank + 1) * SHM_PROCESS_ARITY; i++)
   {
      _mpc_shm_list_head_init(&storage->per_process[i]);
   }

   struct _mpc_shm_cell * cells = (struct _mpc_shm_cell*)(storage->free_lists + storage->freelist_count);
   assume(((void*)storage->free_lists < (void*)cells) && ((void*)cells < ((void*)storage->shm_buffer + segment_size)));


   /* Only the first rank matching  the freelist does the freelist pushed to avoid bad numa effects */

   if(proc_rank < storage->freelist_count)
   {
      int rr_counter = 0;
      unsigned int per_list = storage->cell_count / storage->freelist_count;
      unsigned int leftover = storage->cell_count - (per_list * storage->freelist_count);

      if(!proc_rank)
      {
         for(i = 0; i < leftover; i++)
         {
            cells[i].next = NULL;
            cells[i].prev = NULL;
            _mpc_shm_list_head_push(&storage->free_lists[0 + rr_counter], &cells[i]);
            cells[i].free_list = 0;
            rr_counter = (rr_counter + 1)% (int)SHM_FREELIST_PER_PROC;
         }
      }

      for( i = leftover + proc_rank * per_list ; i < (proc_rank + 1) * per_list + leftover; i++)
      {
         cells[i].next = NULL;
         cells[i].prev = NULL;
         _mpc_shm_list_head_push(&storage->free_lists[proc_rank * SHM_FREELIST_PER_PROC + rr_counter], &cells[i]);
         cells[i].free_list = (char)proc_rank;
         rr_counter = (rr_counter + 1)% (int)SHM_FREELIST_PER_PROC;
      }

   }

   return 0;
}

int _mpc_shm_storage_release(struct _mpc_shm_storage *storage);


#define MPC_SHM_MAX_GET_TRY 16

struct _mpc_shm_cell * _mpc_shm_storage_get_free_cell(struct _mpc_shm_storage *storage)
{
   int my_rank = mpc_common_get_local_process_rank();

   struct _mpc_shm_cell * ret = NULL;

   unsigned int target_list = my_rank % storage->freelist_count;
   int cnt = 0;

   while( !(ret = _mpc_shm_list_head_try_get(&storage->free_lists[target_list])))
   {
      cnt++;
      if(cnt > MPC_SHM_MAX_GET_TRY)
      {
            target_list = (target_list + 1) % storage->freelist_count;
            cnt = 0;
      }
   }

   return ret;
}


void _mpc_shm_storage_send_cell(struct _mpc_shm_storage * storage,
                               mpc_lowcomm_peer_uid_t uid,
                               struct _mpc_shm_cell * cell)
{
   int my_rank = mpc_common_get_local_process_rank();

   assert(0 <= my_rank);

   assume(mpc_lowcomm_monitor_get_gid() == mpc_lowcomm_peer_get_set(uid));

   unsigned int target_rank = mpc_lowcomm_peer_get_rank(uid);
   unsigned int target_list = (size_t)target_rank * storage->process_arity +  (my_rank % storage->process_arity);


   /* Pick one of the ARIRY incoming list */
   struct _mpc_shm_list_head * list = &storage->per_process[target_list];
   unsigned int cnt = 0;

   while(1)
   {
      if(mpc_common_spinlock_trylock(&list->lock) == 0)
      {
         ___mpc_shm_list_head_push_no_lock(list, cell);
         mpc_common_spinlock_unlock(&list->lock);
         break;
      }

      cnt++;
      target_list = (size_t)target_rank * storage->process_arity +  ((my_rank + cnt) % storage->process_arity);
   }
}

void _mpc_shm_storage_free_cell(struct _mpc_shm_storage * storage,
                                struct _mpc_shm_cell * cell)
{
   unsigned char dest = cell->free_list;

   int did_push = 0;

   do
   {
      if(_mpc_shm_list_head_try_push(&storage->free_lists[dest], cell) == 0)
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
   list->pin.shm = _mpc_shm_fragment_factory_register_segment(&rail->network.shm.frag_factory, addr, size);
}

void mpc_shm_unpin(struct sctk_rail_info_s *rail, struct sctk_rail_pin_ctx_list *list)
{
   _mpc_shm_fragment_factory_unregister_segment(&rail->network.shm.frag_factory, list->pin.shm);
}


int mpc_shm_pack_rkey(sctk_rail_info_t *rail,
                      lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        void *p = dest;
        *(uint64_t *)p = memp->pin.shm;
        return sizeof(uint64_t);
}

int mpc_shm_unpack_rkey(sctk_rail_info_t *rail,
                        lcr_memp_t *memp, void *dest)
{
        UNUSED(rail);
        void *p = dest;
        memp->pin.shm = *(uint64_t *)p;

        return sizeof(uint64_t);
}




/***********************
 * MESSAGING FUNCTIONS *
 ***********************/

int mpc_shm_send_am_zcopy(_mpc_lowcomm_endpoint_t *ep,
                                    uint8_t id,
                                    const void *header,
                                    unsigned header_length,
                                    const struct iovec *iov,
                                    size_t iovcnt,
                                    unsigned flags,
                                    lcr_completion_t *comp)
{
   not_implemented();
}


ssize_t mpc_shm_send_am_bcopy(_mpc_lowcomm_endpoint_t *ep,
                              uint8_t id,
                              lcr_pack_callback_t pack,
                              void *arg,
                              unsigned flags)
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
                              ep->dest,
                              cell);

   return payload_length;
}

static inline void __deffered_completion_cb(void *pcomp)
{
   lcr_completion_t *comp = (lcr_completion_t *)pcomp;
   comp->comp_cb(comp);
}

int mpc_shm_get_zcopy(_mpc_lowcomm_endpoint_t *ep,
                           uint64_t local_addr,
                           uint64_t remote_offset,
                           lcr_memp_t *remote_key,
                           size_t size,
                           lcr_completion_t *comp) 
{
   sctk_rail_info_t *rail = ep->rail;

   comp->sent = size;

   uint64_t fragment_id = _mpc_shm_fragment_init(&rail->network.shm.frag_factory, 
                                              remote_key->pin.shm,
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
   get->segment_id = remote_key->pin.shm;
   get->offset = remote_offset;
   get->size = size;

   _mpc_shm_storage_send_cell(&rail->network.shm.storage,
                              ep->dest,
                              cell);

   return MPC_LOWCOMM_SUCCESS;
}





/*****************************
 * INCOMING MESSAGE HANDLING *
 *****************************/


int ____handle_incoming_eager(struct _mpc_shm_storage * storage, sctk_rail_info_t *rail, _mpc_shm_am_hdr_t *hdr )
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


int ____handle_incoming_get(struct _mpc_shm_storage * storage, sctk_rail_info_t *rail, _mpc_shm_am_hdr_t *hdr)
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

      _mpc_shm_storage_send_cell(&rail->network.shm.storage,
                                 get->source,
                                     cell);


      sent += to_send;
   }

   return MPC_LOWCOMM_SUCCESS;
}

int ____handle_incoming_frag(struct _mpc_shm_storage * storage, sctk_rail_info_t *rail, _mpc_shm_am_hdr_t *hdr)
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
      struct _mpc_shm_cell * cell = _mpc_shm_list_head_try_get(&rail->network.shm.storage.per_process[i]);

      if(cell)
      {
         rc = __handle_incoming_message(&rail->network.shm.storage, rail, cell);
         break;
      }
   }

   return rc;
}

/********************
 * INIT AND RELEASE *
 ********************/


void mpc_shm_connect_on_demand(struct sctk_rail_info_s *rail, mpc_lowcomm_peer_uid_t dest)
{
   mpc_common_debug_error("On demand to %llu", dest);
   not_available();
}


void __add_route(mpc_lowcomm_peer_uid_t dest_uid, sctk_rail_info_t *rail)
{
	_mpc_lowcomm_endpoint_t *new_route = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_t) );
	assume(new_route != NULL);
	_mpc_lowcomm_endpoint_init(new_route, dest_uid, rail, _MPC_LOWCOMM_ENDPOINT_DYNAMIC);

   //mpc_mempool_init(&new_route->data.shm.zcopy, 10, 100, sizeof(struct mpc_shm_deffered_completion_s), sctk_malloc, sctk_free);

	/* Make sure the route is flagged connected */
	_mpc_lowcomm_endpoint_set_state(new_route, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
   sctk_rail_add_dynamic_route(rail, new_route);
   mpc_common_debug_error("Add route to %llu", dest_uid);
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

   for(i = 0 ; i < my_node->nb_process; i++)
   {
      __add_route(mpc_lowcomm_monitor_local_uid_of(my_node->process_list[i]), rail);
   }

}

int mpc_shm_get_attr(sctk_rail_info_t *rail,
                     lcr_rail_attr_t *attr)
{
	attr->iface.cap.am.max_iovecs = 1;
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

   if(mpc_common_get_local_process_count() == 1)
   {
      *num_devices_p = 0;
      return MPC_LOWCOMM_SUCCESS;
   }

   *devices_p = sctk_malloc(sizeof(lcr_device_t));
   snprintf((*devices_p)->name, LCR_DEVICE_NAME_MAX, "shmsegment");
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

   mpc_common_debug_error("Process %d over %d", mpc_common_get_local_process_rank(), mpc_common_get_local_process_count());

   rail->connect_on_demand = mpc_shm_connect_on_demand;
   rail->send_am_bcopy = mpc_shm_send_am_bcopy;
   rail->iface_progress = mpc_shm_progress;
   rail->rail_pin_region = mpc_shm_pin;
   rail->rail_unpin_region = mpc_shm_unpin;
   rail->iface_pack_memp = mpc_shm_pack_rkey;
   rail->iface_unpack_memp = mpc_shm_unpack_rkey;
   rail->get_zcopy = mpc_shm_get_zcopy;
   rail->send_am_zcopy = mpc_shm_send_am_zcopy;

#if 0
   rail->driver_finalize = mpc_ofi_release;

   rail->put_zcopy = mpc_ofi_send_put_zcopy;

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