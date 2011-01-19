#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>

#define addtimeval(a,b) do{\
  (a)->tv_sec += (b)->tv_sec;\
  (a)->tv_usec += (b)->tv_usec;\
  while (((a)->tv_usec) > 1000000) {\
    (a)->tv_sec += 1;\
    (a)->tv_usec -= 1000000;\
  }\
}while(0)

#define subtimeval(a,b) do{\
  (a)->tv_sec -= (b)->tv_sec;\
  (a)->tv_usec -= (b)->tv_usec;\
  while (((a)->tv_usec) < 0) {\
    (a)->tv_sec -= 1;\
    (a)->tv_usec += 1000000;\
  }\
}while(0)

#define print_date(a,out) do{\
  char __mot__long__[7];\
  int i;\
  int size;\
  size = 100000;\
  for(i=0;i<7;i++){\
    __mot__long__[i]='\0';\
  }\
  i = 0;\
  while(((((long  )(a)->tv_usec) / size) == 0)&&(size>1)){\
    __mot__long__[i]= '0';\
    i++;\
    size = size / 10;\
  }\
  fprintf(out,"%ld.%s%ld",(long  )(a)->tv_sec,__mot__long__,(long  )(a)->tv_usec);\
  }while(0)

#define give_date(a,out) do{\
  char __mot__long__res[70];\
  char __mot__long__[7];\
  int i;\
  int size;\
  size = 100000;\
  for(i=0;i<7;i++){\
    __mot__long__[i]='\0';\
  }\
  i = 0;\
  while(((((long  )(a)->tv_usec) / size) == 0)&&(size>1)){\
    __mot__long__[i]= '0';\
    i++;\
    size = size / 10;\
  }\
  sprintf(__mot__long__res,"%ld.%s%ld",(long  )(a)->tv_sec,__mot__long__,(long  )(a)->tv_usec);\
  out = atof(__mot__long__res);\
  }while(0)
