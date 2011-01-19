#ifndef MAILLAGE_H
#define MAILLAGE_H

#include <utilspp/machine_types.h>
#include <utilspp/svalloc.h>

#include "paramparallel.h"

//#include <parallelpp/MPC_Scalable.h>

#define CELLINT (u_int_type)0
#define CELLCL1 (u_int_type)1
#define CELLCL2 (u_int_type)2
#define CELLCL3 (u_int_type)4
#define CELLCL4 (u_int_type)8

// retourne le bord pour des mailles de type fantome
inline u_int_type type2pos( const u_int_type ctyp ) {
    switch(ctyp) {
    case CELLCL1: return 0;
    case CELLCL2: return 1;
    case CELLCL3: return 2;
    case CELLCL4: return 3;
    }
    return (u_int_type)(-1);
}

// retourne le type d'une maille fantome correspondant a un bord
inline u_int_type pos2type( const u_int_type pos ) {
    return 1 << pos;
}

class Maillage {

protected:

    int_type nbmailavant[DIM];
    float_type coordorigine[DIM]; // coordonnées de l'origine
    float_type dimension[DIM]; // taille du domaine de calcul

public:

    int_type nbmail[DIM]; // le nb de mailles internes dans chaque direction

    float_type *c_coord[DIM]; //  coordonnées du centre des mailles
    float_type *c_delta[DIM]; //  tailles des mailles dans chaque direction 
    float_type  c_d[DIM]; // DeltaX et DeltaY dans le cas ou maillage a pas constant

    u_int_type c_numminint; // numéro min des mailles internes
    u_int_type c_nummaxint; // numéro max des mailles internes
    u_int_type c_nummincl[DIM*NBCOT]; // numéro min des mailles CL
    u_int_type c_nummaxcl[DIM*NBCOT]; // numéro max des mailles CL
    u_int_type c_nummax;    // numéro max de toutes les mailles (mailles CL comprises)
    u_int_type c_nbblkrec[DIM*NBCOT];
    u_int_type *c_debblkrec[DIM*NBCOT];
    u_int_type *c_finblkrec[DIM*NBCOT];    
    u_int_type *c_type;

    float_type *f_coord[DIM]; // coordonnées du centre des faces  
    int_type f_nummin; // numéro min des faces
    int_type f_numchg; // numéro de changement de direction
    int_type f_nummax; // numéro max des faces

    int_type *caface[DIM][NBCOT]; // passage des numéros de mailles aux numéros de faces
    int_type *facube[NBCOT]; // passage des numéros de faces aux numéros de mailles

    enum cl cltype[DIM*NBCOT];  // nature des CL sur les bords


    // Parallélisme 
    ParamParallel *parallel;


    // constructeur
    // int_type nx = nb de mailles internes dans la direction x
    // int_type ny = nb de mailles internes dans la direction y
    // ParamParallel *parallel = instance du parallelisme Message Passing
    //
    Maillage( int_type nx, int_type ny, ParamParallel *par = NULL ) {

	float_type delta[DIM];

	parallel = par;

	// on affecte les dimensions du domaine
	nbmail[dir_x] = nx;
	nbmail[dir_y] = ny;
	dimension[dir_x] = dimension[dir_y] = 1.;

	if ( parallel ) {
	    int_type q, r, d;

	    for( d = 0 ; d < DIM ; d++ ) {

		delta[d] = dimension[d] / float_type(nbmail[d]);
		
		q = nbmail[d] / parallel->NbSsDom[d];
		r = nbmail[d] % parallel->NbSsDom[d];

		nbmail[d] = q;
		if ( parallel->II[d] < r )
		    nbmail[d]++;

		dimension[d] = nbmail[d]*delta[d];

		nbmailavant[d] = 0;
		for( int_type I = 0 ; I < parallel->II[d] ; I++ ) {
		    nbmailavant[d] += q;
		    if ( I < r ) 
			nbmailavant[d]++;
		}
	    }
	}

	// on suppose que la numérotation des mailles commence à 1
	c_numminint = 1;
	c_nummaxint = nbmail[dir_x]*nbmail[dir_y];

	c_nummincl[0] = c_nummaxint + 1;
	c_nummaxcl[0] = c_nummincl[0] + nbmail[dir_y] - 1;
	c_nummincl[1] = c_nummaxcl[0] + 1;
	c_nummaxcl[1] = c_nummincl[1] + nbmail[dir_y] - 1;
	c_nummincl[2] = c_nummaxcl[1] + 1;
	c_nummaxcl[2] = c_nummincl[2] + nbmail[dir_x] - 1;
	c_nummincl[3] = c_nummaxcl[2] + 1;
	c_nummaxcl[3] = c_nummincl[3] + nbmail[dir_x] - 1;

	c_nummax    = c_nummaxint + 2*(nbmail[dir_x]+nbmail[dir_y]);

	f_nummin    = 1;
	f_numchg    = nbmail[dir_y]*(nbmail[dir_x]+1);
	f_nummax    = f_numchg + nbmail[dir_x]*(nbmail[dir_y]+1);

/* #ifdef  NOMPI */
/* 	if ( parallel ){ */
/* 	  parallel->mpcom->Init_User_Memory(c_nummax+10,DIM+NBCOT*2+NBCOT*DIM+1+8+f_nummax*c_nummaxint*2); */
/* 	} */
/* #endif */
	

	/* construction coordonnées des mailles
	 */
	ConstructionCoordonneesMailles();

	/* construction coordonnées des faces
	 */
	ConstructionCoordonneesFaces();

	/* construction de la connectivité
	 */
	ConstructionConnectivite();

	/* construction du recouvrement
	 */
	ConstructionRecouvrement();

	// on définit les conditions aux limites
	for( int_type pos = 0 ; pos < DIM*NBCOT ; pos++ ) {

/*
  if ( pos < NBCOT ) {
  cltype[pos] = clperiodique;
  } else {
*/
	    cltype[pos] = clflux;
	    //	    }
	    if ( parallel && (parallel->voisin_pos[pos] >= 0) ) {
		cltype[pos] = clvoisin;
	    }
	}
	// dans le cas de la décomposition de domaine et des CL périodiques,
	// on doit modifier la connectivité entre sous-domaines
/*
  if ( parallel ) {
  
  for( int_type pos = 0 ; pos < NBCOT ; pos++ ) {
  
  if ( cltype[pos] == clperiodique ) {
  parallel->CorrectionConnectivite(pos);
  cltype[pos] = clvoisin;
  }
  }
  }
*/
	
    } // fin constructeur


    // destructeur
    // 
    ~Maillage() {

#ifndef  NOMPI
	for( int_type d = 0 ; d < DIM ; d++ ) {
	    svfree( c_coord[d] );
	    svfree( c_delta[d] );
	}
	for( int_type c = 0 ; c < NBCOT ; c++ ) {

	    for( int_type d = 0 ; d < DIM ; d++ ) {
		svfree( caface[d][c] );
	    } // fin d
	    svfree( facube[c] );
	    svfree( f_coord[c] );
	} // fin c
	for( int_type pos = 0 ; pos < DIM*NBCOT ; pos++ ) {
	    svfree( c_debblkrec[pos] );
	    svfree( c_finblkrec[pos] );
	}

	svfree( c_type );

#else
    if ( parallel ){
	for( int_type d = 0 ; d < DIM ; d++ ) {
	    parallel->mpcom->MPC_free( c_coord[d] );
	    parallel->mpcom->MPC_free( c_delta[d] );
	}
	for( int_type c = 0 ; c < NBCOT ; c++ ) {

	    for( int_type d = 0 ; d < DIM ; d++ ) {
		parallel->mpcom->MPC_free( caface[d][c] );
	    } // fin d
	    parallel->mpcom->MPC_free( facube[c] );
	    parallel->mpcom->MPC_free( f_coord[c] );
	} // fin c
	for( int_type pos = 0 ; pos < DIM*NBCOT ; pos++ ) {
	    parallel->mpcom->MPC_free( c_debblkrec[pos] );
	    parallel->mpcom->MPC_free( c_finblkrec[pos] );
	}

	parallel->mpcom->MPC_free( c_type );

    }else{
	for( int_type d = 0 ; d < DIM ; d++ ) {
	    svfree( c_coord[d] );
	    svfree( c_delta[d] );
	}
	for( int_type c = 0 ; c < NBCOT ; c++ ) {

	    for( int_type d = 0 ; d < DIM ; d++ ) {
		svfree( caface[d][c] );
	    } // fin d
	    svfree( facube[c] );
	    svfree( f_coord[c] );
	} // fin c
	for( int_type pos = 0 ; pos < DIM*NBCOT ; pos++ ) {
	    svfree( c_debblkrec[pos] );
	    svfree( c_finblkrec[pos] );
	}

	svfree( c_type );

    }
#endif
    } // fin destructeur
    
    void ConstructionCoordonneesMailles() ;
    void ConstructionCoordonneesFaces() ;
    void ConstructionConnectivite() ;
    void ConstructionRecouvrement() ;

    // ecrit un fichier de sortie graph pour var(t)
    //
    void EcrireSortie( float_type *var, float_type t, 
		       int_type numc, float_type &tg, float_type rg, float_type tfin,
		       const char *nomvar) ;

    // symetrise la variable r sur les mailles fantômes
    virtual void MAJCL( float_type *r ) ;

    // effectue rout[] <- rin[]
    void Affectation( float_type *rout, float_type *rin ) ;

    // effectue rout[] <- val
    void Affectation( float_type *rout, float_type val ) ;

    // Envoie le recouvrement de la position pos de la variable r 
    //
    virtual void IEnvoyerRecouvrement( float_type *r, int_type pos ) ;

    // Reçoit dans les mailles fantômes de r le recouvrement du voisin pos
    //
    virtual void IRecevoirFantome( float_type *r, int_type pos ) ;

    // Attente des fins de requetes
    //
    virtual void WaitEndComm();

} ;

#endif
