#include "sctk_alloc_posix.h"

#include "sctk_config.h"
#include "sctk_spinlock.h"

#ifdef MPC_Threads
#include "sctk_context.h"
#endif

void * sctk_user_mmap (void *start, size_t length, int prot, int flags,int fd, off_t offset) {return mmap(start,length,prot,flags,fd,offset);};

#ifdef SCTK_USE_TLS
  __thread void *sctk_tls_key;
#else
  static sctk_thread_key_t sctk_tls_key;
#endif
  