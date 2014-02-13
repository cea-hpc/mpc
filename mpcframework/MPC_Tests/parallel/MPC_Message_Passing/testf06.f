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
C
C      program main
      subroutine mpc_user_main   
c     test bcast of logical
c     works on suns, needs mpcch fix and heterogeneous test on alpha with PC
      include 'mpcf.h'
      integer myid, numprocs, rc, ierr
      integer errs, toterrs
      logical boo

      call MPC_INIT( ierr )
      call MPC_COMM_RANK( MPC_COMM_WORLD, myid, ierr )
      call MPC_COMM_SIZE( MPC_COMM_WORLD, numprocs, ierr )
C
      errs = 0
      boo = .true.
      call MPC_BCAST(boo,1,MPC_LOGICAL,0,MPC_COMM_WORLD,ierr)
      if (boo .neqv. .true.) then 
         print *, 'Did not broadcast Fortran logical (true)'
         errs = errs + 1
      endif
C
      boo = .false.
      call MPC_BCAST(boo,1,MPC_LOGICAL,0,MPC_COMM_WORLD,ierr)
      if (boo .neqv. .false.) then 
         print *, 'Did not broadcast Fortran logical (false)'
         errs = errs + 1
      endif
      call MPC_Reduce( errs, toterrs, 1, MPC_INTEGER, MPC_SUM, 
     $                 0, MPC_COMM_WORLD, ierr )
      if (myid .eq. 0) then
         if (toterrs .eq. 0) then
            print *, ' No Errors'
         else
            print *, ' Found ', toterrs, ' errors'
         endif
      endif
      call MPC_FINALIZE(rc)
      end
