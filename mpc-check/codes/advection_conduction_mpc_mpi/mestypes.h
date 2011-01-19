#ifndef MESTYPES_H
#define MESTYPES_H

#define DIM   2
#define NBCOT 2

enum direction {
    dir_x, dir_y
};

enum cote {
    gauche, droite
};

enum cl {
    clflux, clperiodique, clvoisin
};

enum MPtype {
    singlethreaded,  // i.e. sequential
    mpi,
    mpc_new,
    mpc
};

#endif
