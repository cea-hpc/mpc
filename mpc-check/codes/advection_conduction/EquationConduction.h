#ifndef EQUATION_CONDUCTION_H
#define EQUATION_CONDUCTION_H

//#define OPT_ALLOCATION

#include <math.h>

#include <parallelpp/TaskInfo.h>

#include "maillage.h"

//#ifdef sgi
//#ifndef min
#define min( a, b ) (((a) < (b)) ? (a) : (b))
//#endif
//#ifndef max
#define max( a, b ) (((a) > (b)) ? (a) : (b))
//#endif
//#endif

class EquationConduction {

public:

    float_type tempsfinal;

    EquationConduction( float_type t ) {
	tempsfinal = t;
    }

    void Tinit( float_type x, float_type y,
		float_type &T0) {
	
	T0 = ( ( x>.7  && x<.95 ) && ( y>.33 && y<.66 ) ?
	       1. : 0.5 );
    }

    void CoefConduc( float_type x, float_type y,
		     float_type &K ) {
	
//	K = ( ( x>.5  && x<1. ) ? 0.5 : 1.0 );
	K = 1;
    }

    void TermeSource( float_type x, float_type y, float_type t,
		      float_type &f ) {

//	f = sin(x)*cos(y)*cos(t);
	f = 1.-t;
    }

} ;


class EqConducDiscret : public EquationConduction, public Maillage {

protected:

    bool sortiegraph; // vaut vrai s'il faut faire des sorties graphiques
    float_type rythgraph;
    float_type tgraph;

    int_type numcycl; // numéro du cycle temps en cours
    int_type nbcyclmax;

public:

    /* grandeurs definies aux mailles
     */
    float_type *T0;
    float_type *Tnew;
    float_type *Told;
    float_type *f;
    float_type *b;  // second membre de la matrice

    /* grandeurs definies aux faces
     */
    float_type *K;

    /* pour la matrice
     */
    float_type **A;    // la matrice au format "creux"
    u_int_type **col;  // par ligne, les colonnes des elements non nuls
    u_int_type *nbcol;  // nombre d'elements non nuls par lignes
    
    float_type cfl;


    EqConducDiscret( int_type nx, int_type ny, float_type tfin, 
		     int_type nbc, ParamParallel *par ) :
	EquationConduction(tfin), Maillage(nx,ny,par), nbcyclmax(nbc) {

/* 	parallel->mpcom->User_Memory_Stats(stdout); */
	// on fait des sorties graph
	sortiegraph = false;
	if (sortiegraph) {
	    rythgraph = tfin / 10.;
	    tgraph = 0.;
	    if ( !parallel )
		system("rm -f ./sorties_serial/*");
	    else 
		if (taskinfo->Ihavetoprint())
		    system("rm -f ./sorties_mp/*");
	}

	/* allocations grandeurs sur les mailles
	 */
#ifndef  NOMPI
	svcalloc( T0,     float_type, c_nummax+1 );
	svcalloc( Tnew,   float_type, c_nummax+1 );
	svcalloc( Told,   float_type, c_nummax+1 );
	svcalloc( f,      float_type, c_nummax+1 );
	svcalloc( b,      float_type, c_nummax+1 );
	svcalloc( nbcol,  u_int_type, c_nummax+1 );
	svcalloc( col,    u_int_type*, c_nummax+1 );
	svcalloc( A,      float_type*, c_nummax+1 );
#else
	if ( parallel ){
	  T0 = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), c_nummax+1 );
	  Tnew = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), c_nummax+1 );
	  Told = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), c_nummax+1 );
	  f = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), c_nummax+1 );
	  b = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), c_nummax+1 );
	  nbcol = (u_int_type*)parallel->mpcom->MPC_calloc(sizeof(u_int_type), c_nummax+1 );
	  col = (u_int_type**)parallel->mpcom->MPC_calloc(sizeof(u_int_type*), c_nummax+1 );
	  A = (float_type**)parallel->mpcom->MPC_calloc(sizeof(float_type*), c_nummax+1 );
	}else{
	  svcalloc( T0,     float_type, c_nummax+1 );
	  svcalloc( Tnew,   float_type, c_nummax+1 );
	  svcalloc( Told,   float_type, c_nummax+1 );
	  svcalloc( f,      float_type, c_nummax+1 );
	  svcalloc( b,      float_type, c_nummax+1 );
	  svcalloc( nbcol,  u_int_type, c_nummax+1 );
	  svcalloc( col,    u_int_type*, c_nummax+1 );
	  svcalloc( A,      float_type*, c_nummax+1 );
	}
#endif
	
/* 	parallel->mpcom->User_Memory_Stats(stdout); */
	/* allocations grandeurs aux faces
	 */
#ifndef  NOMPI
	svcalloc( K, float_type, f_nummax+1 );
#else
	if ( parallel ){
	  K = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), f_nummax+1 );
	} else {
	  svcalloc( K, float_type, f_nummax+1 );
	}
#endif
	/* parallel->mpcom->User_Memory_Stats(stdout); */
	// discrétisation de T(t=0) et du terme source sur les mailles internes
	for( int_type nc = c_numminint ;
	     (u_int_type)nc <= c_nummaxint ; nc++ ) {
	    Tinit( c_coord[dir_x][nc], c_coord[dir_y][nc], T0[nc] );
	    TermeSource( c_coord[dir_x][nc], c_coord[dir_y][nc], 0., f[nc] );
	}

	// mise a jour sur les bords
	MAJCL( T0 );

	/* parallel->mpcom->User_Memory_Stats(stdout); */
	// discretisation du coef de conduction sur les faces
	for( int_type nf = f_nummin ; nf <= f_nummax ; nf++ ) 
	    CoefConduc( f_coord[dir_x][nf], f_coord[dir_y][nf], K[nf] );

	/* Le maillage est fixe,
	   on peut dimensionner la matrice
	*/
	for( u_int_type nc = c_numminint ; nc <= c_nummaxint ; nc++ ) 
	    nbcol[nc] = 1;

	for( int_type nf = f_nummin ; nf <= f_nummax ; nf++ ) {
	    /* on ne compte pas les mailles CL
	     */
	    u_int_type ncg = facube[gauche][nf];
	    u_int_type ncd = facube[droite][nf];
	    if ( c_type[ncg]==CELLINT &&
		 c_type[ncd]==CELLINT ) {
		nbcol[ ncg ]++;
		nbcol[ ncd ]++;
	    } else if ( c_type[ncg]!=CELLINT && 
			cltype[type2pos(c_type[ncg])]==clvoisin ) {
		nbcol[ ncd ]++;
	    } else if ( c_type[ncd]!=CELLINT && 
			cltype[type2pos(c_type[ncd])]==clvoisin ) {
		nbcol[ ncg ]++;
	    }
	}
	for( u_int_type nc = c_numminint ; nc <= c_nummaxint ; nc++ ) {
#ifndef  NOMPI
	    svcalloc( col[nc], u_int_type, nbcol[nc] );
	    svcalloc( A[nc],   float_type, nbcol[nc] );
#else
	    if ( parallel ){
#ifndef OPT_ALLOCATION
/* 	      col[nc] = (u_int_type*)parallel->mpcom->MPC_calloc(sizeof(u_int_type), nbcol[nc] ); */
	      A[nc] = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), nbcol[nc] );
#else
	      void* tmp;
	      tmp = parallel->mpcom->MPC_calloc(sizeof(float_type)+sizeof(u_int_type), nbcol[nc] );
	      A[nc] = (float_type*)tmp;
	      col[nc] = (u_int_type*)((float_type*)tmp + nbcol[nc]);
#endif
	    }else{
	      svcalloc( col[nc], u_int_type, nbcol[nc] );
	      svcalloc( A[nc],   float_type, nbcol[nc] );
	    }
#endif
	}
#ifndef OPT_ALLOCATION
#ifdef  NOMPI
	    if ( parallel ){
	  for( u_int_type nc = c_numminint ; nc <= c_nummaxint ; nc++ ) {
	      col[nc] = (u_int_type*)parallel->mpcom->MPC_calloc(sizeof(u_int_type), nbcol[nc] );
	    }
	}
#endif
#endif
	// on donne une valeur a la cfl
	cfl = 2.;
    }

    virtual ~EqConducDiscret() {

#ifndef  NOMPI
	svfree( T0 );
	svfree( Tnew );
	svfree( Told );
	svfree( f );
	svfree( b );

	for( u_int_type i = c_numminint ; i <= c_nummaxint ; i++ ) {
	    svfree( col[i] );
	    svfree( A[i] );	    
	}
	svfree( col );
	svfree( A );
	svfree( nbcol );

	svfree( K );
#else
    if ( parallel ){
	parallel->mpcom->MPC_free( T0 );
	parallel->mpcom->MPC_free( Tnew );
	parallel->mpcom->MPC_free( Told );
	parallel->mpcom->MPC_free( f );
	parallel->mpcom->MPC_free( b );

	for( u_int_type i = c_numminint ; i <= c_nummaxint ; i++ ) {
#ifndef OPT_ALLOCATION
	    parallel->mpcom->MPC_free( col[i] );
	    parallel->mpcom->MPC_free( A[i] );	    
#else
	    parallel->mpcom->MPC_free( A[i] );
#endif
	}
	parallel->mpcom->MPC_free( col );
	parallel->mpcom->MPC_free( A );
	parallel->mpcom->MPC_free( nbcol );

	parallel->mpcom->MPC_free( K );
    }else{
	svfree( T0 );
	svfree( Tnew );
	svfree( Told );
	svfree( f );
	svfree( b );

	for( u_int_type i = c_numminint ; i <= c_nummaxint ; i++ ) {
	    svfree( col[i] );
	    svfree( A[i] );	    
	}
	svfree( col );
	svfree( A );
	svfree( nbcol );

	svfree( K );
    }
#endif
    }

    // Calcul sur tous les cycles
    void CalculerTousLesCycles() ;

    // on calcule le pas de temps
    virtual void CalculPasDeTemps( float_type &dt ) ;

    // calcul de rho^n+1 sur les mailles internes
    virtual void CalculerUnCycle( float_type t, float_type dt );

    // construction de la matrice et du second membre pour un pas de temps dt
    virtual void ConstructionMatriceEtSecondMembre( float_type t, float_type dt );

    // affichage du systeme lineaire en cours
    virtual void AfficheSystemeLineaire();

    // Resolution du systeme lineaire par la methode du Gradient Conjugue
    virtual void ResolutionParGradientConjugue();

    // Produit matrice-vecteur : A*x = y ou x et y sont dimensionnees au maillage
    virtual void ProduitMatriceVecteur( float_type *x, float_type *y );

    // Produit scalaire dot = x.y ou x et y sont dimensionnees au maillage
    virtual float_type ProduitScalaire( float_type *x, float_type *y );

    // Norme L2 de x ou x est dimensionnee au maillage
    virtual float_type NormeL2( float_type *x );

    // Preconditionnement diagonal : resolution de C.x = b ou C = diag(A)
    void Precond( float_type *b, float_type *x );

} ;

#endif
