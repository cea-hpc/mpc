#include<mpc_lowcomm_datatypes.h>

int mpc_lowcomm_datatype_init_common(){
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_CHAR, char );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_LOGICAL, int );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_BYTE, unsigned char );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_SHORT, short );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INT, int );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_LONG, long );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_FLOAT, float );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_DOUBLE, double );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UNSIGNED_CHAR, unsigned char );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UNSIGNED_SHORT, unsigned short );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UNSIGNED, unsigned int );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UNSIGNED_LONG, unsigned long );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_LONG_DOUBLE, long double );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_LONG_LONG, long long );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UNSIGNED_LONG_LONG, unsigned long long );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INTEGER1, int8_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INTEGER2, int16_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INTEGER4, int32_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INTEGER8, int64_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INTEGER16, int64_t[2] );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_REAL4, float );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_REAL8, double );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_REAL16, long double );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_SIGNED_CHAR, char );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INT8_T, int8_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UINT8_T, uint8_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INT16_T, int16_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UINT16_T, uint16_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INT32_T, int32_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UINT32_T, uint32_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INT64_T, int64_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_UINT64_T, uint64_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_WCHAR, sctk_wchar_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_AINT, size_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_OFFSET, size_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_COUNT, size_t );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_LONG_LONG_INT, long long int );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_C_BOOL, char );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_CHARACTER, char );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_INTEGER, int );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_REAL, float );
	mpc_lowcomm_datatype_init( MPC_LOWCOMM_DOUBLE_PRECISION, double );
}

int mpc_lowcomm_datatype_get_size(int datatype){

}
