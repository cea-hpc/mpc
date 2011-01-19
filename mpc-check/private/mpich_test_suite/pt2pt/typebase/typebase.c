/*
 */
#include "mpi.h"
#include <stdio.h>
#include "test.h"

/* 
 * This program checks that the type inquiry routines work with the 
 * basic types
 */

#define MAX_TYPES 14
/* static int ntypes; */
/* static MPI_Datatype BasicTypes[MAX_TYPES]; */
/* static char         *(BasicTypesName[MAX_TYPES]); */
/* static int          BasicSizes[MAX_TYPES]; */

typedef struct {
 int ntypes;
 MPI_Datatype BasicTypes[MAX_TYPES];
 char         *(BasicTypesName[MAX_TYPES]);
 int          BasicSizes[MAX_TYPES];
} privatise_t;
static privatise_t privatise[4096];

static int __mpc_get_rank(){
  int myrank;
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  return myrank;
}

#define ntypes privatise[__mpc_get_rank()].ntypes
#define BasicTypes privatise[__mpc_get_rank()].BasicTypes
#define BasicTypesName privatise[__mpc_get_rank()].BasicTypesName
#define BasicSizes privatise[__mpc_get_rank()].BasicSizes



/* Prototypes for picky compilers */
void SetupBasicTypes (void);

void 
SetupBasicTypes()
{
    BasicTypes[0] = MPI_CHAR;
    BasicTypes[1] = MPI_SHORT;
    BasicTypes[2] = MPI_INT;
    BasicTypes[3] = MPI_LONG;
    BasicTypes[4] = MPI_UNSIGNED_CHAR;
    BasicTypes[5] = MPI_UNSIGNED_SHORT;
    BasicTypes[6] = MPI_UNSIGNED;
    BasicTypes[7] = MPI_UNSIGNED_LONG;
    BasicTypes[8] = MPI_FLOAT;
    BasicTypes[9] = MPI_DOUBLE;

    BasicTypesName[0] = "MPI_CHAR";
    BasicTypesName[1] = "MPI_SHORT";
    BasicTypesName[2] = "MPI_INT";
    BasicTypesName[3] = "MPI_LONG";
    BasicTypesName[4] = "MPI_UNSIGNED_CHAR";
    BasicTypesName[5] = "MPI_UNSIGNED_SHORT";
    BasicTypesName[6] = "MPI_UNSIGNED";
    BasicTypesName[7] = "MPI_UNSIGNED_LONG";
    BasicTypesName[8] = "MPI_FLOAT";
    BasicTypesName[9] = "MPI_DOUBLE";

    BasicSizes[0] = sizeof(char);
    BasicSizes[1] = sizeof(short);
    BasicSizes[2] = sizeof(int);
    BasicSizes[3] = sizeof(long);
    BasicSizes[4] = sizeof(unsigned char);
    BasicSizes[5] = sizeof(unsigned short);
    BasicSizes[6] = sizeof(unsigned);
    BasicSizes[7] = sizeof(unsigned long);
    BasicSizes[8] = sizeof(float);
    BasicSizes[9] = sizeof(double);

    ntypes = 10;
#ifdef HAVE_LONG_DOUBLE
    BasicTypes[ntypes] = MPI_LONG_DOUBLE;
    BasicSizes[ntypes] = sizeof(long double);
    BasicTypesName[ntypes] = "MPI_LONG_DOUBLE";
    ntypes++;
#endif
    BasicTypes[ntypes] = MPI_BYTE;
    BasicSizes[ntypes] = sizeof(unsigned char);
    BasicTypesName[ntypes] = "MPI_BYTE";
    ntypes++;

#ifdef HAVE_LONG_LONG_INT
    BasicTypes[ntypes] = MPI_LONG_LONG_INT;
    BasicSizes[ntypes] = sizeof(long long);
    BasicTypesName[ntypes] = "MPI_LONG_LONG_INT";
    ntypes++;
#endif
    }

int main( int argc, char **argv )
{
int      i, errs;
int      size;
MPI_Aint extent, lb, ub;
 
MPI_Init( &argc, &argv );

/* This should be run by a single process */

SetupBasicTypes();

errs = 0;
for (i=0; i<ntypes; i++) {
    MPI_Type_size( BasicTypes[i], &size );
    MPI_Type_extent( BasicTypes[i], &extent );
    MPI_Type_lb( BasicTypes[i], &lb );
    MPI_Type_ub( BasicTypes[i], &ub );
    if (size != extent) {
	errs++;
	printf( "size (%d) != extent (%ld) for basic type %s\n", size, 
		(long) extent, BasicTypesName[i] );
	}
    if (size != BasicSizes[i]) {
	errs++;
	printf( "size(%d) != C size (%d) for basic type %s\n", size, 
	       BasicSizes[i], BasicTypesName[i] );
	}
    if (lb != 0) {
	errs++;
	printf( "Lowerbound of %s was %d instead of 0\n", 
	        BasicTypesName[i], (int)lb );
	}
    if (ub != extent) {
	errs++;
	printf( "Upperbound of %s was %d instead of %d\n", 
	        BasicTypesName[i], (int)ub, (int)extent );
	}
    }

if (errs) {
    printf( "Found %d errors in testing C types\n", errs );
    }
else {
    printf( "Found no errors in basic C types\n" );
    }

MPI_Finalize( );
return errs;
}
