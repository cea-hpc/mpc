#ifndef __MPCOMP_STACK_H__
#define __MPCOMP_STACK_H__

#include "mpcomp_enum_types.h"

/** Break circle dependencies include */
struct mpcomp_node_s; 
struct mpcomp_mvp_s;

/* Element of the stack for dynamic work stealing */
typedef struct mpcomp_elem_stack_s
{
	union node_leaf 
	{
		struct mpcomp_node_s *node;
		struct mpcomp_mvp_s *leaf;
	} elem;                               	/**< Stack element 								*/
	mpcomp_elem_stack_type_t type;   		/**< Type of the 'elem' field 				*/
} mpcomp_elem_stack_t;

     
/* Stack structure for dynamic work stealing */
typedef struct mpcomp_stack_node_leaf_s
{
	mpcomp_elem_stack_t **elements;   		/**< List of elements 							*/
	int max_elements;                      /**< Number of max elements 					*/
	int n_elements;                        /**< Corresponds to the head of the stack */
} mpcomp_stack_node_leaf_t;


/* Stack (maybe the same that mpcomp_stack_node_leaf_s structure) */
typedef struct mpcomp_stack 
{
	struct mpcomp_node_s **elements;
	int max_elements;
	int n_elements;             /* Corresponds to the head of the stack */
} mpcomp_stack_t;


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
