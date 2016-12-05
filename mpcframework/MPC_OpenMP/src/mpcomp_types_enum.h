/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
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
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPCOMP_TYPES_ENUM_H__
#define __MPCOMP_TYPES_ENUM_H__

typedef enum mpcomp_combined_mode_e {
  MPCOMP_COMBINED_NONE = 0,
  MPCOMP_COMBINED_SECTION = 1,
  MPCOMP_COMBINED_STATIC_LOOP = 2,
  MPCOMP_COMBINED_DYN_LOOP = 3,
  MPCOMP_COMBINED_GUIDED_LOOP = 4,
  MPCOMP_COMBINED_RUNTIME_LOOP = 5,
  MPCOMP_COMBINED_COUNT = 6
} mpcomp_combined_mode_t;

/* Type of element in the stack for dynamic work stealing */
typedef enum mpcomp_elem_stack_type_e {
  MPCOMP_ELEM_STACK_NODE = 1,
  MPCOMP_ELEM_STACK_LEAF = 2,
  MPCOMP_ELEM_STACK_COUNT = 3,
} mpcomp_elem_stack_type_t;

/* Type of children in the topology tree */
typedef enum mpcomp_children_e {
  MPCOMP_CHILDREN_NODE = 1,
  MPCOMP_CHILDREN_LEAF = 2,
} mpcomp_children_t;

typedef enum mpcomp_context_e {
  MPCOMP_CONTEXT_IN_ORDER = 1,
  MPCOMP_CONTEXT_OUT_OF_ORDER_MAIN = 2,
  MPCOMP_CONTEXT_OUT_OF_ORDER_SUB = 3,
} mpcomp_context_t;

typedef enum mpcomp_topo_obj_type_e {
  MPCOMP_TOPO_OBJ_SOCKET,
  MPCOMP_TOPO_OBJ_CORE,
  MPCOMP_TOPO_OBJ_THREAD,
  MPCOMP_TOPO_OBJ_COUNT
} mpcomp_topo_obj_type_t;

typedef enum mpcomp_mode_e {
  MPCOMP_MODE_SIMPLE_MIXED,
  MPCOMP_MODE_OVERSUBSCRIBED_MIXED,
  MPCOMP_MODE_ALTERNATING,
  MPCOMP_MODE_FULLY_MIXED,
  MPCOMP_MODE_COUNT
} mpcomp_mode_t;

typedef enum mpcomp_affinity_e {
  MPCOMP_AFFINITY_COMPACT,  /* Distribute over logical PUs */
  MPCOMP_AFFINITY_SCATTER,  /* Distribute over memory controllers */
  MPCOMP_AFFINITY_BALANCED, /* Distribute over physical PUs */
  MPCOMP_AFFINITY_NB
} mpcomp_affinity_t;

/*********
 * new meta elt for task initialisation
 */

typedef enum mpcomp_tree_meta_elt_type_e {
  MPCOMP_TREE_META_ELT_MVP,
  MPCOMP_TREE_META_ELT_NODE,
  MPCOMP_TREE_META_ELT_COUNT
} mpcomp_tree_meta_elt_type_t;

typedef union mpcomp_tree_meta_elt_ptr_u {
  struct mpcomp_node_s *node;
  struct mpcomp_mvp_s *mvp;
} mpcomp_tree_meta_elt_ptr_t;

typedef struct mpcomp_tree_meta_elt_s {
  mpcomp_tree_meta_elt_type_t type;
  mpcomp_tree_meta_elt_ptr_t ptr;
} mpcomp_tree_meta_elt_t;

#endif /* __MPCOMP_TYPES_ENUM_H__ */
