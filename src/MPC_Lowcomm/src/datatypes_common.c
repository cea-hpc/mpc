#include<mpc_lowcomm_datatypes.h>
#include<datatypes_common.h>


/**
 * @brief initializes a datatype
 * 
 * @param datatype datatype name
 * @param t type (registering its size)
**/

#define sctk_wchar_t int

#define mpc_lowcomm_datatype_init( datatype ) \
	mpc_lowcomm_datatypes_list[datatype].typesize = sizeof( datatype##_TYPE );\
	sprintf(mpc_lowcomm_datatypes_list[datatype].typename, "%s", #datatype);

mpc_lowcomm_datatype mpc_lowcomm_datatypes_list[MPC_Lowcomm_type_cxx_limit];

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
}

int mpc_lowcomm_datatype_common_get_size(int datatype)
{
    return mpc_lowcomm_datatypes_list[datatype].typesize;
}

char* mpc_lowcomm_datatype_common_get_name(int datatype)
{
    return mpc_lowcomm_datatypes_list[datatype].typename;
}

#define mpc_lowcomm_datatype_composed_init( datatype, type1, type2 )\
	mpc_lowcomm_datatypes_list[datatype].typesize = sizeof(struct{type1 a; type2 b;});\
    sprintf(mpc_lowcomm_datatypes_list[datatype].typename, "%s", #datatype);


int mpc_lowcomm_datatype_init_struct(){
	mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_FLOAT_INT, MPC_LOWCOMM_FLOAT_TYPE, MPC_LOWCOMM_FLOAT_TYPE ) ;
    mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_LONG_INT, MPC_LOWCOMM_LONG_TYPE, MPC_LOWCOMM_INT_TYPE ) ;
    mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_DOUBLE_INT, MPC_LOWCOMM_DOUBLE_TYPE, MPC_LOWCOMM_INT_TYPE ) ;
    mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_SHORT_INT, MPC_LOWCOMM_SHORT_TYPE, MPC_LOWCOMM_INT_TYPE ) ;
    mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_2INT, MPC_LOWCOMM_INT_TYPE, MPC_LOWCOMM_INT_TYPE ) ;
    mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_2FLOAT, MPC_LOWCOMM_FLOAT_TYPE, MPC_LOWCOMM_FLOAT_TYPE ) ;
    mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_COMPLEX, MPC_LOWCOMM_FLOAT_TYPE, MPC_LOWCOMM_FLOAT_TYPE ) ;
    mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_2DOUBLE_PRECISION, MPC_LOWCOMM_DOUBLE_TYPE, MPC_LOWCOMM_DOUBLE_TYPE ) ;
    mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_LONG_DOUBLE_INT, MPC_LOWCOMM_LONG_DOUBLE_TYPE, MPC_LOWCOMM_LONG_DOUBLE_TYPE ) ;
    mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_COMPLEX8, MPC_LOWCOMM_FLOAT_TYPE, MPC_LOWCOMM_FLOAT_TYPE ) ;
    mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_COMPLEX16, MPC_LOWCOMM_DOUBLE_TYPE, MPC_LOWCOMM_DOUBLE_TYPE ) ;
    mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_COMPLEX32, MPC_LOWCOMM_LONG_DOUBLE_TYPE, MPC_LOWCOMM_LONG_DOUBLE_TYPE ) ;
    mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_DOUBLE_COMPLEX, MPC_LOWCOMM_DOUBLE_TYPE, MPC_LOWCOMM_DOUBLE_TYPE ) ;
    mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_2INTEGER, MPC_LOWCOMM_INT_TYPE, MPC_LOWCOMM_INT_TYPE ) ;
    mpc_lowcomm_datatype_composed_init( MPC_LOWCOMM_2REAL, MPC_LOWCOMM_REAL_TYPE, MPC_LOWCOMM_REAL_TYPE );
}