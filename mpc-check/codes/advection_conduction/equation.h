#ifndef EQUATION_H
#define EQUATION_H

#include <math.h>
#include "maillage.h"

//#ifdef sgi
//#ifndef min
#define min( a, b ) (((a) < (b)) ? (a) : (b))
//#endif
//#ifndef max
#define max( a, b ) (((a) > (b)) ? (a) : (b))
//#endif
//#endif

class EquationTransport {

public:

    float_type tempsfinal; // temps à atteindre

    // constructeur
    EquationTransport( float_type t ) {
	tempsfinal = t;
    }

    // la variable rho définie à l'instant 0 sur [0,1]x[0,1]
    void Rho0( float_type x, float_type y,
	       float_type &r0) {

	r0 = ( ( x>.7  && x<.95 ) && ( y>.33 && y<.66 ) ?
	       1. : 0. );
    }

    // le champ de vitesse défini sur [0,1]x[0,1]
    void Vitesse( float_type x, float_type y,
		  float_type *vit) {
	
	vit[0] = -y / sqrt(x*x+y*y);
	vit[1] =  x / sqrt(x*x+y*y);
//	vit[0] = -.5;
//	vit[1] =  0.;
    }

} ;


class EqTranspDiscret : public EquationTransport, public Maillage {

protected:

    bool sortiegraph; // vaut vrai s'il faut faire des sorties graphiques
    float_type rythgraph;
    float_type tgraph;

    int_type numcycl; // numéro du cycle temps en cours
    int_type nbcyclmax;

public:

    float_type *r0;   // rho(t=0) défini sur les mailles
    float_type *rnew; // rho(t=t^n+1) défini sur les mailles
    float_type *rold; // rho(t=t^n) défini sur les mailles

    float_type *vit;   // vit.n définie sur les faces
    float_type maxvit; // max de la norme de la vitesse

    float_type *flux; // flux numérique défini sur les faces

    float_type cfl; // la valeur de la CFL

    // constructeur
    EqTranspDiscret( int_type nx, int_type ny, float_type tfin, 
		     int_type nbc, ParamParallel *par ) :
	EquationTransport(tfin), Maillage(nx,ny,par), nbcyclmax(nbc) {

	float_type v[DIM];
	int_type nf;

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

#ifndef  NOMPI
	svcalloc( r0,   float_type, c_nummax+1 );
	svcalloc( rnew, float_type, c_nummax+1 );
	svcalloc( rold, float_type, c_nummax+1 );

	svcalloc( vit, float_type, f_nummax+1 );

	svcalloc( flux, float_type, f_nummax+1 );
#else
	if ( parallel ){
	  r0 = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), c_nummax+1 );
	  rnew = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), c_nummax+1 );
	  rold = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), c_nummax+1 );
	  vit = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), f_nummax+1 );
	  flux = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), f_nummax+1 );
	}else{
	  svcalloc( r0,   float_type, c_nummax+1 );
	  svcalloc( rnew, float_type, c_nummax+1 );
	  svcalloc( rold, float_type, c_nummax+1 );

	  svcalloc( vit, float_type, f_nummax+1 );

	  svcalloc( flux, float_type, f_nummax+1 );
	}
#endif

	// discrétisation de rho(t=0) sur les mailles internes
	for( int_type nc = c_numminint ;
	     (u_int_type)nc <= c_nummaxint ; nc++ ) {
	    Rho0( c_coord[dir_x][nc], c_coord[dir_y][nc], r0[nc] );
	}

	// mise a jour sur les bords
	MAJCL( r0 );

	// discrétisation de vit.n définie sur les faces
	for( nf = f_nummin ; nf <= f_numchg ; nf++ ) {
	    Vitesse( f_coord[dir_x][nf], f_coord[dir_y][nf], v );
	    vit[nf] = v[dir_x];
	}
	for( nf = f_numchg+1 ; nf <= f_nummax ; nf++ ) {
	    Vitesse( f_coord[dir_x][nf], f_coord[dir_y][nf], v );
	    vit[nf] = v[dir_y];
	}

	// on donne une valeur à la CFL
	cfl = 0.5;
    }

    // destructeur
    ~EqTranspDiscret() {

#ifndef  NOMPI
	svfree( r0 );
	svfree( rnew );
	svfree( rold );

	svfree( vit );
	svfree( flux );
#else
    if ( parallel ){
	parallel->mpcom->MPC_free( r0 );
	parallel->mpcom->MPC_free( rnew );
	parallel->mpcom->MPC_free( rold );

	parallel->mpcom->MPC_free( vit );
	parallel->mpcom->MPC_free( flux );
    }else{
	svfree( r0 );
	svfree( rnew );
	svfree( rold );

	svfree( vit );
	svfree( flux );
    }
#endif
    }

    // Calcul sur tous les cycles
    void CalculerTousLesCycles() ;

    // on calcule le pas de temps
    virtual void CalculPasDeTemps( float_type &dt );

    // calcul de rho^n+1 sur les mailles internes
    virtual void CalculerUnCycle( float_type t, float_type dt );

    // définition du flux numérique upwind
    float_type UpWind( float_type v,
		       float_type r1, float_type r2 ) {
	float_type f;

	f = max(0., v)*r1 + min(0., v)*r2;

	return f;
    }

    // calcul de l'ensemble des flux
    virtual void CalculFlux();
    
    // dans le cas de la décomposition de domaine, prend le plus petit 
    // pas de temps
    virtual void MinPasDeTemps( float_type &dt );

} ;

#endif
