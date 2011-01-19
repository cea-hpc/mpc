#ifndef EQUATION_CONDUCTION_C
#define EQUATION_CONDUCTION_C

#include "EquationConduction.h"



//#ifdef sgi
//#ifndef min
#define min( a, b ) (((a) < (b)) ? (a) : (b))
//#endif
//#ifndef max
#define max( a, b ) (((a) > (b)) ? (a) : (b))
//#endif
//#endif


//
void EqConducDiscret::CalculerTousLesCycles() {
    
    float_type t, dt; // le temps en cours et le pas de temps

    // T^0 

    // on initialise T^n sur toutes les mailles (y compris CL)
    Affectation( Told, T0 );

    // initialisation du pas de temps
    t = 0.;
    numcycl = 0;
    if ( sortiegraph )
	EcrireSortie( T0, t, numcycl, tgraph, rythgraph, tempsfinal, "temp");
    

    do {
      if ( taskinfo->Ihavetoprint() ) {
//	  fprintf(stdout,"t = %g\n",t);
	  fprintf(stdout,"cycl = %d, t = %g\n",numcycl, t);
      }

	// on incrémente le numéro du cycle
	numcycl++;

//       if ( taskinfo->Ihavetoprint() ) {
// //	  fprintf(stdout,"t = %g\n",t);
// 	  fprintf(stdout,"cycl = %d, t = %g pas de temp pas calcule\n",numcycl, t);
//       }
	// on calcule le pas de temps
	CalculPasDeTemps( dt );

//       if ( taskinfo->Ihavetoprint() ) {
// //	  fprintf(stdout,"t = %g\n",t);
// 	  fprintf(stdout,"cycl = %d, t = %g pas de temp calcule\n",numcycl, t);
//       }


	// calcul de rho^n+1 sur les mailles internes
	CalculerUnCycle( t, dt );
//       if ( taskinfo->Ihavetoprint() ) {
// //	  fprintf(stdout,"t = %g\n",t);
// 	  fprintf(stdout,"cycl = %d, t = %g cycle calcule\n",numcycl, t);
//       }

	// on écrit une sortie graphique
	if ( sortiegraph )
	    EcrireSortie( Tnew, t+dt, numcycl, tgraph, rythgraph, tempsfinal, "temp");

	// maj sur les bords
	MAJCL( Tnew );


	// on passe à l'itération suivante
	Affectation( Told, Tnew );
	t += dt;
	
    } while( t < tempsfinal && numcycl < nbcyclmax);

}

// on calcule le pas de temps
void EqConducDiscret::CalculPasDeTemps( float_type &dt ) {

    dt = cfl * min( c_d[0]*c_d[0] , c_d[1]*c_d[1] );

    if ( parallel ) {
	
	float_type dtin = dt;

// 	fprintf(stderr,"parallel->mpcom->AllReduce(&dtin, &dt, 1, MPC::OMIN)\n");
	parallel->mpcom->AllReduce(&dtin, &dt, 1, MPC::OMIN);
// 	fprintf(stderr,"parallel->mpcom->AllReduce(&dtin, &dt, 1, MPC::OMIN) fait\n");
    }
}

// calcul de T^n+1 sur les mailles internes
void EqConducDiscret::CalculerUnCycle( float_type t, float_type dt ) {

    ConstructionMatriceEtSecondMembre( t, dt );
//     fprintf(stderr,"ConstructionMatriceEtSecondMembre( t, dt )\n");

//    AfficheSystemeLineaire();

    ResolutionParGradientConjugue();

/*
  float_type *res;
  svalloc( res, float_type, c_nummax+1 );
  ProduitMatriceVecteur( Tnew, res );
  for( u_int_type nc = c_numminint ; nc <= c_nummaxint ; nc++ )
  printf("diff[%d] = %.6e\n",nc,res[nc]-b[nc]);
  svfree( res );
*/
}


void EqConducDiscret::ConstructionMatriceEtSecondMembre( float_type t, float_type dt ) {

    const float_type deltax = c_d[0];
    const float_type deltay = c_d[1];

    /* construction de la matrice et du second membre
     */
    // initialisation
    for( u_int_type i = c_numminint ; i <= c_nummaxint ; i++ ) {
	
	// diagonale (i.e. A(i,i)) en debut de liste
	col[i][0] = i;
	A[i][0] = 1.;
	nbcol[i] = 1;
    }

    // on parcourt les faces internes
    for( u_int_type d = 0 ; d < DIM ; d++ ) {

	const u_int_type fmin = ( d==0 ? f_nummin : f_numchg+1 );
	const u_int_type fmax = ( d==0 ? f_numchg : f_nummax );

	const float_type coef = dt / ( d==0 ? deltax*deltax : deltay*deltay );

	for( u_int_type nf = fmin ; nf <= fmax ; nf++ ) {
	
	    const u_int_type ncg = facube[gauche][nf];
	    const u_int_type ncd = facube[droite][nf];

	    if ( c_type[ncg]==CELLINT && 
		 c_type[ncd]==CELLINT ) {

		// contribution sur les diagonales
		A[ncg][0] += coef*K[nf];
		A[ncd][0] += coef*K[nf];

		// contribution du voisin droit
		col[ncg][ nbcol[ncg] ] = ncd;
		A[ncg][ nbcol[ncg] ] = -coef*K[nf];
		nbcol[ncg]++;

		// contribution du voisin gauche
		col[ncd][ nbcol[ncd] ] = ncg;
		A[ncd][ nbcol[ncd] ] = -coef*K[nf];
		nbcol[ncd]++;

	    } else if ( c_type[ncg]!=CELLINT &&
			cltype[type2pos(c_type[ncg])]==clvoisin ) {

		// contribution sur la diagonale de la maille voisine interieure
		A[ncd][0] += coef*K[nf];

		// contribution du voisin gauche
		col[ncd][ nbcol[ncd] ] = ncg;
		A[ncd][ nbcol[ncd] ] = -coef*K[nf];
		nbcol[ncd]++;

	    } else if ( c_type[ncd]!=CELLINT && 
			cltype[type2pos(c_type[ncd])]==clvoisin ) {

		// contribution sur la diagonale de la maille voisine interieure
		A[ncg][0] += coef*K[nf];

		// contribution du voisin droit
		col[ncg][ nbcol[ncg] ] = ncd;
		A[ncg][ nbcol[ncg] ] = -coef*K[nf];
		nbcol[ncg]++;
	    }
	}
    } // fin d

    // discrétisation du terme source sur les mailles internes
    for( int_type nc = c_numminint ; nc <= c_nummaxint ; nc++ ) 
	TermeSource( c_coord[dir_x][nc], c_coord[dir_y][nc], t+dt, f[nc] );	

    /* construction second membre
     */
    for( u_int_type nc = c_numminint ; nc <= c_nummaxint ; nc++ )
	b[nc] = dt*f[nc] + Told[nc];

    // contribution des mailles fantomes
    MAJCL( Told );
}


// affichage du systeme lineaire en cours
void EqConducDiscret::AfficheSystemeLineaire() {

    for( u_int_type i = c_numminint ; i <= c_nummaxint ; i++ ) {

	for( u_int_type k = 0 ; k < nbcol[i] ; k++ )
	    fprintf(stdout,"A(%3d,%3d) = %.3e ",i,col[i][k],A[i][k]);

	fprintf(stdout,"   f(%3d) = %.3e\n",i,b[i]);
    }
}

// Resolution du systeme lineaire par la methode du Gradient Conjugue
// A.Tnew = b
//
void EqConducDiscret::ResolutionParGradientConjugue() {

    float_type *r, *z, *p, *q, *x;
    float_type dep, dot1, dot2, dot3, alpha, beta, nor, crit;
    u_int_type iter;
    u_int_type nmax = c_nummaxint-c_numminint+1;

#ifndef  NOMPI
    svalloc( r, float_type, c_nummax+1 );
    svalloc( z, float_type, c_nummax+1 );
    svalloc( p, float_type, c_nummax+1 );
    svalloc( q, float_type, c_nummax+1 );
#else
    if ( parallel ){
      r = (float_type*)parallel->mpcom->MPC_malloc(sizeof(float_type) * (c_nummax+1) );
      z = (float_type*)parallel->mpcom->MPC_malloc(sizeof(float_type) * (c_nummax+1) );
      p = (float_type*)parallel->mpcom->MPC_malloc(sizeof(float_type) * (c_nummax+1) );
      q = (float_type*)parallel->mpcom->MPC_malloc(sizeof(float_type) * (c_nummax+1) );
    }else{
      svalloc( r, float_type, c_nummax+1 );
      svalloc( z, float_type, c_nummax+1 );
      svalloc( p, float_type, c_nummax+1 );
      svalloc( q, float_type, c_nummax+1 );
    }
#endif


    x = Tnew;

    // initialisation
    /*
      x^0 donnee
      r^0 = b - A.x^0
      C.z^0 = r^0
      p^0 = z^0
    */
    Affectation( x, Told );

    ProduitMatriceVecteur( x, q );
    for( u_int_type nc = c_numminint ; nc <= c_nummaxint ; nc++ )
	r[nc] = b[nc] - q[nc];

    Precond( r, z );

    for( u_int_type nc = c_numminint ; nc <= c_nummaxint ; nc++ )
	p[nc] = z[nc];
    /*
     */

    dep = NormeL2( r );

    // on commence les iterations
    for( iter = 0 ; iter < nmax ; iter++ ) {

	// q = A.p
// 	if(parallel->mpcom->Rank() == 0)
// 	  fprintf(stderr,"Iteration %d %s\n",iter,"ProduitMatriceVecteur( p, q )");
	ProduitMatriceVecteur( p, q );

	// alpha = (r|z) / (A.p|p)
// 	if(parallel->mpcom->Rank() == 0)
// 	  fprintf(stderr,"Iteration %d %s\n",iter," ProduitScalaire( r, z )");
	dot1 = ProduitScalaire( r, z );
// 	if(parallel->mpcom->Rank() == 0)
// 	  fprintf(stderr,"Iteration %d %s\n",iter,"ProduitScalaire( q, p )");
	dot2 = ProduitScalaire( q, p );
	alpha = dot1 / dot2;

	// x = x + alpha * p
	// r = r - alpha * A.p
	//
	for( u_int_type nc = c_numminint ; nc <= c_nummaxint ; nc++ ) {
	    x[nc] += alpha * p[nc];
	    r[nc] = r[nc] - alpha * q[nc];
	}

	// Preconditionnement
	Precond( r, z );

	//
// 	if(parallel->mpcom->Rank() == 0)
// 	  fprintf(stderr,"Iteration %d %s\n",iter,"ProduitScalaire( r, z )");
	dot3 = ProduitScalaire( r, z );
	beta = dot3 / dot1;

	// p = r + beta * p
	for( u_int_type nc = c_numminint ; nc <= c_nummaxint ; nc++ )
	    p[nc] = z[nc] + beta * p[nc];

	// critere de convergence
	nor = NormeL2( r );
	crit = nor / dep;
	if ( crit < 1.e-6 ) break;
// 	if(parallel->mpcom->Rank() == 0)
// 	  fprintf(stderr,"Iteration %d fin\n",iter);
    }

    if ( taskinfo->Ihavetoprint() )
	fprintf(stdout,"\tGC - %5d iter - res %.3e\n",(iter==nmax ? nmax : iter+1),crit);

#ifndef  NOMPI
    svfree(r);
    svfree(z);
    svfree(p);
    svfree(q);
#else
    if ( parallel ){
      parallel->mpcom->MPC_free(r);
      parallel->mpcom->MPC_free(z);
      parallel->mpcom->MPC_free(p);
      parallel->mpcom->MPC_free(q);
    }else{
      svfree(r);
      svfree(z);
      svfree(p);
      svfree(q);
    }
#endif
}

// Produit matrice-vecteur : A*x = y ou x et y sont dimensionnees au maillage
//
void EqConducDiscret::ProduitMatriceVecteur( float_type *x, float_type *y ) {

    MAJCL( x );

    for( u_int_type nc = c_numminint ; nc <= c_nummaxint ; nc++ ) {

	const u_int_type nbk = nbcol[nc];
	
	y[nc] = 0.;
	for( u_int_type k = 0 ; k < nbk ; k++ ) {
	    y[nc] += A[nc][k] * x[col[nc][k]];
	}
    }
}

// Produit scalaire dot = x.y ou x et y sont dimensionnees au maillage
//
float_type EqConducDiscret::ProduitScalaire( float_type *x, float_type *y ) {

    float_type dot = 0.;

    for( u_int_type nc = c_numminint ; nc <= c_nummaxint ; nc++ )
	dot += x[nc]*y[nc];

    if ( parallel ) {
	
	float_type dotin = dot;
	parallel->mpcom->AllReduce( &dotin, &dot, 1, MPC::OSUM );
    }

    return dot;
}

// Norme L2 de x ou x est dimensionnee au maillage
//
float_type EqConducDiscret::NormeL2( float_type *x ) {

    float_type norm = ProduitScalaire(x,x);
    
    return sqrt(norm);
}


// Preconditionnement diagonal : resolution de C.x = bb ou C = diag(A)
//
void EqConducDiscret::Precond( float_type *bb, float_type *x ) {

    for( u_int_type nc = c_numminint ; nc <= c_nummaxint ; nc++ )
	x[nc] = bb[nc] / A[nc][0];
}

#endif
 
