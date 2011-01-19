/*
 * flavors.h : differents systemes.
 *
 */

#ifndef UTILS_FLAVORS_H
#define UTILS_FLAVORS_H

/*
 *   Definition des differents drapeaux a partir des definitions
 * fournies par le compilateur.
 *
 */

#if !defined(__unix__) && defined(__unix)
#define __unix__
#endif

#if !defined(unix) && defined(__unix__)
#define unix
#endif

#if !defined(UNIX) && defined(unix)
#define UNIX
#endif
#if !defined(unix) && defined(UNIX)
#define unix
#endif

#if !defined(__unix__) && defined(unix)
#define __unix__
#endif

#if !defined(SGI) && defined(sgi)
#define SGI
#endif
#if !defined(sgi) && defined(SGI)
#define sgi
#endif

#if !defined(IBM) && defined(ibm)
#define IBM
#endif
#if !defined(ibm) && defined(IBM)
#define ibm
#endif

#if !defined(cray) && defined(_CRAYC)
#define cray
#endif

#if !defined(CRAY) && defined(cray)
#define CRAY
#endif
#if !defined(cray) && defined(CRAY)
#define cray
#endif

#if !defined(hp) && defined(hppa)
#define hp
#endif
#if !defined(HP) && defined(hp)
#define HP
#endif
#if !defined(hp) && defined(HP)
#define hp
#endif

#if !defined(HPUX) && (defined(hpux) || (defined(hp) && defined(unix)))
#define HPUX
#endif
#if !defined(hpux) && defined(HPUX)
#define hpux
#endif

#if !defined(IRIX) && (defined(irix) || (defined(sgi) && defined(unix)))
#define IRIX
#endif
#if !defined(irix) && defined(IRIX)
#define irix
#endif

#if !defined(AIX) && (defined(aix) || (defined(ibm) && defined(unix)))
#define AIX
#endif
#if !defined(aix) && defined(AIX)
#define aix
#endif

#if !defined(UNICOS) && (defined(unicos) || (defined(cray) && defined(unix)))
#define UNICOS
#endif
#if !defined(unicos) && defined(UNICOS)
#define unicos
#endif

#if !defined(SUNOS) && (defined(sunos) || (defined(sun) && defined(unix)))
#  define SUNOS
#  if defined(__svr4__)
#    ifndef SOLARIS
#      define SOLARIS
#    endif
#  elif !defined(bsd)
#    define bsd
#  endif
#  if !defined(sun) && defined(SUNOS)
#    define sun
#  endif
#endif

#if !defined(BSD) && defined(bsd)
#  define BSD
#endif
#if !defined(bsd) && defined(BSD)
#  define bsd
#endif

#if !defined(SYSV) && (defined(SOLARIS) || defined(IRIX))
#  define SYSV
#endif

#if !defined(sysv) && defined(SYSV)
#  define sysv
#endif

#if !defined(unix)
#  if defined(IRIX) || defined(HPUX) || defined(AIX) || defined(UNICOS)
#    define unix
#    if !defined(UNIX)
#      define UNIX
#    endif
#  endif
#endif

#ifdef IRIX
#ifndef _SGI_SOURCE
#define _SGI_SOURCE
#endif
/* #define _STANDALONE */	/* A decommenter si sys/ptimers.h absent. */
#endif

/*
 *  Description d'un certain nombre de capacites des differents systemes
 * sur lesquels on peut etre amene a compiler.
 *
 */


#define HAVE_TIMES 1
#define HAVE_GETRUSAGE 1

#ifndef HAVE_TIMES
#define HAVE_TIMES 0
#endif
#ifndef HAVE_GETRUSAGE
#define HAVE_GETRUSAGE 0
#endif
#ifndef HAVE_CLOCK
#define HAVE_CLOCK 0
#endif

#ifndef NOLIB_USER
#define HAVE_LIB_USER 1
#endif

#if defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)
// #undef HAVE_GETRUSAGE
#undef HAVE_TIMES
#endif

#if defined(SUNOS) || defined(SOLARIS) || defined(IRIX) || defined(UNICOS) || defined(LINUX)
#define HAVE_MEMAVAIL 1
#else
#define HAVE_MEMAVAIL 0
#endif

#if defined(SUNOS) || defined(SOLARIS) || defined(IRIX) || defined(UNICOS) \
    || defined(HPUX) || defined(OSF1)|| defined(LINUX)
#define HAVE_FCNTL 1
#define HAVE_LOCKF 1
#define HAVE_FLOCK 1
#else
#define HAVE_FCNTL 0
#define HAVE_LOCKF 0
#define HAVE_FLOCK 0
#endif

#define HAVE_LOCKING	(HAVE_FCNTL || HAVE_LOCKF || HAVE_FLOCK)

#if defined(HPUX) || defined(UNICOS) || defined(AIX)|| defined(OSF1)
#define HAVE_RLIMIT_DATA 0
#define HAVE_VSYSLOG 0
#else
#define HAVE_RLIMIT_DATA 1
#define HAVE_VSYSLOG 1
#endif

#if defined (UNICOS) || defined(AIX) || defined(OSF1) || defined(LINUX)
#define HAVE_DATAAVAIL 1	/* Via sysconf(). */
#else
#define HAVE_DATAAVAIL HAVE_RLIMIT_DATA
#endif

#if defined(UNICOS)
#define HAVE_DATAUSED 0
#else
#define HAVE_DATAUSED 1
#endif

#if defined(UNICOS)
#if !defined(CRAY_YMP) && !defined(CRAY_T3D) && !defined(CRAY_T90)
#define CRAY_YMP
#endif
#endif

#if !defined(HAVE_IEEE_FP) && defined(CRAY)
#define HAVE_IEEE_FP _CRAYIEEE
#endif

#if !defined(HAVE_IEEE_FP)
#define HAVE_IEEE_FP 1
#endif

#define HAVE_CRAY_FP (!HAVE_IEEE_FP)

#ifdef __cplusplus
#define HAVE_VECTOR_INLINE 1
#endif

#ifndef HAVE_VECTOR_INLINE
#define HAVE_VECTOR_INLINE 0
#endif

#ifndef HAVE_PTHREAD_H
#define HAVE_PTHREAD_H 0
#endif

#ifndef MULTITHREADED
#if HAVE_PTHREAD_H
#define MULTITHREADED
#endif
#endif

#ifndef HAVE_USLEEP
#ifdef SUNOS
#define HAVE_USLEEP 1
#else
#define HAVE_USLEEP 0
#endif
#endif

#if (defined(IRIX) || defined(SUNOS) || defined(AIX))  && !defined (__GNUC__)
#   define TEMPLATE_NULL template<>
#else
#   define TEMPLATE_NULL
#endif

#if defined (__GNUC__)
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ > 2)
#undef TEMPLATE_NULL
#define TEMPLATE_NULL template<>
#endif
#endif

#if defined (AIX_SYS)
#undef TEMPLATE_NULL
#define TEMPLATE_NULL template<>
#endif

#if defined (__DECCXX) || defined (__DECC) || defined (__decc) || defined(SUNOS) && ! defined(__GNUC__)
#define __FUNCTION__ ""
#endif
#endif

#if defined(SUNOS) && !defined(__GNUC__)
#define inline 
#endif
   
#if defined(__IBM_COMPILER) || defined(__DEC_COMPILER) || defined(SUNOS) && ! defined(__GNUC__)
#define NO_VARGS_MACRO
#endif 
