#ifndef PARALLELPP_MPC_MPC_C
#define PARALLELPP_MPC_MPC_C


#include "utilspp/mach_types.h"

// inclusion des prototypes des fonctionnalites MPC

#ifndef __INTEL_COMPILER 
#ifdef __GNUG__
#pragma implementation "MPC_MPC.h"
#endif
#endif

#include <mpc.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <typeinfo>
#include <iostream>
#include <stdarg.h>

using namespace std;

#define mpc_assert(op)  if(!(op)) mpc_assert_internal(__LINE__,__FILE__,SCTK_FUNCTION,SCTK_STRING(op))

void mpc_printf(FILE* file,const char *fmt, ...){
  va_list ap;
  char buff[4096];
  int rank;
  
  MPC_Comm_rank(MPC_COMM_WORLD,&rank);

  va_start(ap, fmt);
  sprintf(buff,
	  "[Task %4d]  %s",rank,
	  fmt);
  vfprintf(file, buff, ap);
  va_end(ap);    
}
void mpc_assert_internal(const int line,const char* file,const char* func, const char*op ){
  mpc_printf(stderr,"%s:%d:  %s: Assertion `%s' failed\n",file,line,func ,op);
  abort();
}


#define not_implemented() NOTHING
#define REQCNT_INCREM 1

#ifdef USE_VT
#include "utilspp/VT.h"
#endif
#include "utilspp/svalloc.h"

// inclusion du prototype de la classe MPC_MPC
#include "parallelpp/MPC_MPC.h"
#include "parallelpp/TaskInfo.h"

struct mpc_com_t {
    MPC_Comm mpc_com; // the MPC communicator
    int_type rank;    // the process rank in the communicator mpc_com
    int_type size;    // the size of mpc_com
};

inline MPC_Comm *com2mpc(MPC::comm_id_t *com) {
    return &(((mpc_com_t*)(com->internal_comm_id))->mpc_com);
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
// Gestion de l'erreur d'une fonction MPC
// IN : const char *nomfonc = nom de la fonction membre
// IN : int_type errortype  = type de l'erreur MPC
void MPC_MPC::ErrorMPC(const char *nomfonc,int_type errortype) {

    char message[MPC_MAX_ERROR_STRING];
    int_type resultlen;

    // capture du message d'erreur
    MPC_Error_string((int)errortype,message,&resultlen);

    // Affichage du message d'erreur
    mpc_printf(stderr,"Erreur MPC dans la fonction MPC_MPC::%s:%s\n",nomfonc,message);
    abort();

    // on quitte le programme
    mpc_printf(stderr,"Execution termin\351e !\n");
    MPC_Abort(MPC_COMM_WORLD,1);
}

void *MPC_MPC::MPC_calloc(size_t nmemb, size_t size)
{
    
    void *tmp;
    tmp = calloc( nmemb, size);
    return tmp;
}

void *MPC_MPC::MPC_malloc(size_t size)
{
    
    void *tmp;
    tmp = malloc( size);
    return tmp;
}

void MPC_MPC::MPC_free(void *ptr)
{
    
    free(ptr);
}

void *MPC_MPC::MPC_realloc(void *ptr, size_t size)
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
MPC_MPC::MPC_MPC(int *pargc,char ***pargv, u_int_type nbproc) :
    MPC(pargc,pargv,nbproc) {
  int size;
  int rank;
  int i;
  static mpc_thread_mutex_t lock = MPC_THREAD_MUTEX_INITIALIZER;
  static volatile int done = 0;
  MPC_Init(pargc,pargv);
  MPC_Comm_size(MPC_COMM_WORLD,&size);
  MPC_Comm_rank(MPC_COMM_WORLD,&rank);
  rang_ = rank;
  if(size != nbproc){
    mpc_printf(stderr,"Attention ! Seulement %d processus actifs pour %d demand\351s\n",size,nproc_ );
    abort();
  }

  COMM_WORLD.internal_comm_id = new mpc_com_t;
  mpc_com_t *tmp_com = (mpc_com_t*)COMM_WORLD.internal_comm_id;
  tmp_com->mpc_com = MPC_COMM_WORLD;
  tmp_com->rank = rang_;
  tmp_com->size = nproc_;
  
  //mpc_printf(stderr,"%d / %d\n",rang_,nproc_);

  // construction du type booléen pour MPC
  MPC_Sizeof_datatype(&booltype_ ,sizeof(bool));
  
  // construction du type "nombre complexe" pour MPC
  float_type unreel = 0.;
  MPC_Sizeof_datatype(&complextype_,2*GetMPCType(&unreel));

  // Affectation du communicateur par defaut
  communicateur_ = MPC_COMM_WORLD;

  MPC_pending_req_first = 0;
  MPC_pending_req = NULL;
  MPC_pending_req_free = NULL;
  MPC_pending_req_size = 10;
  MPC_pending_req = (MPC_Request **)realloc(MPC_pending_req,MPC_pending_req_size*sizeof(MPC_Request *));
  MPC_pending_req_free = (MPC_Request **)realloc(MPC_pending_req_free,MPC_pending_req_size*sizeof(MPC_Request *));

  for(i = 0; i < MPC_pending_req_size; i++){
    MPC_pending_req_free[i] = (MPC_Request *)malloc(sizeof(MPC_Request));
    mpc_assert(MPC_pending_req_free[i] != NULL);
    MPC_pending_req[i] = NULL;
    //mpc_printf(stderr,"id = %d %p %p\n",i,MPC_pending_req[i],MPC_pending_req_free[i]);
  }  
 
  mpc_thread_mutex_lock(&lock);
  if(done == 0){
    taskinfo = new TaskInfo_MT();
    done = 1; 
  }
  mpc_thread_mutex_unlock(&lock);

  MPC_Barrier(MPC_COMM_WORLD);

  ((TaskInfo_MT*)taskinfo)->InitTaskInfo();

} // fin constructeur

int_type MPC_MPC::NewRequest() {
  int_type req_id;
  int i;

  if(MPC_pending_req_first < MPC_pending_req_size){
    req_id = MPC_pending_req_first;
    MPC_pending_req[MPC_pending_req_first] = MPC_pending_req_free[MPC_pending_req_first];
    //mpc_printf(stderr,"no resize id = %d %p %p\n",req_id,MPC_pending_req[req_id],MPC_pending_req_free[MPC_pending_req_first]);   
    MPC_pending_req_first ++; 
  } else {
    MPC_pending_req_size += 10;
    MPC_pending_req = (MPC_Request **)realloc(MPC_pending_req,MPC_pending_req_size*sizeof(MPC_Request *));
    MPC_pending_req_free = (MPC_Request **)realloc(MPC_pending_req_free,MPC_pending_req_size*sizeof(MPC_Request *));    
    for(i = MPC_pending_req_first; i < MPC_pending_req_size; i++){
      MPC_pending_req_free[i] = (MPC_Request *)malloc(sizeof(MPC_Request));
      mpc_assert(MPC_pending_req_free[i] != NULL);
      MPC_pending_req[i] = NULL;
      //mpc_printf(stderr,"id = %d %p %p\n",i,MPC_pending_req[i],MPC_pending_req_free[i]);
    }  
    req_id = NewRequest();
    //mpc_printf(stderr,"resize id = %d %p\n",req_id,MPC_pending_req[req_id]);    
  }
  //mpc_printf(stderr,"id = %d %p\n",req_id,MPC_pending_req[req_id]);
  return req_id;
}

void MPC_MPC::FreeRequest( int_type req_id ) {
  MPC_pending_req_free[MPC_pending_req_first -1] = MPC_pending_req[req_id];  
  MPC_pending_req[req_id] = NULL;
  while(MPC_pending_req[MPC_pending_req_first -1] == NULL){
    MPC_pending_req_first--;
  }
  //mpc_printf(stderr,"New id %d\n",MPC_pending_req_first);
}

void MPC_MPC::FreeRequest() {
}

/*
 *====================================================================================
 *====================================================================================
 */
// Destructeur
MPC_MPC::~MPC_MPC() {
    FreeRequest();


}

/*
 *====================================================================================
 *====================================================================================
 */
void MPC_MPC::StartTasks( void (*entry)(struct main_arg*), void *arg) {

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
int_type MPC_MPC::Rank(comm_id_t *com) { 

    if ( !com )
	return rang_; 

    int_type rank = *com2rank(com);

    return rank;
}

/*
 *====================================================================================
 *====================================================================================
 */
u_int_type MPC_MPC::Nproc(comm_id_t *com) const {

    if ( !com )
	return nproc_;

    u_int_type size = *com2size(com);

    return size;
}
/*
 *====================================================================================
 *====================================================================================
 */
void MPC_MPC::ComCreate(comm_id_t *comold, int* liste, int n, comm_id_t *comnew) {

    if ( !comold ) {

	printf("Communicateur initial NULL\n");
	Abort(1);
    }

    int_type code;

    MPC_Comm mpc_comm_old = *com2mpc(comold);
    MPC_Group mpc_group_old, mpc_group_new;

    code = MPC_Comm_group(mpc_comm_old, &mpc_group_old);
    if (code != MPC_SUCCESS) 
	ErrorMPC("ComCreate():MPC_Comm_group()",code);

    code = MPC_Group_incl(mpc_group_old, n, liste, &mpc_group_new);
    if (code != MPC_SUCCESS) 
	ErrorMPC("ComCreate():MPC_Group_incl()",code);

    /* the new communicator creation
     */
    comnew->internal_comm_id = new mpc_com_t;
    code = MPC_Comm_create(mpc_comm_old, mpc_group_new, 
			   com2mpc(comnew));
    if (code != MPC_SUCCESS) 
	ErrorMPC("ComCreate():MPC_Comm_create()",code);
    //

    code = MPC_Group_free(&mpc_group_old);
    if (code != MPC_SUCCESS) 
	ErrorMPC("ComCreate():MPC_Comm_free()",code);   
 
    code = MPC_Group_free(&mpc_group_new);
    if (code != MPC_SUCCESS) 
	ErrorMPC("ComCreate():MPC_Comm_free()",code);

    /* nous completons les champs rank et size
     */
    int_type *prank = com2rank(comnew);
    int_type *psize = com2size(comnew);
    
    code = MPC_Comm_rank(*com2mpc(comnew), prank);
    if (code != MPC_SUCCESS) 
	ErrorMPC("ComCreate():MPC_Comm_rank()",code);

    code = MPC_Comm_size(*com2mpc(comnew), psize);
    if (code != MPC_SUCCESS) 
	ErrorMPC("ComCreate():MPC_Comm_size()",code);
}

/*
 *====================================================================================
 *====================================================================================
 */
void MPC_MPC::ComSplit(comm_id_t *comold, int color, int key, comm_id_t *comnew) {

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
    MPC_Comm mpc_com_old = *com2mpc(comold);

    /* we're creating sub-commuincator by subdividing mpc_com_old into disjoint
       subsets */
    code = MPC_Comm_split( mpc_com_old, color, key, 
			   com2mpc(comnew) );
    if (code != MPC_SUCCESS) 
	ErrorMPC("ComSplit():MPC_Comm_split()",code);    
    
    /* nous completons les champs rank et size
     */
    int_type *prank = com2rank(comnew);
    int_type *psize = com2size(comnew);
    
    code = MPC_Comm_rank(*com2mpc(comnew), prank);
    if (code != MPC_SUCCESS) 
	ErrorMPC("ComCreate():MPC_Comm_rank()",code);

    code = MPC_Comm_size(*com2mpc(comnew), psize);
    if (code != MPC_SUCCESS) 
	ErrorMPC("ComCreate():MPC_Comm_size()",code);
}

/*
 *====================================================================================
 *====================================================================================
 */
void MPC_MPC::ComDelete(comm_id_t *com) {

    if ( !com ) {

	printf("Communicateur initial NULL\n");
	Abort(1);
    }

    if ( com->internal_comm_id == NULL ||
	 *com2mpc(com) == *com2mpc(&COMM_WORLD) ) {

	printf("Impossible de detruire le communicateur NULL ou le communicateur COMM_WORLD\n");
	Abort(1);
    }

    int_type code;

    code = MPC_Comm_free( com2mpc(com) );
    if (code != MPC_SUCCESS) 
	ErrorMPC("ComDelete():MPC_Comm_free()",code);

    delete (mpc_com_t*)com->internal_comm_id;
}

/*
 *====================================================================================
 *====================================================================================
 */
// Effectue une barriere sur tous les processus
// retourne une valeur <0 si erreur
int_type MPC_MPC::Barrier(comm_id_t *com) { 

    int_type code;
    MPC_Comm mpc_com = ( com ? *com2mpc(com) : MPC_COMM_WORLD );

    code = MPC_Barrier(mpc_com);
    if (code != MPC_SUCCESS) 
	ErrorMPC("Barrier():MPC_Barrier()",code);

    return code;
}

/*
 *====================================================================================
 *====================================================================================
 */
// Quitter l'environnement MPC
// Retourne une valeur negative si erreur
int_type MPC_MPC::Exit() { 
    int_type code;
    code = MPC_Finalize();
    if (code != MPC_SUCCESS) ErrorMPC("Exit():MPC_Finalize()",code);    
    return code; 
}


/*
 *====================================================================================
 *====================================================================================
 */
// Retourne le type MPC correspondant au type u_int_type
MPC_Datatype MPC_MPC::GetMPCType(c_u_int_type_p) {

    MPC_Datatype montype;
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
	montype = MPC_UNSIGNED_SHORT;
    } else if (taille == typeuint) {
	// entier standard code sur 4 octets
	montype = MPC_UNSIGNED;
    } else if (taille == typeulong) {
	// entier long code sur 8 octets
	montype = MPC_UNSIGNED_LONG;
    } else {
	// taille inconnue
      mpc_printf(stderr,"Type entier inconnu. Arr\352t !\n");
	abort();
    }

    return montype;
}


/*
 *====================================================================================
 *====================================================================================
 */
// Retourne le type MPC correspondant au type int
MPC_Datatype MPC_MPC::GetMPCType(c_int_type_p) {

    MPC_Datatype montype;
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
	montype = MPC_SHORT;
    } else if (taille == typeint) {
	// entier standard code sur 4 octets
	montype = MPC_INT;
    } else if (taille == typelong) {
	// entier long code sur 8 octets
	montype = MPC_LONG;
    } else {
	// taille inconnue
      mpc_printf(stderr,"Type entier inconnu. Arr\352t !\n");
	abort();
    }

    return montype;
}

/*
 *====================================================================================
 *====================================================================================
 */
// Retourne le type MPC correspondant au type float_type
MPC_Datatype MPC_MPC::GetMPCType(c_float_type_p) {

    MPC_Datatype montype;
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
	montype = MPC_FLOAT;
    }
    else if (taille == typedouble) {
	// reel standard code sur 8 octets
	montype = MPC_DOUBLE;
    }
    else if (taille == typelgdbl) {
	// reel long code sur 16 octets
	montype = MPC_LONG_DOUBLE;
    }
    else {
	// taille inconnue
      mpc_printf(stderr,"Type entier inconnu. Arr\352t !\n");
	abort();
    }

    return montype;
}

/*
 *====================================================================================
 *====================================================================================
 */
MPC_Datatype MPC_MPC::GetMPCType(c_short_p) {
    return MPC_SHORT;
}

/*
 *====================================================================================
 *====================================================================================
 */
MPC_Datatype MPC_MPC::GetMPCType(c_bool_p) {
    return booltype_;
}

/*
 *====================================================================================
 *====================================================================================
 */
MPC_Datatype MPC_MPC::GetMPCType(void*) {
    return MPC_BYTE;
}

/*
 *====================================================================================
 *====================================================================================
 */
template<typename T>
int_type MPC_MPC::mpc_ISendScal(u_int_type dest,u_int_type msgtag,
				T *adr,u_int_type longueur,
				message_status_t *req, 
				comm_id_t *com) {
    MPC_Datatype montype;
    int_type code, mgtg, src, nproc;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPC_Comm mpc_com = *com2mpc(tmp_com);

    int_type req_id = NewRequest();

    // determination du type MPC
    montype = GetMPCType(adr);

    // Construction du message tag
    src = *com2rank(tmp_com);
    nproc = *com2size(tmp_com);
    mgtg = src + nproc * dest;

//     int_type new_size;
//     MPC_Type_size(montype, &new_size);
//     printf("cpu%d -> cpu %d : %d octets\n", src, dest, longueur*new_size);
    // Envoi non bloquant
    code = MPC_Isend( (void*)adr, (int)longueur, montype,
		      (int)dest, (int)mgtg, mpc_com, MPC_pending_req[req_id]);
    if (code != MPC_SUCCESS) 
	ErrorMPC("mpc_ISendScal():MPC_Isend()",code);

    if (req)
	req->status = req_id;

    return code;
}

/*
 *====================================================================================
 *====================================================================================
 */
template<typename T>
int_type MPC_MPC::mpc_IRecvScal(u_int_type prov,u_int_type msgtag,
				T *adr,u_int_type longueur,
				message_status_t *req, 
				comm_id_t *com) {

    // MPC_Status status;
    MPC_Datatype montype;
    int_type code, mgtg, dest, nproc;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPC_Comm mpc_com = *com2mpc(tmp_com);

    int_type req_id = NewRequest();

    // determination du type MPC
    montype = GetMPCType(adr);

    // Construction du message tag
    dest = *com2rank(tmp_com);
    nproc = *com2size(tmp_com);
    mgtg = prov + nproc * dest;

//     int_type new_size;
//     MPC_Type_size(montype, &new_size);
//     printf("cpu%d <- cpu %d : %d octets\n", dest, prov, longueur*new_size);
    // reception non bloquante
    code = MPC_Irecv( (void*)adr, (int)longueur, montype,
		      (int)prov, (int)mgtg, mpc_com, MPC_pending_req[req_id]);
    if (code != MPC_SUCCESS)
	ErrorMPC("mpc_IRecvScal():MPC_Irecv()",code);

    if (req)
	req->status = req_id;

    return code; 
}

int_type MPC_MPC::Wait(struct message_status_t *req) {
  MPC_Status status;
    if ( !req ) {
	printf("MPC_MPC::Wait : requete NULL impossible !\n");
	Abort(1);
    }

    int_type req_id = req->status;

//     if ( !flag_req_allocated[req_id] ) {
// 	printf("MPC_MPC::Wait : requete %d non allouee !\n", req_id);
// 	Abort(1);
//     }

    int_type code = MPC_Wait(MPC_pending_req[req_id], &(status)) ;
    if (code != MPC_SUCCESS)
	ErrorMPC("Wait():MPC_Wait()",code);

    FreeRequest( req_id );

    return code ;
}


/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPC::WaitAll(int_type nb_in, message_status_t *req_in) {
  int i;
  MPC_Status status;

  if(nb_in){
    for(i = 0; i < nb_in; i++){
      Wait(&(req_in[i]));
    }
  } else {
    for(i = MPC_pending_req_first -1; i >= 0; i--){
      MPC_Wait(MPC_pending_req[i], &(status)) ;
      FreeRequest( i );
    }    
  }


    return 0; 
}


/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPC::WaitSome(int_type nb_in, message_status_t *req_in,
			   int_type &nb_out, int_type *indices_out) {
    int_type code = 0;

#if 0
    if ( nb_in ) {

	//	ReallocBlock( (nb_in*sizeof(MPC_Request)/sizeof(int)) + 1 );
	//	MPC_Request *tab_req = (MPC_Request *)block;
	MPC_Request *tab_req = new MPC_Request[nb_in];
	
	for( int_type r = 0 ; r < nb_in ; r++ )
	    tab_req[r] = mpcreq[ req_in[r].status ];
	
	code = MPC_Waitsome( nb_in, tab_req, &nb_out, indices_out, mpcstatuses );
	if (code != MPC_SUCCESS)
	    ErrorMPC("WaitAll():MPC_Waitall()",code);

	for( int_type r = 0 ; r < nb_out ; r++ ) {
	    mpcreq[ req_in[ indices_out[r] ].status ] = tab_req[ indices_out[r] ];
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
int_type MPC_MPC::OpenPack( int_type nb, 
			    u_int_type *begindex, u_int_type *endindex,
			    message_status_t *req ) {
  int_type req_id = NewRequest();
  req->status = req_id;
  
  //mpc_printf(stderr,"open with id = %d %p\n",req_id,MPC_pending_req[req_id]);
  MPC_Default_pack(nb,begindex,endindex,MPC_pending_req[req_id]);

  return 0;
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPC::OpenPack( message_status_t *req ) {
    int_type req_id = NewRequest();
    req->status = req_id;
    
    MPC_Open_pack(MPC_pending_req[req_id]);
   
    return 0;
}


/*
 *====================================================================================
 *====================================================================================
 */
template<typename T>
void MPC_MPC::mpc_AddPack( T *adr, message_status_t *req ) {
  MPC_Add_pack_default(adr,GetMPCType(adr),MPC_pending_req[req->status]);
}

/*
 *====================================================================================
 *====================================================================================
 */
template<typename T>
void MPC_MPC::mpc_AddPack( T *adr, 
			   int_type nb_displs, int_type_p displs, 
			   message_status_t *req ) {
  MPC_Add_pack(adr, (int)nb_displs, 
	       (mpc_pack_indexes_t*)displs, (mpc_pack_indexes_t*)displs,
	       GetMPCType(adr),MPC_pending_req[req->status]);
}

/*
 *====================================================================================
 *====================================================================================
 */
template<typename T>
void MPC_MPC::mpc_AddPack( T *adr, 
			   int_type nb, u_int_type *bind, u_int_type *eind,
			   message_status_t *req ) {
  MPC_Add_pack(adr, nb, 
	       bind, eind,
	       GetMPCType(adr),MPC_pending_req[req->status]);
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPC::ISendPack( int_type destination, u_int_type msgtag, 
			     message_status_t *req, comm_id_t *com ) {
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPC_Comm mpc_com = *com2mpc(tmp_com);
    return MPC_Isend_pack(destination,msgtag,mpc_com,MPC_pending_req[req->status]);
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPC::IRecvPack( int_type source, u_int_type msgtag, 
			     message_status_t *req, comm_id_t *com ) {
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPC_Comm mpc_com = *com2mpc(tmp_com);
    return MPC_Irecv_pack(source,msgtag,mpc_com,MPC_pending_req[req->status]);
}

/*
 *====================================================================================
 *====================================================================================
 */
// Retourne l'operation de reduction MPC correspondant a l'operation MPC
MPC_Op MPC_MPC::GetMPCOperation(enum MPC_operation op) {
    MPC_Op mpcop;

    switch(op) {

    case OMIN : mpcop = MPC_MIN; break;

    case OMAX : mpcop = MPC_MAX; break;

    case OSUM : mpcop = MPC_SUM; break;

    default:
	// operation inconnue
      mpc_printf(stderr,"Opération de réduction inconnue. Arrêt !\n");
	abort();
    }

    return mpcop;
}


/*
 *====================================================================================
 *====================================================================================
 */
template<typename T>
int_type MPC_MPC::mpc_AllReduce( const T *const sendbuf, T *const recvbuf, u_int_type longueur,
				 enum MPC_operation op, comm_id_t *com ) {
#ifdef USE_VT
    VT_begin( VT_ALLREDUCE );
#endif
    MPC_Datatype montype;
    MPC_Op monop;
    int_type code;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPC_Comm mpc_com = *com2mpc(tmp_com);

    // type MPC correspondant au type T
    montype = GetMPCType(sendbuf);

    // recuperation de l'operation MPC
    monop = GetMPCOperation(op);

    // on effectue la reduction-diffusion
    code = MPC_Allreduce( (void*)sendbuf, (void*)recvbuf, (int)longueur,
			  montype, monop, mpc_com );

    if (code != MPC_SUCCESS)
	ErrorMPC("mpc_AllReduce():MPC_Allreduce()",code);
#ifdef USE_VT
    VT_end( VT_ALLREDUCE );
#endif

    return code;
}


/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPC::BroadCast( void *buf, int_type count, int_type root,
			     comm_id_t *com ) {
    int_type code;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPC_Comm mpc_com = *com2mpc(tmp_com);

    code = MPC_Bcast( buf, count, MPC_BYTE, root, mpc_com);
    if (code != MPC_SUCCESS)
	ErrorMPC("BroadCast():MPC_Bcast()",code);
    
    return code;
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPC::GatherV( void *sendbuf, int_type sendcount,
			   void *recvbuf, int_type *recvcounts, 
			   int_type* displs ,
			   int_type root, comm_id_t *com ) {
    int_type code;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPC_Comm mpc_com = *com2mpc(tmp_com);

    code = MPC_Gatherv( sendbuf, sendcount, MPC_BYTE,
		        recvbuf, recvcounts, displs, MPC_BYTE,
			root, mpc_com );
    if (code != MPC_SUCCESS)
	ErrorMPC("GatherV():MPC_Gatherv()",code);

    return code;
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPC::ScatterV( void *sendbuf, int_type *sendcounts, 
			    int_type* displs ,
			    void *recvbuf, int_type recvcount,
			    int_type root, comm_id_t *com ) {
    int_type code;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPC_Comm mpc_com = *com2mpc(tmp_com);

    code = MPC_Scatterv( sendbuf, sendcounts, displs, MPC_BYTE,
			 recvbuf, recvcount, MPC_BYTE,
			 root, mpc_com );
    if (code != MPC_SUCCESS)
	ErrorMPC("ScatterV():MPC_Scatterv()",code);

    return code;
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPC::AllGatherV( void *sendbuf, int_type sendcount,
			      void *recvbuf, int_type *recvcounts, 
			      int_type* displs, 
			      comm_id_t *com ) {
    int_type code;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPC_Comm mpc_com = *com2mpc(tmp_com);

    code = MPC_Allgatherv( sendbuf, sendcount, MPC_BYTE,
			   recvbuf, recvcounts, displs, MPC_BYTE,
			   mpc_com );
    if (code != MPC_SUCCESS)
	ErrorMPC("AllGatherV():MPC_Allgatherv()",code);

    return code;
}

/*
 *====================================================================================
 *====================================================================================
 */
int_type MPC_MPC::AllToAllV( void *sendbuf, int_type *sendcounts, 
			     int_type* sdispls ,
			     void *recvbuf, int_type *recvcounts, 
			     int_type* rdispls,
			     comm_id_t *com ) {
    int_type code;
    comm_id_t *tmp_com = ( com ? com : &COMM_WORLD );
    MPC_Comm mpc_com = *com2mpc(tmp_com);

    code = MPC_Alltoallv( sendbuf, sendcounts, sdispls, MPC_BYTE,
			  recvbuf, recvcounts, rdispls, MPC_BYTE,
			  mpc_com );
    if (code != MPC_SUCCESS)
	ErrorMPC("AllToAllV():MPC_Alltoallv()",code);

    return code;
}

// ==================

int_type MPC_MPC::ISendScal(u_int_type dest,u_int_type msgtag,
			   u_int_type * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpc_ISendScal( dest, msgtag, adr, longueur, req, com ); 
}

int_type MPC_MPC::ISendScal(u_int_type dest,u_int_type msgtag,
			   int_type * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpc_ISendScal( dest, msgtag, adr, longueur, req, com ); 
}

int_type MPC_MPC::ISendScal(u_int_type dest,u_int_type msgtag,
			   float_type * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpc_ISendScal( dest, msgtag, adr, longueur, req, com ); 
}

int_type MPC_MPC::ISendScal(u_int_type dest,u_int_type msgtag,
			   short * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpc_ISendScal( dest, msgtag, adr, longueur, req, com ); 
}
int_type MPC_MPC::ISendScal(u_int_type dest,u_int_type msgtag,
			   void * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpc_ISendScal( dest, msgtag, adr, longueur, req, com ); 
}

// ==================

int_type MPC_MPC::IRecvScal(u_int_type prov,u_int_type msgtag,
			   u_int_type * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpc_IRecvScal( prov, msgtag, adr, longueur, req, com );
}

int_type MPC_MPC::IRecvScal(u_int_type prov,u_int_type msgtag,
			   int_type * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpc_IRecvScal( prov, msgtag, adr, longueur, req, com );
}

int_type MPC_MPC::IRecvScal(u_int_type prov,u_int_type msgtag,
			   float_type * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpc_IRecvScal( prov, msgtag, adr, longueur, req, com );
}

int_type MPC_MPC::IRecvScal(u_int_type prov,u_int_type msgtag,
			   short * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpc_IRecvScal( prov, msgtag, adr, longueur, req, com );
}

int_type MPC_MPC::IRecvScal(u_int_type prov,u_int_type msgtag,
			   void * adr,u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com) {
    return mpc_IRecvScal( prov, msgtag, adr, longueur, req, com );
}

// ==================

int_type MPC_MPC::AddPack(short* adr, message_status_t *req) { 
    mpc_AddPack( adr, req ); 
    return 0; 
}

int_type MPC_MPC::AddPack(u_int_type *adr, message_status_t *req) { 
    mpc_AddPack( adr, req ); 
    return 0; 
}

int_type MPC_MPC::AddPack(int_type *adr, message_status_t *req) { 
    mpc_AddPack( adr, req ); 
    return 0; 
}

int_type MPC_MPC::AddPack(float_type *adr, message_status_t *req) { 
    mpc_AddPack( adr, req ); 
    return 0; 
}

int_type MPC_MPC::AddPack(bool *adr, message_status_t *req) { 
    mpc_AddPack( adr, req ); 
    return 0; 
}

int_type MPC_MPC::AddPack(long *adr, message_status_t *req) { 
    mpc_AddPack( adr, req ); 
    return 0; 
}

// ==================

int_type MPC_MPC::AddPack(short* adr, int_type nb_displs, int_type_p displs, 
			  message_status_t *req) { 
    mpc_AddPack( adr, nb_displs, displs, req ); 
    return 0; 
}

int_type MPC_MPC::AddPack(u_int_type *adr, int_type nb_displs, int_type_p displs, 
			  message_status_t *req) { 
    mpc_AddPack( adr, nb_displs, displs, req ); 
    return 0; 
}

int_type MPC_MPC::AddPack(int_type *adr, int_type nb_displs, int_type_p displs, 
			  message_status_t *req) { 
    mpc_AddPack( adr, nb_displs, displs, req ); 
    return 0; 
}

int_type MPC_MPC::AddPack(float_type *adr, int_type nb_displs, int_type_p displs, 
			  message_status_t *req) { 
    mpc_AddPack( adr, nb_displs, displs, req ); 
    return 0; 
}

int_type MPC_MPC::AddPack(bool *adr, int_type nb_displs, int_type_p displs, 
			  message_status_t *req) { 
    mpc_AddPack( adr, nb_displs, displs, req ); 
    return 0; 
}
int_type MPC_MPC::AddPack(long *adr, int_type nb_displs, int_type_p displs, 
			  message_status_t *req) { 
    mpc_AddPack( adr, nb_displs, displs, req ); 
    return 0; 
}


// ==================

// ==================

int_type MPC_MPC::AddPack(short* adr, 
			  int_type nb, u_int_type *bind, u_int_type *eind,
			  message_status_t *req) { 
    mpc_AddPack( adr, nb, bind, eind, req ); 
    return 0; 
}

int_type MPC_MPC::AddPack(long* adr, 
			  int_type nb, u_int_type *bind, u_int_type *eind,
			  message_status_t *req) { 
    mpc_AddPack( adr, nb, bind, eind, req ); 
    return 0; 
}

int_type MPC_MPC::AddPack(u_int_type *adr, 
			  int_type nb, u_int_type *bind, u_int_type *eind,
			  message_status_t *req) { 
    mpc_AddPack( adr, nb, bind, eind, req ); 
    return 0; 
}

int_type MPC_MPC::AddPack(int_type *adr, 
			  int_type nb, u_int_type *bind, u_int_type *eind,
			  message_status_t *req) { 
    mpc_AddPack( adr, nb, bind, eind, req ); 
    return 0; 
}

int_type MPC_MPC::AddPack(float_type *adr, 
			  int_type nb, u_int_type *bind, u_int_type *eind,
			  message_status_t *req) { 
    mpc_AddPack( adr, nb, bind, eind, req ); 
    return 0; 
}

int_type MPC_MPC::AddPack(bool *adr, 
			  int_type nb, u_int_type *bind, u_int_type *eind,
			  message_status_t *req) { 
    mpc_AddPack( adr, nb, bind, eind, req ); 
    return 0; 
}

// ==================

int_type MPC_MPC::AllReduce(c_int_type_p sendbuf, int_type_c_p recvbuf, u_int_type longueur,
		       enum MPC_operation op, comm_id_t *com) { 
    return mpc_AllReduce(sendbuf,recvbuf,longueur,op,com); 
}

int_type MPC_MPC::AllReduce(c_u_int_type_p sendbuf, u_int_type_c_p recvbuf, u_int_type longueur,
		       enum MPC_operation op, comm_id_t *com) { 
    return mpc_AllReduce(sendbuf,recvbuf,longueur,op,com); 
}

int_type MPC_MPC::AllReduce(c_float_type_p sendbuf, float_type_c_p recvbuf, u_int_type longueur,
		       enum MPC_operation op, comm_id_t *com) { 
    return mpc_AllReduce(sendbuf,recvbuf,longueur,op,com); 
}

int_type MPC_MPC::AllReduce(c_short_p sendbuf, short_c_p recvbuf, u_int_type longueur,
		       enum MPC_operation op, comm_id_t *com) { 
    return mpc_AllReduce(sendbuf,recvbuf,longueur,op,com); 
}

#endif
