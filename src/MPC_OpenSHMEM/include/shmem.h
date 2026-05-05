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
/* #   - BEAULIEU Corentin corentin.beaulieu@cea.fr                       # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef MPC_OPENSHMEM_API_H_
#define MPC_OPENSHMEM_API_H_

#ifdef __cplusplus
	extern "C"
	{
#endif

#include <stddef.h>
#include <stdint.h>

/**
 * @defgroup MPC_OPENSHMEM MPC OpenSHMEM
 * @{
 * @}
 */

/**
 * @defgroup MPC_OPENSHMEM_API OpenSHMEM API
 * @ingroup MPC_OPENSHMEM
 * @{
 * @brief OpenSHMEM specification definitions
 *
 * @warning The implementation of the specification is partial
 */

/************
 * Constants *
 *************/

// NOTE: Only define constants once
#ifndef MPC_OPENSHMEM_PROFILING_API_H_

	/**
	 * @name OpenSHMEM thread support levels
	 * @{
	 */
	/**
	 * @var SHMEM_THREAD_SINGLE
	 * @brief No support for multithreaded application is granted
	 */
	/**
	 * @var SHMEM_THREAD_FUNNELED
	 * @brief Support for only one thread invoking OpenSHMEM API at a time is
	 * granted
	 */
	/**
	 * @var SHMEM_THREAD_SERIALIZED
	 * @brief Support for non concurrent OpenSHMEM API calls is granted
	 */
	/**
	 * @var SHMEM_THREAD_MULTIPLE
	 * @brief Full support for multithreading in OpenSHMEM calls
	 */

#if __STDC_VERSION__ >= 202300L
		constexpr int SHMEM_THREAD_SINGLE     = 0;
		constexpr int SHMEM_THREAD_FUNNELED   = 1024;
		constexpr int SHMEM_THREAD_SERIALIZED = 2048;
		constexpr int SHMEM_THREAD_MULTIPLE   = 4096;
#else
		static const int SHMEM_THREAD_SINGLE     = 0;
		static const int SHMEM_THREAD_FUNNELED   = 1024;
		static const int SHMEM_THREAD_SERIALIZED = 2048;
		static const int SHMEM_THREAD_MULTIPLE   = 4096;
#endif

	/** @} */ // OpenSHMEM thread support levels

	/**
	 * @name OpenSHMEM library information
	 * @{
	 */
	/**
	 * @var SHMEM_MAJOR_VERSION
	 * @brief Integer representing the major version of OpenSHMEM Specification in use
	 */
	/**
	 * @var SHMEM_MINOR_VERSION
	 * @brief Integer representing the minor version of OpenSHMEM Specification in use
	 */
	/**
	 * @var SHMEM_MAX_NAME_LEN
	 * @brief Integer representing the maximum length of SHMEM_VENDOR_STRING
	 */
	/**
	 * @var SHMEM_VENDOR_STRING
	 * @brief String representing vendor defined information
	 */

#if __STDC_VERSION__ >= 202300L
		constexpr int SHMEM_MAJOR_VERSION = 1;
		[[deprecated]] constexpr int _SHMEM_MAJOR_VERSION = 1;
		constexpr int SHMEM_MINOR_VERSION = 6;
		[[deprecated]] constexpr int _SHMEM_MINOR_VERSION = 6;
		constexpr int SHMEM_MAX_NAME_LEN = 32;
		[[deprecated]] constexpr int _SHMEM_MAX_NAME_LEN     = 32;
		constexpr char SHMEM_VENDOR_STRING[]                 = "MPC";
		[[deprecated]] constexpr char _SHMEM_VENDOR_STRING[] = "MPC";

#else
		static const int SHMEM_MAJOR_VERSION = 1;
		__attribute__((deprecated)) static const int _SHMEM_MAJOR_VERSION = 1;
		static const int SHMEM_MINOR_VERSION = 6;
		__attribute__((deprecated)) static const int _SHMEM_MINOR_VERSION = 6;
		static const int SHMEM_MAX_NAME_LEN = 32;
		__attribute__((deprecated)) static const int _SHMEM_MAX_NAME_LEN = 32;
		static const char SHMEM_VENDOR_STRING[] = "MPC";
		__attribute__((deprecated)) static const char _SHMEM_VENDOR_STRING[] = "MPC";
#endif

	/** @} */ // OpenSHMEM library information

#endif /* MPC_OPENSHMEM_PROFILING_API_H_ */

/**
 * @defgroup MPC_OPENSHMEM_API_GENERAL Setup, Exit and Query
 * @{
 * @brief OpenSHMEM defined Setup, Exit and Query routines
 */

/**
 * @brief Initialize the OpenSHMEM runtime
 */
void shmem_init(void);

/**
 * @brief Deprecated version of shmem_init
 */
#if __STDC_VERSION__ >= 202300L
	[[deprecated]] void start_pes();

#else
	__attribute__((deprecated)) void start_pes();

#endif

/**
 * @brief Query the rank of the local task
 *
 * @return Rank of the local task (in the world context)
 */
int shmem_my_pe(void);

/**
 * @function _my_pe
 * @brief Deprecated version of shmem_my_pe
 */
#if __STDC_VERSION__ >= 202300L
	[[deprecated]] int _my_pe(void);

#else
	__attribute__((deprecated)) int _my_pe(void);

#endif

/**
 * @brief Query the number of the tasks
 *
 * @return Number of tasks (in the world context)
 */
int shmem_n_pes(void);

/**
 * @function _num_pes
 * @brief Deprecated version of shmem_n_pes
 */
#if __STDC_VERSION__ >= 202300L
	[[deprecated]] int _num_pes(void);

#else
	__attribute__((deprecated)) int _num_pes(void);

#endif

/**
 * @brief Finalize the OpenSHMEM runtime
 */
void shmem_finalize(void);

#if !defined(__cplusplus) && __STDC_VERSION__ >= 201100L

	/**
	 * @brief Force terminate the entire program
	 *
	 * Any invocation of this routine must lead to the termination of all PEs.
	 * I/O are flushed and resources released accordingly to C/C++ requirements on
	 * program termination.
	 *
	 * @param status Exit status from the main program
	 */
	_Noreturn void shmem_global_exit(int status);

#else

	/**
	 * @brief Force terminate the entire program
	 *
	 * Any invocation of this routine must lead to the termination of all PEs.
	 * I/O are flushed and resources released accordingly to C/C++ requirements on
	 * program termination.
	 *
	 * @param status Exit status from the main program
	 */
	void shmem_global_exit(int status);

#endif

/**
 * @brief Initialize the OpenSHMEM runtime to guarantee a certain level of multithreading
 *
 * @param[in]  requested Level of multithreading requested
 * @param[out] provided  Level of multithreading actually provided by the runtime
 *
 * @return               0 upon success, a non-zero value otherwise
 */
int shmem_init_thread(int requested, int *provided);

/** @} */ // MPC_OPENSHMEM_API_GENERAL


/**
 * @defgroup MPC_OPENSHMEM_API_MEM Memory Management
 * @{
 * @brief OpenSHMEM defined memory management routines
 */

/**
 * @brief Allocate memory on the symmetric heap
 *
 * @param[in] size Size in Bytes of the memory region to allocate
 *
 * @retval     addr     Symmetric address to the allocated memory region
 * @retval     NULL     if the allocation failed
 */
void *shmem_malloc(size_t size);

/**
 * @function shmalloc
 * @brief Deprecated version of shmem_malloc
 */
#if __STDC_VERSION__ >= 202300L
	[[deprecated]] void *shmalloc(size_t size);

#else
	__attribute__((deprecated)) void *shmalloc(size_t size);

#endif


/**
 * @brief Free a previously allocated region on the symmetric heap
 *
 * @param[in] ptr Pointer to the symmetric memory region to free
 */
void shmem_free(void *ptr);

/**
 * @function shfree
 * @brief Deprecated version of shmem_free
 */
#if __STDC_VERSION__ >= 202300L
	[[deprecated]] void shfree(void *ptr);

#else
	__attribute__((deprecated)) void shfree(void *ptr);

#endif

/**
 * @brief Resize an allocation of symmetric memory
 *
 * @param[in] ptr  Symmetric address of an object in the symmetric heap
 * @param[in] size The size, in Bytes, of a block to be allocated from the symmetric heap
 *
 * @retval		addr		Symmetric address to the resized memory region
 * @retval		NULL		if the allocation failed
 */
void *shmem_realloc(void *ptr, size_t size);

/**
 * @function shrealloc
 * @brief Deprecated version of shmem_free
 */
#if __STDC_VERSION__ >= 202300L
	[[deprecated]] void *shrealloc(void *ptr, size_t size);

#else
	__attribute__((deprecated)) void *shrealloc(void *ptr, size_t size);

#endif

/**
 * @brief Allocate a zeroed block of symmetric memory
 *
 * @param[in] count The number of elements to allocate
 * @param[in] size  The size in bytes of each element to allocate
 *
 * @retval		addr		Symmetric address to the resized memory region
 * @retval		NULL		if the allocation failed
 */
void *shmem_calloc(size_t count, size_t size);

/** @} */ // MPC_OPENSHMEM_API_MEM


/**
 * @defgroup MPC_OPENSHMEM_API_COMM Basic Communications
 * @{
 * @brief OpenSHMEM defined basic one-sided communications
 */

/**
 * @defgroup MPC_OPENSHMEM_API_COMM_PUT Put Communications
 * @{
 * @brief OpenSHMEM put-like defined communications
 */

/**
 * @name shmem_TYPE_put
 * @{
 * @brief OpenSHMEM put operation with multiple contigue data
 *
 * @param[in] dest   Symmetric address in which the data is put
 * @param[in] src    Local address from which the data is read
 * @param[in] nelems Number of elements of type TYPE to send
 * @param[in] pe     Unique identifier (rank) of the target
 */
void shmem_float_put(float *dest, const float *src, size_t nelems, int pe);
void shmem_double_put(double *dest, const double *src, size_t nelems, int pe);
void shmem_longdouble_put(long double *dest, const long double *src, size_t nelems, int pe);
void shmem_char_put(char *dest, const char *src, size_t nelems, int pe);
void shmem_schar_put(signed char *dest, const signed char *src, size_t nelems, int pe);
void shmem_short_put(short *dest, const short *src, size_t nelems, int pe);
void shmem_int_put(int *dest, const int *src, size_t nelems, int pe);
void shmem_long_put(long *dest, const long *src, size_t nelems, int pe);
void shmem_longlong_put(long long *dest, const long long *src, size_t nelems, int pe);
void shmem_uchar_put(unsigned char *dest, const unsigned char *src, size_t nelems, int pe);
void shmem_ushort_put(unsigned short *dest, const unsigned short *src, size_t nelems, int pe);
void shmem_uint_put(unsigned int *dest, const unsigned int *src, size_t nelems, int pe);
void shmem_ulong_put(unsigned long *dest, const unsigned long *src, size_t nelems, int pe);
void shmem_ulonglong_put(unsigned long long *dest, const unsigned long long *src, size_t nelems, int pe);
void shmem_int8_t_put(int8_t *dest, const int8_t *src, size_t nelems, int pe);
void shmem_uint8_t_put(uint8_t *dest, const uint8_t *src, size_t nelems, int pe);
void shmem_int16_t_put(int16_t *dest, const int16_t *src, size_t nelems, int pe);
void shmem_uint16_t_put(uint16_t *dest, const uint16_t *src, size_t nelems, int pe);
void shmem_int32_t_put(int32_t *dest, const int32_t *src, size_t nelems, int pe);
void shmem_uint32_t_put(uint32_t *dest, const uint32_t *src, size_t nelems, int pe);
void shmem_int64_t_put(int64_t *dest, const int64_t *src, size_t nelems, int pe);
void shmem_uint64_t_put(uint64_t *dest, const uint64_t *src, size_t nelems, int pe);
void shmem_size_t_put(size_t *dest, const size_t *src, size_t nelems, int pe);
void shmem_ptrdiff_t_put(ptrdiff_t *dest, const ptrdiff_t *src, size_t nelems, int pe);

/** @} */ // shmem_TYPE_put

/**
 * @brief OpenSHMEM put operation with a size in bytes instead of a count of typed objects
 *
 * @param[in] dest   Symmetric address in which the data is put
 * @param[in] source Local address from which the data is read
 * @param[in] size   Size in bytes to send
 * @param[in] pe     Unique identifier (rank) of the target
 */
void shmem_putmem(void *dest, const void *source, size_t size, int pe);

/**
 * @name shmem_TYPE_p
 * @{
 * @brief OpenSHMEM put operation with a single data
 *
 * @param[in] dest  Symmetric address in which the data is put
 * @param[in] value Local value to send
 * @param[in] pe    Unique identifier (rank) of the target
 */
void shmem_float_p(float *dest, const float value, int pe);
void shmem_double_p(double *dest, const double value, int pe);
void shmem_longdouble_p(long double *dest, const long double value, int pe);
void shmem_char_p(char *dest, const char value, int pe);
void shmem_schar_p(signed char *dest, const signed char value, int pe);
void shmem_short_p(short *dest, const short value, int pe);
void shmem_int_p(int *dest, const int value, int pe);
void shmem_long_p(long *dest, const long value, int pe);
void shmem_longlong_p(long long *dest, const long long value, int pe);
void shmem_uchar_p(unsigned char *dest, const unsigned char value, int pe);
void shmem_ushort_p(unsigned short *dest, const unsigned short value, int pe);
void shmem_uint_p(unsigned int *dest, const unsigned int value, int pe);
void shmem_ulong_p(unsigned long *dest, const unsigned long value, int pe);
void shmem_ulonglong_p(unsigned long long *dest, const unsigned long long value, int pe);

void shmem_int8_t_p(int8_t *dest, const int8_t value, int pe);
void shmem_uint8_t_p(uint8_t *dest, const uint8_t value, int pe);
void shmem_int16_t_p(int16_t *dest, const int16_t value, int pe);
void shmem_uint16_t_p(uint16_t *dest, const uint16_t value, int pe);
void shmem_int32_t_p(int32_t *dest, const int32_t value, int pe);
void shmem_uint32_t_p(uint32_t *dest, const uint32_t value, int pe);
void shmem_int64_t_p(int64_t *dest, const int64_t value, int pe);
void shmem_uint64_t_p(uint64_t *dest, const uint64_t value, int pe);
void shmem_size_t_p(size_t *dest, const size_t value, int pe);
void shmem_ptrdiff_t_p(ptrdiff_t *dest, const ptrdiff_t value, int pe);

/** @} */ // shmem_TYPE_p
/** @} */ // MPC_OPENSHMEM_API_COMM_PUT


/**
 * @defgroup MPC_OPENSHMEM_API_COMM_GET Get Communications
 * @{
 * @brief OpenSHMEM get-like defined communications
 */

/**
 * @name shmem_TYPE_get
 * @{
 * @brief OpenSHMEM get operation with multiple contigue data
 *
 * @param[in] dest   Local address in which the data is put
 * @param[in] src    Symmetric address from which the data is read
 * @param[in] nelems Number of elements of type TYPE to receive
 * @param[in] pe     Unique identifier (rank) of the target
 */
void shmem_float_get(float *dest, const float *src, size_t nelems, int pe);
void shmem_double_get(double *dest, const double *src, size_t nelems, int pe);
void shmem_longdouble_get(long double *dest, const long double *src, size_t nelems, int pe);
void shmem_char_get(char *dest, const char *src, size_t nelems, int pe);
void shmem_schar_get(signed char *dest, const signed char *src, size_t nelems, int pe);
void shmem_short_get(short *dest, const short *src, size_t nelems, int pe);
void shmem_int_get(int *dest, const int *src, size_t nelems, int pe);
void shmem_long_get(long *dest, const long *src, size_t nelems, int pe);
void shmem_longlong_get(long long *dest, const long long *src, size_t nelems, int pe);
void shmem_uchar_get(unsigned char *dest, const unsigned char *src, size_t nelems, int pe);
void shmem_ushort_get(unsigned short *dest, const unsigned short *src, size_t nelems, int pe);
void shmem_uint_get(unsigned int *dest, const unsigned int *src, size_t nelems, int pe);
void shmem_ulong_get(unsigned long *dest, const unsigned long *src, size_t nelems, int pe);
void shmem_ulonglong_get(unsigned long long *dest, const unsigned long long *src, size_t nelems, int pe);
void shmem_int8_t_get(int8_t *dest, const int8_t *src, size_t nelems, int pe);
void shmem_uint8_t_get(uint8_t *dest, const uint8_t *src, size_t nelems, int pe);
void shmem_int16_t_get(int16_t *dest, const int16_t *src, size_t nelems, int pe);
void shmem_uint16_t_get(uint16_t *dest, const uint16_t *src, size_t nelems, int pe);
void shmem_int32_t_get(int32_t *dest, const int32_t *src, size_t nelems, int pe);
void shmem_uint32_t_get(uint32_t *dest, const uint32_t *src, size_t nelems, int pe);
void shmem_int64_t_get(int64_t *dest, const int64_t *src, size_t nelems, int pe);
void shmem_uint64_t_get(uint64_t *dest, const uint64_t *src, size_t nelems, int pe);
void shmem_size_t_get(size_t *dest, const size_t *src, size_t nelems, int pe);
void shmem_ptrdiff_t_get(ptrdiff_t *dest, const ptrdiff_t *src, size_t nelems, int pe);

/** @} */ // shmem_TYPE_get

/**
 * @brief OpenSHMEM get operation with a size in bytes instead of a count of typed objects
 *
 * @param[in] dest   Local address in which the data is put
 * @param[in] source Symmetric address from which the data is read
 * @param[in] size   Size in bytes to send
 * @param[in] pe     Unique identifier (rank) of the target
 */
void shmem_getmem(void *dest, const void *source, size_t size, int pe);

/**
 * @name shmem_TYPE_g
 * @{
 * @brief OpenSHMEM get operation with a single data
 *
 * @param[in] src Symmetric address from which the data is read
 * @param[in] pe  Unique identifier (rank) of the target
 *
 * @return        TYPE data stored in #src on the rank #pe
 */
float shmem_float_g(const float *src, int pe);
double shmem_double_g(const double *src, int pe);
long double shmem_longdouble_g(const long double *src, int pe);
char shmem_char_g(const char *src, int pe);
signed char shmem_schar_g(const signed char *src, int pe);
short shmem_short_g(const short *src, int pe);
int shmem_int_g(const int *src, int pe);
long shmem_long_g(const long *src, int pe);
long long shmem_longlong_g(const long long *src, int pe);
unsigned char shmem_uchar_g(const unsigned char *src, int pe);
unsigned short shmem_ushort_g(const unsigned short *src, int pe);
unsigned int shmem_uint_g(const unsigned int *src, int pe);
unsigned long shmem_ulong_g(const unsigned long *src, int pe);
unsigned long long shmem_ulonglong_g(const unsigned long long *src, int pe);

int8_t shmem_int8_t_g(const int8_t *src, int pe);
uint8_t shmem_uint8_t_g(const uint8_t *src, int pe);
int16_t shmem_int16_t_g(const int16_t *src, int pe);
uint16_t shmem_uint16_t_g(const uint16_t *src, int pe);
int32_t shmem_int32_t_g(const int32_t *src, int pe);
uint32_t shmem_uint32_t_g(const uint32_t *src, int pe);
int64_t shmem_int64_t_g(const int64_t *src, int pe);
uint64_t shmem_uint64_t_g(const uint64_t *src, int pe);
size_t shmem_size_t_g(const size_t *src, int pe);
ptrdiff_t shmem_ptrdiff_t_g(const ptrdiff_t *src, int pe);

/** @} */ // shmem_TYPE_g
/** @} */ // MPC_OPENSHMEM_API_COMM_GET


/**
 * @brief OpenSHMEM put operation with multiple contigue data
 *
 * @note Generic counterpart of the #shmem_TYPE_put functions.
 *
 * @param[in] dest   Symmetric address in which the data is put
 * @param[in] src    Local address from which the data is read
 * @param[in] nelems Number of elements of any supported type to send
 * @param[in] pe     Unique identifier (rank) of the target
 */
#define shmem_put(dest, src, nelems, pe)      \
		_Generic(src,                         \
	float *: shmem_float_put,                 \
	double *: shmem_double_put,               \
	long double *: shmem_longdouble_put,      \
	char *: shmem_char_put,                   \
	signed char *: shmem_schar_put,           \
	short *: shmem_short_put,                 \
	int *: shmem_int_put,                     \
	long *: shmem_long_put,                   \
	long long *: shmem_longlong_put,          \
	unsigned char *: shmem_uchar_put,         \
	unsigned short *: shmem_ushort_put,       \
	unsigned int *: shmem_uint_put,           \
	unsigned long *: shmem_ulong_put,         \
	unsigned long long *: shmem_ulonglong_put \
	)(dest, src, nelems, pe)

/**
 * @brief OpenSHMEM put operation with a single data
 *
 * @note Generic counterpart of the #shmem_TYPE_p functions.
 *
 * @param[in] dest  Symmetric address in which the data is put
 * @param[in] value Local value to be send
 * @param[in] pe    Unique identifier (rank) of the target
 */
#define shmem_p(dest, value, pe)          \
		_Generic(value,                   \
	float: shmem_float_p,                 \
	double: shmem_double_p,               \
	long double: shmem_longdouble_p,      \
	char: shmem_char_p,                   \
	signed char: shmem_schar_p,           \
	short: shmem_short_p,                 \
	int: shmem_int_p,                     \
	long: shmem_long_p,                   \
	long long: shmem_longlong_p,          \
	unsigned char: shmem_uchar_p,         \
	unsigned short: shmem_ushort_p,       \
	unsigned int: shmem_uint_p,           \
	unsigned long: shmem_ulong_p,         \
	unsigned long long: shmem_ulonglong_p \
	)(dest, value, pe)

/**
 * @brief OpenSHMEM get operation with multiple contigue data
 *
 * @note Generic counterpart of the #shmem_TYPE_get functions.
 *
 * @param[in] dest   Local address in which the data is put
 * @param[in] src    Symmetric address from which the data is read
 * @param[in] nelems Number of elements of any supported type to send
 * @param[in] pe     Unique identifier (rank) of the target
 */
#define shmem_get(dest, src, nelems, pe)      \
		_Generic(dest,                        \
	float *: shmem_float_get,                 \
	double *: shmem_double_get,               \
	long double *: shmem_longdouble_get,      \
	char *: shmem_char_get,                   \
	signed char *: shmem_schar_get,           \
	short *: shmem_short_get,                 \
	int *: shmem_int_get,                     \
	long *: shmem_long_get,                   \
	long long *: shmem_longlong_get,          \
	unsigned char *: shmem_uchar_get,         \
	unsigned short *: shmem_ushort_get,       \
	unsigned int *: shmem_uint_get,           \
	unsigned long *: shmem_ulong_get,         \
	unsigned long long *: shmem_ulonglong_get \
	)(dest, src, nelems, pe)

/**
 * @brief OpenSHMEM get operation with a single data
 *
 * @note Generic counterpart of the #shmem_TYPE_g functions.
 *
 * @param[in] src Symmetric address from which the data read
 * @param[in] pe  Unique identifier (rank) of the target
 *
 * @return        data stored in #src on the rank #pe
 */
#define shmem_g(src, pe)                    \
		_Generic(src,                       \
	float *: shmem_float_g,                 \
	double *: shmem_double_g,               \
	long double *: shmem_longdouble_g,      \
	char *: shmem_char_g,                   \
	signed char *: shmem_schar_g,           \
	short *: shmem_short_g,                 \
	int *: shmem_int_g,                     \
	long *: shmem_long_g,                   \
	long long *: shmem_longlong_g,          \
	unsigned char *: shmem_uchar_g,         \
	unsigned short *: shmem_ushort_g,       \
	unsigned int *: shmem_uint_g,           \
	unsigned long *: shmem_ulong_g,         \
	unsigned long long *: shmem_ulonglong_g \
	)(src, pe)

/** @} */ // MPC_OPENSHMEM_API_COMM


/**
 * @defgroup MPC_OPENSHMEM_API_ATO Atomic Communications
 * @{
 * @brief OpenSHMEM defined atomic communications
 */

/**
 * @name shmem_TYPE_atomic_fetch
 * @{
 *
 * @brief Perform an atomic fetch on a given symmetric object
 *
 * @param[in] src Symmetric address of the element to fetch
 * @param[in] pe  Unique identifier (rank) of the target
 *
 * @return        Fetched data
 */
float shmem_float_atomic_fetch(const float *src, int pe);
double shmem_double_atomic_fetch(const double *src, int pe);
char shmem_char_atomic_fetch(const char *src, int pe);
signed char shmem_schar_atomic_fetch(const signed char *src, int pe);
short shmem_short_atomic_fetch(const short *src, int pe);
int shmem_int_atomic_fetch(const int *src, int pe);
long shmem_long_atomic_fetch(const long *src, int pe);
long long shmem_longlong_atomic_fetch(const long long *src, int pe);
unsigned char shmem_uchar_atomic_fetch(const unsigned char *src, int pe);
unsigned short shmem_ushort_atomic_fetch(const unsigned short *src, int pe);
unsigned int shmem_uint_atomic_fetch(const unsigned int *src, int pe);
unsigned long shmem_ulong_atomic_fetch(const unsigned long *src, int pe);
unsigned long long shmem_ulonglong_atomic_fetch(const unsigned long long *src, int pe);
int8_t shmem_int8_t_atomic_fetch(const int8_t *src, int pe);
uint8_t shmem_uint8_t_atomic_fetch(const uint8_t *src, int pe);
int16_t shmem_int16_t_atomic_fetch(const int16_t *src, int pe);
uint16_t shmem_uint16_t_atomic_fetch(const uint16_t *src, int pe);
int32_t shmem_int32_t_atomic_fetch(const int32_t *src, int pe);
uint32_t shmem_uint32_t_atomic_fetch(const uint32_t *src, int pe);
int64_t shmem_int64_t_atomic_fetch(const int64_t *src, int pe);
uint64_t shmem_uint64_t_atomic_fetch(const uint64_t *src, int pe);
size_t shmem_size_t_atomic_fetch(const size_t *src, int pe);
ptrdiff_t shmem_ptrdiff_t_atomic_fetch(const ptrdiff_t *src, int pe);

#if __STDC_VERSION__ >= 202300L
	[[deprecated]] float shmem_float_fetch(const float *src, int pe);

	[[deprecated]] double shmem_double_fetch(const double *src, int pe);

	[[deprecated]] int shmem_int_fetch(const int *src, int pe);

	[[deprecated]] long shmem_long_fetch(const long *src, int pe);

	[[deprecated]] long long shmem_longlong_fetch(const long long *src, int pe);

#else
	__attribute__((deprecated)) float shmem_float_fetch(const float *src, int pe);

	__attribute__((deprecated)) double shmem_double_fetch(const double *src, int pe);

	__attribute__((deprecated)) int shmem_int_fetch(const int *src, int pe);

	__attribute__((deprecated)) long shmem_long_fetch(const long *src, int pe);

	__attribute__((deprecated)) long long shmem_longlong_fetch(const long long *src, int pe);

#endif

/** @} */ // shmem_TYPE_atomic_fetch

/**
 * @name shmem_TYPE_atomic_set
 * @{
 *
 * @brief Perform an atomic set on a given symmetric object
 *
 * @param[in] dest  Symmetric address of the element to set
 * @param[in] value Value to put in remote memory
 * @param[in] pe    Unique identifier (rank) of the target
 */
void shmem_float_atomic_set(float *dest, float value, int pe);
void shmem_double_atomic_set(double *dest, double value, int pe);
void shmem_char_atomic_set(char *dest, char value, int pe);
void shmem_schar_atomic_set(signed char *dest, signed char value, int pe);
void shmem_short_atomic_set(short *dest, short value, int pe);
void shmem_int_atomic_set(int *dest, int value, int pe);
void shmem_long_atomic_set(long *dest, long value, int pe);
void shmem_longlong_atomic_set(long long *dest, long long value, int pe);
void shmem_uchar_atomic_set(unsigned char *dest, unsigned char value, int pe);
void shmem_ushort_atomic_set(unsigned short *dest, unsigned short value, int pe);
void shmem_uint_atomic_set(unsigned int *dest, unsigned int value, int pe);
void shmem_ulong_atomic_set(unsigned long *dest, unsigned long value, int pe);
void shmem_ulonglong_atomic_set(unsigned long long *dest, unsigned long long value, int pe);
void shmem_int8_t_atomic_set(int8_t *dest, int8_t value, int pe);
void shmem_uint8_t_atomic_set(uint8_t *dest, uint8_t value, int pe);
void shmem_int16_t_atomic_set(int16_t *dest, int16_t value, int pe);
void shmem_uint16_t_atomic_set(uint16_t *dest, uint16_t value, int pe);
void shmem_int32_t_atomic_set(int32_t *dest, int32_t value, int pe);
void shmem_uint32_t_atomic_set(uint32_t *dest, uint32_t value, int pe);
void shmem_int64_t_atomic_set(int64_t *dest, int64_t value, int pe);
void shmem_uint64_t_atomic_set(uint64_t *dest, uint64_t value, int pe);
void shmem_size_t_atomic_set(size_t *dest, size_t value, int pe);
void shmem_ptrdiff_t_atomic_set(ptrdiff_t *dest, ptrdiff_t value, int pe);

#if __STDC_VERSION__ >= 202300L
	[[deprecated]] void shmem_float_set(float *dest, float value, int pe);

	[[deprecated]] void shmem_double_set(double *dest, double value, int pe);

	[[deprecated]] void shmem_int_set(int *dest, int value, int pe);

	[[deprecated]] void shmem_long_set(long *dest, long value, int pe);

	[[deprecated]] void shmem_longlong_set(long long *dest, long long value, int pe);

#else
	__attribute__((deprecated)) void shmem_float_set(float *dest, float value, int pe);

	__attribute__((deprecated)) void shmem_double_set(double *dest, double value, int pe);

	__attribute__((deprecated)) void shmem_int_set(int *dest, int value, int pe);

	__attribute__((deprecated)) void shmem_long_set(long *dest, long value, int pe);

	__attribute__((deprecated)) void shmem_longlong_set(long long *dest, long long value, int pe);

#endif

/** @} */ // shmem_TYPE_atomic_set

/**
 * @name shmem_TYPE_atomic_compare_swap
 * @{
 *
 * @brief Perform an atomic compare and swap on a given symmetric object
 *
 * @param[in] dest  Symmetric address of the element to set
 * @param[in] cond  Value expected in the remote memory
 * @param[in] value Value to put in remote memory
 * @param[in] pe    Unique identifier (rank) of the target
 *
 * @retval  old_data        Previously held data (if condition passed)
 * @retval  current_data    Value currently in the remote memory (otherwise)
 */
float shmem_float_atomic_compare_swap(float *dest, float cond, float value, int pe);
double shmem_double_atomic_compare_swap(double *dest, double cond, double value, int pe);
char shmem_char_atomic_compare_swap(char *dest, char cond, char value, int pe);
signed char shmem_schar_atomic_compare_swap(signed char *dest, signed char cond, signed char value, int pe);
short shmem_short_atomic_compare_swap(short *dest, short cond, short value, int pe);
int shmem_int_atomic_compare_swap(int *dest, int cond, int value, int pe);
long shmem_long_atomic_compare_swap(long *dest, long cond, long value, int pe);
long long shmem_longlong_atomic_compare_swap(long long *dest, long long cond, long long value, int pe);
unsigned char shmem_uchar_atomic_compare_swap(unsigned char *dest, unsigned char cond, unsigned char value, int pe);
unsigned short shmem_ushort_atomic_compare_swap(unsigned short *dest, unsigned short cond,
                                                unsigned short value, int pe);
unsigned int shmem_uint_atomic_compare_swap(unsigned int *dest, unsigned int cond, unsigned int value, int pe);
unsigned long shmem_ulong_atomic_compare_swap(unsigned long *dest, unsigned long cond, unsigned long value, int pe);
unsigned long long shmem_ulonglong_atomic_compare_swap(unsigned long long *dest, unsigned long long cond,
                                                       unsigned long long value, int pe);
int8_t shmem_int8_t_atomic_compare_swap(int8_t *dest, int8_t cond, int8_t value, int pe);
uint8_t shmem_uint8_t_atomic_compare_swap(uint8_t *dest, uint8_t cond, uint8_t value, int pe);
int16_t shmem_int16_t_atomic_compare_swap(int16_t *dest, int16_t cond, int16_t value, int pe);
uint16_t shmem_uint16_t_atomic_compare_swap(uint16_t *dest, uint16_t cond, uint16_t value, int pe);
int32_t shmem_int32_t_atomic_compare_swap(int32_t *dest, int32_t cond, int32_t value, int pe);
uint32_t shmem_uint32_t_atomic_compare_swap(uint32_t *dest, uint32_t cond, uint32_t value, int pe);
int64_t shmem_int64_t_atomic_compare_swap(int64_t *dest, int64_t cond, int64_t value, int pe);
uint64_t shmem_uint64_t_atomic_compare_swap(uint64_t *dest, uint64_t cond, uint64_t value, int pe);
size_t shmem_size_t_atomic_compare_swap(size_t *dest, size_t cond, size_t value, int pe);
ptrdiff_t shmem_ptrdiff_t_atomic_compare_swap(ptrdiff_t *dest, ptrdiff_t cond, ptrdiff_t value, int pe);

#if __STDC_VERSION__ >= 202300L
	[[deprecated]] int shmem_int_cswap(int *dest, int cond, int value, int pe);

	[[deprecated]] long shmem_long_cswap(long *dest, long cond, long value, int pe);

	[[deprecated]] long long shmem_longlong_cswap(long long *dest, long long cond, long long value, int pe);

#else
	__attribute__((deprecated)) int shmem_int_cswap(int *dest, int cond, int value, int pe);

	__attribute__((deprecated)) long shmem_long_cswap(long *dest, long cond, long value, int pe);

	__attribute__((deprecated)) long long shmem_longlong_cswap(long long *dest, long long cond,
	                                                           long long value, int pe);

#endif

/** @} */ // shmem_TYPE_atomic_compare_swap

/**
 * @name shmem_TYPE_atomic_swap
 * @{
 *
 * @brief Perform an atomic swap on a given symmetric object
 *
 * @param[in] dest  Symmetric address of the element to set
 * @param[in] value Value to put in remote memory
 * @param[in] pe    Unique identifier (rank) of the target
 *
 * @retval  old_data        Previously held data
 */
float shmem_float_atomic_swap(float *dest, float value, int pe);
double shmem_double_atomic_swap(double *dest, double value, int pe);
char shmem_char_atomic_swap(char *dest, char value, int pe);
signed char shmem_schar_atomic_swap(signed char *dest, signed char value, int pe);
short shmem_short_atomic_swap(short *dest, short value, int pe);
int shmem_int_atomic_swap(int *dest, int value, int pe);
long shmem_long_atomic_swap(long *dest, long value, int pe);
long long shmem_longlong_atomic_swap(long long *dest, long long value, int pe);
unsigned char shmem_uchar_atomic_swap(unsigned char *dest, unsigned char value, int pe);
unsigned short shmem_ushort_atomic_swap(unsigned short *dest, unsigned short value, int pe);
unsigned int shmem_uint_atomic_swap(unsigned int *dest, unsigned int value, int pe);
unsigned long shmem_ulong_atomic_swap(unsigned long *dest, unsigned long value, int pe);
unsigned long long shmem_ulonglong_atomic_swap(unsigned long long *dest, unsigned long long value, int pe);
int8_t shmem_int8_t_atomic_swap(int8_t *dest, int8_t value, int pe);
uint8_t shmem_uint8_t_atomic_swap(uint8_t *dest, uint8_t value, int pe);
int16_t shmem_int16_t_atomic_swap(int16_t *dest, int16_t value, int pe);
uint16_t shmem_uint16_t_atomic_swap(uint16_t *dest, uint16_t value, int pe);
int32_t shmem_int32_t_atomic_swap(int32_t *dest, int32_t value, int pe);
uint32_t shmem_uint32_t_atomic_swap(uint32_t *dest, uint32_t value, int pe);
int64_t shmem_int64_t_atomic_swap(int64_t *dest, int64_t value, int pe);
uint64_t shmem_uint64_t_atomic_swap(uint64_t *dest, uint64_t value, int pe);
size_t shmem_size_t_atomic_swap(size_t *dest, size_t value, int pe);
ptrdiff_t shmem_ptrdiff_t_atomic_swap(ptrdiff_t *dest, ptrdiff_t value, int pe);

#if __STDC_VERSION__ >= 202300L
	[[deprecated]] float shmem_float_swap(float *dest, float value, int pe);

	[[deprecated]] double shmem_double_swap(double *dest, double value, int pe);

	[[deprecated]] int shmem_int_swap(int *dest, int value, int pe);

	[[deprecated]] long shmem_long_swap(long *dest, long value, int pe);

	[[deprecated]] long long shmem_longlong_swap(long long *dest, long long value, int pe);

#else
	__attribute__((deprecated)) float shmem_float_swap(float *dest, float value, int pe);

	__attribute__((deprecated)) double shmem_double_swap(double *dest, double value, int pe);

	__attribute__((deprecated)) int shmem_int_swap(int *dest, int value, int pe);

	__attribute__((deprecated)) long shmem_long_swap(long *dest, long value, int pe);

	__attribute__((deprecated)) long long shmem_longlong_swap(long long *dest, long long value, int pe);

#endif

/** @} */ // shmem_TYPE_atomic_swap

/**
 * @name shmem_TYPE_atomic_fetch_add
 * @{
 *
 * @brief Perform an atomic fetch and add on a given symmetric object
 *
 * @param[in] dest  Symmetric address of the element to set
 * @param[in] value Value to add to the remote memory object
 * @param[in] pe    Unique identifier (rank) of the target
 *
 * @retval  old_data        Previously held data
 */
float shmem_float_atomic_fetch_add(float *dest, float value, int pe);
double shmem_double_atomic_fetch_add(double *dest, double value, int pe);
char shmem_char_atomic_fetch_add(char *dest, char value, int pe);
signed char shmem_schar_atomic_fetch_add(signed char *dest, signed char value, int pe);
short shmem_short_atomic_fetch_add(short *dest, short value, int pe);
int shmem_int_atomic_fetch_add(int *dest, int value, int pe);
long shmem_long_atomic_fetch_add(long *dest, long value, int pe);
long long shmem_longlong_atomic_fetch_add(long long *dest, long long value, int pe);
unsigned char shmem_uchar_atomic_fetch_add(unsigned char *dest, unsigned char value, int pe);
unsigned short shmem_ushort_atomic_fetch_add(unsigned short *dest, unsigned short value, int pe);
unsigned int shmem_uint_atomic_fetch_add(unsigned int *dest, unsigned int value, int pe);
unsigned long shmem_ulong_atomic_fetch_add(unsigned long *dest, unsigned long value, int pe);
unsigned long long shmem_ulonglong_atomic_fetch_add(unsigned long long *dest, unsigned long long value, int pe);
int8_t shmem_int8_t_atomic_fetch_add(int8_t *dest, int8_t value, int pe);
uint8_t shmem_uint8_t_atomic_fetch_add(uint8_t *dest, uint8_t value, int pe);
int16_t shmem_int16_t_atomic_fetch_add(int16_t *dest, int16_t value, int pe);
uint16_t shmem_uint16_t_atomic_fetch_add(uint16_t *dest, uint16_t value, int pe);
int32_t shmem_int32_t_atomic_fetch_add(int32_t *dest, int32_t value, int pe);
uint32_t shmem_uint32_t_atomic_fetch_add(uint32_t *dest, uint32_t value, int pe);
int64_t shmem_int64_t_atomic_fetch_add(int64_t *dest, int64_t value, int pe);
uint64_t shmem_uint64_t_atomic_fetch_add(uint64_t *dest, uint64_t value, int pe);
size_t shmem_size_t_atomic_fetch_add(size_t *dest, size_t value, int pe);
ptrdiff_t shmem_ptrdiff_t_atomic_fetch_add(ptrdiff_t *dest, ptrdiff_t value, int pe);

#if __STDC_VERSION__ >= 202300L
	[[deprecated]] int shmem_int_fadd(int *dest, int value, int pe);

	[[deprecated]] long shmem_long_fadd(long *dest, long value, int pe);

	[[deprecated]] long long shmem_longlong_fadd(long long *dest, long long value, int pe);

#else
	__attribute__((deprecated)) int shmem_int_fadd(int *dest, int value, int pe);

	__attribute__((deprecated)) long shmem_long_fadd(long *dest, long value, int pe);

	__attribute__((deprecated)) long long shmem_longlong_fadd(long long *dest, long long value, int pe);

#endif
/** @} */ // shmem_TYPE_atomic_fetch_add

/**
 * @name shmem_TYPE_atomic_add
 * @{
 *
 * @brief Perform an atomic add on a given symmetric object
 *
 * @param[in] dest  Symmetric address of the element to set
 * @param[in] value Value to add to the remote memory object
 * @param[in] pe    Unique identifier (rank) of the target
 */
void shmem_float_atomic_add(float *dest, float value, int pe);
void shmem_double_atomic_add(double *dest, double value, int pe);
void shmem_char_atomic_add(char *dest, char value, int pe);
void shmem_schar_atomic_add(signed char *dest, signed char value, int pe);
void shmem_short_atomic_add(short *dest, short value, int pe);
void shmem_int_atomic_add(int *dest, int value, int pe);
void shmem_long_atomic_add(long *dest, long value, int pe);
void shmem_longlong_atomic_add(long long *dest, long long value, int pe);
void shmem_uchar_atomic_add(unsigned char *dest, unsigned char value, int pe);
void shmem_ushort_atomic_add(unsigned short *dest, unsigned short value, int pe);
void shmem_uint_atomic_add(unsigned int *dest, unsigned int value, int pe);
void shmem_ulong_atomic_add(unsigned long *dest, unsigned long value, int pe);
void shmem_ulonglong_atomic_add(unsigned long long *dest, unsigned long long value, int pe);
void shmem_int8_t_atomic_add(int8_t *dest, int8_t value, int pe);
void shmem_uint8_t_atomic_add(uint8_t *dest, uint8_t value, int pe);
void shmem_int16_t_atomic_add(int16_t *dest, int16_t value, int pe);
void shmem_uint16_t_atomic_add(uint16_t *dest, uint16_t value, int pe);
void shmem_int32_t_atomic_add(int32_t *dest, int32_t value, int pe);
void shmem_uint32_t_atomic_add(uint32_t *dest, uint32_t value, int pe);
void shmem_int64_t_atomic_add(int64_t *dest, int64_t value, int pe);
void shmem_uint64_t_atomic_add(uint64_t *dest, uint64_t value, int pe);
void shmem_size_t_atomic_add(size_t *dest, size_t value, int pe);
void shmem_ptrdiff_t_atomic_add(ptrdiff_t *dest, ptrdiff_t value, int pe);

#if __STDC_VERSION__ >= 202300L
	[[deprecated]] void shmem_int_add(int *dest, int value, int pe);

	[[deprecated]] void shmem_long_add(long *dest, long value, int pe);

	[[deprecated]] void shmem_longlong_add(long long *dest, long long value, int pe);

#else
	__attribute__((deprecated)) void shmem_int_add(int *dest, int value, int pe);

	__attribute__((deprecated)) void shmem_long_add(long *dest, long value, int pe);

	__attribute__((deprecated)) void shmem_longlong_add(long long *dest, long long value, int pe);

#endif

/** @} */ // shmem_TYPE_atomic_add

/**
 * @name shmem_TYPE_atomic_fetch_inc
 * @{
 *
 * @brief Perform an atomic fetch and increment on a given symmetric object
 *
 * @param[in] dest Symmetric address of the element to increment
 * @param[in] pe   Unique identifier (rank) of the target
 *
 * @retval  old_data        Previously held data
 */
float shmem_float_atomic_fetch_inc(float *dest, int pe);
double shmem_double_atomic_fetch_inc(double *dest, int pe);
char shmem_char_atomic_fetch_inc(char *dest, int pe);
signed char shmem_schar_atomic_fetch_inc(signed char *dest, int pe);
short shmem_short_atomic_fetch_inc(short *dest, int pe);
int shmem_int_atomic_fetch_inc(int *dest, int pe);
long shmem_long_atomic_fetch_inc(long *dest, int pe);
long long shmem_longlong_atomic_fetch_inc(long long *dest, int pe);
unsigned char shmem_uchar_atomic_fetch_inc(unsigned char *dest, int pe);
unsigned short shmem_ushort_atomic_fetch_inc(unsigned short *dest, int pe);
unsigned int shmem_uint_atomic_fetch_inc(unsigned int *dest, int pe);
unsigned long shmem_ulong_atomic_fetch_inc(unsigned long *dest, int pe);
unsigned long long shmem_ulonglong_atomic_fetch_inc(unsigned long long *dest, int pe);
int8_t shmem_int8_t_atomic_fetch_inc(int8_t *dest, int pe);
uint8_t shmem_uint8_t_atomic_fetch_inc(uint8_t *dest, int pe);
int16_t shmem_int16_t_atomic_fetch_inc(int16_t *dest, int pe);
uint16_t shmem_uint16_t_atomic_fetch_inc(uint16_t *dest, int pe);
int32_t shmem_int32_t_atomic_fetch_inc(int32_t *dest, int pe);
uint32_t shmem_uint32_t_atomic_fetch_inc(uint32_t *dest, int pe);
int64_t shmem_int64_t_atomic_fetch_inc(int64_t *dest, int pe);
uint64_t shmem_uint64_t_atomic_fetch_inc(uint64_t *dest, int pe);
size_t shmem_size_t_atomic_fetch_inc(size_t *dest, int pe);
ptrdiff_t shmem_ptrdiff_t_atomic_fetch_inc(ptrdiff_t *dest, int pe);

#if __STDC_VERSION__ >= 202300L
	[[deprecated]] int shmem_int_finc(int *dest, int pe);

	[[deprecated]] long shmem_long_finc(long *dest, int pe);

	[[deprecated]] long long shmem_longlong_finc(long long *dest, int pe);

#else
	__attribute__((deprecated)) int shmem_int_finc(int *dest, int pe);

	__attribute__((deprecated)) long shmem_long_finc(long *dest, int pe);

	__attribute__((deprecated)) long long shmem_longlong_finc(long long *dest, int pe);

#endif
/** @} */ // shmem_TYPE_atomic_fetch_inc

/**
 * @name shmem_TYPE_atomic_inc
 * @{
 *
 * @brief Perform an atomic increment on a given symmetric object
 *
 * @param[in] dest Symmetric address of the element to increment
 * @param[in] pe   Unique identifier (rank) of the target
 */
void shmem_float_atomic_inc(float *dest, int pe);
void shmem_double_atomic_inc(double *dest, int pe);
void shmem_char_atomic_inc(char *dest, int pe);
void shmem_schar_atomic_inc(signed char *dest, int pe);
void shmem_short_atomic_inc(short *dest, int pe);
void shmem_int_atomic_inc(int *dest, int pe);
void shmem_long_atomic_inc(long *dest, int pe);
void shmem_longlong_atomic_inc(long long *dest, int pe);
void shmem_uchar_atomic_inc(unsigned char *dest, int pe);
void shmem_ushort_atomic_inc(unsigned short *dest, int pe);
void shmem_uint_atomic_inc(unsigned int *dest, int pe);
void shmem_ulong_atomic_inc(unsigned long *dest, int pe);
void shmem_ulonglong_atomic_inc(unsigned long long *dest, int pe);
void shmem_int8_t_atomic_inc(int8_t *dest, int pe);
void shmem_uint8_t_atomic_inc(uint8_t *dest, int pe);
void shmem_int16_t_atomic_inc(int16_t *dest, int pe);
void shmem_uint16_t_atomic_inc(uint16_t *dest, int pe);
void shmem_int32_t_atomic_inc(int32_t *dest, int pe);
void shmem_uint32_t_atomic_inc(uint32_t *dest, int pe);
void shmem_int64_t_atomic_inc(int64_t *dest, int pe);
void shmem_uint64_t_atomic_inc(uint64_t *dest, int pe);
void shmem_size_t_atomic_inc(size_t *dest, int pe);
void shmem_ptrdiff_t_atomic_inc(ptrdiff_t *dest, int pe);

#if __STDC_VERSION__ >= 202300L
	[[deprecated]] void shmem_int_inc(int *dest, int pe);

	[[deprecated]] void shmem_long_inc(long *dest, int pe);

	[[deprecated]] void shmem_longlong_inc(long long *dest, int pe);

#else
	__attribute__((deprecated)) void shmem_int_inc(int *dest, int pe);

	__attribute__((deprecated)) void shmem_long_inc(long *dest, int pe);

	__attribute__((deprecated)) void shmem_longlong_inc(long long *dest, int pe);

#endif
/** @} */ // shmem_TYPE_atomic_inc

/**
 * @brief OpenSHMEM atomic fetch operation
 *
 * @note Generic counterpart of the #shmem_TYPE_atomic_fetch functions.
 *
 * @param[in] src Symmetric address from which the data is read
 * @param[in] pe  Unique identifier (rank) of the target
 */
#define shmem_atomic_fetch(src, pe)                    \
		_Generic(src,                                  \
	float *: shmem_float_atomic_fetch,                 \
	double *: shmem_double_atomic_fetch,               \
	char *: shmem_char_atomic_fetch,                   \
	signed char *: shmem_schar_atomic_fetch,           \
	short *: shmem_short_atomic_fetch,                 \
	int *: shmem_int_atomic_fetch,                     \
	long *: shmem_long_atomic_fetch,                   \
	long long *: shmem_longlong_atomic_fetch,          \
	unsigned char *: shmem_uchar_atomic_fetch,         \
	unsigned short *: shmem_ushort_atomic_fetch,       \
	unsigned int *: shmem_uint_atomic_fetch,           \
	unsigned long *: shmem_ulong_atomic_fetch,         \
	unsigned long long *: shmem_ulonglong_atomic_fetch \
	)(src, pe)

#define shmem_fetch(src, pe)           \
		_Generic(src,                  \
	float *: shmem_float_fetch,        \
	double *: shmem_double_fetch,      \
	int *: shmem_int_fetch,            \
	long *: shmem_long_fetch,          \
	long long *: shmem_longlong_fetch, \
	)(src, pe)

/**
 * @brief OpenSHMEM atomic set operation
 *
 * @note Generic counterpart of the #shmem_TYPE_atomic_set functions.
 *
 * @param[in] dest  Symmetric address of the element to set
 * @param[in] value Value to put in remote memory
 * @param[in] pe    Unique identifier (rank) of the target
 */
#define shmem_atomic_set(dest, value, pe)          \
		_Generic(value,                            \
	float: shmem_float_atomic_set,                 \
	double: shmem_double_atomic_set,               \
	char: shmem_char_atomic_set,                   \
	signed char: shmem_schar_atomic_set,           \
	short: shmem_short_atomic_set,                 \
	int: shmem_int_atomic_set,                     \
	long: shmem_long_atomic_set,                   \
	long long: shmem_longlong_atomic_set,          \
	unsigned char: shmem_uchar_atomic_set,         \
	unsigned short: shmem_ushort_atomic_set,       \
	unsigned int: shmem_uint_atomic_set,           \
	unsigned long: shmem_ulong_atomic_set,         \
	unsigned long long: shmem_ulonglong_atomic_set \
	)(dest, value, pe)

#define shmem_set(dest, value, pe) \
		_Generic(value,            \
	float: shmem_float_set,        \
	double: shmem_double_set,      \
	int: shmem_int_set,            \
	long: shmem_long_set,          \
	long long: shmem_longlong_set, \
	)(dest, value, pe)

/**
 * @brief OpenSHMEM atomic compare and swap operation
 *
 * @note Generic counterpart of the #shmem_TYPE_atomic_compare_swap functions.
 *
 * @param[in] dest  Symmetric address of the element to swap
 * @param[in] cond  Value expected in the remote memory
 * @param[in] value Value to put in remote memory
 * @param[in] pe    Unique identifier (rank) of the target
 *
 * @retval  old_data        Previously held data (if condition passed)
 * @retval  current_data    Value currently in the remote memory (otherwise)
 */
#define shmem_atomic_compare_swap(dest, cond, value, pe)    \
		_Generic(value,                                     \
	float: shmem_float_atomic_compare_swap,                 \
	double: shmem_double_atomic_compare_swap,               \
	char: shmem_char_atomic_compare_swap,                   \
	signed char: shmem_schar_atomic_compare_swap,           \
	short: shmem_short_atomic_compare_swap,                 \
	int: shmem_int_atomic_compare_swap,                     \
	long: shmem_long_atomic_compare_swap,                   \
	long long: shmem_longlong_atomic_compare_swap,          \
	unsigned char: shmem_uchar_atomic_compare_swap,         \
	unsigned short: shmem_ushort_atomic_compare_swap,       \
	unsigned int: shmem_uint_atomic_compare_swap,           \
	unsigned long: shmem_ulong_atomic_compare_swap,         \
	unsigned long long: shmem_ulonglong_atomic_compare_swap \
	)(dest, cond, value, pe)

#define shmem_cswap(dest, cond, value, pe) \
		_Generic(value,                    \
	int: shmem_int_cswap,                  \
	long: shmem_long_cswap,                \
	long long: shmem_longlong_cswap,       \
	)(dest, cond, value, pe)

/**
 * @brief OpenSHMEM atomic swap operation
 *
 * @note Generic counterpart of the #shmem_TYPE_atomic_swap functions.
 *
 * @param[in] dest  Symmetric address of the element to swap
 * @param[in] value Value to put in remote memory
 * @param[in] pe    Unique identifier (rank) of the target
 *
 * @retval  old_data        Previously held data
 */
#define shmem_atomic_swap(dest, value, pe)          \
		_Generic(value,                             \
	float: shmem_float_atomic_swap,                 \
	double: shmem_double_atomic_swap,               \
	char: shmem_char_atomic_swap,                   \
	signed char: shmem_schar_atomic_swap,           \
	short: shmem_short_atomic_swap,                 \
	int: shmem_int_atomic_swap,                     \
	long: shmem_long_atomic_swap,                   \
	long long: shmem_longlong_atomic_swap,          \
	unsigned char: shmem_uchar_atomic_swap,         \
	unsigned short: shmem_ushort_atomic_swap,       \
	unsigned int: shmem_uint_atomic_swap,           \
	unsigned long: shmem_ulong_atomic_swap,         \
	unsigned long long: shmem_ulonglong_atomic_swap \
	)(dest, value, pe)

#define shmem_swap(dest, value, pe) \
		_Generic(value,             \
	float: shmem_float_swap,        \
	double: shmem_double_swap,      \
	int: shmem_int_swap,            \
	long: shmem_long_swap,          \
	long long: shmem_longlong_swap, \
	)(dest, value, pe)

/**
 * @brief OpenSHMEM atomic fetch and add operation
 *
 * @note Generic counterpart of the #shmem_TYPE_atomic_fetch_add functions.
 *
 * @param[in] dest  Symmetric address of the element to fetch and add to
 * @param[in] value Value to add to the remote memory object
 * @param[in] pe    Unique identifier (rank) of the target
 *
 * @retval  old_data        Previously held data
 */
#define shmem_atomic_fetch_add(dest, value, pe)          \
		_Generic(value,                                  \
	float: shmem_float_atomic_fetch_add,                 \
	double: shmem_double_atomic_fetch_add,               \
	char: shmem_char_atomic_fetch_add,                   \
	signed char: shmem_schar_atomic_fetch_add,           \
	short: shmem_short_atomic_fetch_add,                 \
	int: shmem_int_atomic_fetch_add,                     \
	long: shmem_long_atomic_fetch_add,                   \
	long long: shmem_longlong_atomic_fetch_add,          \
	unsigned char: shmem_uchar_atomic_fetch_add,         \
	unsigned short: shmem_ushort_atomic_fetch_add,       \
	unsigned int: shmem_uint_atomic_fetch_add,           \
	unsigned long: shmem_ulong_atomic_fetch_add,         \
	unsigned long long: shmem_ulonglong_atomic_fetch_add \
	)(dest, value, pe)

#define shmem_fadd(dest, value, pe) \
		_Generic(value,             \
	int: shmem_int_fadd,            \
	long: shmem_long_fadd,          \
	long long: shmem_longlong_fadd, \
	)(dest, value, pe)

/**
 * @brief OpenSHMEM atomic add operation
 *
 * @note Generic counterpart of the #shmem_TYPE_atomic_add functions.
 *
 * @param[in] dest  Symmetric address of the element to add to
 * @param[in] value Value to add to the remote memory object
 * @param[in] pe    Unique identifier (rank) of the target
 */
#define shmem_atomic_add(dest, value, pe)          \
		_Generic(value,                            \
	float: shmem_float_atomic_add,                 \
	double: shmem_double_atomic_add,               \
	char: shmem_char_atomic_add,                   \
	signed char: shmem_schar_atomic_add,           \
	short: shmem_short_atomic_add,                 \
	int: shmem_int_atomic_add,                     \
	long: shmem_long_atomic_add,                   \
	long long: shmem_longlong_atomic_add,          \
	unsigned char: shmem_uchar_atomic_add,         \
	unsigned short: shmem_ushort_atomic_add,       \
	unsigned int: shmem_uint_atomic_add,           \
	unsigned long: shmem_ulong_atomic_add,         \
	unsigned long long: shmem_ulonglong_atomic_add \
	)(dest, value, pe)

#define shmem_add(dest, value, pe) \
		_Generic(value,            \
	int: shmem_int_add,            \
	long: shmem_long_add,          \
	long long: shmem_longlong_add, \
	)(dest, value, pe)

/**
 * @brief OpenSHMEM atomic fetch and increment operation
 *
 * @note Generic counterpart of the #shmem_TYPE_atomic_fetch_inc functions.
 *
 * @param[in] dest Symmetric address of the element to fetch and increment
 * @param[in] pe   Unique identifier (rank) of the target
 *
 * @retval  old_data        Previously held data
 */
#define shmem_atomic_fetch_inc(dest, pe)                   \
		_Generic(dest,                                     \
	float *: shmem_float_atomic_fetch_inc,                 \
	double *: shmem_double_atomic_fetch_inc,               \
	char *: shmem_char_atomic_fetch_inc,                   \
	signed char *: shmem_schar_atomic_fetch_inc,           \
	short *: shmem_short_atomic_fetch_inc,                 \
	int *: shmem_int_atomic_fetch_inc,                     \
	long *: shmem_long_atomic_fetch_inc,                   \
	long long *: shmem_longlong_atomic_fetch_inc,          \
	unsigned char *: shmem_uchar_atomic_fetch_inc,         \
	unsigned short *: shmem_ushort_atomic_fetch_inc,       \
	unsigned int *: shmem_uint_atomic_fetch_inc,           \
	unsigned long *: shmem_ulong_atomic_fetch_inc,         \
	unsigned long long *: shmem_ulonglong_atomic_fetch_inc \
	)(dest, pe)

#define shmem_finc(dest, pe)        \
		_Generic(value,             \
	int: shmem_int_finc,            \
	long: shmem_long_finc,          \
	long long: shmem_longlong_finc, \
	)(dest, pe)

/**
 * @brief OpenSHMEM atomic increment operation
 *
 * @note Generic counterpart of the #shmem_TYPE_atomic_inc functions.
 *
 * @param[in] dest Symmetric address of the element to increment
 * @param[in] pe   Unique identifier (rank) of the target
 */
#define shmem_atomic_inc(dest, pe)                   \
		_Generic(dest,                               \
	float *: shmem_float_atomic_inc,                 \
	double *: shmem_double_atomic_inc,               \
	char *: shmem_char_atomic_inc,                   \
	signed char *: shmem_schar_atomic_inc,           \
	short *: shmem_short_atomic_inc,                 \
	int *: shmem_int_atomic_inc,                     \
	long *: shmem_long_atomic_inc,                   \
	long long *: shmem_longlong_atomic_inc,          \
	unsigned char *: shmem_uchar_atomic_inc,         \
	unsigned short *: shmem_ushort_atomic_inc,       \
	unsigned int *: shmem_uint_atomic_inc,           \
	unsigned long *: shmem_ulong_atomic_inc,         \
	unsigned long long *: shmem_ulonglong_atomic_inc \
	)(dest, pe)

#define shmem_inc(dest, pe)        \
		_Generic(value,            \
	int: shmem_int_inc,            \
	long: shmem_long_inc,          \
	long long: shmem_longlong_inc, \
	)(dest, pe)
/** @} */ // MPC_OPENSHMEM_API_ATO


/**
 * @defgroup MPC_OPENSHMEM_API_COLL_SYNC Collectives Synchronizations
 * @{
 * @brief OpenSHMEM defined Collective Synchronizations routines
 */

/**
 * @brief Wait until the completion of all pending communications
 */
void shmem_quiet(void);

/**
 * @brief Assess the ordering of communications
 */
void shmem_fence(void);

/**
 * @brief Synchronize all tasks and guarantee the completion of all pending communication
 */
void shmem_barrier_all(void);

/** @} */ // MPC_OPENSHMEM_API_COLL_SYNC
/** @} */ // MPC_OPENSHMEM_API

#ifdef __cplusplus
	} // extern "C"
#endif

#endif // !MPC_OPENSHMEM_API_H_
