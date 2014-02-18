/*
 * mpimem.h
 *
 *   Extracted from mpich2
 *   (C) 2001 by Argonne National Laboratory.
 *
 *  Created on: 1 juil. 2011
 *      Author: SHT
 */

#ifndef MPIMEM_H_
#define MPIMEM_H_

#include "mpl.h"

#define MPIU_Snprintf MPL_snprintf
#define MPIU_Exit exit
#define MPIU_Internal_error_printf printf

int MPIU_Strncpy( char *outstr, const char *instr, size_t maxlen );
int MPIU_Strnapp( char *, const char *, size_t );


#endif /* MPIMEM_H_ */
