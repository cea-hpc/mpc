
#include <mpi.h>
#include <string.h>
#include <stdlib.h>
#include <machine_types.h>
#include <MPC.h>
#include <TaskInfo.h>
#include <chrono.h>
#include <MPC_MPI.h>
#include <MPC_MPC.h>
#include <mpc.h>

#ifdef CONV
#include "equation.h"
#else
#include "EquationConduction.h"
#endif

#include "mestypes.h"


#ifndef PATH_MAX
#define PATH_MAX 256
#endif

char* prog_name = NULL;
char donnee_file[500];
//char * donnee_file;

extern TaskInfo *taskinfo;

struct param_t {
    int_type nx;
    int_type ny;
    float_type tfin;
  int_type nbcyclmax;
    enum MPtype model;
    int_type npx;
    int_type npy;
    int_type npX;
    int_type npY;
    int nb_proc;
} ;

int get_pos_prog_name(){
  int i;
  i = strlen(prog_name);
  while(prog_name[i] == ' '){
    prog_name[i] = '\0';
    i--;
  }
  while(prog_name[i] != '/'){
    i--;
  }
  i++;
  return i;
}

void lecture_donnees( struct param_t *param ) {
    FILE *fd;
    char line[PATH_MAX];
    int_type nblin;

    param->nx = param->ny = 1;
    param->tfin = 1.;
    param->nbcyclmax = 1000000;
    param->model = singlethreaded;
    param->npx = param->npy = 1;
    param->npX = param->npY = 1;
    param->nb_proc = 0;

//     fprintf(stderr,"%s",prog_name);

    sprintf(donnee_file,"DONNEES_%s",&(prog_name[get_pos_prog_name()]));
//    fprintf(stderr,"%s",donnee_file);
    fd = NULL;
    fd = fopen(donnee_file,"r");
    if ( !fd ) {
      fprintf(stderr,"Impossible to open file %s for reading\n",donnee_file);
      fd = fopen("DONNEES","r");
      if ( !fd ) {
	fprintf(stderr,"Impossible to open file DONNEES for reading\n");
	exit(1);
      }
    }

    for ( fgets(line,PATH_MAX,fd), nblin = 0 ; 
	  !feof(fd); 
	  fgets(line,PATH_MAX,fd), nblin++ ) {
	if ( !line || (line[0] != '#') ) {
	    
	    if ( strstr(line, "xcells=") ) {
		param->nx = atoi(line+strlen("xcells=")) ;

	    } else if ( strstr(line, "ycells=") ) {
		param->ny = atoi(line+strlen("ycells=")) ;

	    } else if ( strstr(line, "tfinal=") ) {
		param->tfin = atof(line+strlen("tfinal=")) ;

	    } else if ( strstr(line, "cyclmax=") ) {
		param->nbcyclmax = atoi(line+strlen("cyclmax=")) ;

	    } else if ( strstr(line, "xsubdom=") ) {
		param->npx = atoi(line+strlen("xsubdom=")) ;

	    } else if ( strstr(line, "ysubdom=") ) {
		param->npy = atoi(line+strlen("ysubdom=")) ;

	    } else if ( strstr(line, "xproc=") ) {
		param->npX = atoi(line+strlen("xproc="));

	    } else if ( strstr(line, "yproc=") ) {
		param->npY = atoi(line+strlen("yproc="));

	    } else if ( strstr(line, "nb_proc=") ) {
		param->nb_proc = atoi(line+strlen("nb_proc="));

	    }else if ( strstr(line, "mpctype=") ) {

		if ( strstr(line+strlen("mpctype="), "seq") ) {
		    param->model = singlethreaded;
		} else if ( strstr(line+strlen("mpctype="), "mpi") ) {
		    param->model = mpi;
		} else if ( strstr(line+strlen("mpctype="), "mpc") ) {
		    param->model = mpc;
		} else if ( strstr(line+strlen("mpctype="), "pthread") ) {
		    param->model = mpc_new;
		} else if ( strstr(line+strlen("mpctype="), "ethread") ) {
		    param->model = mpc_new;
		} else if ( strstr(line+strlen("mpctype="), "ethread_mxn") ) {
		    param->model = mpc_new;
		} else if ( strstr(line+strlen("mpctype="), "marcel") ) {
		    param->model = mpc_new;
		} else {
		    fprintf(stderr,"error line %d : mpc type unknown : %s\n",
			    nblin, line+strlen("mpctype="));
		    exit(1);
		}

	    } 
	}
    }

    fclose(fd);
}

void affiche_donnees( struct param_t *param ) {
  char network[500];
    char threads[500];
    int length;
    fprintf(stdout,"************************************\n");
    fprintf(stdout,"* Domain : %dx%d cells\n", param->nx, param->ny);
    fprintf(stdout,"* Final Date : %g\n", param->tfin );
    fprintf(stdout,"* Max iter : %d\n", param->nbcyclmax );
    switch( param->model ) {
    case singlethreaded:
	fprintf(stdout,"* Parallelism : none\n"); break;
    case mpi:
      fprintf(stdout,"* Parallelism : MPI\n"); break;
    case mpc:
      MPC_Get_multithreading(threads,500);
      MPC_Get_networking(network,500);
      fprintf(stdout,"* Parallelism : MPC %s/%s\n",threads,network); break;
    }
    if ( param->model != singlethreaded )
	fprintf(stdout,"* Domain Decomposition : %dx%d\n", param->npx, param->npy);
    fprintf(stdout,"************************************\n");
}

void run( struct main_arg *mesarg ) {
//  return;

#ifdef CONV
  EqTranspDiscret *eq;
#else
  EqConducDiscret *eq;
#endif

  ParamParallel *pp;
  struct param_t *donnees;
  int_type nx, ny, taskid, cyclm;
  float_type tfin;
  MPC *mpcom;
  struct timeval tbeg;
  struct timeval tend;
  FILE *fd;

  mpcom = mesarg->mpcom;
  donnees = (struct param_t *)(mesarg->taskarg);
  taskid = ( mpcom ? mpcom->Rank() : 0 );

//  fprintf(stderr,"%d started\n",taskid);

  taskinfo->SetTaskID( &taskid );
  if ( taskinfo->Ihavetoprint() )
    affiche_donnees( donnees );

  nx = donnees->nx;
  ny = donnees->ny;
  tfin = donnees->tfin;
  cyclm = donnees->nbcyclmax;

  pp = ( mpcom ? new ParamParallel(donnees->npx,donnees->npy,mpcom) : NULL );

#ifdef CONV
  eq = new EqTranspDiscret(nx,ny, tfin, cyclm, pp);
#else
  eq = new EqConducDiscret(nx,ny, tfin, cyclm, pp);
#endif
    
  if(mpcom != NULL)
    mpcom->Barrier();
  if((mpcom == NULL) || (!mpcom->Rank())){
    gettimeofday(&tbeg, NULL);
  }

  eq->CalculerTousLesCycles();

  if(mpcom != NULL)
    mpcom->Barrier();
  if((mpcom == NULL) || (!mpcom->Rank())){
    gettimeofday(&tend, NULL);
    subtimeval(&tend,&tbeg);
    fd = fopen(donnee_file,"w");
    print_date(&tend,fd);
  }

//  fprintf(stderr,"%d ended\n",taskid);
}


int main( int argc, char **argv ) {
  prog_name = argv[0];

  struct param_t donnees;

  lecture_donnees( &donnees );
  if((donnees.nx < donnees.npx) || (donnees.ny < donnees.npy))
    return( -1);
  
  sprintf(donnee_file,"RES_%s",&(prog_name[get_pos_prog_name()]));

  switch( donnees.model ) {
  case singlethreaded:
    {
      struct main_arg mesarg = {argc, argv, (void*)&donnees, NULL}; 
      taskinfo = new TaskInfo();
      run( &mesarg );
      delete taskinfo;
    }
    break;
    
#if !NOMPI
  case mpi:
    {
      MPC *mpcom = new MPC_MPI(&argc, &argv, donnees.npx*donnees.npy);

      mpcom->StartTasks( run, &donnees );

      delete mpcom;
    }
    break;
#endif
  case mpc:
    {
      MPC *mpcom = new MPC_MPC(&argc, &argv, donnees.npx*donnees.npy);

      mpcom->StartTasks( run, &donnees );

      delete mpcom;
    }
    break;
  }
  
  return 0;
}
