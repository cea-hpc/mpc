/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:33:54 CEST 2021                                        # */
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
/* # - Adrien Roussel <adrien.roussel@cea.fr>                             # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* #                                                                      # */
/* ######################################################################## */
#include <mpc_config.h>
#include "mpcomp_alloc.h"
#include "mpcomp_core.h"

#include <mpc_topology.h>

static mpc_omp_alloc_list_t mpcomp_global_allocators;

#if MPC_HAVE_LIBVMEM
  #include <libvmem.h>
  static VMEM *vmp;
#endif

static inline void mpc_omp_alloc_init_default_trait(mpc_omp_alloc_t * alloc)
{
  int i;
  for(i = omp_atk_sync_hint; i <= omp_atk_partition; ++i)
  {
    int idx = i - 1;
    alloc->traits[idx].key = i;

    switch (i)
    {
      case omp_atk_sync_hint :
        alloc->traits[idx].value = omp_atv_contended & omp_atv_sequential;
        break;

      case omp_atk_alignment :
        alloc->traits[idx].value = 32;
        break;

      case omp_atk_access :
        alloc->traits[idx].value = omp_atv_all;
        break;

      case omp_atk_pool_size :
        alloc->traits[idx].value = 0; /* pool size to def */
        break;

      case omp_atk_fallback :
        alloc->traits[idx].value = omp_atv_default_mem_fb;
        break;

      case omp_atk_fb_data :
        alloc->traits[idx].value = 0x0;
        break;

      case omp_atk_pinned :
        alloc->traits[idx].value = omp_atv_false;
        break;

      case omp_atk_partition :
        alloc->traits[idx].value = omp_atv_environment;
        break;

      default:
        break;
    }
  }
}

void
mpc_omp_alloc_init_allocators()
{

  assume(mpcomp_global_allocators.list);

	mpc_common_spinlock_init( &( mpcomp_global_allocators.lock ), 0 );

	mpc_common_spinlock_lock( &mpcomp_global_allocators.lock );

  mpc_omp_alloc_t* alloc_set = mpcomp_global_allocators.list;
  mpcomp_global_allocators.nb_init_allocators = 0;
  mpcomp_global_allocators.last_index = -1;
  mpcomp_global_allocators.recycling_info.nb_recycl_allocators = 0;

  int i;
  /* Initialize Pre-defined traits for all the allocators */
  for(i = omp_atk_sync_hint-1; i < omp_atk_partition; ++i)
  {
    mpc_omp_alloc_init_default_trait(&alloc_set[i]);
    mpcomp_global_allocators.nb_init_allocators += 1;
  }
  mpcomp_global_allocators.last_index = mpcomp_global_allocators.nb_init_allocators;

	mpc_common_spinlock_unlock( &mpcomp_global_allocators.lock );

  /* Pre-defined memory areas (See Section 2.11.2 from OpenMP 5.0 Specification */
  alloc_set[omp_default_mem_alloc].memspace = omp_default_mem_space;
  alloc_set[omp_large_cap_mem_alloc].memspace = omp_large_cap_mem_space;
  alloc_set[omp_const_mem_alloc].memspace = omp_const_mem_space;
  alloc_set[omp_high_bw_mem_alloc].memspace = omp_high_bw_mem_space;
  alloc_set[omp_low_lat_mem_alloc].memspace = omp_low_lat_mem_space;

  /* Implementation defined */
  alloc_set[omp_cgroup_mem_alloc].memspace = omp_default_mem_space;
  alloc_set[omp_pteam_mem_alloc].memspace = omp_default_mem_space;
  alloc_set[omp_thread_mem_alloc].memspace = omp_default_mem_space;

  /* Set pre-defined behaviors (Section 2.11.2 Table 2.10 from OpenMP 5.0 Specification */
  alloc_set[omp_cgroup_mem_alloc].traits[omp_atk_access - 1].value = omp_atv_cgroup;
  alloc_set[omp_pteam_mem_alloc].traits[omp_atk_access - 1].value = omp_atv_pteam;
  alloc_set[omp_thread_mem_alloc].traits[omp_atk_access - 1].value = omp_atv_thread;

#if MPC_HAVE_LIBVMEM /* if nvmem lib is found */
  int is_nvdimm = mpc_topology_has_nvdimm();
  if(is_nvdimm)
  {
    if ((vmp = vmem_create("/pmem/node0", VMEM_MIN_POOL)) == NULL)
    {
      perror("vmem_create");
      exit(1);
    }
  }
#endif

}

omp_allocator_handle_t
omp_init_allocator(
  omp_memspace_handle_t memspace,
  int ntraits,
  const omp_alloctrait_t traits[]
)
{
  mpc_omp_init();

  omp_allocator_handle_t idx = -1;

	mpc_common_spinlock_lock( &mpcomp_global_allocators.lock );

  /* check nb list elements if there is enough place */
  if( mpcomp_global_allocators.nb_init_allocators >= MPC_OMP_MAX_ALLOCATORS)
  {
	  mpc_common_spinlock_unlock( &mpcomp_global_allocators.lock );
    return idx;
  }

  /* Is it possible to recycle an allocator from the main list ? */
  if(mpcomp_global_allocators.recycling_info.nb_recycl_allocators >= 1)
  {

    /* decr nb recycled elements */
    idx = --mpcomp_global_allocators.recycling_info.nb_recycl_allocators;
    /* pop idx from recycling queue's tail */
    idx = mpcomp_global_allocators.recycling_info.idx_list[idx];
  }
  else
  { /* Otherwise create a new entry in the main list */

    /* Pop idx from the last index of the allocator list
     * & Update last index */
    idx = ++mpcomp_global_allocators.last_index;
  }

	mpc_common_spinlock_unlock( &mpcomp_global_allocators.lock );

  /* check idx format */
  assert(idx >= 0 && idx < MPC_OMP_MAX_ALLOCATORS);

  mpcomp_global_allocators.list[idx].memspace = memspace;

  int i;
  for(i = 0; i < ntraits; ++i)
  {
    switch (traits[i].key)
    {
      case omp_atk_sync_hint :
        if((traits[i].value >= omp_atv_contended) && (traits[i].value <= omp_atv_private))
        {
          mpcomp_global_allocators.list[idx].traits[ (traits[i].key) - 1].value = traits[i].value;
        }
        break;

      case omp_atk_alignment :
        if(traits[i].value > 0)
        {
          mpcomp_global_allocators.list[idx].traits[ (traits[i].key) - 1].value = traits[i].value;
        }
        break;

      case omp_atk_access :
        if((traits[i].value >= omp_atv_all) && (traits[i].value <= omp_atv_cgroup))
        {
          mpcomp_global_allocators.list[idx].traits[ (traits[i].key) - 1].value = traits[i].value;
        }
        break;

      case omp_atk_pool_size :
        if(traits[i].value > 0)
        {
          mpcomp_global_allocators.list[idx].traits[ (traits[i].key) - 1].value = traits[i].value;
        }
        break;

      case omp_atk_fallback :
        /* TODO: Do some verifications here... */
          mpcomp_global_allocators.list[idx].traits[ (traits[i].key) - 1].value = traits[i].value;
        break;

      case omp_atk_fb_data :
        /* TODO: Do some verifications here... */
          mpcomp_global_allocators.list[idx].traits[ (traits[i].key) - 1].value = traits[i].value;
        break;

      case omp_atk_pinned :
        if((traits[i].value == omp_atv_true) || (traits[i].value == omp_atv_false))
        {
          mpcomp_global_allocators.list[idx].traits[ (traits[i].key) - 1].value = traits[i].value;
        }
        break;

      case omp_atk_partition :
        if((traits[i].value >= omp_atv_environment) && (traits[i].value <= omp_atv_interleaved))
        {
          mpcomp_global_allocators.list[idx].traits[ (traits[i].key) - 1].value = traits[i].value;
        }
        break;

      default:
        break;
    }
  }

  return idx;
}

void
omp_destroy_allocator(omp_allocator_handle_t allocator)
{
  mpc_omp_init();

	mpc_common_spinlock_lock( &mpcomp_global_allocators.lock );

  /* Remove idx from the list */
  mpcomp_global_allocators.nb_init_allocators -= 1;
  /* Default trait : Add a new entry in the recycling list */

  /* Pick the last index entry && check */
  /* Then incr the number of recycled allocators */
  int idx = mpcomp_global_allocators.recycling_info.nb_recycl_allocators++;
  assert(idx < MPC_OMP_MAX_ALLOCATORS);
  /* Add the index in the recycling list */
  mpcomp_global_allocators.recycling_info.idx_list[idx] = allocator;

	mpc_common_spinlock_unlock( &mpcomp_global_allocators.lock );
}

/*
 * This function modifies the def_allocator_var ICV
 */
void
omp_set_default_allocator(omp_allocator_handle_t allocator)
{
  mpc_omp_init();

  mpc_omp_thread_t *t;
  t = (mpc_omp_thread_t*) mpc_omp_tls;

  t->default_allocator = allocator;
}

/*
 * This function gives the values of the def_allocator_var ICV
 */
omp_allocator_handle_t
omp_get_default_allocator(void)
{
  mpc_omp_init();

  mpc_omp_thread_t *t;
  t = (mpc_omp_thread_t*) mpc_omp_tls;

  return t->default_allocator;
}

#ifdef __cplusplus
void *
omp_alloc(
  size_t size,
  omp_allocator_handle_t allocator = omp_null_allocator
)
#else
void *
omp_alloc(size_t size, omp_allocator_handle_t allocator)
#endif
{
  mpc_omp_init();

  void* ret = NULL;

  int alignment = mpcomp_global_allocators.list[allocator].traits[omp_atk_alignment - 1].value;
  int fb_value = mpcomp_global_allocators.list[allocator].traits[omp_atk_fallback - 1].value;
  int fb_data =  mpcomp_global_allocators.list[allocator].traits[omp_atk_fb_data - 1].value;
  int memspace = mpcomp_global_allocators.list[allocator].memspace;

main_treat :
  if(alignment <= 0) alignment = 32;

  if(memspace ==  omp_high_bw_mem_space)
  {
    int mcdram_node = mpc_topology_get_mcdram_node();
    if(mcdram_node != -1)
    {
      ret = (void*) sctk_malloc_on_node(size, mcdram_node);
      mpc_common_debug_info("[MCDRAM ALLOC] %d bytes allocated on node #%d (%s)\n", size, mcdram_node, __func__);
      return ret;
    }
    else
    {
      mpc_common_debug_warning("<WARNING> No MCDRAM memory bank found...\n");
      goto fallback;
    }
  }
  else if(memspace == omp_large_cap_mem_space)
  {
    if(mpc_topology_has_nvdimm() != 0)
    {
#if ENABLE_NVMEM
      ret = (void*) vmem_malloc(vmp, size);
#else
      ret = (void*) sctk_malloc(size);
#endif
    }
    else
    {
      mpc_common_debug_warning("<WARNING> No NVDIMM device found...\n");
      goto fallback;
    }
  }
  else
  {
    fallback :
    switch(fb_value)
    {
      case omp_atv_null_fb :
        ret = NULL;
        break;

      case omp_atv_abort_fb :
        assert(false);
        break;

      case omp_atv_allocator_fb :
        allocator = fb_data;

        /* Reset value */
        alignment = mpcomp_global_allocators.list[allocator].traits[omp_atk_alignment - 1].value;
        fb_value =  mpcomp_global_allocators.list[allocator].traits[omp_atk_fallback - 1].value;
        fb_data =   mpcomp_global_allocators.list[allocator].traits[omp_atk_fb_data - 1].value;
        memspace =  mpcomp_global_allocators.list[allocator].memspace;

        goto main_treat;

        break;

      default:
      case omp_atv_default_mem_fb :
        sctk_posix_memalign(&ret, alignment, size);
        break;
    }
  }

  return ret;
}

#ifdef __cplusplus
void
omp_free(
  void *ptr,
  omp_allocator_handle_t allocator = omp_null_allocator
)
#else
void
omp_free(void *ptr, omp_allocator_handle_t allocator)
#endif
{
  mpc_omp_init();

  if(ptr == NULL)
  {
    return ;
  }

  int memspace = mpcomp_global_allocators.list[allocator].memspace;

  if(memspace ==  omp_large_cap_mem_space)
  {
#if ENABLE_NVMEM
    vmem_free(vmp, ptr);
#else
    sctk_free(ptr);
#endif
  }
  else
  {
    sctk_free(ptr);
  }
}
