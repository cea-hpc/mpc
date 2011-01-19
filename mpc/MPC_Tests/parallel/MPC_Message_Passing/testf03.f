C ############################# MPC License ############################## 
C # Wed Nov 19 15:19:19 CET 2008                                         # 
C # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # 
C #                                                                      # 
C # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # 
C # This file is part of the MPC Runtime.                                # 
C #                                                                      # 
C # This software is governed by the CeCILL-C license under French law   # 
C # and abiding by the rules of distribution of free software.  You can  # 
C # use, modify and/ or redistribute the software under the terms of     # 
C # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # 
C # following URL http://www.cecill.info.                                # 
C #                                                                      # 
C # The fact that you are presently reading this means that you have     # 
C # had knowledge of the CeCILL-C license and that you accept its        # 
C # terms.                                                               # 
C #                                                                      # 
C # Authors:                                                             # 
C #   - PERACHE Marc marc.perache@cea.fr                                 # 
C #                                                                      # 
C ######################################################################## 
C -*- Mode: Fortran; -*- 
c      program testf03
      subroutine mpc_user_main
      include 'mpcf.h'
      integer rank
      integer size
      integer ierr
      integer parity
      character*50 vecteur
      character*50 msg
      integer destsrc
      integer status(100)
      call MPC_INIT (ierr)
      call MPC_COMM_RANK (MPC_COMM_WORLD, rank, ierr)
      call MPC_COMM_SIZE (MPC_COMM_WORLD, size, ierr)
      vecteur='nothing'
      msg='coucou'
      parity = mod(rank,2)
      if(parity .EQ. 0) then 
            destsrc = (rank + 1) 
            destsrc = mod(destsrc,size)
            call MPC_SEND(msg,9,MPC_CHAR,destsrc,0,MPC_COMM_WORLD,ierr)
	    destsrc = (rank + size - 1) 
            destsrc = mod(destsrc,size)
            call MPC_RECV(vecteur,9,MPC_CHAR,destsrc,0,MPC_COMM_WORLD,
     $        status,ierr)
      else
	    destsrc = (rank + size - 1) 
            destsrc = mod(destsrc,size)
            call MPC_RECV(vecteur,9,MPC_CHAR,destsrc,0,MPC_COMM_WORLD,
     $        status,ierr)
            destsrc = (rank + 1) 
            destsrc = mod(destsrc,size)
            call MPC_SEND(msg,9,MPC_CHAR,destsrc,0,MPC_COMM_WORLD,ierr)
      endif
      if (vecteur .NE. msg) then 
            print *,'Erreur'
            call MPC_ABORT(MPC_COMM_WOLRD,1,ierr)
      endif
      call MPC_FINALIZE (ierr) 
      end 
c      end
