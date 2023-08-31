#ifndef MPC_LOWCOMM_DT_COMMON
#define MPC_LOWCOMM_DT_COMMON

#include <mpc_lowcomm_datatypes.h>
#include <mpc_common_types.h>


/** \struct Common datatype structure */
typedef struct _mpc_lowcomm_datatype_common_s
{
	_mpc_lowcomm_general_datatype_t type_struct; /**< Unified datatype description */
	char                            name[64];    /**< Type name */
} _mpc_lowcomm_datatype_common_t;

/* *********************
* defining type sizes *
* *********************/

/* Integers */
#define MPC_LOWCOMM_CHAR_TYPE                  char
#define MPC_LOWCOMM_SHORT_TYPE                 short
#define MPC_LOWCOMM_INT_TYPE                   int
#define MPC_LOWCOMM_LONG_TYPE                  long
#define MPC_LOWCOMM_LONG_LONG_INT_TYPE         long long int
#define MPC_LOWCOMM_LONG_LONG_TYPE             long long
#define MPC_LOWCOMM_SIGNED_CHAR_TYPE           char
#define MPC_LOWCOMM_UNSIGNED_CHAR_TYPE         unsigned char
#define MPC_LOWCOMM_UNSIGNED_SHORT_TYPE        unsigned short
#define MPC_LOWCOMM_UNSIGNED_TYPE              unsigned int
#define MPC_LOWCOMM_UNSIGNED_LONG_TYPE         unsigned long
#define MPC_LOWCOMM_UNSIGNED_LONG_LONG_TYPE    unsigned long long

/* Floats */
#define MPC_LOWCOMM_FLOAT_TYPE                 float
#define MPC_LOWCOMM_DOUBLE_TYPE                double
#define MPC_LOWCOMM_LONG_DOUBLE_TYPE           long double

/* C Types */
#define MPC_LOWCOMM_WCHAR_TYPE                 sctk_wchar_t
#define MPC_LOWCOMM_C_BOOL_TYPE                bool
#define MPC_LOWCOMM_INT8_T_TYPE                int8_t
#define MPC_LOWCOMM_INT16_T_TYPE               int16_t
#define MPC_LOWCOMM_INT32_T_TYPE               int32_t
#define MPC_LOWCOMM_INT64_T_TYPE               int64_t
#define MPC_LOWCOMM_UINT8_T_TYPE               uint8_t
#define MPC_LOWCOMM_UINT16_T_TYPE              uint16_t
#define MPC_LOWCOMM_UINT32_T_TYPE              uint32_t
#define MPC_LOWCOMM_UINT64_T_TYPE              uint64_t

/* MPI Types */
#define MPC_LOWCOMM_AINT_TYPE                  size_t
#define MPC_LOWCOMM_OFFSET_TYPE                size_t
#define MPC_LOWCOMM_COUNT_TYPE                 size_t

/* Misc */
#define MPC_LOWCOMM_BYTE_TYPE                  unsigned char
#define MPC_LOWCOMM_PACKED_TYPE                void

/* C++ Types */
#define MPC_LOWCOMM_CXX_BOOL_TYPE              bool

/* FORTRAN Types */
#define MPC_LOWCOMM_INTEGER_TYPE               int
#define MPC_LOWCOMM_REAL_TYPE                  float
#define MPC_LOWCOMM_DOUBLE_PRECISION_TYPE      double
#define MPC_LOWCOMM_LOGICAL_TYPE               int
#define MPC_LOWCOMM_CHARACTER_TYPE             char

/* F08 Types */
#define MPC_LOWCOMM_INTEGER1_TYPE              int8_t
#define MPC_LOWCOMM_INTEGER2_TYPE              int16_t
#define MPC_LOWCOMM_INTEGER4_TYPE              int32_t
#define MPC_LOWCOMM_INTEGER8_TYPE              int64_t
#define MPC_LOWCOMM_INTEGER16_TYPE             int64_t[2]
#define MPC_LOWCOMM_REAL2_TYPE                 char[2] /* should use _Float16 on compatible platforms */
#define MPC_LOWCOMM_REAL4_TYPE                 float
#define MPC_LOWCOMM_REAL8_TYPE                 double
#define MPC_LOWCOMM_REAL16_TYPE                long double


int _mpc_lowcomm_datatype_init_common();

#endif
