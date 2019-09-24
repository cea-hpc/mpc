/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #   - TCHIBOUKDJIAN Marc marc.tchiboukdjian@exascale-computing.eu      # */
/* #   - BOUHROUR Stephane stephane.bouhrour@exascale-computing.eu        # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_COMMON_INCLUDE_TOPOLOGY_GRAPH_H_
#define MPC_COMMON_INCLUDE_TOPOLOGY_GRAPH_H_


/* used by option graphic */
void create_placement_rendering(int pu, int master_pu,int task_id);

/* used by option text */
void create_placement_text(int os_pu, int os_master_pu, int task_id, int vp, int rank_open_mp, int* min_idex, int pid);

/* Get the os index from the topology_compute_node where the current thread is binding */
int sctk_get_cpu_compute_node_topology();

/* Get the logical index from the os one from the topology_compute_node */
int sctk_get_logical_from_os_compute_node_topology(unsigned int cpu_os);

/* Get the os index from the logical one from the topology_compute_node */
int sctk_get_cpu_compute_node_topology_from_logical( int logical_pu);

void topology_graph_init(void);
void topology_graph_render(void);

void mpc_common_topology_graph_lock_graphic();
void mpc_common_topology_graph_unlock_graphic();
void mpc_common_topology_graph_notify_thread(int task_id);

#endif /* MPC_COMMON_INCLUDE_TOPOLOGY_GRAPH_H_ */
