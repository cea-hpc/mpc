#include "mpcomp_internal.h"
#include "mpcomp_tree_utils.h"

#if MPCOMP_TASK

int 
mpcomp_is_leaf(int globalRank)
{
     mpcomp_thread_t *t;
     mpcomp_instance_t *instance;

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     instance = t->instance;

     return (globalRank >= (instance->tree_array_size - instance->nb_mvps));
}

/* Return the ith neighbour of the element at rank 'globalRank' in the tree_array */
int 
mpcomp_get_neighbour(int globalRank, int i)
{
     mpcomp_thread_t *t;
     mpcomp_instance_t *instance;
     int j, r, firstRank, id, nbSubleaves, vectorSize;
     int *treeBase, *treeLevelSize, *path;
     
     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     instance = t->instance;
     treeLevelSize = instance->tree_level_size;
     treeBase = instance->tree_base;

#if MPCOMP_OVERFLOW_CHECKING
     sctk_assert(globalRank >= 0 && globalRank < instance->tree_array_size); 
#endif /* MPCOMP_OVERFLOW_CHECKING */

     if(mpcomp_is_leaf(globalRank)) {
	  mpcomp_mvp_t *mvp = (mpcomp_mvp_t *) (t->mvp->tree_array[globalRank]);
	  path = mvp->path;
	  vectorSize = mvp->father->depth + 1;
     } else {
	  mpcomp_node_t *n = t->mvp->tree_array[globalRank];
	  path = n->path;
	  vectorSize = n->depth;
     }

     int v[vectorSize], res[vectorSize];

     r = i;
     id = 0;
     firstRank = 0;
     nbSubleaves = 1;
     for (j=0; j<vectorSize; j++) {
	  int base = treeBase[vectorSize-1-j]; 
	  int level_size = treeLevelSize[j];

#if MPCOMP_OVERFLOW_CHECKING
	  sctk_assert(vectorSize-1-j >= 0 && vectorSize-1-j < t->mvp->father->depth + 1); 
	  sctk_assert(j >= 0 && j <= t->mvp->father->depth + 1); 
#endif /* MPCOMP_OVERFLOW_CHECKING */
	  /* Somme de I avec le codage de 0 dans le vecteur de l'arbre */
	  v[j] = r % base;
	  r /= base;

#if MPCOMP_OVERFLOW_CHECKING
	  sctk_assert(vectorSize-1-j >= 0); 
#endif /* MPCOMP_OVERFLOW_CHECKING */

	  /* Addition sans retenue avec le vecteur de n */
	  res[j] = (path[vectorSize-1-j] + v[j]) % base;	 

	  /* Calcul de l'identifiant du voisin dans l'arbre */
	  id += res[j] * nbSubleaves;
	  nbSubleaves *= base;
	  firstRank += level_size;
     }

     return id + firstRank;
}

/* Return the ancestor of element at rank 'globalRank' at depth 'depth' */
int 
mpcomp_get_ancestor(int globalRank, int depth)
{
     mpcomp_thread_t *t;
     int i, ancestor_id, firstRank, nbSubLeaves, currentDepth;
     int *levelSize, *treeBase, *path;

     /* If it's the root, ancestor is itself */
     if (globalRank == 0)
	  return 0;

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

#if MPCOMP_OVERFLOW_CHECKING
     sctk_assert(globalRank >= 0 && globalRank < t->instance->tree_array_size); 
#endif /* MPCOMP_OVERFLOW_CHECKING */

     if(mpcomp_is_leaf(globalRank)) {
	  mpcomp_mvp_t *mvp = (mpcomp_mvp_t *) (t->mvp->tree_array[globalRank]);
	  path = mvp->path;
	  currentDepth = mvp->father->depth + 1;
     } else {
	  mpcomp_node_t *n = t->mvp->tree_array[globalRank];
	  path = n->path;
	  currentDepth = n->depth;
     }

     if (currentDepth == depth)
	  return globalRank;

#if MPCOMP_OVERFLOW_CHECKING
     sctk_assert(depth >= 0 && depth < currentDepth); 
#endif /* MPCOMP_OVERFLOW_CHECKING */

     levelSize = t->instance->tree_level_size;
     treeBase = t->instance->tree_base;
     ancestor_id =0;
     firstRank = 0;
     nbSubLeaves = 1;
     for (i=0; i<depth; i++) {
	  ancestor_id += path[depth-1-i] * nbSubLeaves;
	  nbSubLeaves *= treeBase[depth-1-i]; 
	  firstRank += levelSize[i];
     }

     return firstRank + ancestor_id;
}


/* Recursive call for checking neighbourhood from node n */
static void 
__mpcomp_task_check_neighbourhood_r(mpcomp_node_t *n)
{
     mpcomp_thread_t *t;
     int i, j;

     sctk_assert(n != NULL);

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     for (j=1; j<t->instance->tree_level_size[n->depth]; j++) {
	  fprintf(stderr, "neighbour n°%d of %d: %d\n", j, n->tree_array_rank, 
		  mpcomp_get_neighbour(n->tree_array_rank, j));
     }

     switch (n->child_type) {
     case MPCOMP_CHILDREN_NODE:
	  for (i=0; i<n->nb_children; i++) {
	       mpcomp_node_t *child = n->children.node[i];

	       /* Call recursively for all children nodes */
	       __mpcomp_task_check_neighbourhood_r(child);
	  }
	  break;
	  
     case MPCOMP_CHILDREN_LEAF:
	  for (i=0; i<n->nb_children; i++) {
	       /* All the children are leafs */
	       
	       mpcomp_mvp_t *mvp;
	       
	       mvp = n->children.leaf[i];
	       sctk_assert(mvp != NULL);

	       for (j=1; j<t->instance->tree_level_size[n->depth+1]; j++) {
		    fprintf(stderr, "neighbour n°%d of %d: %d\n", j, mvp->tree_array_rank, 
			    mpcomp_get_neighbour(mvp->tree_array_rank, j));
	       }
	  }
	  break ;
	  
     default:
	  sctk_nodebug("not reachable");
     }
     
     return;
}


/* Check all neighbour of all nodes */
void __mpcomp_task_check_neighbourhood(void)
{
     mpcomp_thread_t *t;

     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     __mpcomp_task_check_neighbourhood_r(t->mvp->root);
}
#endif /* MPCOMP_TASK */
