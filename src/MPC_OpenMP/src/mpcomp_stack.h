#ifndef __MPC_OMP_STACK_H__
#define __MPC_OMP_STACK_H__

#include "mpcomp_types_stack.h"

/* Stack primitives */
mpc_omp_stack_t *_mpc_omp_create_stack(int max_elements);
int _mpc_omp_is_stack_empty(mpc_omp_stack_t *s);
void _mpc_omp_push(mpc_omp_stack_t *s, struct mpc_omp_node_s *n);
struct mpc_omp_node_s *_mpc_omp_pop(mpc_omp_stack_t *s);
void mpc_omp_free_stack(mpc_omp_stack_t *s);

mpc_omp_stack_node_leaf_t *_mpc_omp_create_stack_node_leaf(int max_elements);
int _mpc_omp_is_stack_node_leaf_empty(mpc_omp_stack_node_leaf_t *s);
void _mpc_omp_push_node(mpc_omp_stack_node_leaf_t *s, struct mpc_omp_node_s *n);
void _mpc_omp_push_leaf(mpc_omp_stack_node_leaf_t *s, struct mpc_omp_mvp_s *n);
mpc_omp_elem_stack_t *_mpc_omp_pop_elem_stack(mpc_omp_stack_node_leaf_t *s);
void mpc_omp_free_stack_node_leaf(mpc_omp_stack_node_leaf_t *s);

#endif /* __MPC_OMP_STACK_H__ */
