#include "datatypes_common.h"

#include "mpc_common_debug.h"

#include <stdio.h>

/**
 * @brief initializes a datatype
 * 
 * @param datatype datatype name
 * @param t type (registering its size)
 */

#define mpc_lowcomm_datatype_init( datatype ) \
	mpc_lowcomm_datatypes_list[datatype].typesize = sizeof( datatype ## _TYPE );\
	sprintf(mpc_lowcomm_datatypes_list[datatype].typename, "%s_%s", "MPI", #datatype);

_mpc_lowcomm_datatype_ctx_t mpc_lowcomm_datatypes_list[MPC_LOWCOMM_TYPE_COMMON_LIMIT];

int mpc_lowcomm_datatype_init_common(){
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_CHAR );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_LOGICAL );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_BYTE );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_SHORT );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INT );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_LONG );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_FLOAT );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_DOUBLE );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UNSIGNED_CHAR );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UNSIGNED_SHORT );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UNSIGNED );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UNSIGNED_LONG );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_LONG_DOUBLE );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_LONG_LONG );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UNSIGNED_LONG_LONG );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INTEGER1 );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INTEGER2 );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INTEGER4 );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INTEGER8 );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INTEGER16 );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_REAL4 );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_REAL8 );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_REAL16 );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_SIGNED_CHAR );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INT8_T );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UINT8_T );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INT16_T );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UINT16_T );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INT32_T );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UINT32_T );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INT64_T );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UINT64_T );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_WCHAR );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_AINT );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_OFFSET );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_COUNT );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_LONG_LONG_INT );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_C_BOOL );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_CHARACTER );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INTEGER );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_REAL );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_DOUBLE_PRECISION );
	
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_PACKED );

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_datatype_common_get_size(mpc_lowcomm_datatype_t datatype)
{
    return mpc_lowcomm_datatypes_list[datatype].typesize;
}

char* mpc_lowcomm_datatype_common_get_name(mpc_lowcomm_datatype_t datatype)
{
    return mpc_lowcomm_datatypes_list[datatype].typename;
}


int mpc_lowcomm_datatype_common_set_name(mpc_lowcomm_datatype_t datatype, char *name)
{
	if(datatype < MPC_LOWCOMM_TYPE_COMMON_LIMIT){
    	sprintf(mpc_lowcomm_datatypes_list[datatype].typename, "%s", name);
		return MPC_LOWCOMM_SUCCESS;
	}
	return MPC_LOWCOMM_ERR_TYPE;
}


void mpc_lowcomm_datatype_common_display( mpc_lowcomm_datatype_t datatype )
{
	if ( !mpc_lowcomm_datatype_is_common( datatype ) )
	{
		mpc_common_debug_error( "Unknown datatype provided to %s\n", __FUNCTION__ );
		abort();
	}

	mpc_common_debug_error( "=============COMMON=================" );
	mpc_common_debug_error( "NAME %s", mpc_lowcomm_datatype_common_get_name( datatype ) );
	mpc_common_debug_error( "SIZE %ld", mpc_lowcomm_datatype_common_get_size(datatype) );
	mpc_common_debug_error( "====================================" );
}
