// ----------------------------------------------------------------------------------
// /HIST/ David Dureau - 03/12/99 - Creation
// /HIST/ David Dureau - 20/03/00:
// /HIST/  ajout des membres de communication pour l'enum amr_var_categorie
//
// /PURP/ Definition de la classe abstraite MPC (Message Passing Class).
// /PURP/
// /PURP/ Cette classe doit encapsuler les environnements de programmation
// /PURP/ par message PVM et MPI.
// /PURP/ Elle definit des operations simples comme:
// /PURP/   - connaitre le rang du processus parmi tous les processus existant;
// /PURP/   - effectuer des communications point a point:
// /PURP/        * envoi et reception de donnees de types scalaires simples;
// /PURP/        * envoi et reception de paquets;
// /PURP/   - barriere;
// /PURP/
// /PURP/ Des operations de communications collectives sont prevues
// ----------------------------------------------------------------------------------


#ifndef PARALLELPP_MPC_H
#define PARALLELPP_MPC_H

#ifndef __IDENT_P
#define __IDENT_P 0
#endif

#if __IDENT_P
#pragma ident "(C) CEA-HERA 2D $Id: MPC.h,v 1.1.1.1 2006/02/02 09:42:46 perache Exp $"
#endif

#ifndef __INTEL_COMPILER
#ifdef __GNUG__
#pragma interface
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include "utilspp/mach_types.h"
#include "time.h"



#define RET_NO  \
          do { \
	    fprintf(stderr,"%s line %d Not implemented !\n",__FILE__,__LINE__);	\
             abort(); \
             return -1; \
          } while(0)

#define NOTHING  \
          do { \
             fprintf(stderr,"%s line %d Not implemented !\n",__FILE__,__LINE__); \
             abort(); \
          } while(0)

typedef int_type DataFormatDescriptor_t;
#define NILDESCR   DataFormatDescriptor_t(-1)

struct main_arg;

class MPC {

protected:
  
    // Nombre de processus actifs
    u_int_type nproc_;

    // pointeur sur le nombre d'arguments du run, executable compris
    int *pargc_;

    // pointeur sur la liste des arguments du run, executable compris
    char ***pargv_;


public:

    typedef struct comm_id {
	void* internal_comm_id;
    } comm_id_t;

    struct message_status_t {
      int status;
      void* message;
      void* com;
      
      const message_status_t& operator=(const message_status_t& from) {
	status = from.status;
	message = from.message;
	com = from.com;
	return *this;
      }
    };
  
    comm_id_t COMM_WORLD;

    // Definition du mode d'encodage du paquet
    enum MPC_encoding {
	// Pour machines heterogenes
	DATADEFAULT,  // formatage pour portabilite, message duplique en memoire
	// Pour machines homogenes
	DATARAW,      // pas de formatage, message duplique en memoire
	DATAINPLACE   // pas de formatage, message non duplique 
    };

    // Définition des opérations possibles pour les réductions
    enum MPC_operation {
	OMIN, // recherche le min
	OMAX, // recherche le max
	OSUM // effectue la somme
    };

    // Constructeur 
    // IN : pargc = pointeur sur le nombre d'arguments du run, executable compris
    // IN : pargv = pointeur sur la liste des arguments du run, executable compris
    // IN : nbproc = le nombre de processus a activer
    MPC(int *pargc,char ***pargv,u_int_type nbproc) {
	pargc_ = pargc;
	pargv_ = pargv;
	nproc_ = nbproc;
    }

    // Destructeur
    virtual ~MPC() {}

    // debute les taches entry en mode parallele avec l'argument arg
    virtual void StartTasks( void (*entry)(struct main_arg*), void *arg) { NOTHING; }

    // Retourne le nombre de processus actifs
    virtual u_int_type Nproc(comm_id_t *com = NULL) const {
      NOTHING; return 0;
    }

    virtual void *MPC_calloc(size_t nmemb, size_t size) {  NOTHING; return NULL;}

    virtual void *MPC_malloc(size_t size) {  NOTHING; return NULL;}

    virtual void MPC_free(void *ptr) {  NOTHING;}

    virtual void *MPC_realloc(void *ptr, size_t size) {  NOTHING; return NULL;}

    virtual void ComCreate(comm_id_t *comold, int* liste, int n, 
			   comm_id_t *comnew){ NOTHING; }

    virtual void ComSplit(comm_id_t *comold, int color, int key, 
			  comm_id_t *comnew){ NOTHING; }

    virtual void ComDelete(comm_id_t *com){ NOTHING; }

    virtual int_type Rank(comm_id_t *com = NULL) { RET_NO; }

    virtual int_type Barrier(comm_id_t *com = NULL) { RET_NO; }

    virtual int_type Exit() { RET_NO; }

    virtual void Abort( int_type errcode ) { NOTHING; }

    virtual int_type ISendScal(u_int_type dest,u_int_type msgtag,
			       u_int_type * adr,u_int_type longueur,
			       message_status_t *req = NULL, 
			       comm_id_t *com = NULL) { RET_NO; } 


    virtual int_type ISendScal(u_int_type dest,u_int_type msgtag,
			       int_type * adr,u_int_type longueur,
			       message_status_t *req = NULL,
			       comm_id_t *com = NULL) { RET_NO; }

    virtual int_type ISendScal(u_int_type dest,u_int_type msgtag,
			       float_type * adr,u_int_type longueur,
			       message_status_t *req = NULL,
			       comm_id_t *com = NULL) { RET_NO; }


    virtual int_type ISendScal(u_int_type dest,u_int_type msgtag,
			       char *adr,u_int_type longueur,
			       message_status_t *req = NULL,
			       comm_id_t *com = NULL) { RET_NO; }

    virtual int_type ISendScal(u_int_type dest,u_int_type msgtag,
			       void *adr,u_int_type longueur,
			       message_status_t *req = NULL,
			       comm_id_t *com = NULL) { RET_NO; }



    virtual int_type IRecvScal(u_int_type prov,u_int_type msgtag,
			       u_int_type * adr,u_int_type longueur,
			       message_status_t *req = NULL,
			       comm_id_t *com = NULL) { RET_NO; }

    virtual int_type IRecvScal(u_int_type prov,u_int_type msgtag,
			       int_type * adr,u_int_type longueur,
			       message_status_t *req = NULL,
			       comm_id_t *com = NULL) { RET_NO; }

    virtual int_type IRecvScal(u_int_type prov,u_int_type msgtag,
			       float_type * adr,u_int_type longueur,
			       message_status_t *req = NULL,
			       comm_id_t *com = NULL) { RET_NO; }

    virtual int_type IRecvScal(u_int_type prov,u_int_type msgtag,
			       char *adr,u_int_type longueur,
			       message_status_t *req = NULL,
			       comm_id_t *com = NULL) { RET_NO; }

    virtual int_type IRecvScal(u_int_type prov,u_int_type msgtag,
			       void *adr,u_int_type longueur,
			       message_status_t *req = NULL,
			       comm_id_t *com = NULL) { RET_NO; }

    virtual int_type WaitAll(int_type nb_in = 0, message_status_t *req_in = NULL) { RET_NO; }

    virtual int_type Wait(struct message_status_t *req) { RET_NO; }

    virtual int_type WaitSome(int_type nb_in, message_status_t *req_in,
			      int_type &nb_out, int_type *indices_out) { RET_NO; }


    /*
      Envoi/réception de paquet
    */
    // avec les memes indices
    virtual int_type OpenPack( int_type nb, u_int_type *bind,
			       u_int_type *eind,
			       message_status_t *req ) { RET_NO; }

    // avec des indices differents
    virtual int_type OpenPack( message_status_t *req ) { RET_NO; }

    virtual int_type AddPack(short* adr, message_status_t *req) { RET_NO; }
    virtual int_type AddPack(long* adr, message_status_t *req) { RET_NO; }
    virtual int_type AddPack(u_int_type *adr, message_status_t *req) { RET_NO; }
    virtual int_type AddPack(int_type *adr, message_status_t *req) { RET_NO; }
    virtual int_type AddPack(float_type *adr, message_status_t *req) { RET_NO; }
    virtual int_type AddPack(bool *adr, message_status_t *req) { RET_NO; }
    /* les indices sont definis par un tableau
     */
    virtual int_type AddPack(short* adr, 
			     int_type nb_displs, int_type_p displs,
			     message_status_t *req) { RET_NO; }

    virtual int_type AddPack(long* adr, 
			     int_type nb_displs, int_type_p displs,
			     message_status_t *req) { RET_NO; }

    virtual int_type AddPack(u_int_type *adr, 
			     int_type nb_displs, int_type_p displs,
			     message_status_t *req) { RET_NO; }
    virtual int_type AddPack(int_type *adr, 
			     int_type nb_displs, int_type_p displs,
			     message_status_t *req) { RET_NO; }
    virtual int_type AddPack(float_type *adr, 
			     int_type nb_displs, int_type_p displs,
			     message_status_t *req) { RET_NO; }
    virtual int_type AddPack(bool *adr, 
			     int_type nb_displs, int_type_p displs,
			     message_status_t *req) { RET_NO; }


    /* les indices sont definis par un ensemble de blocs contigus
     */
    virtual int_type AddPack(short* adr, 
			     int_type nb, u_int_type *bind, u_int_type *eind,
			     message_status_t *req) { RET_NO; }

    virtual int_type AddPack(long* adr, 
			     int_type nb, u_int_type *bind, u_int_type *eind,
			     message_status_t *req) { RET_NO; }

    virtual int_type AddPack(u_int_type *adr, 
			     int_type nb, u_int_type *bind, u_int_type *eind,
			     message_status_t *req) { RET_NO; }
    virtual int_type AddPack(int_type *adr, 
			     int_type nb, u_int_type *bind, u_int_type *eind,
			     message_status_t *req) { RET_NO; }
    virtual int_type AddPack(float_type *adr, 
			     int_type nb, u_int_type *bind, u_int_type *eind,
			     message_status_t *req) { RET_NO; }
    virtual int_type AddPack(bool *adr, 
			     int_type nb, u_int_type *bind, u_int_type *eind,
			     message_status_t *req) { RET_NO; }



    virtual int_type ISendPack( int_type destination, u_int_type msgtag, 
				message_status_t *req,
				comm_id_t *com = NULL ) { RET_NO; }

    virtual int_type IRecvPack( int_type source, u_int_type msgtag,
				message_status_t *req,
				comm_id_t *com = NULL ) { RET_NO; }


    virtual int_type AllReduce(c_int_type_p sendbuf, int_type_c_p recvbuf, u_int_type longueur,
			       enum MPC_operation op,
			       comm_id_t *com = NULL) { RET_NO; }
    
    virtual int_type AllReduce(c_u_int_type_p sendbuf, u_int_type_c_p recvbuf, u_int_type longueur,
			       enum MPC_operation op,
			       comm_id_t *com = NULL) { RET_NO; }

    virtual int_type AllReduce(c_float_type_p sendbuf, float_type_c_p recvbuf, u_int_type longueur,
			       enum MPC_operation op,
			       comm_id_t *com = NULL) { RET_NO; }

    virtual int_type AllReduce(c_short_p sendbuf, short_c_p recvbuf, u_int_type longueur,
			       enum MPC_operation op,
			       comm_id_t *com = NULL) { RET_NO; }


    virtual int_type BroadCast( void *buf, int_type count, int_type root,
				comm_id_t *com = NULL ) { RET_NO; }

    virtual int_type GatherV( void *sendbuf, int_type sendcount,
			      void *recvbuf, int_type *recvcounts, 
			      int_type* displs ,
			      int_type root, comm_id_t *com = NULL ) { RET_NO; }

    virtual int_type ScatterV( void *sendbuf, int_type *sendcounts, 
			       int_type* displs ,
			       void *recvbuf, int_type recvcount,
			       int_type root, comm_id_t *com = NULL ) { RET_NO; }

    virtual int_type AllGatherV( void *sendbuf, int_type sendcount,
				 void *recvbuf, int_type *recvcounts, 
				 int_type* displs, 
				 comm_id_t *com = NULL) { RET_NO; }

    virtual int_type AllToAllV( void *sendbuf, int_type *sendcounts, 
				int_type* sdispls ,
				void *recvbuf, int_type *recvcounts, 
				int_type* rdispls,
				comm_id_t *com = NULL ) { RET_NO; }

};

struct main_arg {

    int_type argc; // nb d'arguments dans la ligne de commande
    char **argv;   // liste des arguments

    void *taskarg; // les arguments de la tache

    MPC *mpcom;   // protocole de communication entre taches
};

extern int _MYRANK_;
extern int _MYPROC_;
#endif
