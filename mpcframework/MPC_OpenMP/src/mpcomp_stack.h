#ifndef __MPCOMP_STACK_H__
#define __MPCOMP_STACK_H__

#include "mpcomp_types_stack.h"

/* Stack primitives */
mpcomp_stack_t * __mpcomp_create_stack( int max_elements );
int __mpcomp_is_stack_empty( mpcomp_stack_t *s );
void __mpcomp_push( mpcomp_stack_t *s, struct mpcomp_node_s *n );
struct mpcomp_node_s * __mpcomp_pop( mpcomp_stack_t *s );
void __mpcomp_free_stack( mpcomp_stack_t *s );

mpcomp_stack_node_leaf_t * __mpcomp_create_stack_node_leaf(int max_elements);
int __mpcomp_is_stack_node_leaf_empty(mpcomp_stack_node_leaf_t *s);
void __mpcomp_push_node(mpcomp_stack_node_leaf_t *s, struct mpcomp_node_s *n);
void __mpcomp_push_leaf(mpcomp_stack_node_leaf_t *s, struct mpcomp_mvp_s *n);
mpcomp_elem_stack_t * __mpcomp_pop_elem_stack(mpcomp_stack_node_leaf_t *s);
void __mpcomp_free_stack_node_leaf(mpcomp_stack_node_leaf_t *s);

#endif /* __MPCOMP_STACK_H__ */
