#ifndef OMP_GOMP_CONSTANTS_H_
#define OMP_GOMP_CONSTANTS_H_

/* GOMP bit flags */
/* https://github.com/gcc-mirror/gcc/blob/master/include/gomp-constants.h#L202 */
# define GOMP_TASK_FLAG_UNTIED      (1 << 0)
# define GOMP_TASK_FLAG_FINAL       (1 << 1)
# define GOMP_TASK_FLAG_MERGEABLE   (1 << 2)
# define GOMP_TASK_FLAG_DEPEND      (1 << 3)
# define GOMP_TASK_FLAG_PRIORITY    (1 << 4)
# define GOMP_TASK_FLAG_UP          (1 << 8)
# define GOMP_TASK_FLAG_GRAINSIZE   (1 << 9)
# define GOMP_TASK_FLAG_IF          (1 << 10)
# define GOMP_TASK_FLAG_NOGROUP     (1 << 11)

/* tasks dependencies flags */
typedef enum    gomp_omp_task_dep_type_e {
  GOMP_OMP_TASK_DEP_NONE     = 0,
  GOMP_OMP_TASK_DEP_IN       = 1,
  GOMP_OMP_TASK_DEP_OUT      = 2,
  GOMP_OMP_TASK_DEP_INOUT    = 3,
  GOMP_OMP_TASK_DEP_COUNT    = 4
}               gomp_omp_task_dep_type_t;

#endif /* OMP_GOMP_CONSTANTS_H_ */
