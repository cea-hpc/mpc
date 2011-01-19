// ----------------------------------------------------------------------------------
// /HIST/ David Dureau - 08/12/99 - Creation
//
// /PURP/ Definition de la classe MPC_MPI, heritee de MPC
// /PURP/
// /PURP/ Il s'agit de surcharger les methodes pures de MPC
// /PURP/ a partir de l'environnement MPI (message Passing Interface)
// ----------------------------------------------------------------------------------


#ifndef PARALLELPP_MPC_MPI_H
#define PARALLELPP_MPC_MPI_H


#if NOMPI
#undef MPI_MPC_MPI_H
#else
#define MPI_MPC_MPI_H
#endif

#ifdef MPI_MPC_MPI_H

#ifndef __INTEL_COMPILER 
#ifdef __GNUG__
#pragma interface
#endif
#endif


#include <mpi.h>


#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>

/* #include "utilspp/traits.h" */

#include "parallelpp/MPC.h"

#define BLOCKSIZE 131072
#define BLOCKSIZEMAX 131072000 // à l'origine 13107200


struct elt_mpipack_t {
    char *adr;      // the address of the array to pack
    MPI_Datatype elt_type;  // the corresponding MPI type
} ;

// struct for point to point communication of a package
struct pt2pt_mpipack_t {
    int_type src;
    int_type dest;
    int_type nbindex;
    u_int_type *begindex;
    int_type *blockindex;
    int_type count;
    struct elt_mpipack_t *packarray;
    int_type szmax;
} ;


// Definition de la classe
class MPC_MPI : public MPC {

protected:
  
    MPI_Comm communicateur_; // communicateur 
    int_type mpibufpos_; // position courante dans le buffer
    int_type *mpibufsize_; // taille max du buffer
    void **mpibuf_; // pointeur sur le buffer
    int_type rang_;  // numero du processus (0 <= rang_ <= nproc_-1)
  
    int_type  pk_nbindex;
    int_type *pk_index;

    MPI_Request *mpireq;
    MPI_Status  *mpistatuses;
    pt2pt_mpipack_t *pk_msg;
    int_type *unpack_id;
    int_type reqcntmax;
    int_type unpack_count;

    int_type blockcnt;
    int_type *block;

    int_type sz_lenght1;
    int_type *lenght1;

    MPI_Datatype booltype_;
    MPI_Datatype complextype_;

    int_type nb_type_commit;
    int_type nb_type_free;

    int_type cnt_req_free;
    int_type *stack_req_free;

    bool *flag_req_allocated;

    void *MPC_BOTTOM_;

    void ErrorMPI(const char *nomfonc,int_type errortype);

    int_type NewRequest();
    void FreeRequest(int_type req_id);
    void FreeRequest();

    // Retourne le type MPI correspondant au type float_type
    MPI_Datatype GetMPIType(c_float_type_p);

    // Retourne le type MPI correspondant au type u_int_type
    MPI_Datatype GetMPIType(c_u_int_type_p);

    // Retourne le type MPI correspondant au type int
    MPI_Datatype GetMPIType(c_int_type_p);

    MPI_Datatype GetMPIType(c_short_p);

    MPI_Datatype GetMPIType(c_bool_p);

    MPI_Datatype GetMPIType(void *);

    // Retourne l'operation de reduction MPI correspondant a l'operation MPC
    MPI_Op GetMPIOperation(enum MPC_operation);


    template<typename T>
    int_type mpi_ISendScal( u_int_type dest, u_int_type msgtag,
			    T *adr, u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com );

    template<typename T>
    int_type mpi_IRecvScal( u_int_type prov, u_int_type msgtag,
			    T *adr, u_int_type longueur,
			    message_status_t *req, 
			    comm_id_t *com );

    void ReallocBlock(c_int_type nb_needed);
    void ReallocLenght1(c_int_type nb_needed);

    template<typename T>
    void mpi_AddPack( T *adr, message_status_t *req );

    template<typename T>
    void mpi_AddPack( T *adr, int_type nb_displs, int_type_p displs, message_status_t *req );

    template<typename T>
    void mpi_AddPack( T *adr, int_type nb, u_int_type *bind, u_int_type *eind, 
		      message_status_t *req );

    template<typename T>
    int_type mpi_AllReduce( const T *const sendbuf, T *const recvbuf, u_int_type longueur,
			    enum MPC_operation op, comm_id_t *com );

public:

    // Constructeur 
    // IN : pargc = pointeur sur le nombre d'arguments du run, executable compris
    // IN : pargv = pointeur sur la liste des arguments du run, executable compris
    // IN : nbproc = le nombre de processus a activer
    MPC_MPI(int *pargc,char ***pargv,u_int_type nbproc);

    // Destructeur
    virtual ~MPC_MPI();

    virtual void StartTasks( void (*entry)(struct main_arg*), void *arg) ;

  void *MPC_calloc(size_t nmemb, size_t size);

  void *MPC_malloc(size_t size);

  void MPC_free(void *ptr);

  void *MPC_realloc(void *ptr, size_t size);

    // Retourne le rang du processus courant
    // C'est un entier compris entre 0 et MPC::Nproc()-1
    // retourne une valeur negative si erreur
    int_type Rank(comm_id_t *com = NULL);

    u_int_type Nproc(comm_id_t *com = NULL) const;

    // Effectue une barriere sur tous les processus
    // retourne une valeur <0 si erreur
    int_type Barrier(comm_id_t *com = NULL);

    virtual void ComCreate(comm_id_t *comold, int* liste, int n, 
			   comm_id_t *comnew);

    virtual void ComSplit(comm_id_t *comold, int color, int key, 
			  comm_id_t *comnew);

    virtual void ComDelete(comm_id_t *com);

    // Quitter l'environnement MPC
    // Retourne une valeur negative si erreur
    int_type Exit();

    void Abort( int_type errcode ) { MPI_Abort( communicateur_, errcode ); }

    // Communication point_type a point
    // Envoi non bloquant d'un tableau de type entier non signe
    // IN : u_int_type  dest       = numero de rang du processus a qui est envoye le message
    // IN : u_int_type   msgtag    = etiquette du message
    // IN : u_int_type   *adr      = adresse du buffer de donnees contigues a envoyer
    // IN : u_int_type   longueur  = nombre d'elements du buffer a envoyer
    // OUT : int_type  SendScal = 0 si OK, <0 si erreur
    int_type ISendScal(u_int_type dest,u_int_type msgtag,
		       u_int_type * adr,u_int_type longueur,
		       message_status_t *req = NULL, 
		       comm_id_t *com = NULL);

    // Communication point_type a point
    // Envoi non bloquant d'un tableau de type entier
    // IN : u_int_type  dest       = numero de rang du processus a qui est envoye le message
    // IN : u_int_type   msgtag    = etiquette du message
    // IN : int_type     *adr      = adresse du buffer de donnees contigues a envoyer
    // IN : u_int_type   longueur  = nombre d'elements du buffer a envoyer
    // OUT : int_type  SendScal = 0 si OK, <0 si erreur
    int_type ISendScal(u_int_type dest,u_int_type msgtag,
		       int_type * adr,u_int_type longueur,
		       message_status_t *req = NULL, 
		       comm_id_t *com = NULL);

    // Communication point_type a point
    // Envoi non bloquant d'un tableau de type reel
    // IN : u_int_type  dest       = numero de rang du processus a qui est envoye le message
    // IN : u_int_type   msgtag    = etiquette du message
    // IN : float_type   *adr      = adresse du buffer de donnees contigues a envoyer
    // IN : u_int_type   longueur  = nombre d'elements du buffer a envoyer
    // OUT : int_type  SendScal = 0 si OK, <0 si erreur
    int_type ISendScal(u_int_type dest,u_int_type msgtag,
		       float_type * adr,u_int_type longueur,
		       message_status_t *req = NULL, 
		       comm_id_t *com = NULL);

    int_type ISendScal(u_int_type dest,u_int_type msgtag,
		       short * adr,u_int_type longueur,
		       message_status_t *req = NULL, 
		       comm_id_t *com = NULL);

    int_type ISendScal(u_int_type dest,u_int_type msgtag,
		       char *adr,u_int_type longueur,
		       message_status_t *req = NULL, 
		       comm_id_t *com = NULL) {
      fprintf(stderr,"Not implemented");abort();
	return 1;
    }

    int_type ISendScal(u_int_type dest,u_int_type msgtag,
		       void *adr,u_int_type longueur,
		       message_status_t *req = NULL, 
		       comm_id_t *com = NULL);

    // Communication point_type a point
    // Reception non bloquante d'un tableau de type entier non signe
    // IN : u_int_type  prov       = numero de rang du processus qui a envoye le message
    // IN : u_int_type   msgtag    = etiquette du message
    // IN : u_int_type   *adr      = adresse du buffer qui va recevoir les donnees contigues
    // IN : u_int_type   longueur  = nombre d'elements a recevoir et a stocker dans le buffer
    // OUT : int_type  RecvScal = 0 si OK, <0 si erreur
    int_type IRecvScal(u_int_type prov,u_int_type msgtag,
		       u_int_type * adr,u_int_type longueur,
		       message_status_t *req = NULL, 
		       comm_id_t *com = NULL);

    // Communication point_type a point
    // Reception non bloquante d'un tableau de type entier
    // IN : u_int_type  prov       = numero de rang du processus qui a envoye le message
    // IN : u_int_type   msgtag    = etiquette du message
    // IN : int_type     *adr      = adresse du buffer qui va recevoir les donnees contigues
    // IN : u_int_type   longueur  = nombre d'elements a recevoir et a stocker dans le buffer
    // OUT : int_type  RecvScal = 0 si OK, <0 si erreur
    int_type IRecvScal(u_int_type prov,u_int_type msgtag,
		       int_type * adr,u_int_type longueur,
		       message_status_t *req = NULL, 
		       comm_id_t *com = NULL);

    // Communication point_type a point
    // Reception non bloquante d'un tableau de type reel
    // IN : u_int_type  prov       = numero de rang du processus qui a envoye le message
    // IN : u_int_type   msgtag    = etiquette du message
    // IN : float_type   *adr      = adresse du buffer qui va recevoir les donnees contigues
    // IN : u_int_type   longueur  = nombre d'elements a recevoir et a stocker dans le buffer
    // OUT : int_type  RecvScal = 0 si OK, <0 si erreur
    int_type IRecvScal(u_int_type prov,u_int_type msgtag,
		       float_type * adr,u_int_type longueur,
		       message_status_t *req = NULL, 
		       comm_id_t *com = NULL);

    int_type IRecvScal(u_int_type prov,u_int_type msgtag,
		       short * adr,u_int_type longueur,
		       message_status_t *req = NULL, 
		       comm_id_t *com = NULL);

    int_type IRecvScal(u_int_type prov,u_int_type msgtag,
		       char *adr,u_int_type longueur,
		       message_status_t *req = NULL, 
		       comm_id_t *com = NULL) {
      fprintf(stderr,"Not implemented");abort();return 0;
    }

    int_type IRecvScal(u_int_type prov,u_int_type msgtag,
		       void *adr,u_int_type longueur,
		       message_status_t *req = NULL, 
		       comm_id_t *com = NULL);


    /*
      Envoi/réception de paquet
    */
    // avec les memes indices
    virtual int_type OpenPack( int_type nb, 
			       u_int_type *bind, u_int_type *eind,
			       message_status_t *req ) ;

    // avec des indices differents
    virtual int_type OpenPack( message_status_t *req );

    virtual int_type AddPack(short* adr, message_status_t *req) ;
    virtual int_type AddPack(u_int_type *adr, message_status_t *req) ;
    virtual int_type AddPack(int_type *adr, message_status_t *req) ;
    virtual int_type AddPack(float_type *adr, message_status_t *req) ;
    virtual int_type AddPack(bool *adr, message_status_t *req) ;
    virtual int_type AddPack(long *adr, message_status_t *req) ;

    /* les indices sont definis par un tableau
     */
    virtual int_type AddPack(short* adr, int_type nb_displs, int_type_p displs, 
			     message_status_t *req) ;
    virtual int_type AddPack(long* adr, int_type nb_displs, int_type_p displs, 
			     message_status_t *req) ;


    virtual int_type AddPack(u_int_type *adr, int_type nb_displs, int_type_p displs, 
			     message_status_t *req) ;

    virtual int_type AddPack(int_type *adr, int_type nb_displs, int_type_p displs, 
			     message_status_t *req) ;

    virtual int_type AddPack(float_type *adr, int_type nb_displs, int_type_p displs, 
			     message_status_t *req) ;

    virtual int_type AddPack(bool *adr, int_type nb_displs, int_type_p displs, 
			     message_status_t *req) ;

    /* les indices sont definis par un ensemble de blocs contigus
     */
    virtual int_type AddPack(short* adr, 
			     int_type nb, u_int_type *bind, u_int_type *eind,
			     message_status_t *req);

    virtual int_type AddPack(long* adr, 
			     int_type nb, u_int_type *bind, u_int_type *eind,
			     message_status_t *req);

    virtual int_type AddPack(u_int_type *adr, 
			     int_type nb, u_int_type *bind, u_int_type *eind,
			     message_status_t *req);
    virtual int_type AddPack(int_type *adr, 
			     int_type nb, u_int_type *bind, u_int_type *eind,
			     message_status_t *req);
    virtual int_type AddPack(float_type *adr, 
			     int_type nb, u_int_type *bind, u_int_type *eind,
			     message_status_t *req);
    virtual int_type AddPack(bool *adr, 
			     int_type nb, u_int_type *bind, u_int_type *eind,
			     message_status_t *req);


    virtual int_type ISendPack( int_type destination, u_int_type msgtag,
				message_status_t *req, 
				comm_id_t *com = NULL ) ;

    virtual int_type IRecvPack( int_type source, u_int_type msgtag,
				message_status_t *req, 
				comm_id_t *com = NULL ) ;

    virtual int_type WaitAll(int_type nb_in = 0, message_status_t *req_in = NULL) ;

    virtual int_type Wait(struct message_status_t *req) ;

    virtual int_type WaitSome(int_type nb_in, message_status_t *req_in,
			      int_type &nb_out, int_type *indices_out) ;

    // Communication collective
    // Réduction et diffusion pour les entiers signés
    // IN  : int_type *sendbuf = adresse du buffer contenant les donnees a reduire
    // OUT : int_type *recvbuf = adresse du buffer qui va recevoir le resultat de la diffusion
    // IN  : u_int_type longueur = taille du buffer d'adresse sendbuf
    // IN  : enum MPC_operation op = opération de réduction effectuée
    // Cette méthode effectue une opération de réduction sur tous les éléments de sendbuf et de
    // tous les processus puis diffuse le résultat dans recvbuf


    int_type AllReduce(const short *const sendbuf, short *const recvbuf, u_int_type longueur,
		       enum MPC_operation op, comm_id_t *com = NULL) ;

    int_type AllReduce(const int_type *const sendbuf, int_type *const recvbuf, u_int_type longueur,
		       enum MPC_operation op, comm_id_t *com = NULL) ;

    // Communication collective
    // Réduction et diffusion pour les entiers non signés
    int_type AllReduce(const u_int_type *const sendbuf, u_int_type
		       *const recvbuf, u_int_type longueur,
		       enum MPC_operation op, comm_id_t *com = NULL) ;

    // Communication collective
    // Réduction et diffusion pour les réels
    int_type AllReduce(const float_type * const sendbuf, float_type
		       *const recvbuf, u_int_type longueur,
		       enum MPC_operation op, comm_id_t *com = NULL) ;

    int_type BroadCast( void *buf, int_type count, int_type root,
			comm_id_t *com = NULL );

    int_type GatherV( void *sendbuf, int_type sendcount,
		      void *recvbuf, int_type *recvcounts, 
		      int_type* displs ,
		      int_type root, comm_id_t *com = NULL );

    int_type ScatterV( void *sendbuf, int_type *sendcounts, 
		       int_type* displs ,
		       void *recvbuf, int_type recvcount,
		       int_type root, comm_id_t *com = NULL );

    int_type AllGatherV( void *sendbuf, int_type sendcount,
			 void *recvbuf, int_type *recvcounts, 
			 int_type* displs, 
			 comm_id_t *com = NULL);

    int_type AllToAllV( void *sendbuf, int_type *sendcounts, 
			int_type* sdispls ,
			void *recvbuf, int_type *recvcounts, 
			int_type* rdispls,
			comm_id_t *com = NULL );
};
#endif

#endif
