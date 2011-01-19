
#ifndef UTILS_MACH_TYPES_H
#define UTILS_MACH_TYPES_H


/* format des entiers et des réels dans /usr/local/lm/include */

#include <utilspp/machine_types.h>
/*
#if  defined (KAI) ||  defined (OSF1)
#define size_t unsigned int
extern wchar_t *wcstok(wchar_t *wcs, const wchar_t *delim, wchar_t **ptr);
size_t wcsftime(wchar_t *s, size_t maxsize, const wchar_t *format, const
    struct tm *timeptr);
#endif
*/

# undef size_t

#ifdef __cplusplus

#if defined (OSF1)
#define __STL_HAS_NAMESPACES 
#define _NAMESPACES
#endif

/*
  #include <complex>
  using std::complex;

  typedef complex<float_type> complex_float_type;
  typedef const complex_float_type c_complex_float;
  typedef const complex_float_type & c_complex_float_r;
  typedef complex_float_type* complex_float_p;
  typedef complex_float_type** complex_float_pp;
  typedef complex_float_type* const complex_float_c_p;
  typedef const complex_float_type* const c_complex_float_p;
*/

typedef const int_type  c_int;
typedef const u_int_type  c_u_int;
typedef const float_type  c_float;
typedef const bool c_bool;
typedef const short c_short;

typedef c_int & c_int_r;
typedef c_u_int & c_u_int_r;
typedef c_float & c_float_r;
typedef c_bool & c_bool_r;
typedef c_short & c_short_r;

typedef int_type & int_r;
typedef u_int_type & u_int_r;
typedef float_type & float_r;
typedef bool & bool_r;
typedef short & short_r;

typedef float_type* float_p;
typedef u_int_type* u_int_p;
typedef int_type* int_p;
typedef char* char_p;
typedef bool* bool_p;
typedef short* short_p;

typedef float_p & float_p_r;
typedef u_int_p & u_int_p_r;
typedef int_p & int_p_r;
typedef char_p & char_p_r;
typedef bool_p & bool_p_r;
typedef short_p & short_p_r;

typedef float_type** float_pp;
typedef u_int_type** u_int_pp;
typedef int_type** int_pp;
typedef char** char_pp;
typedef bool** bool_pp;
typedef short** short_pp;

typedef float_p const float_c_p;
typedef u_int_p const u_int_c_p;
typedef int_p const int_c_p;
typedef char* const char_c_p;
typedef bool* const bool_c_p;
typedef short* const short_c_p;

typedef const float_type *const c_float_p;
typedef const int_type *const  c_int_p;
typedef const u_int_type *const c_u_int_p;
typedef const char* const c_char_p;
typedef const bool* const c_bool_p;
typedef const short* const c_short_p;

typedef float_type *const *const pp_c_float;
typedef u_int_type *const *const pp_c_u_int;
typedef int_type *const *const pp_c_int;

typedef const float_type *const *const c_float_pp;
typedef const u_int_type *const *const  c_u_int_pp;
typedef const int_type *const *const c_int_pp;

#endif

#endif
