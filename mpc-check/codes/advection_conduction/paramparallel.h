#ifndef PARAMPARALLEL_H
#define PARAMPARALLEL_H

#include "parallelpp/MPC.h"

#include "mestypes.h"


class ParamParallel {

public:

    int_type NbSsDom[DIM]; // nombre de sous-domaines par direction
    int_type NbSsDomTot;  // nombre total de sous-domaines

    MPC *mpcom; // classe permettant les communications par Message Passing

    int_type voisin_dc[DIM][NBCOT]; // sous-domaines voisins par direction et par côté
    int_type voisin_pos[DIM*NBCOT]; // sous-domaines voisins par position

    int_type monrang; // le rang du processus dans l'univers
    int_type II[DIM];    // position II[dir_x]=ligne colonne=II[dir_y] 
                         // du sous-domaine parmi les autres sous-domaines


    // constructeur
    ParamParallel( int_type nbssdx, int_type nbssdy, MPC *mp ) {

	//int_type d, c, pos;


	// affectation de la décomposition de domaine
	NbSsDom[dir_x] = nbssdx;
	NbSsDom[dir_y] = nbssdy;
	NbSsDomTot     = nbssdx*nbssdy;

	// allocation de l'instance de MPC
	mpcom = mp;

	// on affecte le rang une bonne fois pour toute
	monrang = mpcom->Rank();

	// on détermine les voisins
	II[dir_x] = monrang / nbssdy;
	II[dir_y] = monrang % nbssdy;

	// on détermine la connectivité
	voisin_pos[0] = voisin_dc[dir_x][gauche] 
	    = ( II[dir_x] == 0 ? -1 : monrang-nbssdy );
	voisin_pos[1] = voisin_dc[dir_x][droite] 
	    = ( II[dir_x] == nbssdx-1 ? -1 : monrang+nbssdy );
	voisin_pos[2] = voisin_dc[dir_y][gauche] 
	    = ( II[dir_y] == 0 ? -1 : monrang-1 );
	voisin_pos[3] = voisin_dc[dir_y][droite] 
	    = ( II[dir_y] == nbssdy-1 ? -1 : monrang+1 );

    } // fin constructeur

    // modifie la connectivité des sous-domaines dans le cas de CL
    // périodique
    void CorrectionConnectivite(int_type pos) {

	switch( pos ) {

	case 0:
	    if ( II[dir_x] == 0 ) {
		voisin_pos[0] = voisin_dc[dir_x][gauche] 
		    = (NbSsDom[dir_x]-1)*NbSsDom[dir_y] + II[dir_y];
	    }
	    break;

	case 1:
	    if ( II[dir_x] == NbSsDom[dir_x]-1 ) {
		voisin_pos[1] = voisin_dc[dir_x][droite] 
		    = II[dir_y];
	    }
	    break;

	}
    }
    
    // destructeur
    ~ParamParallel() {}

} ;

#endif
