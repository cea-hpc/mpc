// David Dureau - 06/12/99
// Implementation de la classe MPC_MPI heritee de MPC
// Il s'agit d'implementer des fonctionnalites MPI

#ifndef PARALLELPP_MPC_MPI_C
#define PARALLELPP_MPC_MPI_C

#if NOMPI
#undef MPI_MPC_MPI_C
#else
#define MPI_MPC_MPI_C
#endif


#include "utilspp/mach_types.h"

#ifdef MPI_MPC_MPI_C
// inclusion des prototypes des fonctionnalites MPI

#ifndef __INTEL_COMPILER 
#ifdef __GNUG__
#pragma implementation "MPC_MPI.h"
#endif
#endif

#include <mpi.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <typeinfo>
#include <iostream>

using namespace std;


#ifdef USE_VT
#include "utilspp/VT.h"
#endif
#include "utilspp/svalloc.h"

// inclusion du prototype de la classe MPC_MPI
#include "parallelpp/MPC_MPI.h"
#include "parallelpp/TaskInfo.h"


#define PACKARRAY_BLK_SIZE 40
#define MEM_BLK 1000

#define REQCNT_INCREM 1

struct mpc_com_t {
    MPI_Comm mpi_com; // the MPI communicator
    int_type rank;    // the process rank in the communicator mpi_com
    int_type size;    // the size of mpi_com
};

inline MPI_Comm *com2mpi(MPC::comm_id_t *com) {
    return &(((mpc_com_t*)(com->internal_comm_id))->mpi_com);
}

inline int_type *com2rank(MPC::comm_id_t *com) {
    return &(((mpc_com_t*)(com->internal_comm_id))->rank);
}

inline int_type *com2size(MPC::comm_id_t *com) {
    return &(((mpc_com_t*)(com->internal_comm_id))->size);
}

/*
 *====================================================================================
 *====================================================================================
 */
// Gestion de l'erreur d'une fonction MPI
// IN : const char *nomfonc = nom de la fonction membre
// IN : int_type errortype  = type de l'erreur MPI
void MPC_MPI::ErrorMPI(const char *nomfonc,int_type errortype) {

    char message[MPI_MAX_ERROR_STRING];
    int_type resultlen;

    // capture du message d'erreur
    MPI_Error_string((int)errortype,message,&resultlen);

    // Affichage du message d'erreur
    fprintf(stderr,"Erreur MPI dans la fonction MPC_MPI::%s:%s\n",nomfonc,message);
    abort();

    // on quitte le programme
    fprintf(stderr,"Execution termin\351e !\n");
    MPI_Abort(communicateur_,1);
}

void *MPC_MPI::MPC_calloc(size_t nmemb, size_t size)
{
    
    void *tmp;
    tmp = calloc( nmemb, size);
    return tmp;
}

void *MPC_MPI::MPC_malloc(size_t size)
{
    
    void *tmp;
    tmp = malloc( size);
    return tmp;
}

void MPC_MPI::MPC_free(void *ptr)
{
    
    free(ptr);
}

void *MPC_MPI::MPC_realloc(void *ptr, size_t size)
{
    
    void *tmp;
    tmp = realloc( ptr, size);
    return tmp;

}


/*
 *====================================================================================
 *====================================================================================
 */
// Constructeur 
// IN : pargc = pointeur sur le nombre d'arguments du run, executable compris
// IN : pargv = pointeur sur la liste des arguments du run, executable compris
// IN : nbproc = le nombre de processus a activer
MPC_MPI::MPC_MPI(int *pargc,char ***pargv, u_int_type nbproc) :
    MPC(pargc,pargv,nbproc) {

    int_type nprocMPI, rang, flag;
    int_type code;

    // Affectation du communicateur par defaut
    communicateur_ = MPI_COMM_WORLD;

    // Activation de l'environnement MPI
    MPI_Initialized(&flag);
    if ( !flag )
	MPI_Init(pargc_,pargv_);

    // Activation de la gestion des erreurs
    MPI_Errhandler_set(communicateur_,MPI_ERRORS_RETURN);

    // verification du nombre de processus actifs
    code = MPI_Comm_size(communicateur_,&nprocMPI);
    if (code != MPI_SUCCESS) ErrorMPI("MPC_MPI():MPI_Comm_size()",code);

    // verification du nombre de processus actifs
    if (nprocMPI != (int)nproc_) {
	// avertir qu'il y a moins de processus actifs que prevus
      fprintf(stderr,"Attention ! Seulement %d processus actifs pour %d demand\351s\n",nprocMPI,nproc_ );
	// affectation du nouveau nombre de processus actifs
	nproc_ = nprocMPI;
    }

    // determination du rang
    code = MPI_Comm_rank(communicateur_,&rang);
    rang_ = (int)rang;
    if (code != MPI_SUCCESS) ErrorMPI("MPC_MPI()::MPI_Comm_rank()",code);

    // construction de COMM_WORLD
    COMM_WORLD.internal_comm_id = new mpc_com_t;
    mpc_com_t *tmp_com = (mpc_com_t*)COMM_WORLD.internal_comm_id;
    tmp_com->mpi_com = MPI_COMM_WORLD;
    tmp_com->rank = rang_;
    tmp_com->size = nproc_;

    // position courante initiale dans le buffer
    mpibufpos_ = 0;

    reqcntmax = 0;
    unpack_count = 0;

    blockcnt = 0;
    block = NULL;

    mpibufsize_ = NULL;
    mpibuf_ = NULL;
    pk_msg = NULL;
    mpireq = NULL;
    mpistatuses = NULL;
    unpack_id = NULL;

    sz_lenght1 = 0;
    lenght1 = NULL;

    // construction du type booléen pour MPI
    MPI_Type_contiguous( sizeof(bool), MPI_BYTE, &booltype_ );
    MPI_Type_commit( &booltype_ );

    // construction du type "nombre complexe" pour MPI
    float_type unreel = 0.;
    MPI_Type_contiguous( 2, GetMPIType(&unreel), &complextype_ );
    MPI_Type_commit( &complextype_ );
    

    _MYRANK_ = rang_;
    _MYPROC_ = rang_;

#ifdef __SCTK_MPC_MPI_H_
#warning "for mpc_mpi flavor"
    {
      static mpc_thread_mutex_t lock = MPC_THREAD_MUTEX_INITIALIZER;
      static volatile int done = 0;
      mpc_thread_mutex_lock(&lock);
      if(done == 0){
	taskinfo = new TaskInfo_MT();
	done = 1; 
      }
      mpc_thread_mutex_unlock(&lock);
      MPI_Barrier(MPI_COMM_WORLD);
      ((TaskInfo_MT*)taskinfo)->InitTaskInfo();
    }
#else
    taskinfo = new TaskInfo();
#endif

    nb_type_commit = 0;
    nb_type_free = 0;

    cnt_req_free = 0;
    stack_req_free = NULL;

    flag_req_allocated = NULL;

    /* Addresse absolue
       Attention, a manipuler avec precautions !
       Ici, on surcharge l'addresse absolue donnee par MPI par celle
       fournie par le script HERA.
       On surcharge l'addresse que si la variable d'environnement MMPC_BOTTOM
       est definie.
     */
    char *mpc_bottom_env;

    mpc_bottom_env = getenv("MPC_BOTTOM");

    if ( mpc_bottom_env ) {

	MPC_BOTTOM_ = (void*)atol(mpc_bottom_env);

    } else {

	MPC_BOTTOM_ = MPI_BOTTOM;
    }


} // fin constructeur

int_type MPC_MPI::NewRequest() {

    if ( cnt_req_free == 0 ) {

	int_type reqcntold = reqcntmax;
        reqcntmax += REQCNT_INCREM;
      
	svrealloc(mpibufsize_, int_type, reqcntmax);
	svrealloc(mpibuf_, void*, reqcntmax);
      
	svrealloc(pk_msg, pt2pt_mpipack_t, reqcntmax);
	svrealloc(mpireq, MPI_Request, reqcntmax);
	svrealloc(mpistatuses, MPI_Status, reqcntmax);
	svrealloc(stack_req_free, int_type, reqcntmax);
	svrealloc(flag_req_allocated, bool, reqcntmax);
	
	svrealloc(unpack_id, int_type, reqcntmax);

	for(int_type r = reqcntold ; r < reqcntmax ; r++) {
	  
  	    mpibufsize_[r] = 0;
	    mpibuf_[r] = NULL;
	  
	    pk_msg[r].count = 0; 
	    pk_msg[r].szmax = 0;
	    pk_msg[r].packarray = NULL;
	  
	    unpack_id[r] = -1;

	    stack_req_free[cnt_req_free++] = r;
	    flag_req_allocated[r] = false;
	}
    }
    

    cnt_req_free--;
    int_type req_id = stack_req_free[cnt_req_free];
    flag_req_allocated[req_id] = true;

    return req_id;
}

void MPC_MPI::FreeRequest( int_type req_id ) {

    if ( flag_req_allocated[req_id] ) {

	flag_req_allocated[req_id] = false;
	stack_req_free[cnt_req_free++] = req_id;

    }
}

void MPC_MPI::FreeRequest() {

    for(int_type r = 0 ; r < reqcntmax ; r++) {

        if (mpibufsize_[r]) {
	    svfree(mpibuf_[r]);
	    mpibuf_[r] = NULL;
	    mpibufsize_[r] = 0;
	}

	if (pk_msg[r].szmax) {
	    svfree(pk_msg[r].packarray);
	    pk_msg[r].packarray = NULL;
	    pk_msg[r].szmax = 0;
	}
    }

    if (reqcntmax) {

        svfree(mpibufsize_);	
	svfree(mpibuf_);
      
	svfree(pk_msg);
	svfree(mpireq);
	svfree(mpistatuses);
	
	svfree(unpack_id);

	svfree(stack_req_free);
	svfree(flag_req_allocated);

	mpibufsize_ = NULL;
	mpibuf_ = NULL;
	pk_msg = NULL;
	mpireq = NULL;
	mpistatuses = NULL;
	unpack_id = NULL;

	stack_req_free = NULL;
	flag_req_allocated = NULL;
    }
}

/*
 *====================================================================================
 *====================================================================================
 */
// Destructeur
MPC_MPI::~MPC_MPI() {

    FreeRequest();

    delete (mpc_com_t*)(COMM_WORLD.internal_comm_id);

    if ( blockcnt )
	free( block );

    if ( sz_lenght1 )
        free( lenght1 );

    // libération des types MPI
    MPI_Type_free( &booltype_ );
    MPI_Type_free( &complextype_ );


#ifdef __SCTK_MPC_MPI_H_
    {
      static mpc_thread_mutex_t lock = MPC_THREAD_MUTEX_INITIALIZER;
      static volatile int done = 0;
      MPI_Barrier(MPI_COMM_WORLD);
      mpc_thread_mutex_lock(&lock);
      if(done == 0){
	delete taskinfo;
	done = 1; 
      }
      mpc_thread_mutex_unlock(&lock);
    }
#else
    delete taskinfo;
#endif

    // on quitte l'environnment MPC_MPI
    Exit();
}

/*
 *====================================================================================
 *====================================================================================
 */
void MPC_MPI::StartTasks( void (*entry)(struct main_arg*), void *arg) {

    struct main_arg myargs;

    myargs.argc = *pargc_;
    myargs.argv = *pargv_;
    myargs.taskarg = arg;
    myargs.mpcom = this;

    entry( &myargs );
}

/*
 *====================================================================================
 *====================================================================================
 */
// Retourne le rang du processus courant
// C'est un entier compris entre 0 et MPC::Nproc()-1
// retourne une valeur negative si erreur
int_type MPC_MPI::Rank(comm_id_t *com) { 

    if ( !com )
	return rang_; 

    int_type rank = *com2rank(com);

    return rank;
}

/*
 *====================================================================================
 *====================================================================================
 */
u_int_type MPC_MPI::Nproc(comm_id_t *com) const {

    if ( !com )
	return nproc_;

    u_int_type size = *com2size(com);

    return size;
}

/*
 *====================================================================================
 *====================================================================================
 */
void MPC_MPI::ComCreate(comm_id_t *comold, int* liste, int n, comm_id_t *comnew) {

    if ( !comold ) {

	printf("Communicateur initial NULL\n");
	Abort(1);
    }

    int_type code;

    MPI_Comm mpi_comm_old = *com2mpi(comold);
    MPI_Group mpi_group_old, mpi_group_new;

    code = MPI_Comm_group(mpi_comm_old, &mpi_group_old);
    if (code != MPI_SUCCESS) 
	ErrorMPI("ComCreate():MPI_Comm_group()",code);

    code = MPI_Group_incl(mpi_group_old, n, liste, &mpi_group_new);
    if (code != MPI_SUCCESS) 
	ErrorMPI("ComCreate():MPI_Group_incl()",code);

    /* the new communicator creation
     */
    comnew->internal_comm_id = new mpc_com_t;
    code = MPI_Comm_create(mpi_comm_old, mpi_group_new, 
			   com2mpi(comnew));
    if (code != MPI_SUCCESS) 
	ErrorMPI("ComCreate():MPI_Comm_create()",code);
    //

    code = MPI_Group_free(&mpi_group_old);
    if (code != MPI_SUCCESS) 
	ErrorMPI("ComCreate():MPI_Comm_free()",code);   
 
    code = MPI_Group_free(&mpi_group_new);
    if (code != MPI_SUCCESS) 
	ErrorMPI("ComCreate():MPI_Comm_free()",code);

    /* nous completons les champs rank et size
     */
    int_type *prank = com2rank(comnew);
    int_type *psize = com2size(comnew);
    
    code = MPI_Comm_rank(*com2mpi(comnew), prank);
    if (code != MPI_SUCCESS) 
	ErrorMPI("ComCreate():MPI_Comm_rank()",code);

    code = MPI_Comm_size(*com2mpi(comnew), psize);
    if (code != MPI_SUCCESS) 
	ErrorMPI("ComCreate():MPI_Comm_size()",code);
}

/*
 *====================================================================================
 *====================================================================================
 */
void MPC_MPI::ComSplit(comm_id_t *comold, int color, int key, comm_id_t *comnew) {

    if ( !comold || !comold->internal_comm_id ) {

	printf("Communicateur initial NULL\n");
	Abort(1);
    }

    if ( !comnew ) {

	printf("Communicateur final NULL\n");
	Abort(1);
    }

    int_type code;

    comnew->internal_comm_id = new mpc_com_t;
    MPI_Comm mpi_com_old = *com2mpi(comold);

    /* we're creating sub-commuincator by subdividing mpi_com_old into disjoint
       subsets */
    code = MPI_Comm_split( mpi_com_old, color, key, 
			   com2mpi(comnew) );
    if (code != MPI_SUCCESS) 
	ErrorMPI("ComSplit():MPI_Comm_split()",code);    
    
    /* nous completons les champs rank et size
     */
    int_type *prank = com2rank(comnew);
    int_type *psize = com2size(comnew);
    
    code = MPI_Comm_rank(*com2mpi(comnew), prank);
    if (code != MPI_SUCCESS) 
	ErrorMPI("ComCreate():MPI_Comm_rank()",code);

    code = MPI_Comm_size(*com2mpi(comnew), psize);
    if (code != MPI_SUCCESS) 
	ErrorMPI("ComCreate():MPI_Comm_size()",code);
}

/*
 *====================================================================================
 *====================================================================================
 */
void MPC_MPI::ComDelete(comm_id_t *com) {

    if ( !com ) {

	printf("Communicateur initial NULL\n");
	Abort(1);
    }

    if ( com->internal_comm_id == NULL ||
	 *com2mpi(com) == *com2mpi(&COMM_WORLD) ) {

	printf("Impossible de detruire le communicateur NULL ou le communicateur COMM_WORLD\n");
	Abort(1);
    }

    int_type code;

    code = MPI_Comm_free( com2mpi(com) );
    if (code != MPI_SUCCESS) 
	ErrorMPI("ComDelete():MPI_Comm_free()",code);

    delete (mpc_com_t*)com->internal_comm_id;
}

/*
 *====================================================================================
 *====================================================================================
 */
// Effectue une barriere sur tous les processus
// retourne une valeur <0 si erreur
int_type MPC_MPI::Barrier(comm_id_t *com) { 

    int_type code;
    MPI_Comm mpi_com = ( com ? *com2mpi(com) : MPI_COMM_WORLD );

    code = MPI_Barrier(mpi_com);
    if (code != MPI_SUCCESS) 
	ErrorMPI("Barrier():MPI_Barrier()",code);

    return code;
}

/*
 *====================================================================================
 *====================================================================================
 */
// Quitter l'environnement MPC
// Retourne une valeur negative si erreur
int_type MPC_MPI::Exit() { 
    int_type code;
    code = MPI_Finalize();
    if (code != MPI_SUCCESS) ErrorMPI("Exit():MPI_Finalize()",code);    
    return code; 
}


/*
 *====================================================================================
 *====================================================================================
 */
// Retourne le type MPI correspondant au type u_int_type
MPI_Datatype MPC_MPI::GetMPIType(c_u_int_type_p) {

    MPI_Datatype montype;
    int_type taille;
    const int_type typeushort=sizeof(unsigned short),
	typeuint=sizeof(unsigned int),
	typeulong=sizeof(unsigned long);

    // je determine le type exact: short, int ou long
    // taille en octet du type int
    taille = sizeof(u_int_type);

    // selection du type
    if (taille == typeushort) {
	// entier court code sur 2 octets
	montype = MPI_UNSIGNED_SHORT;
    } else if (taille == typeuint) {
	// entier standard code sur 4 octets
	montype = MPI_UNSIGNED;
    } else if (taille == typeulong) {
	// entier long code sur 8 octets
	montype = MPI_UNSIGNED_LONG;
    } else {
	// taille inconnue
      fprintf(stderr,"Type entier inconnu. Arr\352t !\n");
	abort();
    }

    return montype;
}


/*
 *====================================================================================
 *====================================================================================
 */
// Retourne le type MPI correspondant au type int
MPI_Datatype MPC_MPI::GetMPIType(c_int_type_p) {

    MPI_Datatype montype;
    int_type taille;
    const int_type typeshort=sizeof(short),
	typeint=sizeof(int),
	typelong=sizeof(long);

    // je determine le type exact: short, int ou long
    // taille en octet du type int
    taille = sizeof(int);

    // selection du type
    if (taille == typeshort) {
	// entier court code sur 2 octets
	montype = MPI_SHORT;
    } else if (taille == typeint) {
	// entier standard code sur 4 octets
	montype = MPI_INT;
    } else if (taille == typelong) {
	// entier long code sur 8 octets
	montype = MPI_LONG;
    } else {
	// taille inconnue
      fprintf(stderr,"Type entier inconnu. Arr\352t !\n");
	abort();
    }

    return montype;
}

/*
 *====================================================================================
 *====================================================================================
 */
// Retourne le type MPI correspondant au type float_type
MPI_Datatype MPC_MPI::GetMPIType(c_float_type_p) {

    MPI_Datatype montype;
    int_type taille;
    const int_type typefloat=sizeof(float),
	typedouble=sizeof(double),
	typelgdbl=sizeof(long double);

    // je determine le type exact: float, double ou long double
    // taille en octet du type int
    taille = sizeof(float_type);

    // selection du type
    if (taille == typefloat) {
	// reel court code sur 4 octets
	montype = MPI_FLOAT;
    }
    else if (taille == typedouble) {
	// reel standard code sur 8 octets
	montype = MPI_DOUBLE;
    }
    else if (taille == typelgdbl) {
	// reel long code sur 16 octets
	montype = MPI_LONG_DOUBLE;
    }
    else {
	// taille inconnue
      fprintf(stderr,"Type entier inconnu. Arr\352t !\n");
	abort();
    }

    return montype;
}

/*
 *====================================================================================
 *====================================================================================
 */
MPI_Datatype MPC_MPI::GetMPIType(c_short_p) {
    return MPI_SHORT;
}

/*
 *====================================================================================
 *====================================================================================
 */
MPI_Datatype MPC_MPI::GetMPIType(c_bool_p) {
    return booltype_;
}

/*
 *====================================================================================
 *====================================================================================
 */
MPI_Datatype MPC_MPI::GetMPIType(void*) {
    return MPI_BYTE;
}

/*
 *====================================================================================
 *====================================================================================
 */
template<typename T>
int_type MPC_MPI::mpi_ISendScal(u_int_type dest,u_int_type msgtag,
				T *adr,u_int_type longueur,
				message_status_t *req, 
				comm_id_t *com) {
    MPI_Datatype montype;
    int_type code, mgtg, src, nproc;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPI_Comm mpi_com = *com2mpi(tmp_com);

    int_type req_id = NewRequest();

    // determination du type MPI
    montype = GetMPIType(adr);

    // Construction du message tag
    src = *com2rank(tmp_com);
    nproc = *com2size(tmp_com);
    mgtg = src + nproc * dest;

//     int_type new_size;
//     MPI_Type_size(montype, &new_size);
//     printf("cpu%d -> cpu %d : %d octets\n", src, dest, longueur*new_size);
    // Envoi non bloquant
    code = MPI_Isend( (void*)adr, (int)longueur, montype,
		      (int)dest, (int)mgtg, mpi_com, &(mpireq[req_id]));
    if (code != MPI_SUCCESS) 
	ErrorMPI("mpi_ISendScal():MPI_Isend()",code);

    if (req)
	req->status = req_id;

    return code;
}

/*
 *====================================================================================
 *====================================================================================
 */
template<typename T>
int_type MPC_MPI::mpi_IRecvScal(u_int_type prov,u_int_type msgtag,
				T *adr,u_int_type longueur,
				message_status_t *req, 
				comm_id_t *com) {

    // MPI_Status status;
    MPI_Datatype montype;
    int_type code, mgtg, dest, nproc;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPI_Comm mpi_com = *com2mpi(tmp_com);

    int_type req_id = NewRequest();

    // determination du type MPI
    montype = GetMPIType(adr);

    // Construction du message tag
    dest = *com2rank(tmp_com);
    nproc = *com2size(tmp_com);
    mgtg = prov + nproc * dest;

//     int_type new_size;
//     MPI_Type_size(montype, &new_size);
//     printf("cpu%d <- cpu %d : %d octets\n", dest, prov, longueur*new_size);
    // reception non bloquante
    code = MPI_Irecv( (void*)adr, (int)longueur, montype,
		      (int)prov, (int)mgtg, mpi_com, &(mpireq[req_id]));
    if (code != MPI_SUCCESS)
	ErrorMPI("mpi_IRecvScal():MPI_Irecv()",code);

    if (req)
	req->status = req_id;

    return code; 
}

int_type MPC_MPI::Wait(struct message_status_t *req) {

    if ( !req ) {
	printf("MPC_MPI::Wait : requete NULL impossible !\n");
	Abort(1);
    }

    int_type req_id = req->status;

    if ( !flag_req_allocated[req_id] ) {
	printf("MPC_MPI::Wait : requete %d non allouee !\n", req_id);
	Abort(1);
    }

    int_type code = MPI_Wait(&(mpireq[req_id]), &(mpistatuses[req_id])) ;
    if (code != MPI_SUCCESS)
	ErrorMPI("Wait():MPI_Wait()",code);

    FreeRequest( req_id );

    return code ;
}


/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPI::WaitAll(int_type nb_in, message_status_t *req_in) {
#ifdef USE_VT
    VT_begin( VT_WAITALL );
#endif
    int_type code;
 
    code = MPI_SUCCESS;
    
    int_type nb_req = ( nb_in ? nb_in : reqcntmax );
    if ( nb_req ) {

// 	ReallocBlock( (nb_req*sizeof(MPI_Request)/sizeof(int)) + 1 );
// 	MPI_Request *tab_req = (MPI_Request *)block;
	MPI_Request *tab_req = new MPI_Request[nb_req];

	if (req_in) {

	    for( int_type r = 0 ; r < nb_in ; r++ )
		tab_req[r] = mpireq[ req_in[r].status ];

	} else {

	    int_type r = 0;
	    for( int_type rtot = 0 ; rtot < reqcntmax ; rtot++ )
		if ( flag_req_allocated[rtot] )
		    tab_req[r++] = mpireq[ rtot ];

	    nb_req = r;
	}

	code = MPI_Waitall( nb_req, tab_req, mpistatuses );
	if (code != MPI_SUCCESS)
	    ErrorMPI("WaitAll():MPI_Waitall()",code);

	if (req_in) {

	    for( int_type r = 0 ; r < nb_req ; r++ ) {
		mpireq[ req_in[r].status ] = tab_req[r];
		FreeRequest( req_in[r].status );
	    }

	} else {

	    int_type r = 0;
	    for( int_type rtot = 0 ; rtot < reqcntmax ; rtot++ )
		if ( flag_req_allocated[rtot] ) {
		    mpireq[ rtot ] = tab_req[r];
		    FreeRequest( rtot );
		    r++;
		}
	}

	delete[] tab_req;
    }

    if (nb_type_commit != nb_type_free) {
	printf("Nombres de commit different du nombre de free : %d <> %d\n",
	       nb_type_commit, nb_type_free);

	MPI_Abort(communicateur_, 1);
    }
    nb_type_commit = 0;
    nb_type_free = 0;    

#ifdef USE_VT
    VT_end( VT_WAITALL );
#endif
    return code; 
}


/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPI::WaitSome(int_type nb_in, message_status_t *req_in,
			   int_type &nb_out, int_type *indices_out) {
    int_type code = 0;

#if 0
    if ( nb_in ) {

	//	ReallocBlock( (nb_in*sizeof(MPI_Request)/sizeof(int)) + 1 );
	//	MPI_Request *tab_req = (MPI_Request *)block;
	MPI_Request *tab_req = new MPI_Request[nb_in];
	
	for( int_type r = 0 ; r < nb_in ; r++ )
	    tab_req[r] = mpireq[ req_in[r].status ];
	
	code = MPI_Waitsome( nb_in, tab_req, &nb_out, indices_out, mpistatuses );
	if (code != MPI_SUCCESS)
	    ErrorMPI("WaitAll():MPI_Waitall()",code);

	for( int_type r = 0 ; r < nb_out ; r++ ) {
	    mpireq[ req_in[ indices_out[r] ].status ] = tab_req[ indices_out[r] ];
	    FreeRequest( req_in[ indices_out[r] ].status );
	}

	delete[] tab_req;
    }
#else
    code = WaitAll(nb_in, req_in);

    nb_out = nb_in;

    for( int_type r = 0 ; r < nb_in ; r++ )
	indices_out[r] = r;
#endif

    return code;
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPI::OpenPack( int_type nb, 
			    u_int_type *begindex, u_int_type *endindex,
			    message_status_t *req ) {

    int_type req_id = NewRequest();
    req->status = req_id;
    
    pk_msg[req_id].nbindex = nb;
    pk_msg[req_id].begindex = begindex;
    pk_msg[req_id].blockindex = NULL;
    pk_msg[req_id].count = 0;


    if (nb)
	pk_msg[req_id].blockindex = (int_type*)malloc(nb*sizeof(int));

    for( int_type i = 0 ; i < nb ; i++ )
	pk_msg[req_id].blockindex[i] = endindex[i] - begindex[i] + 1;

    return 0;
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPI::OpenPack( message_status_t *req ) {

    int_type req_id = NewRequest();
    req->status = req_id;
    
    pk_msg[req_id].nbindex = 0;
    pk_msg[req_id].begindex = NULL;
    pk_msg[req_id].blockindex = NULL;
    pk_msg[req_id].count = 0;

    
    return 0;
}

/*
 *====================================================================================
 *====================================================================================
 */
void MPC_MPI::ReallocBlock(c_int_type nb_needed) {

    if (nb_needed > blockcnt) {
        int_type howmany = nb_needed / MEM_BLK;
	blockcnt = (howmany+1) * MEM_BLK;
	block = (int_type*)realloc( block, blockcnt*sizeof(int_type) );
    }
}

/*
 *====================================================================================
 *====================================================================================
 */
void MPC_MPI::ReallocLenght1(c_int_type nb_needed) {

    if (nb_needed > sz_lenght1) {
        int_type howmany = nb_needed / MEM_BLK;
	c_int_type sz_old = sz_lenght1;
	sz_lenght1 = (howmany+1) * MEM_BLK;
	lenght1 = (int_type*)realloc( lenght1, sz_lenght1*sizeof(int_type) );

	for( int_type i = sz_old ; i < sz_lenght1 ; i++ )
	  lenght1[i] = 1;
    }
}

/*
 *====================================================================================
 *====================================================================================
 */
template<typename T>
void MPC_MPI::mpi_AddPack( T *adr, message_status_t *req ) {

    int_type req_id = req->status;

    if ( pk_msg[req_id].szmax == pk_msg[req_id].count ) {
	pk_msg[req_id].szmax +=  PACKARRAY_BLK_SIZE;
	pk_msg[req_id].packarray = 
	    (elt_mpipack_t*)realloc( pk_msg[req_id].packarray, 
				  pk_msg[req_id].szmax*sizeof(elt_mpipack_t) );
    }

    elt_mpipack_t *pelt = &(pk_msg[req_id].packarray[pk_msg[req_id].count]);


    int_type code;
    MPI_Datatype unit_type = GetMPIType(adr);  // the MPI type of one element

    code = MPI_Type_indexed( pk_msg[req_id].nbindex, pk_msg[req_id].blockindex,
			     (int*)pk_msg[req_id].begindex, unit_type,
			     &(pelt->elt_type) );
    if (code != MPI_SUCCESS)
	ErrorMPI("mpi_AddPack:MPI_Type_indexed",code);

    code = MPI_Type_commit( &(pelt->elt_type) );
    nb_type_commit++;
    if (code != MPI_SUCCESS)
	ErrorMPI("mpi_AddPack:MPI_Type_commit",code);

    pelt->adr = (char*)adr;
    pk_msg[req_id].count++;
}

/*
 *====================================================================================
 *====================================================================================
 */
template<typename T>
void MPC_MPI::mpi_AddPack( T *adr, 
			   int_type nb_displs, int_type_p displs, 
			   message_status_t *req ) {

    int_type req_id = req->status;

    int_type code;

    if ( pk_msg[req_id].szmax == pk_msg[req_id].count ) {
	pk_msg[req_id].szmax +=  PACKARRAY_BLK_SIZE;
	pk_msg[req_id].packarray = 
	    (elt_mpipack_t*)realloc( pk_msg[req_id].packarray, 
				  pk_msg[req_id].szmax*sizeof(elt_mpipack_t) );
    }

    elt_mpipack_t *pelt = &(pk_msg[req_id].packarray[pk_msg[req_id].count]);

    /*
      We'll make an indexed type from the T's MPI data type
     */
    MPI_Datatype oldtype = GetMPIType(adr); // the MPI type of one element

    ReallocLenght1(nb_displs);

    code = MPI_Type_indexed( nb_displs, lenght1, displs, oldtype,
		      &pelt->elt_type );
    if (code != MPI_SUCCESS)
      ErrorMPI("mpi_AddPack:MPI_Type_indexed",code);

    code = MPI_Type_commit( &pelt->elt_type );
    nb_type_commit++;
    if (code != MPI_SUCCESS)
      ErrorMPI("mpi_AddPack:MPI_Type_commit",code);
    /* end of the MPI type creation */


    pelt->adr = (char*)adr;
    pk_msg[req_id].count++;
}

/*
 *====================================================================================
 *====================================================================================
 */
template<typename T>
void MPC_MPI::mpi_AddPack( T *adr, 
			   int_type nb, u_int_type *bind, u_int_type *eind,
			   message_status_t *req ) {

    int_type req_id = req->status;

    int_type code;

    if ( pk_msg[req_id].szmax == pk_msg[req_id].count ) {
	pk_msg[req_id].szmax +=  PACKARRAY_BLK_SIZE;
	pk_msg[req_id].packarray = 
	    (elt_mpipack_t*)realloc( pk_msg[req_id].packarray, 
				  pk_msg[req_id].szmax*sizeof(elt_mpipack_t) );
    }

    elt_mpipack_t *pelt = &(pk_msg[req_id].packarray[pk_msg[req_id].count]);

    /*
      We'll make an indexed type from the T's MPI data type
     */
    MPI_Datatype oldtype = GetMPIType(adr); // the MPI type of one element

    ReallocBlock(nb);

    for( int_type i = 0 ; i < nb ; i++ )
	block[i] = eind[i] - bind[i] + 1;

    code = MPI_Type_indexed( nb, block, (int_type*)bind, oldtype,
			     &pelt->elt_type );
    if (code != MPI_SUCCESS)
      ErrorMPI("mpi_AddPack:MPI_Type_indexed",code);

    code = MPI_Type_commit( &pelt->elt_type );
    nb_type_commit++;
    if (code != MPI_SUCCESS)
      ErrorMPI("mpi_AddPack:MPI_Type_commit",code);
    /* end of the MPI type creation */


    pelt->adr = (char*)adr;
    pk_msg[req_id].count++;
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPI::ISendPack( int_type destination, u_int_type msgtag, 
			     message_status_t *req, comm_id_t *com ) {
#ifdef USE_VT
    VT_begin( VT_ISENDPACK );
#endif
    int_type code;
    int_type req_id = req->status;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPI_Comm mpi_com = *com2mpi(tmp_com);

//    src = *com2rank(tmp_com);
    //   nproc = *com2size(tmp_com);

    //  int_type bigelt_size, new_size, i;
    int_type i;
    
    elt_mpipack_t *vartab = pk_msg[req_id].packarray; // the arrays to pack
    int_type nbvar = pk_msg[req_id].count;        // the number of arrays to pack

    if ( nbvar == 1 ) {

// 	MPI_Type_size(vartab[0].elt_type, &new_size);
// 	printf("cpu%d -> cpu %d : %d octets\n", src, destination, new_size);
	code = MPI_Isend( vartab[0].adr, 1, vartab[0].elt_type, destination, 
			  msgtag, mpi_com, &(mpireq[req_id]) );
	if (code != MPI_SUCCESS)
	    ErrorMPI("ISendPack:MPI_Isend",code);

	/* we're freeing the unique derived data type
	 */
	code = MPI_Type_free(&(vartab[0].elt_type));
	nb_type_free++;
	if (code != MPI_SUCCESS)
	    ErrorMPI("ISendPack:MPI_Type_free",code);	    

    } else {

	MPI_Datatype msg_type;
	MPI_Datatype *arr_of_types;
	MPI_Aint *arr_of_displs;

	int_type fact = (sizeof(int) + sizeof(MPI_Datatype) + sizeof(MPI_Aint)) 
	    / sizeof(int) + 1;

	//
	ReallocBlock(fact*nbvar);

	arr_of_types = (MPI_Datatype *)(block + nbvar);
	arr_of_displs = (MPI_Aint *)(arr_of_types + nbvar);

	/* struct type construction -> msg_type
	 */
	for( i = 0 ; i < nbvar ; i++ ) {

	    block[i] = 1;
	    MPI_Address(vartab[i].adr, arr_of_displs + i );
	    arr_of_types[i] = vartab[i].elt_type;
	}

	code = MPI_Type_struct(nbvar, block, arr_of_displs, arr_of_types, &msg_type);
	if (code != MPI_SUCCESS)
	    ErrorMPI("ISendPack:MPI_Type_struct",code);
	
	code = MPI_Type_commit(&msg_type);
	nb_type_commit++;
	if (code != MPI_SUCCESS)
	    ErrorMPI("ISendPack:MPI_Type_commit",code);

	/* we're sending the message without packing
	 */
// 	MPI_Type_size(msg_type, &new_size);
// 	printf("cpu%d -> cpu %d : %d octets\n", src, destination, new_size);
	code = MPI_Isend( MPC_BOTTOM_, 1, msg_type, destination, 
			  msgtag, mpi_com, &(mpireq[req_id]) );
	if (code != MPI_SUCCESS)
	    ErrorMPI("ISendPack:MPI_Isend",code);

	/* we're freeing the derived data type
	 */
	code = MPI_Type_free(&msg_type);
	nb_type_free++;
	if (code != MPI_SUCCESS)
	    ErrorMPI("ISendPack:MPI_Type_free",code);

	for( i = 0 ; i < nbvar ; i++ ) {

	    code = MPI_Type_free(&(vartab[i].elt_type));
	    nb_type_free++;
	    if (code != MPI_SUCCESS)
		ErrorMPI("ISendPack:MPI_Type_free",code);	    
	}
    }

    if (pk_msg[req_id].blockindex)
	free( pk_msg[req_id].blockindex );


#ifdef USE_VT
    VT_end( VT_ISENDPACK );
#endif
    return 0;
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPI::IRecvPack( int_type source, u_int_type msgtag, 
			     message_status_t *req, comm_id_t *com ) {
#ifdef USE_VT
    VT_begin( VT_IRECVPACK );
#endif
    int_type code;
    int_type req_id = req->status;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPI_Comm mpi_com = *com2mpi(tmp_com);

  //   dest = *com2rank(tmp_com);
//     nproc = *com2size(tmp_com);

    int_type   i;

    elt_mpipack_t *vartab = pk_msg[req_id].packarray; // the arrays to unpack
    int_type nbvar = pk_msg[req_id].count;        // the number of arrays to unpack

    if ( nbvar == 1) {

	/* we're receiving the message without unpacking
	 */
// 	MPI_Type_size(vartab[0].elt_type, &new_size);
// 	printf("cpu%d <- cpu %d : %d octets\n", dest, source, new_size);
	code = MPI_Irecv( vartab[0].adr, 1, vartab[0].elt_type, source, 
			  msgtag, mpi_com, &(mpireq[req_id]) );
	if (code != MPI_SUCCESS)
	    ErrorMPI("IRecvPack:MPI_Irecv",code);

	/* we're freeing the derived data type and all the others
	 */
	code = MPI_Type_free(&(vartab[0].elt_type));
	nb_type_free++;
	if (code != MPI_SUCCESS)
	    ErrorMPI("IRecvPack:MPI_Type_free",code);	    

    } else {

	MPI_Datatype msg_type;
	MPI_Datatype *arr_of_types;
	MPI_Aint *arr_of_displs;

	int_type fact = (sizeof(int) + sizeof(MPI_Datatype) + sizeof(MPI_Aint)) 
	    / sizeof(int) + 1;

	//
	ReallocBlock(fact*nbvar);

	arr_of_types = (MPI_Datatype *)(block + nbvar);
	arr_of_displs = (MPI_Aint *)(arr_of_types + nbvar);

	/* struct type construction -> msg_type
	 */
	for( i = 0 ; i < nbvar ; i++ ) {

	    block[i] = 1;
	    MPI_Address(vartab[i].adr, arr_of_displs + i );
	    arr_of_types[i] = vartab[i].elt_type;
	}

	code = MPI_Type_struct(nbvar, block, arr_of_displs, arr_of_types, &msg_type);
	if (code != MPI_SUCCESS)
	    ErrorMPI("IRecvPack:MPI_Type_struct",code);
	
	code = MPI_Type_commit(&msg_type);
	nb_type_commit++;
	if (code != MPI_SUCCESS)
	    ErrorMPI("IRecvPack:MPI_Type_commit",code);

	/* we're receiving the message without unpacking
	 */
// 	MPI_Type_size(msg_type, &new_size);
// 	printf("cpu%d <- cpu %d : %d octets\n", dest, source, new_size);
	code = MPI_Irecv( MPC_BOTTOM_, 1, msg_type, source, 
			  msgtag, mpi_com, &(mpireq[req_id]) );
	if (code != MPI_SUCCESS)
	    ErrorMPI("IRecvPack:MPI_Irecv",code);

	/* we're freeing the derived data type and all the others
	 */
	code = MPI_Type_free(&msg_type);
	nb_type_free++;
	if (code != MPI_SUCCESS)
	    ErrorMPI("IRecvPack:MPI_Type_free",code);
	
	for( i = 0 ; i < nbvar ; i++ ) {

	    code = MPI_Type_free(&(vartab[i].elt_type));
	    nb_type_free++;
	    if (code != MPI_SUCCESS)
		ErrorMPI("IRecvPack:MPI_Type_free",code);	    
	}
    }

    if (pk_msg[req_id].blockindex)
	free( pk_msg[req_id].blockindex );


#ifdef USE_VT
    VT_end( VT_IRECVPACK );
#endif
    return 0;
}

/*
 *====================================================================================
 *====================================================================================
 */
// Retourne l'operation de reduction MPI correspondant a l'operation MPC
MPI_Op MPC_MPI::GetMPIOperation(enum MPC_operation op) {
    MPI_Op mpiop;

    switch(op) {

    case OMIN : mpiop = MPI_MIN; break;

    case OMAX : mpiop = MPI_MAX; break;

    case OSUM : mpiop = MPI_SUM; break;

    default:
	// operation inconnue
      fprintf(stderr,"Opération de réduction inconnue. Arrêt !\n");
	abort();
    }

    return mpiop;
}


/*
 *====================================================================================
 *====================================================================================
 */
template<typename T>
int_type MPC_MPI::mpi_AllReduce( const T *const sendbuf, T *const recvbuf, u_int_type longueur,
				 enum MPC_operation op, comm_id_t *com ) {
#ifdef USE_VT
    VT_begin( VT_ALLREDUCE );
#endif
    MPI_Datatype montype;
    MPI_Op monop;
    int_type code;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPI_Comm mpi_com = *com2mpi(tmp_com);

    // type MPI correspondant au type T
    montype = GetMPIType(sendbuf);

    // recuperation de l'operation MPI
    monop = GetMPIOperation(op);

    // on effectue la reduction-diffusion
    code = MPI_Allreduce( (void*)sendbuf, (void*)recvbuf, (int)longueur,
			  montype, monop, mpi_com );

    if (code != MPI_SUCCESS)
	ErrorMPI("mpi_AllReduce():MPI_Allreduce()",code);
#ifdef USE_VT
    VT_end( VT_ALLREDUCE );
#endif

    return code;
}


/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPI::BroadCast( void *buf, int_type count, int_type root,
			     comm_id_t *com ) {
    int_type code;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPI_Comm mpi_com = *com2mpi(tmp_com);

    code = MPI_Bcast( buf, count, MPI_BYTE, root, mpi_com);
    if (code != MPI_SUCCESS)
	ErrorMPI("BroadCast():MPI_Bcast()",code);
    
    return code;
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPI::GatherV( void *sendbuf, int_type sendcount,
			   void *recvbuf, int_type *recvcounts, 
			   int_type* displs ,
			   int_type root, comm_id_t *com ) {
    int_type code;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPI_Comm mpi_com = *com2mpi(tmp_com);

    code = MPI_Gatherv( sendbuf, sendcount, MPI_BYTE,
		        recvbuf, recvcounts, displs, MPI_BYTE,
			root, mpi_com );
    if (code != MPI_SUCCESS)
	ErrorMPI("GatherV():MPI_Gatherv()",code);

    return code;
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPI::ScatterV( void *sendbuf, int_type *sendcounts, 
			    int_type* displs ,
			    void *recvbuf, int_type recvcount,
			    int_type root, comm_id_t *com ) {
    int_type code;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPI_Comm mpi_com = *com2mpi(tmp_com);

    code = MPI_Scatterv( sendbuf, sendcounts, displs, MPI_BYTE,
			 recvbuf, recvcount, MPI_BYTE,
			 root, mpi_com );
    if (code != MPI_SUCCESS)
	ErrorMPI("ScatterV():MPI_Scatterv()",code);

    return code;
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPI::AllGatherV( void *sendbuf, int_type sendcount,
			      void *recvbuf, int_type *recvcounts, 
			      int_type* displs, 
			      comm_id_t *com ) {
    int_type code;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPI_Comm mpi_com = *com2mpi(tmp_com);

    code = MPI_Allgatherv( sendbuf, sendcount, MPI_BYTE,
			   recvbuf, recvcounts, displs, MPI_BYTE,
			   mpi_com );
    if (code != MPI_SUCCESS)
	ErrorMPI("AllGatherV():MPI_Allgatherv()",code);

    return code;
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPI::AllToAllV( void *sendbuf, int_type *sendcounts, 
			     int_type* sdispls ,
			     void *recvbuf, int_type *recvcounts, 
			     int_type* rdispls,
			     comm_id_t *com ) {
    int_type code;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPI_Comm mpi_com = *com2mpi(tmp_com);

    code = MPI_Alltoallv( sendbuf, sendcounts, sdispls, MPI_BYTE,
			  recvbuf, recvcounts, rdispls, MPI_BYTE,
			  mpi_com );
    if (code != MPI_SUCCESS)
	ErrorMPI("AllToAllV():MPI_Alltoallv()",code);

    return code;
}

// ==================

int_type MPC_MPI::ISendScal(u_int_type dest,u_int_type msgtag,
			   u_int_type * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpi_ISendScal( dest, msgtag, adr, longueur, req, com ); 
}

int_type MPC_MPI::ISendScal(u_int_type dest,u_int_type msgtag,
			   int_type * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpi_ISendScal( dest, msgtag, adr, longueur, req, com ); 
}

int_type MPC_MPI::ISendScal(u_int_type dest,u_int_type msgtag,
			   float_type * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpi_ISendScal( dest, msgtag, adr, longueur, req, com ); 
}

int_type MPC_MPI::ISendScal(u_int_type dest,u_int_type msgtag,
			   short * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpi_ISendScal( dest, msgtag, adr, longueur, req, com ); 
}
int_type MPC_MPI::ISendScal(u_int_type dest,u_int_type msgtag,
			   void * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpi_ISendScal( dest, msgtag, adr, longueur, req, com ); 
}

// ==================

int_type MPC_MPI::IRecvScal(u_int_type prov,u_int_type msgtag,
			   u_int_type * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpi_IRecvScal( prov, msgtag, adr, longueur, req, com );
}

int_type MPC_MPI::IRecvScal(u_int_type prov,u_int_type msgtag,
			   int_type * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpi_IRecvScal( prov, msgtag, adr, longueur, req, com );
}

int_type MPC_MPI::IRecvScal(u_int_type prov,u_int_type msgtag,
			   float_type * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpi_IRecvScal( prov, msgtag, adr, longueur, req, com );
}

int_type MPC_MPI::IRecvScal(u_int_type prov,u_int_type msgtag,
			   short * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpi_IRecvScal( prov, msgtag, adr, longueur, req, com );
}

int_type MPC_MPI::IRecvScal(u_int_type prov,u_int_type msgtag,
			   void * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpi_IRecvScal( prov, msgtag, adr, longueur, req, com );
}

// ==================

int_type MPC_MPI::AddPack(short* adr, message_status_t *req) { 
    mpi_AddPack( adr, req ); 
    return 0; 
}

int_type MPC_MPI::AddPack(u_int_type *adr, message_status_t *req) { 
    mpi_AddPack( adr, req ); 
    return 0; 
}

int_type MPC_MPI::AddPack(int_type *adr, message_status_t *req) { 
    mpi_AddPack( adr, req ); 
    return 0; 
}

int_type MPC_MPI::AddPack(float_type *adr, message_status_t *req) { 
    mpi_AddPack( adr, req ); 
    return 0; 
}

int_type MPC_MPI::AddPack(bool *adr, message_status_t *req) { 
    mpi_AddPack( adr, req ); 
    return 0; 
}

int_type MPC_MPI::AddPack(long *adr, message_status_t *req) { 
    mpi_AddPack( adr, req ); 
    return 0; 
}

// ==================

int_type MPC_MPI::AddPack(short* adr, int_type nb_displs, int_type_p displs, 
			  message_status_t *req) { 
    mpi_AddPack( adr, nb_displs, displs, req ); 
    return 0; 
}

int_type MPC_MPI::AddPack(u_int_type *adr, int_type nb_displs, int_type_p displs, 
			  message_status_t *req) { 
    mpi_AddPack( adr, nb_displs, displs, req ); 
    return 0; 
}

int_type MPC_MPI::AddPack(int_type *adr, int_type nb_displs, int_type_p displs, 
			  message_status_t *req) { 
    mpi_AddPack( adr, nb_displs, displs, req ); 
    return 0; 
}

int_type MPC_MPI::AddPack(float_type *adr, int_type nb_displs, int_type_p displs, 
			  message_status_t *req) { 
    mpi_AddPack( adr, nb_displs, displs, req ); 
    return 0; 
}

int_type MPC_MPI::AddPack(bool *adr, int_type nb_displs, int_type_p displs, 
			  message_status_t *req) { 
    mpi_AddPack( adr, nb_displs, displs, req ); 
    return 0; 
}
int_type MPC_MPI::AddPack(long *adr, int_type nb_displs, int_type_p displs, 
			  message_status_t *req) { 
    mpi_AddPack( adr, nb_displs, displs, req ); 
    return 0; 
}


// ==================

// ==================

int_type MPC_MPI::AddPack(short* adr, 
			  int_type nb, u_int_type *bind, u_int_type *eind,
			  message_status_t *req) { 
    mpi_AddPack( adr, nb, bind, eind, req ); 
    return 0; 
}

int_type MPC_MPI::AddPack(long* adr, 
			  int_type nb, u_int_type *bind, u_int_type *eind,
			  message_status_t *req) { 
    mpi_AddPack( adr, nb, bind, eind, req ); 
    return 0; 
}

int_type MPC_MPI::AddPack(u_int_type *adr, 
			  int_type nb, u_int_type *bind, u_int_type *eind,
			  message_status_t *req) { 
    mpi_AddPack( adr, nb, bind, eind, req ); 
    return 0; 
}

int_type MPC_MPI::AddPack(int_type *adr, 
			  int_type nb, u_int_type *bind, u_int_type *eind,
			  message_status_t *req) { 
    mpi_AddPack( adr, nb, bind, eind, req ); 
    return 0; 
}

int_type MPC_MPI::AddPack(float_type *adr, 
			  int_type nb, u_int_type *bind, u_int_type *eind,
			  message_status_t *req) { 
    mpi_AddPack( adr, nb, bind, eind, req ); 
    return 0; 
}

int_type MPC_MPI::AddPack(bool *adr, 
			  int_type nb, u_int_type *bind, u_int_type *eind,
			  message_status_t *req) { 
    mpi_AddPack( adr, nb, bind, eind, req ); 
    return 0; 
}

// ==================

int_type MPC_MPI::AllReduce(c_int_type_p sendbuf, int_type_c_p recvbuf, u_int_type longueur,
		       enum MPC_operation op, comm_id_t *com) { 
    return mpi_AllReduce(sendbuf,recvbuf,longueur,op,com); 
}

int_type MPC_MPI::AllReduce(c_u_int_type_p sendbuf, u_int_type_c_p recvbuf, u_int_type longueur,
		       enum MPC_operation op, comm_id_t *com) { 
    return mpi_AllReduce(sendbuf,recvbuf,longueur,op,com); 
}

int_type MPC_MPI::AllReduce(c_float_type_p sendbuf, float_type_c_p recvbuf, u_int_type longueur,
		       enum MPC_operation op, comm_id_t *com) { 
    return mpi_AllReduce(sendbuf,recvbuf,longueur,op,com); 
}

int_type MPC_MPI::AllReduce(c_short_p sendbuf, short_c_p recvbuf, u_int_type longueur,
		       enum MPC_operation op, comm_id_t *com) { 
    return mpi_AllReduce(sendbuf,recvbuf,longueur,op,com); 
}

#endif

#endif
