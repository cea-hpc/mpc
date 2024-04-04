/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include "lcp_mem.h"

#include <mpc_common_datastructure.h>
#include <mpc_common_debug.h>
#include <mpc_common_spinlock.h>

#include "lcp_context.h"
#include "lcp_manager.h"
#include "lcp_def.h"
#include "lcp_prototypes.h"

#include <sctk_alloc.h>
#include <rail.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

/*****************************
 * IMPLEMENTATION OF THE MMU *
 *****************************/

#define MPC_LCP_MMU_HT_SIZE 4096

#define MPC_LCP_MMU_MAX_ENTRIES 1024
#define MPC_LCP_MMU_MAX_SIZE (size_t)(1024*1024*1024)

//FIXME: change doc from ctx to mngr

struct lcp_pinning_entry
{
        mpc_common_spinlock_t lock;
        uint64_t refcount;
        const void * start;
        size_t size;
        lcp_manager_h mngr;
        lcp_mem_h mem_entry;

        struct lcp_pinning_entry *prev;
        struct lcp_pinning_entry *next;
};

struct lcp_pinning_entry * lcp_pinning_entry_new(const void * start, size_t size, lcp_manager_h mngr, lcp_mem_h mem_entry)
{
        struct lcp_pinning_entry * ret = sctk_malloc(sizeof(struct lcp_pinning_entry));
        assume(ret);

        ret->refcount = 0;
        mpc_common_spinlock_init(&ret->lock, 0);

        ret->start = start;
        ret->size = size;
        ret->mngr = mngr;
        ret->mem_entry = mem_entry;
        /* Backlink to the MMU CTX here */
        mem_entry->pointer_to_mmu_ctx = ret;

        return ret;
}

int lcp_pinning_entry_contains(struct lcp_pinning_entry *entry, 
                               const void * start, size_t size, bmap_t bitmap)
{
        if( (entry->start <= start) && ((start + size) <= (entry->start + entry->size))
            && (entry->size == size) && MPC_BITMAP_AND(entry->mem_entry->bm, bitmap))
        {
                mpc_common_nodebug("(%p - %lld) is in (%p -- %lld)", start, size, entry->start, entry->size);
                return 1;
        }

        return 0;
}

void lcp_pinning_entry_acquire(struct lcp_pinning_entry *entry)
{
        mpc_common_spinlock_lock(&entry->lock);
        entry->refcount++;
        mpc_common_spinlock_unlock(&entry->lock);
}

void lcp_pinning_entry_relax(struct lcp_pinning_entry *entry)
{
        mpc_common_spinlock_lock(&entry->lock);
        entry->refcount--;
        mpc_common_spinlock_unlock(&entry->lock);
}

uint64_t lcp_pinning_entry_refcount(struct lcp_pinning_entry *entry)
{
        uint64_t ret = 0;
        mpc_common_spinlock_lock(&entry->lock);
        ret = entry->refcount;
        mpc_common_spinlock_unlock(&entry->lock);

        return ret;
}

int lcp_pinning_entry_list_init(struct lcp_pinning_entry_list *list)
{
        list->entry = NULL;
        list->entries_count = 0;
        list->total_size = 0;

        list->max_entries_count = _mpc_lowcomm_config_proto_get()->max_mmu_entries;
        list->max_total_size = _mpc_lowcomm_config_proto_get()->mmu_max_size;

        mpc_common_rw_lock_init(&list->lock);
        return 0;
}

int lcp_pinning_entry_list_release(struct lcp_pinning_entry_list *list, int (*free_cb)(struct lcp_pinning_entry *entry))
{
        int rc = MPC_LOWCOMM_SUCCESS;
        struct lcp_pinning_entry * tmp = list->entry;
        struct lcp_pinning_entry * to_free = NULL;

        while(tmp)
        {
                to_free = tmp;
                tmp = tmp->next;

                if(free_cb)
                {
                        rc = (free_cb)(to_free);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                }

                sctk_free(to_free);
        }

err:
        return rc;
}


struct lcp_pinning_entry * lcp_pinning_entry_list_find_no_lock(struct lcp_pinning_entry_list * list, 
                                                               const void * start, size_t size, bmap_t bitmap)
{
        struct lcp_pinning_entry * ret = NULL;

        struct lcp_pinning_entry * tmp = list->entry;

        while(tmp)
        {
                if(lcp_pinning_entry_contains(tmp, start, size, bitmap))
                {
                        ret = tmp;
                        /* We acquire in the read lock to avoid races */
                        lcp_pinning_entry_acquire(ret);

                        break;
                }
                tmp = tmp->next;
        }

        return ret;
}

struct lcp_pinning_entry * lcp_pinning_entry_list_find(struct lcp_pinning_entry_list * list, 
                                                       const void * start, 
                                                       size_t size, bmap_t bitmap)
{
        struct lcp_pinning_entry * ret = NULL;

        mpc_common_spinlock_read_lock(&list->lock);

        ret = lcp_pinning_entry_list_find_no_lock(list, start, size, bitmap);

        mpc_common_spinlock_read_unlock(&list->lock);

        return ret;
}



int __lcp_pinning_entry_list_remove(struct lcp_pinning_entry_list * list, struct lcp_pinning_entry *entry, int do_lock)
{
        if(do_lock)
        {
                mpc_common_spinlock_write_lock(&list->lock);
        }

        /* Looked up entry is first elem directly remove */
        if(!entry->prev)
        {
                assume(list->entry == entry);
                list->entry = entry->next;
        }
        else
        {
                entry->prev->next = entry->next;
                if(entry->next)
                {
                        entry->next->prev = entry->prev;
                }
        }

        list->entries_count--;
        list->total_size -= entry->size;

        if(do_lock)
        {
                mpc_common_spinlock_write_unlock(&list->lock);
        }

        lcp_pinning_mmu_unpin(entry->mngr, entry->mem_entry);
        sctk_free(entry);

        return 0;
}


int lcp_pinning_entry_list_remove_no_lock(struct lcp_pinning_entry_list * list, struct lcp_pinning_entry *entry)
{
        return __lcp_pinning_entry_list_remove(list, entry, 0);
}

int lcp_pinning_entry_list_remove(struct lcp_pinning_entry_list * list, struct lcp_pinning_entry *entry)
{
        return __lcp_pinning_entry_list_remove(list, entry, 1);
}


int lcp_mem_register_with_bitmap(lcp_manager_h mngr,
                                lcp_mem_h *mem_p,
                                bmap_t bitmap,
                                const void *buffer,
                                size_t length,
                                unsigned flags);

int lcp_pinning_entry_list_decimate_no_lock(struct lcp_pinning_entry_list * list, ssize_t size_to_decimate)
{
        struct lcp_pinning_entry * last = list->entry;

        if(last)
        {
                while(last->next)
                {
                        last = last->next;
                }
        }

        /* Now we can process from the older elements */
        while(last && (0<=size_to_decimate))
        {
                struct lcp_pinning_entry * upper = last->prev;

                if(lcp_pinning_entry_refcount(last) == 0)
                {
                        size_to_decimate -= (long int)last->size;
                        mpc_common_tracepoint_fmt("PINNING removing a buffer of %lld", last->size);
                        lcp_pinning_entry_list_remove_no_lock(list, last);
                }

                last = upper;
        }

        return 0;
}

struct lcp_pinning_entry * lcp_pinning_entry_list_push(struct lcp_pinning_entry_list * list, 
                                                       const void * buffer, size_t len, lcp_manager_h mngr, 
                                                       bmap_t bitmap, unsigned flags)
{
        struct lcp_pinning_entry * ret = NULL;

        mpc_common_spinlock_write_lock(&list->lock);

        ret = lcp_pinning_entry_list_find_no_lock(list, buffer, len, bitmap);

        if(!ret)
        {
                /* Check capacity */
                if(list->max_entries_count <= list->entries_count)
                {
                        /* Decimate one */
                        mpc_common_debug("PINNING Decimate one due to count limit");
                        lcp_pinning_entry_list_decimate_no_lock(list, 0);
                }

                if(list->max_total_size <= (list->total_size + len))
                {
                        mpc_common_debug("PINNING Decimate many due to size limit MAXSIZE %llu CURRENTTOTAL %llu REQLEN %llu", list->max_total_size, list->total_size, len);

                        lcp_pinning_entry_list_decimate_no_lock(list, (list->total_size + len) - list->max_total_size);
                }

                lcp_mem_h mem_p = NULL;
                lcp_mem_register_with_bitmap(mngr,
                        &mem_p,
                        bitmap,
                        buffer,
                        len, flags);
                ret = lcp_pinning_entry_new(buffer, len, mngr, mem_p);
                lcp_pinning_entry_acquire(ret);

                ret->prev = NULL;
                ret->next = list->entry;

                if(list->entry)
                {
                        list->entry->prev = ret;
                }

                list->entry = ret;

                list->entries_count++;
                list->total_size += len;

                mpc_common_debug("PINNING new segment for size %ld @ %p [MMU count : %llu, MMU TOTAL %g MB]", len, buffer, list->entries_count, (double)list->total_size/(1024.0*1024.0));

        }

        mpc_common_spinlock_write_unlock(&list->lock);

        return ret;
}

//FIXME: void return type instead?
int lcp_pinning_mmu_init(struct lcp_pinning_mmu **mmu_p, unsigned flags)
{
        UNUSED(flags); //NOTE: for later optimization.
        int rc = MPC_LOWCOMM_SUCCESS;
        struct lcp_pinning_mmu *mmu = NULL;

        mmu = sctk_malloc(sizeof(struct lcp_pinning_mmu));
        if (mmu == NULL) {
                mpc_common_debug_error("LCP MEM: could not allocate MMU.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        lcp_pinning_entry_list_init(&mmu->list);

        *mmu_p = mmu;
err:
        return rc;
}

int __unpin(struct lcp_pinning_entry *entry)
{
        return lcp_mem_deregister(entry->mngr, entry->mem_entry);
}

int lcp_pinning_mmu_release(struct lcp_pinning_mmu *mmu)
{
        lcp_pinning_entry_list_release(&mmu->list, __unpin);

        sctk_free(mmu);
        mmu = NULL;

        return MPC_LOWCOMM_SUCCESS;
}


//NOTE: implements a FIFO like caching algorithm.
lcp_mem_h lcp_pinning_mmu_pin(lcp_manager_h mngr, const void *addr, 
                              size_t size, bmap_t bitmap, unsigned flags)
{
        struct lcp_pinning_entry * exists = lcp_pinning_entry_list_find(&mngr->mmu->list, addr, size, bitmap);

        if(exists)
        {
                /* Was acquired in the find */
                mpc_common_debug("MEM: pinning entry exists: addr=%p, "
                                 "size=%llu, bitmap=%x", addr, size, bitmap);
                return exists->mem_entry;
        }

        exists = lcp_pinning_entry_list_push(&mngr->mmu->list, addr, 
                                             size, mngr, bitmap, flags);
        assume(exists);

        return exists->mem_entry;
}

int lcp_pinning_mmu_unpin(lcp_manager_h mngr, lcp_mem_h mem)
{
        UNUSED(mngr);
        struct lcp_pinning_entry * mmu_entry = (struct lcp_pinning_entry *)mem->pointer_to_mmu_ctx;
        assert(mmu_entry != NULL);
        lcp_pinning_entry_relax(mmu_entry);

        /* Remove from list. */
        mpc_common_spinlock_lock(&mngr->mngr_lock);
        mpc_list_del(&mmu_entry->mem_entry->elem);
        mpc_common_spinlock_unlock(&mngr->mngr_lock);

        return 0;
}

/**
 * @brief Pack data as rkey buffer.
 *
 * @param ctx lcp context
 * @param mem data to pack
 * @param dest packed rkey buffer
 * @return size_t length of output buffer
 */
//FIXME: A memory range described by lcp_mem_h is unique (same address, same
//       length. As a consequence, transport memory key will be identical for
//       homogenous tranports, so no need to pack it for all rail in case of
//       multirail.
size_t lcp_mem_rkey_pack(lcp_manager_h mngr, lcp_mem_h mem, void *dest)
{
        int i;
        void *rkey_buf;
        size_t packed_size = 0;
        sctk_rail_info_t *iface = NULL;

        rkey_buf = dest;
        *(bmap_t *)rkey_buf = mem->bm;
        packed_size += sizeof(bmap_t);
        for (i=0; i < mngr->num_ifaces; i++) {
                if (MPC_BITMAP_GET(mem->bm, i)) {
                        iface = mngr->ifaces[i];
                        //FIXME: no error handling in case returned size is < 0
                        packed_size += iface->iface_pack_memp(iface, &mem->mems[i],
                                                              rkey_buf + packed_size);
                }
        }

        return packed_size;
}

/**
 * @brief Pack data as rkey and return buffer size.
 *
 * @param ctx lcp context
 * @param mem memory to pack
 * @param rkey_buf_p output rkey buffer
 * @param rkey_len output rkey buffer length
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_mem_pack(lcp_manager_h mngr, lcp_mem_h mem, 
                 void **rkey_buf_p, int *rkey_len)
{
        int i;
        size_t packed_size = 0;
        lcr_rail_attr_t attr;
        void *rkey_buf;
        sctk_rail_info_t *iface = NULL;

        packed_size += sizeof(bmap_t);
        for (i = 0; i < mngr->num_ifaces; i++) {
                if (MPC_BITMAP_GET(mem->bm, i)) {
                        iface = mngr->ifaces[i];
                        iface->iface_get_attr(iface, &attr);

                        packed_size += attr.mem.size_packed_mkey;
                }
        }

        rkey_buf = sctk_malloc(packed_size);

        lcp_mem_rkey_pack(mngr, mem, rkey_buf);

        *rkey_buf_p = rkey_buf;
        *rkey_len   = packed_size;

        return MPC_LOWCOMM_SUCCESS;
}

void lcp_mem_release_rkey_buf(void *rkey_buffer)
{
        sctk_free(rkey_buffer);
}

int lcp_mem_unpack(lcp_manager_h mngr, lcp_mem_h *mem_p, 
                   void *src, size_t size)
{
        int i, rc = MPC_LOWCOMM_SUCCESS;
        size_t unpacked_size = 0;
        sctk_rail_info_t *iface = NULL;
        void *p = src;
        lcp_mem_h mem;

        mem = sctk_malloc(sizeof(struct lcp_mem));
        if (mem == NULL) {
                mpc_common_debug_error("LCP: could not allocate memory domain.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        mem->mems       = sctk_malloc(mngr->num_ifaces * sizeof(lcr_memp_t));
        mem->num_ifaces = 0;
        if (mem->mems == NULL) {
                mpc_common_debug_error("LCP: could not allocate memory pins.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(mem->mems, 0, mngr->num_ifaces * sizeof(lcr_memp_t));

        mem->bm = *(bmap_t *)p; unpacked_size += sizeof(bmap_t);
        for (i=0; i<mngr->num_ifaces; i++) {
                if (MPC_BITMAP_GET(mem->bm, i)) {
                        iface = mngr->ifaces[i];
                        unpacked_size += iface->iface_unpack_memp(iface, 
                                                                  &mem->mems[i], 
                                                                  p + unpacked_size);
                        mem->num_ifaces++;
                        if (unpacked_size == size) {
                                break;
                        }
                }
        }

        mpc_common_debug_log("LCP: Memory unpacking, size=%lu, unpacked_size=%lu", size, unpacked_size);
        assert(size == unpacked_size);

        *mem_p = mem;
err:
        return rc;
}

int lcp_mem_create(lcp_manager_h mngr, lcp_mem_h *mem_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_mem_h mem;

        mem = sctk_malloc(sizeof(struct lcp_mem));
        if (mem == NULL) {
                mpc_common_debug_error("LCP: could not allocate memory domain");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        //NOTE: allocation on the number of interfaces even though the memories
        //      depend on the bitmap. Fits better with the bitmap.
        mem->mems       = sctk_malloc(mngr->num_ifaces * sizeof(lcr_memp_t));
        mem->num_ifaces = 0; // num_iface is set later based on bitmap.
        mem->pointer_to_mmu_ctx = NULL;
        if (mem->mems == NULL) {
                mpc_common_debug_error("LCP: could not allocate memory pins");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(mem->mems, 0, mngr->num_ifaces * sizeof(lcr_memp_t));

        *mem_p = mem;
err:
        return rc;
}

void lcp_mem_delete(lcp_mem_h mem)
{
        /* First, remove memory from the active list. */
        mpc_common_spinlock_lock(&mem->mngr->mngr_lock);
        mpc_list_del(&mem->elem);
        mpc_common_spinlock_lock(&mem->mngr->mngr_lock);

        sctk_free(mem->mems);
        sctk_free(mem);
        mem = NULL;
}

int lcp_mem_reg_from_map(lcp_manager_h mngr,
                         lcp_mem_h mem,
                         bmap_t bm,
                         const void *buffer,
                         size_t length,
                         unsigned flags)
{
        int i, rc = MPC_LOWCOMM_SUCCESS;
        sctk_rail_info_t *iface = NULL;

        /* Pin the memory and create memory handles */
        for (i=0; i<mngr->num_ifaces; i++) {
                if (MPC_BITMAP_GET(bm, i)) {
                        iface = mngr->ifaces[i];

                        if (!(iface->cap & LCR_IFACE_CAP_RMA)) {
                                mpc_common_debug_error("LCP MEM: iface %s does "
                                                       "not have RMA capabilities",
                                                       iface->network_name);
                                rc = MPC_LOWCOMM_ERROR;
                                goto err;
                        }

                        rc = lcp_iface_do_register_mem(iface, &mem->mems[i], 
                                                       buffer, length,
                                                       flags);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                        mem->num_ifaces++;
                }
        }

err:
        return rc;
}

int lcp_mem_register_with_bitmap(lcp_manager_h mngr,
                                 lcp_mem_h *mem_p,
                                 bmap_t bitmap,
                                 const void *buffer,
                                 size_t length,
                                 unsigned flags)
{
        lcp_mem_h mem;
        int rc = lcp_mem_create(mngr, &mem);

        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        mem->length = length;
        mem->base_addr = (uint64_t)buffer;
        mem->bm = bitmap;

        rc = lcp_mem_reg_from_map(mngr, mem, bitmap, 
                                  buffer, length, flags); 
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        *mem_p = mem;

err:
        return rc;
}


//FIXME: what append if a memory could not get registered ? Miss error handling:
//       if a subset could not be registered, perform the communication on the
//       successful memory pins ?
int lcp_mem_register(lcp_manager_h mngr,
                     lcp_mem_h *mem_p,
                     lcp_mem_param_t *param)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        lcp_mem_h mem;
        bmap_t bm = MPC_BITMAP_INIT;
        lcr_rail_attr_t attr;

        if (param == NULL) {
                mpc_common_debug_error("LCP MEM: param field must be set.");
                return LCP_ERROR; 
        }

        if (!(param->field_mask & LCP_MEM_SIZE_FIELD) ) {
                mpc_common_debug_error("LCP MEM: must specify size field.");
                return LCP_ERROR;
        }
        
        if (!(param->field_mask & LCP_MEM_ADDR_FIELD) ) {
                mpc_common_debug_error("LCP MEM: must specify address field.");
                return LCP_ERROR;
        }

        if (param->address == NULL && !(param->flags & LCP_MEM_REGISTER_ALLOCATE)) {
                mpc_common_debug_error("LCP MEM: must specify address field.");
                not_implemented();
        }

        if (param->flags & LCP_MEM_REGISTER_ALLOCATE) {
                not_implemented();
        }

        /* Compute bitmap registration. Strategy is to register on all
         * interfaces that have the capability. */
        for (i = 0; mngr->num_ifaces; i++) {
		mngr->ifaces[i]->iface_get_attr(mngr->ifaces[i], &attr);
                
                if (attr.iface.cap.flags & LCR_IFACE_CAP_RMA) {
                        MPC_BITMAP_SET(bm, i);
                }
        }

        mem = lcp_pinning_mmu_pin(mngr, param->address, param->size, 
                                  bm, param->flags);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        *mem_p = mem;

err:
        return rc;
}

int lcp_mem_query(lcp_mem_h mem, lcp_mem_attr_t *attr)
{
       if (attr->field_mask & LCP_MEM_ADDR_FIELD) {
               attr->address = (void *)mem->base_addr;
       }

       if (attr->field_mask & LCP_MEM_SIZE_FIELD) {
               attr->size = mem->length;
       }

       return LCP_SUCCESS;
}

/**
 * @brief Post a buffer
 *
 * @param ctx lcp context
 * @param mem_p memory pointer
 * @param buffer buffer to post
 * @param length length of buffer
 * @param tag tag of post
 * @param flags flag of post
 * @param tag_ctx tag context
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_mem_post_from_map(lcp_manager_h mngr, 
                          lcp_mem_h mem, 
                          bmap_t bm,
                          void *buffer,
                          size_t length,
                          lcr_tag_t tag,
                          unsigned flags,
                          lcr_tag_context_t *tag_ctx)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i = 0, k, nb_posted = 0, nb_ifaces = 0;
        lcr_rail_attr_t attr;
        lcr_tag_t ign = { 0 };
        size_t iovcnt = 0;
        struct iovec iov[1];
        sctk_rail_info_t *iface = mngr->ifaces[mngr->priority_iface];

        //FIXME: check 0 length memory registration

        /* Count the number of interface to use */
        for (k = 0; k < mngr->num_ifaces; k++) {
                if (MPC_BITMAP_GET(bm, i)) {
                        nb_ifaces++;
                }
        }

        mem->length     = length;
        mem->base_addr  = (uint64_t)buffer;
        mem->flags      = flags;
        mem->bm         = bm;

        iov[0].iov_base = buffer;
        iov[0].iov_len  = length;
        iovcnt++;

        iface->iface_get_attr(iface, &attr);

        //FIXME: this behavior is multirail specific while is should not be.
        while (length > nb_posted * attr.iface.cap.rndv.max_get_zcopy) {
                if (!MPC_BITMAP_GET(bm, i)) {
                        i = (i + 1) % mngr->num_ifaces;
                        continue;
                }
                iface = mngr->ifaces[i];
                rc = iface->post_tag_zcopy(iface, tag, ign, iov, 
                                           iovcnt, flags, tag_ctx);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP MEM: could not post on "
                                               "interface");
                        goto err;
                }

                /* If memory is persistent, only one post per iface. */
                if ((++nb_posted == nb_ifaces) &&
                    (flags & LCR_IFACE_TM_PERSISTANT_MEM)) {
                        break;
                }
                i++;
        }

err:
        return rc;
}

int lcp_mem_unpost(lcp_manager_h mngr, lcp_mem_h mem, lcr_tag_t tag) 
{
        int i;
        int rc = MPC_LOWCOMM_SUCCESS;
        sctk_rail_info_t *iface = NULL;

        /* Memory should be unposted only if it has been previously posted as
         * persistent */
        if (!(mem->flags & LCR_IFACE_TM_PERSISTANT_MEM)) {
                return MPC_LOWCOMM_SUCCESS;
        }

        for (i=0; i < mngr->num_ifaces; i++) {
                if (MPC_BITMAP_GET(mem->bm, i)) {
                        iface = mngr->ifaces[i];
                        rc = iface->unpost_tag_zcopy(iface, tag);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                               goto err;
                        }
                }
        }

        lcp_mem_delete(mem);

err:
        return rc;
}

int lcp_mem_deregister(lcp_manager_h mngr, lcp_mem_h mem)
{
        int i;
        int rc = MPC_LOWCOMM_SUCCESS;

        for (i=0; i<mngr->num_ifaces; i++) {
                if (MPC_BITMAP_GET(mem->bm, i)) {
                        rc = lcp_iface_do_unregister_mem(mngr->ifaces[i],
                                                         &mem->mems[i]);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                }
        }

        lcp_mem_delete(mem);

err:
        return rc;
}
