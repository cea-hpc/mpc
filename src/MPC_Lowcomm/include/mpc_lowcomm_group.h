#ifndef MPC_LOWCOMM_GROUP_INC_H
#define MPC_LOWCOMM_GROUP_INC_H

/************************
* _MPC_LOWCOMM_GROUP_T *
************************/

#define MPC_GROUP_UNDEFINED    -1

struct _mpc_lowcomm_group_s;
typedef struct _mpc_lowcomm_group_s mpc_lowcomm_group_t;

typedef enum
{
	MPC_GROUP_IDENT,
	MPC_GROUP_SIMILAR,
	MPC_GROUP_UNEQUAL
}mpc_lowcomm_group_eq_e;

mpc_lowcomm_group_eq_e mpc_lowcomm_group_compare(mpc_lowcomm_group_t *g1,
                                                 mpc_lowcomm_group_t *g2);

int mpc_lowcomm_group_free(mpc_lowcomm_group_t **group);
unsigned int mpc_lowcomm_group_size(mpc_lowcomm_group_t *g);
int mpc_lowcomm_group_rank(mpc_lowcomm_group_t *g);
int mpc_lowcomm_group_rank_for(mpc_lowcomm_group_t *g, int rank);
int mpc_lowcomm_group_translate_ranks(mpc_lowcomm_group_t *g1,
                                      int n,
                                      const int ranks1[],
                                      mpc_lowcomm_group_t *g2,
                                      int ranks2[]);

mpc_lowcomm_group_t * mpc_lowcomm_group_dup(mpc_lowcomm_group_t * g);

#endif /* MPC_LOWCOMM_GROUP_INC_H */
