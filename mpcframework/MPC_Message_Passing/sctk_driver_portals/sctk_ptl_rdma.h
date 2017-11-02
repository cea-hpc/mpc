#ifndef __SCTK_PTL_RDMA_H_
#define __SCTK_PTL_RDMA_H_

int sctk_ptl_rdma_fetch_and_op_gate( sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type );
void sctk_ptl_rdma_fetch_and_op(  sctk_rail_info_t *rail,
		sctk_thread_ptp_message_t *msg,
		void * fetch_addr,
		struct  sctk_rail_pin_ctx_list * local_key,
		void * remote_addr,
		struct  sctk_rail_pin_ctx_list * remote_key,
		void * add,
		RDMA_op op,
		RDMA_type type );
int sctk_ptl_rdma_cas_gate( sctk_rail_info_t *rail, size_t size, RDMA_type type );
void sctk_ptl_rdma_cas(   sctk_rail_info_t *rail,
		sctk_thread_ptp_message_t *msg,
		void *  res_addr,
		struct  sctk_rail_pin_ctx_list * local_key,
		void * remote_addr,
		struct  sctk_rail_pin_ctx_list * remote_key,
		void * comp,
		void * new,
		RDMA_type type );
void sctk_ptl_rdma_write(  sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
		void * src_addr, struct sctk_rail_pin_ctx_list * local_key,
		void * dest_addr, struct  sctk_rail_pin_ctx_list * remote_key,
		size_t size );
void sctk_ptl_rdma_read(  sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
		void * src_addr,  struct  sctk_rail_pin_ctx_list * remote_key,
		void * dest_addr, struct  sctk_rail_pin_ctx_list * local_key,
		size_t size );
/* Pinning */
void sctk_ptl_pin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list, void * addr, size_t size );
void sctk_ptl_unpin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list );
#endif /* ifndef __SCTK_PTL_RDMA_H_ */
