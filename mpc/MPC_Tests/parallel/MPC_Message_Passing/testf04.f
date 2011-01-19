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
C     Test support for MPC_STATUS_IGNORE and MPC_STATUSES_IGNORE
      include 'mpcf.h'
      integer nreqs
      parameter (nreqs = 100)
      integer reqs(MPC_REQUEST_SIZE,nreqs)
      integer stats(MPC_STATUS_SIZE,nreqs)
      integer ierr, rank, i, res, size,expect
      integer errs
      character*50 vecteur
      character*50 msg

      vecteur='nothing'
      msg='coucou'
      ierr = -1
      errs = 0
      call mpc_init( ierr )
      if (ierr .ne. MPC_SUCCESS) then
         errs = errs + 1
         print *, 'Unexpected return from MPC_INIT', ierr 
      endif

      ierr = -1
      call mpc_comm_size( MPC_COMM_WORLD, size, ierr )
      call mpc_comm_rank( MPC_COMM_WORLD, rank, ierr )
      if (ierr .ne. MPC_SUCCESS) then
         errs = errs + 1
         print *, 'Unexpected return from MPC_COMM_WORLD', ierr 
      endif
      do i=1, nreqs, 2
         ierr = -1
         call mpc_isend(vecteur , 10, MPC_CHAR, rank, i,
     $        MPC_COMM_WORLD, reqs(1,i), ierr )
         if (ierr .ne. MPC_SUCCESS) then
            errs = errs + 1
            print *, 'Unexpected return from MPC_ISEND', ierr 
         endif
         ierr = -1
         call mpc_irecv(msg , 10, MPC_CHAR, rank, i,
     $        MPC_COMM_WORLD, reqs(1,i+1), ierr )
         if (ierr .ne. MPC_SUCCESS) then
            errs = errs + 1
            print *, 'Unexpected return from MPC_IRECV', ierr 
         endif
      enddo

      ierr = -1
      call mpc_waitall( nreqs, reqs, stats, ierr )
      if (ierr .ne. MPC_SUCCESS) then
         errs = errs + 1
         print *, 'Unexpected return from MPC_WAITALL', ierr 
      endif
      if (vecteur .NE. msg) then 
            print *,'Erreur'
            call MPC_ABORT(MPC_COMM_WOLRD,1,ierr)
      endif

      ierr = -1
      call mpc_allreduce(rank,res,1,MPC_INT,MPC_SUM,MPC_COMM_WORLD,ierr)
      expect = (size * (size -1)) / 2
      if (res .NE. expect) then 
            print *,'Erreur'
            call MPC_ABORT(MPC_COMM_WOLRD,1,ierr)
      endif

      call mpc_finalize( ierr )
      end
