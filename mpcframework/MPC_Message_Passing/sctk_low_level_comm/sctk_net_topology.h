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
/* #                                                                      # */
/* ######################################################################## */
#ifndef SCTK_NET_TOPOLOGY
#define SCTK_NET_TOPOLOGY

#include <sctk_rail.h>

/************************************************************************/
/* sctk_Torus_t                                                         */
/************************************************************************/


/** This statically defines the maximum size of a Node */
#define MAX_SCTK_FAST_NODE_DIM 10

/** This statically defines the minimum number of ranks for each dimension */
#define SCTK_TORUS_MIN_PROC_IN_DIM 4


/**
* \brief Implements a static Node
*
* The sctk_Node_t is a static Node which maximum dimension
* is determined by the define (MAX_SCTK_FAST_NODE_DIM)
*/
typedef struct sctk_Node_t
{
	int c[MAX_SCTK_FAST_NODE_DIM];				/**< Node storage */
	int neigh[MAX_SCTK_FAST_NODE_DIM][4];			/**< Node neigbours storage */
	uint8_t breakdown[MAX_SCTK_FAST_NODE_DIM * 2];
	uint8_t d;                        			/**< Node dimension */
	int id;							/**< Node ident */
} sctk_Node_t;



/**
* \brief Implements a Torus topology
*
* The sctk_Torus_t implements a d-Torus with dimension
* varying from 1 to MAX_SCTK_FAST_NODE_DIM
*
* The sctk_Torus_t is able to compute the neighbor
* of a given rank and also to compute a route
* using distance heuristic
*
*/
typedef struct sctk_Torus_t
{
	int node_count;            		/**< Number of nodes in the Torus */
	uint8_t dimension;               	/**< Torus dimension */
	uint8_t size_last_dimension;	/**< minimum number of ranks for the last dimension  */
	int node_regular; 			/**< node_count when it is a perfect torus */
	int node_left; 			 	/**< zero when it is a perfect torus */
	sctk_Node_t last_node;		 /**< useful when it is an imperfect torus */
	sctk_Node_t local_node;		 /**< useful when it is an imperfect torus */
} sctk_Torus_t;



/**
* \brief Initialize a Torus or torus topology
* \param node_count number of nodes in the Torus (task count in MPI)
* \param dimension dimension of the Torus/torus ( from 1 to MAX_SCTK_FAST_NODE_DIM )
* This call can handle any non null node_count.
*
*/
void sctk_Torus_init ( int node_count, uint8_t dimension );


/**
* \brief Compute the distance between two Nodes
* \param a integer, the value of the coordinate of the first node
* \param b integer, the value of the coordinate of the second node
* \param sdim integer, the size of the current dimension
* \return distance beetween a and b
*
*/
int sctk_Torus_Node_distance(int a, int b, unsigned sdim);

/**
* \brief Release a sctk_Torus_t
* After this call any call reffering the Torus
* is likely to fail ...
*
*/
void sctk_Torus_release ();

/**
* \brief search the next node for the route
* \param dest node destination of message
* \return the id of the nearest node
*/
int sctk_Torus_route_next ( sctk_Node_t *dest );


int sctk_route_torus ( int dest, sctk_rail_info_t *rail );

void sctk_route_torus_init ( sctk_rail_info_t *rail );

/*
* Utilities
*/

/**
* \brief give a neighbour of a node
* \param unsigned int the dimension which is different between the cuurent node and its neighbour
* \param unsigned int 0 it's the "left" neigbour, 1 is the "right"
* \return the coordinate of the neighbour in the dimension i
*/
int sctk_Torus_neighbour_dimension ( unsigned i, unsigned j );

/**
* \brief Zero a Node
* \param Node pointer to sctk_Node_t
*
* This call will set all the dimensions
* of the given tupple to 0
*/
void sctk_Node_zero(sctk_Node_t *Node);

/**
* \brief Display a Node to stdout
* \param Node pointer to sctk_Node_t
*/
void sctk_Node_print(sctk_Node_t *Node);

/**
* \brief Initialize a Node
* \param Node pointer to an sctk_Node_t
* \param int the ident of the node
*/
void sctk_Node_init ( sctk_Torus_t *torus, sctk_Node_t *Node, int id );

/**
* \brief Compute the id (~rank) of a given coordinate in the Torus
* \param coord coordinates
* \return id associated to these coordinates
*
* Once the sctk_Torus_t is initialized this call is thread safe
* This call will abort if check are enabled and entries invalid
*/
int sctk_Node_id ( sctk_Torus_t *torus, sctk_Node_t *coord );

/**
* \brief Release a Node
* \param Node pointer to sctk_Node_t
*
* This call just set the dimension to 0
* making the Node unusable
*/
void sctk_Node_release(sctk_Node_t *Node);

/**
* \brief Set a Node by copy
* \param Node pointer to sctk_Node_t, it will be updated
* \param Node pointer to sctk_Node_t
*/
void sctk_Node_set_from(sctk_Node_t *Node, sctk_Node_t *NodeToCopy);

/************************************************************************/
/* None                                                                 */
/************************************************************************/

void sctk_route_none_init ( sctk_rail_info_t *rail );

/************************************************************************/
/* Ring                                                                 */
/************************************************************************/

void sctk_route_ring_init ( sctk_rail_info_t *rail );
int sctk_route_ring ( int dest, sctk_rail_info_t *rail );


/************************************************************************/
/* Fully Connected                                                      */
/************************************************************************/

int sctk_route_fully ( int dest, sctk_rail_info_t *rail );
void sctk_route_fully_init ( sctk_rail_info_t *rail );

/************************************************************************/
/* On-demand                                                            */
/************************************************************************/

int sctk_route_ondemand ( int dest, sctk_rail_info_t *rail );
void sctk_route_ondemand_init ( sctk_rail_info_t *rail );

#endif /* SCTK_NET_TOPOLOGY */
