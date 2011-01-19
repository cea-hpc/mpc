#ifndef PARALLELPP_MPC_C
#define PARALLELPP_MPC_C

#include <string.h>

#ifndef __INTEL_COMPILER
#ifdef __GNUG__
#pragma implementation "MPC.h"
#endif
#endif

#include "parallelpp/TaskInfo.h"
#include "parallelpp/MPC.h"

TaskInfo *taskinfo;

int _MYRANK_ = 0;
int _MYPROC_ = 0;


#endif
