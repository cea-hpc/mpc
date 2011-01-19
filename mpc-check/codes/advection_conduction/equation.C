#ifndef EQUATION_C
#define EQUATION_C

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <TaskInfo.h>

#include "equation.h"
#include <MPC.h>



// Calcul sur tous les cycles
void EqTranspDiscret::CalculerTousLesCycles() {
//	fprintf(stderr,"CalculerTousLesCycles\n");
    
    float_type t, dt; // le temps en cours et le pas de temps

    // rho^0 et vit sont déjà définis

    // on initialise rho^n sur toutes les mailles (y compris CL)
//	fprintf(stderr,"Affectation\n");
    Affectation( rold, r0 );

    // initialisation du pas de temps
    t = 0.;
    numcycl = 0;
//	fprintf(stderr,"if\n");
    if ( sortiegraph )
	EcrireSortie( r0, t, numcycl, tgraph, rythgraph, tempsfinal, "rho");


//	fprintf(stderr,"Boucle\n");
    do {
	if ( taskinfo->Ihavetoprint() )	  
//	  fprintf(stdout,"t = %g\n",t);
	  fprintf(stdout,"cycl = %d, t = %g\n",numcycl, t);

	// on incrémente le numéro du cycle
	numcycl++;

// 	if ( taskinfo->Ihavetoprint() )	  
// 	fprintf(stdout,"CalculPasDeTemps\n");
	// on calcule le pas de temps
	CalculPasDeTemps( dt );


// 	if ( taskinfo->Ihavetoprint() )	  
// 	fprintf(stdout,"CalculerUnCycle\n");
	// calcul de rho^n+1 sur les mailles internes
	CalculerUnCycle( t, dt );

	// on écrit une sortie graphique
	if ( sortiegraph )
	    EcrireSortie( rnew, t+dt, numcycl, tgraph, rythgraph, tempsfinal, "rho");

// 	if ( taskinfo->Ihavetoprint() )	  
// 	fprintf(stdout,"maj sur les bords\n");

	// maj sur les bords
	MAJCL( rnew );


	// on passe à l'itération suivante
	Affectation( rold, rnew );
	t += dt;
	
    } while( t < tempsfinal && numcycl < nbcyclmax);

}

// on calcule le pas de temps
void EqTranspDiscret::CalculPasDeTemps( float_type &dt ) {
//	fprintf(stderr,"CalculPasDeTemps\n");

    int_type d,/* c,*/ nc, nfg, nfd;
    float_type tinv, *tmp;

    // allocation et initialisation de tmp
#ifndef  NOMPI
    svcalloc( tmp, float_type, c_nummaxint+1 );
#else
    if ( parallel ){
      tmp = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), c_nummaxint+1 );
    }else{
      svcalloc( tmp, float_type, c_nummaxint+1 );
    }
#endif

// #ifdef  NOMPI
//     if ( parallel )
//       parallel->mpcom->MPC_prefetch_datas();
// #endif

    // on parcourt toutes les mailles internes
    for( d = 0 ; d < DIM ; d++ ) {
	for( nc = c_numminint ; (u_int_type)nc <= c_nummaxint ; nc++ ) {

	    nfg = caface[d][gauche][nc];
	    nfd = caface[d][droite][nc];

	    tinv = max( fabs(vit[nfg]), fabs(vit[nfd]) ) / c_delta[d][nc];
	    tmp[nc] += tinv;
	}
    }

    // on prend le max sur toutes les mailles
    tinv = 0;
    for( nc = c_numminint ; (u_int_type)nc <= c_nummaxint ; nc++ ) {
	tinv = max( tinv, tmp[nc] );
    }    

    // on peut calculer le pas de temps
    dt = cfl / tinv;
    dt = min( dt, tempsfinal );

    // dans le cas de la décomposition de domaine, prend le plus petit 
    // pas de temps
    MinPasDeTemps( dt );

    // liberation
#ifndef  NOMPI
    svfree( tmp );
#else
    if ( parallel ){
      parallel->mpcom->MPC_free( tmp );
    }else{
      svfree( tmp );
    }
#endif
}

// calcul de l'ensemble des flux
void EqTranspDiscret::CalculFlux() {
//	fprintf(stderr,"CalculFlux\n");
    float_type v, rg, rd;
    int_type nf, ncg, ncd;

    // on parcourt toutes les faces
    for( nf = f_nummin ; nf <= f_nummax ; nf++ ) {

	// récupération des mailles
	ncg = facube[gauche][nf];
	ncd = facube[droite][nf];

	v  = vit[nf];
	rg = rold[ncg];
	rd = rold[ncd];

	flux[nf] = UpWind(v, rg, rd);
    }
}


// calcul de rho^n+1 sur les mailles internes
void EqTranspDiscret::CalculerUnCycle( float_type t, float_type dt ) {
//	fprintf(stderr,"CalculerUnCycle\n");

    int_type nc, d, /*c,*/ nfg, nfd;

// #ifdef  NOMPI
//     if ( parallel )
//       parallel->mpcom->MPC_prefetch_datas();
// #endif

    // on calcule le flux UpWind
    CalculFlux();

#ifdef CACHE_EFFECTS_DISCRIM

    for( d = 0 ; d < DIM ; d++ ) {

	for( nc = c_numminint ; (u_int_type)nc <= c_nummaxint ; nc++ ) {

	    nfg = caface[d][gauche][nc];
	    nfd = caface[d][droite][nc];

	    rnew[nc] += ((dt*c_delta[d][nc])*( flux[nfg] + flux[nfd] ));
	    rnew[nc] += rnew[nc]*dt;
	    rnew[nc] += rnew[nc]*( flux[nfg] + flux[nfd] );
	    rnew[nc] += rnew[nc]*(dt*c_delta[d][nc]);
	    /*
	    rnew[nc] += ((dt*c_delta[d][nc])*( flux[nfg] + flux[nfd] ));
	    rnew[nc] += rnew[nc]*dt;
	    rnew[nc] += rnew[nc]*( flux[nfg] + flux[nfd] );
	    rnew[nc] += rnew[nc]*(dt*c_delta[d][nc]);
	    rnew[nc] += ((dt*c_delta[d][nc])*( flux[nfg] + flux[nfd] ));
	    rnew[nc] += rnew[nc]*dt;
	    rnew[nc] += rnew[nc]*( flux[nfg] + flux[nfd] );
	    rnew[nc] += rnew[nc]*(dt*c_delta[d][nc]);
	    rnew[nc] += ((dt*c_delta[d][nc])*( flux[nfg] + flux[nfd] ));
	    rnew[nc] += rnew[nc]*dt;
	    rnew[nc] += rnew[nc]*( flux[nfg] + flux[nfd] );
	    rnew[nc] += rnew[nc]*(dt*c_delta[d][nc]);
	    rnew[nc] += ((dt*c_delta[d][nc])*( flux[nfg] + flux[nfd] ));
	    rnew[nc] += rnew[nc]*dt;
	    rnew[nc] += rnew[nc]*( flux[nfg] + flux[nfd] );
	    rnew[nc] += rnew[nc]*(dt*c_delta[d][nc]);
	    */
	}
    }

#endif

    Affectation( rnew, rold );

    // on calcule r^n+1 sur toutes les mailles internes
    for( d = 0 ; d < DIM ; d++ ) {

	for( nc = c_numminint ; (u_int_type)nc <= c_nummaxint ; nc++ ) {

	    nfg = caface[d][gauche][nc];
	    nfd = caface[d][droite][nc];

	    rnew[nc] += (dt/c_delta[d][nc])*( flux[nfg] - flux[nfd] );
	}
    }



    /* On construit un déséquilibrage de charge
       - on considère un rectangle de dimension 0.025x1 qui se translate
       selon la direction x à une vitesse de 1
       - si à l'instant t une maille se trouve dans cette zone, on
       ajoute un surcoût de travail
    */
    /*Desequilibrage*/
#ifdef UNBALANCED_WORKLOAD
    for( nc = c_numminint ; nc <= c_nummaxint ; nc++ ) {

      float_type X = c_coord[dir_x][nc];
      float_type bidon[200];
  
      if ( (X > t) && (X < t+0.1) ) {
  
	for( int_type j = 0 ; j < 2 ; j++ ) {
	  for( int_type i = 0 ; i < 50 ; i++) {
	    bidon[i] = 1. / sqrt( 1. + float_type(i+j) );
	    bidon[i] *= 2.;
	  }
	}
      }
  
    }
#endif
   /*Fin desequilibre*/

    /* On construit un déséquilibrage de charge
       - on considère une zone délimitée par 2 rayons (rint < rext) de telle
       manière que le volume (i.e. surface) de cette zone soit constante au 
       cours du temps et telle que rext decroisse linéairement avec le temps.
       - si à l'instant t une maille se trouve dans cette zone, on
       ajoute un surcoût de travail
     */
/*
    float_type rext = 1. - t;  // rayon exterieur
    float_type rint = pow( pow(rext,3.) - 1./64., 1./3. ); // rayon intérieur

    for( nc = c_numminint ; nc <= c_nummaxint ; nc++ ) {

	float_type X = c_coord[dir_x][nc];
	float_type Y = c_coord[dir_y][nc];
	float_type rayon = sqrt( X*X + Y*Y );
	float_type bidon[250];

	if ( (rayon > rext) && (rayon < rint) ) {
	    
	    for( int_type j = 0 ; j < 10 ; j++ ) {
		for( int_type i = 0 ; i < 250 ; i++) {
		    bidon[i] = 1. / sqrt( 1. + float_type(i+j) );
		    bidon[i] *= 2.;
		}
	    }
	}

    }
*/

} 


// dans le cas de la décomposition de domaine, prend le plus petit 
// pas de temps
//
void EqTranspDiscret::MinPasDeTemps( float_type &dt ) {
//	fprintf(stderr,"MinPasDeTemps\n");

    if ( parallel ) {
	float_type tmp = dt;

	parallel->mpcom->AllReduce( &tmp, &dt, 1, MPC::OMIN);
    }

}

#endif
