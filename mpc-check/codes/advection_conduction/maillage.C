#ifndef MAILLAGE_C
#define MAILLAGE_C

#include <MPC.h>
#include <TaskInfo.h>

#include "paramparallel.h"
#include "maillage.h"

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

void Maillage::ConstructionCoordonneesMailles() {
  //MPC_LOG_IN();

    int_type nc, ii[DIM];
    float_type delta[DIM];


    delta[dir_x] = dimension[dir_x] / float_type(nbmail[dir_x]);
    delta[dir_y] = dimension[dir_y] / float_type(nbmail[dir_y]);

    // construction de l'origine du domaine
    coordorigine[dir_x] = 0.;
    coordorigine[dir_y] = 0.;

    if ( parallel ) {
	coordorigine[dir_x] += nbmailavant[dir_x]*delta[dir_x];
	coordorigine[dir_y] += nbmailavant[dir_y]*delta[dir_y];
    }

    // on alloue les coordonnées
    for( int_type d = 0 ; d < DIM ; d++ ) {
#ifndef  NOMPI
      svcalloc( c_coord[d], float_type, c_nummax+1 );
      svcalloc( c_delta[d], float_type, c_nummax+1 );
#else
      if ( parallel ){
	c_coord[d] = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), c_nummax+1 );
	c_delta[d] = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), c_nummax+1 );
      }else{
	svcalloc( c_coord[d], float_type, c_nummax+1 );
	svcalloc( c_delta[d], float_type, c_nummax+1 );
      }
#endif
    }
	    
    // on parcourt les mailles internes
    nc = 1;
    for( ii[dir_x] = 0 ; ii[dir_x] < nbmail[dir_x] ; ii[dir_x]++ ) {

	for( ii[dir_y] = 0 ; ii[dir_y] < nbmail[dir_y] ; ii[dir_y]++, nc++ ) {

	    for( int_type d = 0 ; d < DIM ; d++ ) {
		c_coord[d][nc] = coordorigine[d] + delta[d] / 2. + ii[d] * delta[d];
		c_delta[d][nc] = delta[d] ;

	    } // fin d		
	} // fin j
    } // fin i

    // puis, on parcourt les mailles du bord (i.e. les mailles fantômes)
    // bord gauche
    ii[dir_x] = -1;
    for( ii[dir_y] = 0 ; ii[dir_y] < nbmail[dir_y] ; ii[dir_y]++, nc++ ) {

	for( int_type d = 0 ; d < DIM ; d++ ) {
	    c_coord[d][nc] = coordorigine[d] + delta[d] / 2. + ii[d] * delta[d];
	    c_delta[d][nc] = delta[d] ;
		
	} // fin d		
    } // fin j

    // bord droit
    ii[dir_x] = nbmail[dir_x];
    for( ii[dir_y] = 0 ; ii[dir_y] < nbmail[dir_y] ; ii[dir_y]++, nc++ ) {

	for( int_type d = 0 ; d < DIM ; d++ ) {
	    c_coord[d][nc] = coordorigine[d] + delta[d] / 2. + ii[d] * delta[d];
	    c_delta[d][nc] = delta[d] ;
		    
	} // fin d		
    } // fin j
	    
    // bord bas
    ii[dir_y] = -1;
    for( ii[dir_x] = 0 ; ii[dir_x] < nbmail[dir_x] ; ii[dir_x]++, nc++ ) {
		
	for( int_type d = 0 ; d < DIM ; d++ ) {
	    c_coord[d][nc] = coordorigine[d] + delta[d] / 2. + ii[d] * delta[d];
	    c_delta[d][nc] = delta[d] ;
		    
	} // fin d		
    } // fin i

    // bord haut
    ii[dir_y] = nbmail[dir_y];
    for( ii[dir_x] = 0 ; ii[dir_x] < nbmail[dir_x] ; ii[dir_x]++, nc++ ) {

	for( int_type d = 0 ; d < DIM ; d++ ) {
	    c_coord[d][nc] = coordorigine[d] + delta[d] / 2. + ii[d] * delta[d];
	    c_delta[d][nc] = delta[d] ;
		    
	} // fin d		
    } // fin i

    for( int_type d = 0 ; d < DIM ; d++ )
	c_d[d] = c_delta[d][c_numminint];
//MPC_LOG_OUT();
}


void Maillage::ConstructionCoordonneesFaces() {
  //MPC_LOG_IN();

    int_type nf = 1, ii[DIM];
    float_type delta[DIM];

    // allocation
    for( int_type c = 0 ; c < NBCOT ; c++ ) {
#ifndef  NOMPI
      svcalloc( f_coord[c], float_type, f_nummax+1 );
#else
      if ( parallel ){
	f_coord[c] = (float_type*)parallel->mpcom->MPC_calloc(sizeof(float_type), f_nummax+1 );
      }else{
	svcalloc( f_coord[c], float_type, f_nummax+1 );
      }
#endif
    }

    delta[dir_x] = dimension[dir_x] / float_type(nbmail[dir_x]);
    delta[dir_y] = dimension[dir_y] / float_type(nbmail[dir_y]);

    for( ii[dir_x] = 0 ; ii[dir_x] <= nbmail[dir_x] ; ii[dir_x]++ ) {

	for( ii[dir_y] = 0 ; ii[dir_y] < nbmail[dir_y] ; ii[dir_y]++, nf++ ) {
		    
	    f_coord[dir_x][nf] = coordorigine[dir_x] + ii[dir_x] * delta[dir_x];
	    f_coord[dir_y][nf] = coordorigine[dir_y] + ii[dir_y] * delta[dir_y] + delta[dir_y] / 2.;

	} // fin j
    } // fin i

    for( ii[dir_y] = 0 ; ii[dir_y] <= nbmail[dir_y] ; ii[dir_y]++ ) {

	for( ii[dir_x] = 0 ; ii[dir_x] < nbmail[dir_x] ; ii[dir_x]++, nf++ ) {
		    
	    f_coord[dir_y][nf] = coordorigine[dir_y] + ii[dir_y] * delta[dir_y];
	    f_coord[dir_x][nf] = coordorigine[dir_x] + ii[dir_x] * delta[dir_x] + delta[dir_x] / 2.;

	} // fin j
    } // fin i	    

  //MPC_LOG_OUT();	    
}


void Maillage::ConstructionConnectivite() {
  //MPC_LOG_IN();

    int_type nc, ii[DIM], tmp, nf1, nf2, nf3, nf4;

    // allocations
    for( int_type c = 0 ; c < NBCOT ; c++ ) {

#ifndef  NOMPI
      svcalloc( facube[c], int_type, f_nummax+1 );
#else
      if ( parallel ){
	facube[c] = (int_type*)parallel->mpcom->MPC_calloc(sizeof(int_type), f_nummax+1 );
      }else{
	svcalloc( facube[c], int_type, f_nummax+1 );
      }
#endif
      for( int_type d = 0 ; d < DIM ; d++ ) {
#ifndef  NOMPI
	svcalloc( caface[d][c], int_type, c_nummax+1 );
#else
	if ( parallel ){
	  caface[d][c] = (int_type*)parallel->mpcom->MPC_calloc(sizeof(int_type), c_nummax+1 );
	}else{
	  svcalloc( caface[d][c], int_type, c_nummax+1 );
	}
#endif
	} // fin d
    } // fin c

    // construction caface ( cubes -> faces ) et facube ( faces -> cubes )
    // ... pour les mailles internes
    nc = 1;
    tmp = (nbmail[dir_x]+1)*nbmail[dir_y] + 1;
    for( ii[dir_x] = 0 ; ii[dir_x] < nbmail[dir_x] ; ii[dir_x]++ ) {
		
	for( ii[dir_y] = 0 ; ii[dir_y] < nbmail[dir_y] ; ii[dir_y]++, nc++ ) {

	    nf1 = 1 + ii[dir_y] + ii[dir_x]*nbmail[dir_y];
	    nf2 = 1 + ii[dir_y] + (ii[dir_x]+1)*nbmail[dir_y];
	    nf3 = tmp + ii[dir_x] + ii[dir_y]*nbmail[dir_x];
	    nf4 = tmp + ii[dir_x] + (ii[dir_y]+1)*nbmail[dir_x];

	    caface[dir_x][gauche][nc] = nf1;
	    caface[dir_x][droite][nc] = nf2;
	    caface[dir_y][gauche][nc] = nf3;
	    caface[dir_y][droite][nc] = nf4;

	    facube[droite][nf1] = nc;
	    facube[gauche][nf2] = nc;
	    facube[droite][nf3] = nc;
	    facube[gauche][nf4] = nc;
	}
    }

    // .. pour les mailles fantômes du bord gauche
    tmp = 1;
    for( ii[dir_y] = 0 ; ii[dir_y] < nbmail[dir_y] ; ii[dir_y]++, nc++ ) {

	nf2 = tmp + ii[dir_y];
		
	caface[dir_x][gauche][nc] = 0;
	caface[dir_x][droite][nc] = nf2;
	caface[dir_y][gauche][nc] = 0;
	caface[dir_y][droite][nc] = 0;

	facube[gauche][nf2] = nc;
    }

    // .. pour les mailles fantômes du bord droit
    tmp = nbmail[dir_x]*nbmail[dir_y] + 1;
    for( ii[dir_y] = 0 ; ii[dir_y] < nbmail[dir_y] ; ii[dir_y]++, nc++ ) {

	nf1 = tmp + ii[dir_y];
		
	caface[dir_x][gauche][nc] = nf1;
	caface[dir_x][droite][nc] = 0;
	caface[dir_y][gauche][nc] = 0;
	caface[dir_y][droite][nc] = 0;

	facube[droite][nf1] = nc;
    }

    // .. pour les mailles fantômes du bord bas
    tmp = (nbmail[dir_x]+1)*nbmail[dir_y] + 1;
    for( ii[dir_x] = 0 ; ii[dir_x] < nbmail[dir_x] ; ii[dir_x]++, nc++ ) {

	nf4 = tmp + ii[dir_x];
		
	caface[dir_x][gauche][nc] = 0;
	caface[dir_x][droite][nc] = 0;
	caface[dir_y][gauche][nc] = 0;
	caface[dir_y][droite][nc] = nf4;

	facube[gauche][nf4] = nc;
    }

    // .. pour les mailles fantômes du bord haut
    tmp = f_nummax - nbmail[dir_x] + 1;
    for( ii[dir_x] = 0 ; ii[dir_x] < nbmail[dir_x] ; ii[dir_x]++, nc++ ) {

	nf3 = tmp + ii[dir_x];
		
	caface[dir_x][gauche][nc] = 0;
	caface[dir_x][droite][nc] = 0;
	caface[dir_y][gauche][nc] = nf3;
	caface[dir_y][droite][nc] = 0;

	facube[droite][nf3] = nc;
    }

  //MPC_LOG_OUT();
}


void Maillage::ConstructionRecouvrement() {
  //MPC_LOG_IN();

    int_type pos, ind, nc, d, c, copp, nf, ncv;

    for( pos = 0 ; pos < DIM*NBCOT ; pos++ ) {

	// nombre de mailles dans le recouvrement
	c_nbblkrec[pos] = c_nummaxcl[pos] - c_nummincl[pos] + 1;

	// allocations
#ifndef  NOMPI
	svalloc( c_debblkrec[pos], u_int_type, c_nbblkrec[pos] );
	svalloc( c_finblkrec[pos], u_int_type, c_nbblkrec[pos] );
#else
    if ( parallel ){
      c_debblkrec[pos] = 
	(u_int_type*)parallel->mpcom->MPC_malloc(sizeof(u_int_type) * c_nbblkrec[pos] );
      c_finblkrec[pos] = 
	(u_int_type*)parallel->mpcom->MPC_malloc(sizeof(u_int_type) * c_nbblkrec[pos] );
    }else{
	svalloc( c_debblkrec[pos], u_int_type, c_nbblkrec[pos] );
	svalloc( c_finblkrec[pos], u_int_type, c_nbblkrec[pos] );
    }
#endif

	d = pos / NBCOT;
	c = pos % NBCOT;
	copp = ( c == 0 ? 1 : 0 );

	// on parcourt les mailles du recouvrement
	for( nc = c_nummincl[pos], ind = 0 ;
	     (u_int_type)nc <= c_nummaxcl[pos] ; nc++, ind++ ) {
	
	    nf  = caface[d][copp][nc];
	    ncv = facube[ copp ][ nf ];

	    // on met la maille du recouvrement
	    c_debblkrec[pos][ind] = ncv;
	    c_finblkrec[pos][ind] = ncv;
	}	

    } // fin pos

    // allocation du tableau des types de mailles
#ifndef  NOMPI
    svcalloc( c_type, u_int_type, c_nummax+1 );
#else
    if ( parallel ){
      c_type = (u_int_type*)parallel->mpcom->MPC_calloc(sizeof(u_int_type), c_nummax+1 );
    }else{
      svcalloc( c_type, u_int_type, c_nummax+1 );
    }
#endif
    
    for( nc = c_numminint ; (u_int_type)nc <= c_nummaxint ; nc++ )
	c_type[nc] = CELLINT;

    for( pos = 0 ; (u_int_type)pos < DIM*NBCOT ; pos++ ) {

	const u_int_type ctyp = pos2type(pos);
	
 	// on parcourt les mailles fantomes
	for( nc = c_nummincl[pos] ; (u_int_type)nc <= c_nummaxcl[pos] ; nc++ )
	    c_type[nc] = ctyp;
   }
  //MPC_LOG_OUT();
}

// symetrise la variable r sur les mailles fantômes
void Maillage::MAJCL( float_type *r ) {
  //MPC_LOG_IN();

    int_type nc, ncv, nf, pos, copp, posopp, ncopp;


    for( int_type d = 0 ; d < DIM ; d++ ) {
	for( int_type c = 0 ; c < NBCOT ; c++ ) {

	    pos = c + d*NBCOT;
	    copp = ( c == 0 ? 1 : 0 );
	    posopp = copp + d*NBCOT;

	    switch( cltype[pos] ) {

	    case clvoisin:

		if ( parallel->voisin_dc[d][c] >= 0 )
		    IEnvoyerRecouvrement( r, pos );
 		if ( parallel->voisin_dc[d][copp] >= 0 ) 
		    IRecevoirFantome( r, posopp );

		break;

	    case clflux:
		for( nc = c_nummincl[pos] ;
		     (u_int_type)nc <= c_nummaxcl[pos] ; nc++ ) {

		    nf  = caface[d][copp][nc];
		    ncv = facube[ copp ][ nf ];

		    r[nc] = r[ncv];
		}
		
 		if ( parallel && (parallel->voisin_dc[d][copp] >= 0) )
		    IRecevoirFantome( r, posopp ); 
		break;

	    case clperiodique:
		ncopp = c_nummincl[posopp];
		for( nc = c_nummincl[pos] ;
		     (u_int_type)nc <= c_nummaxcl[pos] ; nc++, ncopp++ ) {
		    
		    // on récupère la maille fantôme de l'autre côté
		    // Rem: on suppose que le maillage est régulier
		    nf  = caface[d][c][ncopp];
		    ncv = facube[ c ][ nf ];

		    r[nc] = r[ncv];
		}
		break;

	    default: break;
	    }

	} // fin c	

	WaitEndComm();
    } // fin d

  //MPC_LOG_OUT();
}

// effectue rout[] <- rin[] sur toutes les mailles
void Maillage::Affectation( float_type *rout, float_type *rin ) {
  //MPC_LOG_IN();

    for( int_type nc = c_numminint ; (u_int_type)nc <= c_nummax ; nc++ ) {
	rout[nc] = rin[nc];
    }

  //MPC_LOG_OUT();
}

// effectue rout[] <- val sur toutes les mailles
void Maillage::Affectation( float_type *rout, float_type val ) {
  //MPC_LOG_IN();

    for( int_type nc = c_numminint ; (u_int_type)nc <= c_nummax ; nc++ ) {
	rout[nc] = val;
    }

  //MPC_LOG_OUT();
}

// Envoie le recouvrement de la position pos de la variable r 
//
void Maillage::IEnvoyerRecouvrement( float_type *r, int_type pos ) {
  //MPC_LOG_IN();

    int_type rangvois;
    MPC::message_status_t req;

    //fprintf(stderr,"IEnvoyerRecouvrement\n");
    // numéro du sous-dom à qui on va envoyer le recouvrement
    rangvois = parallel->voisin_pos[pos];

    // init
    parallel->mpcom->OpenPack(c_nbblkrec[pos], c_debblkrec[pos], c_finblkrec[pos],&req);

    parallel->mpcom->AddPack( r ,&req);

    // envoi du paquet
    parallel->mpcom->ISendPack( rangvois,0 ,&req);
    //fprintf(stderr,"IEnvoyerRecouvrement fait\n");
  //MPC_LOG_OUT();
}

// Reçoit dans les mailles fantômes de r le recouvrement du voisin pos
//
void Maillage::IRecevoirFantome( float_type *r, int_type pos ) {
  //MPC_LOG_IN();

    int_type rangvois;
    MPC::message_status_t req;

    //fprintf(stderr,"IRecevoirFantome\n");
    // numéro du sous-dom à qui on va envoyer le recouvrement
    rangvois = parallel->voisin_pos[pos];

    // init
    parallel->mpcom->OpenPack(1, &(c_nummincl[pos]), &(c_nummaxcl[pos]),&req);

    parallel->mpcom->AddPack( r ,&req);

    // réception du paquet
    parallel->mpcom->IRecvPack( rangvois ,0,&req);
    //fprintf(stderr,"IRecevoirFantome fait\n");
  //MPC_LOG_OUT();
}

// Attente des fins de requetes
void Maillage::WaitEndComm() {
  //MPC_LOG_IN();
  //fprintf(stderr,"WaitEndComm\n");
    if ( parallel )
	parallel->mpcom->WaitAll();
  //fprintf(stderr,"WaitEndComm fait\n");
  //MPC_LOG_OUT();
}

// ecrit un fichier de sortie graph pour var(t)
//
void Maillage::EcrireSortie( float_type *var, float_type t, 
			     int_type numc, float_type &tg, float_type rg, float_type tfin,
			     const char *nomvar) {
  //MPC_LOG_IN();

    char nomfic[PATH_MAX];
    FILE *fd;
    int_type nc, d;

    if (!(numc==0 || t>tg || t>tfin))
	return;

    tg += rg;

    // attention temporaire
    if (parallel)
	parallel->mpcom->Barrier();

    if ( taskinfo->Ihavetoprint() )
      fprintf(stdout,"Écriture du cycle %d ... ",numc);

    // création du nom de fichier
    if ( !parallel ) {
	sprintf( nomfic, "./sorties_serial/%s_cycl%04d.dat", nomvar, numc );
    } else {
	sprintf( nomfic, "./sorties_mp/%s_cycl%04d_dom%04d.dat", 
		 nomvar, numc, parallel->monrang );
    }

    // création du fichier
    fd = fopen( nomfic, "w");

    // pour toutes les mailles internes, on écrit les infos:
    //    X Y var(X,Y)
    for( nc = c_numminint ; (u_int_type)nc <= c_nummaxint ; nc++ ) {

	for( d = 0 ; d < DIM ; d++ ) {
	    fprintf( fd, "%15.11f ", c_coord[d][nc] );
	}
	fprintf( fd, "%15.11f\n", var[nc] );

    } // fin nc

    // fermeture du fichier
    fclose( fd );

    // on complète le fichier de commandes gnuplot
    // on ouvre le fichier à la fin
    if ( !parallel ) {

	char nomfic2[PATH_MAX];

	sprintf(nomfic2,"./sorties_serial/%s_allcycl.gnu",nomvar);
	fd = fopen( nomfic2, "a");

	// ecriture de la ligne de commande
	fprintf( fd, "set title 't=%6.4f'\nsplot '%s' notitle with points\npause -1\n", t, nomfic);
	
	// fermeture du fichier de commandes
	fclose( fd );

    } else {
	if ( parallel->monrang == 0 ) {

	    char nomfic2[PATH_MAX];

	    sprintf(nomfic2,"./sorties_mp/%s_allcycl.gnu",nomvar);
	    fd = fopen( nomfic2, "a");
	    
	    // ecriture de la ligne de commande
	    fprintf( fd, "set title 't=%6.4f'\nsplot '%s' notitle with points", t, nomfic);
	    for( int_type np = 1 ; np < parallel->NbSsDomTot ; np++ ) {
		sprintf( nomfic, "./sorties_mp/%s_cycl%04d_dom%04d.dat", nomvar, numc, np );
		fprintf( fd, ", '%s' notitle with points", nomfic);
	    }
	    fprintf( fd, "\npause -1\n");
	    
	    // fermeture du fichier de commandes
	    fclose( fd );
	}
    }

    if (parallel)
	parallel->mpcom->Barrier();

    if ( taskinfo->Ihavetoprint() )
      fprintf(stdout," terminée.\n");
  //MPC_LOG_OUT();
}

#endif
