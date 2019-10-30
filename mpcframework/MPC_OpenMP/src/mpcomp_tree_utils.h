#ifndef __SCTK_MPCOMP_TREE_UTILS_H__
#define __SCTK_MPCOMP_TREE_UTILS_H__

int mpcomp_is_leaf(mpcomp_instance_t *instance, int globalRank);
int mpcomp_get_neighbour(int globalRank, int i);
int mpcomp_get_ancestor(int globalRank, int depth);
/* Check all neighbour of all nodes */
void __mpcomp_task_check_neighbourhood(void);

#endif /* __SCTK_MPCOMP_TREE_UTILS_H__ */
