#ifndef __MACHINES_TYPES__
#define __MACHINES_TYPES__

typedef unsigned int u_int_type;
typedef int int_type;
typedef double float_type;

#if defined(__INTEL_COMPILER) || defined(__DEC_COMPILER) || defined(__IBM_COMPILER)
#ifndef  __cplusplus
typedef char bool;
#endif
#endif

typedef float_type* float_type_p;
typedef u_int_type* u_int_type_p;
typedef int_type* int_type_p;
typedef char* char_p;
typedef bool* bool_p;
typedef short* short_p;

typedef const int_type  c_int_type;
typedef const u_int_type  c_u_int_type;
typedef const float_type  c_float_type;
typedef const bool c_bool;
typedef const short c_short;



typedef const float_type *const c_float_type_p;
typedef const int_type *const  c_int_type_p;
typedef const u_int_type *const c_u_int_type_p;
typedef const char* const c_char_p;
typedef const bool* const c_bool_p;
typedef const short* const c_short_p;



typedef float_type_p const float_type_c_p;
typedef u_int_type_p const u_int_type_c_p;
typedef int_type_p const int_type_c_p;
typedef char* const char_c_p;
typedef bool* const bool_c_p;
typedef short* const short_c_p;

#endif
