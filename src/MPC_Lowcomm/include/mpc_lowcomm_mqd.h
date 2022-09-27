#ifndef MPC_LOWCOMM_MQD_H
#define MPC_LOWCOMM_MQD_H

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Name of the library hosting MQD
 *        (for MPC it is self-hosted)
 *
 */
extern char MPIR_dll_name[];

/*********
* TYPES *
*********/

typedef long   mqs_tword_t;
typedef size_t mqs_taddr_t;

typedef struct
{
	int short_size;
	int int_size;
	int long_size;
	int long_long_size;
	int pointer_size;
}mqs_target_type_sizes;

/**********************
* CONSTANTS AND ENUM *
**********************/

typedef enum
{
	mqs_lang_c     = 'c',
	mqs_lang_cplus = 'C',
	mqs_lang_f77   = 'f',
	mqs_lang_f90   = 'F'
}mqs_lang_code;

typedef enum
{
	mqs_pending_sends,
	mqs_pending_receives,
	mqs_unexpected_messages
}mqs_op_class;

enum
{
	MQS_INTERFACE_COMPATIBILITY = 2
};

enum mqs_status
{
	mqs_st_pending,
	mqs_st_matched,
	mqs_st_complete
};

typedef enum
{
	mqs_ok              = 0,
	mqs_no_information,
	mqs_end_of_list,
	mqs_first_user_code = 100
}mqs_err;

enum
{
	MQS_INVALID_PROCESS = -1
};


/*************
* MQS IMAGE *
*************/

struct mpc_lowcomm_mqs_image;
typedef struct mpc_lowcomm_mqs_image *    mqs_image;

/***************
* MQS PROCESS *
***************/

struct mpc_lowcomm_mqs_process;
typedef struct mpc_lowcomm_mqs_process *  mqs_process;

typedef int (*mqs_get_global_rank_ft)(mqs_process *process);
typedef mqs_image * (*mqs_get_image_ft)(mqs_process *process);
typedef int (*msq_fetch_data_ft)(mqs_process *process, mqs_taddr_t addr, int size, void *buff);
typedef int (*msq_target_to_host_ft)(mqs_process *process, void *indata, void *outdata, int size);

/* Ignored */
typedef struct
{
	mqs_get_global_rank_ft msq_get_global_rank_fp;
	mqs_get_image_ft       mqs_get_image_fp;
	msq_fetch_data_ft      msq_fetch_data_fp;
	msq_target_to_host_ft  msq_target_to_host_fp;
}mqs_process_callbacks;

/********************
* MQS COMMUNICATOR *
********************/

typedef struct
{
	mqs_taddr_t unique_id;
	mqs_tword_t local_rank;
	mqs_tword_t size;
	char        name[64];
}mqs_communicator;

/*****************
* MQS OPERATION *
*****************/

typedef struct
{
	/* Field for all messages */
	int         status;
	mqs_tword_t desired_local_rank;
	mqs_tword_t desired_global_rank;
	int         tag_wild;
	mqs_tword_t desired_tag;
	mqs_taddr_t desired_length;
	int         system_buffer;
	mqs_taddr_t buffer;
	mqs_taddr_t comm_id;

	/* Fields valid if status >= matched */
	mqs_tword_t actual_local_rank;
	mqs_tword_t actual_global_rank;
	mqs_tword_t actual_tag;
	mqs_tword_t actual_length;

	char        extra_text[5][64];
}mqs_pending_operation;

/**********************
* DEBUGGER CALLBACKS *
**********************/

/* Not implemented yet
 * as we are inline we are not much
 * dependent from them yet we define
 * these structs to make the debugger
 * happy just in case */

typedef void * mqs_image_info;
typedef void * mqs_process_info;


typedef struct
{
	void *mqs_malloc_fp;
	void *mqs_free_fp;
	void *mqs_dprint_fp;
	void *mqs_errorstring_fp;
	void *mps_put_image_info_fp;
	void *mqs_get_image_info_fp;
	void *mqs_put_process_info_fp;
	void *mpq_get_process_info_fp;
}mqs_basic_callbacks;

typedef struct
{
	void *mqs_get_type_size_fp;
	void *mqs_find_function_fp;
	void *mqs_find_symbol_fp;
	void *mqs_find_type_fp;
	void *mqs_field_offsep_fp;
	void *mqs_sizeof_fp;
}mqs_image_callbacks;

/***************************
* SETUP RELATED FUNCTIONS *
***************************/

void mqs_setup_basic_callbacks(const mqs_basic_callbacks *cb);
char *mqs_version_string();
int mqs_version_compatibility();
int mqs_dll_taddr_width();
char *mqs_dll_error_string(int error_code);

/**************************************
* EXECUTABLE IMAGE RELATED FUNCTIONS *
**************************************/

int mqs_setup_image(mqs_image *image, mqs_image_callbacks *cb);
int mqs_image_has_queues(mqs_image *image, char **message);
int mqs_destroy_image_info(mqs_image_info *info);

/*****************************
* PROCESS RELATED FUNCTIONS *
*****************************/

int mqsx_rewind_process(void);
int mqs_setup_process(mqs_process *process, const mqs_process_callbacks *cb);
int mqs_process_has_queues(mqs_process *process, char **message);
int mqs_destroy_process_info(mqs_process_info *processinfo);

/*******************
* QUERY FUNCTIONS *
*******************/

/* Communicator */
int mqs_update_communicator_list(mqs_process *process);
int mqs_setup_communicator_iterator(mqs_process *process);
int mqs_get_communicator(mqs_process *process, mqs_communicator *mqs_comm);
int mqs_get_comm_group(mqs_process *process, int *ranks);
int mqs_next_communicator(mqs_process *process);

/* Operations */
int mqs_setup_operation_iterator(mqs_process *process, int opclass);
int mqs_next_operation(mqs_process *process, mqs_pending_operation *op);

/****************
 * DUMP HELPERS *
 ****************/

void mqsx_dump_communicators(mqs_process * proc);
void mqsx_dump_comms(mqs_process * proc);

int mqsx_dump_communicators_json(mqs_process * proc, FILE *out);
int mqsx_dump_comms_json(mqs_process * proc, FILE *out);

#ifdef __cplusplus
}
#endif

#endif /* MPC_LOWCOMM_MQD */
