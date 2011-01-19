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
c	program main
        subroutine mpc_user_main
	include 'mpcf.h'
	integer count, errcnt, size, rank, ierr, i
	integer comm
	logical fnderr
	integer max_size
	integer world_rank
	parameter (max_size=100)
	integer intin(max_size), intout(max_size), intsol(max_size)
	real    realin(max_size), realout(max_size), realsol(max_size)
	double precision dblein(max_size), dbleout(max_size),
     *                   dblesol(max_size)
	complex cplxin(max_size), cplxout(max_size), cplxsol(max_size)
	logical login(max_size), logout(max_size), logsol(max_size)
C
C
C
C       Declare work areas
C
	call MPC_INIT( ierr )

	errcnt = 0
	comm = MPC_COMM_WORLD
	call MPC_COMM_RANK( comm, rank, ierr )
	world_rank = rank
	call MPC_COMM_SIZE( comm, size, ierr )
	count = 10

C Test sum 
	if (world_rank .eq. 0) print *, ' MPC_SUM'

       fnderr = .false.
       do 23000 i=1,count
	intin(i) = i
        intsol(i) = i*size
	intout(i) = 0
23000	continue
       call MPC_Allreduce( intin, intout, count, 
     *      MPC_INTEGER, MPC_SUM, comm, ierr )
              do 23001 i=1,count
        if (intout(i).ne.intsol(i)) then
            if (world_rank .eq. 0) print *, i, intout(i), ' <> ',
     *      intsol(i)
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23001	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *, 
     *      'Error for type MPC_INTEGER and op MPC_SUM'
	endif


       fnderr = .false.
       do 23002 i=1,count
	realin(i) = i
        realsol(i) = i*size
	realout(i) = 0
23002	continue
       call MPC_Allreduce( realin, realout, count, 
     *      MPC_REAL, MPC_SUM, comm, ierr )
              do 23003 i=1,count
        if (realout(i).ne.realsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23003	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_REAL and op MPC_SUM'
	endif


       fnderr = .false.
       do 23004 i=1,count
	dblein(i) = i
        dblesol(i) = i*size
	dbleout(i) = 0
23004	continue
       call MPC_Allreduce( dblein, dbleout, count, 
     *      MPC_DOUBLE_PRECISION, MPC_SUM, comm, ierr )
              do 23005 i=1,count
        if (dbleout(i).ne.dblesol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23005	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_DOUBLE_PRECISION and op MPC_SUM'
	endif


       fnderr = .false.
       do 23006 i=1,count
	cplxin(i) = i
        cplxsol(i) = i*size
	cplxout(i) = 0
23006	continue
       call MPC_Allreduce( cplxin, cplxout, count, 
     *      MPC_COMPLEX, MPC_SUM, comm, ierr )
              do 23007 i=1,count
        if (cplxout(i).ne.cplxsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23007	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_COMPLEX and op MPC_SUM'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *, 'Found ', 
     *       errcnt, ' errors on ', rank, ' for MPC_SUM'
	endif
	errcnt = 0

C Test product 
	if (world_rank .eq. 0) print *, ' MPC_PROD'

       fnderr = .false.
       do 23008 i=1,count
	intin(i) = i
        intsol(i) = (i)**(size)
	intout(i) = 0
23008	continue
       call MPC_Allreduce( intin, intout, count, 
     *      MPC_INTEGER, MPC_PROD, comm, ierr )
              do 23009 i=1,count
        if (intout(i).ne.intsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23009	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_INTEGER and op MPC_PROD'
	endif


       fnderr = .false.
       do 23010 i=1,count
	realin(i) = i
        realsol(i) = (i)**(size)
	realout(i) = 0
23010	continue
       call MPC_Allreduce( realin, realout, count, 
     *      MPC_REAL, MPC_PROD, comm, ierr )
              do 23011 i=1,count
        if (realout(i).ne.realsol(i)) then
            if (world_rank .eq. 0) print *, i, realout(i), ' <> ',
     *      realsol(i)
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23011	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_REAL and op MPC_PROD'
	endif


       fnderr = .false.
       do 23012 i=1,count
	dblein(i) = i
        dblesol(i) = (i)**(size)
	dbleout(i) = 0
23012	continue
       call MPC_Allreduce( dblein, dbleout, count, 
     *      MPC_DOUBLE_PRECISION, MPC_PROD, comm, ierr )
              do 23013 i=1,count
        if (dbleout(i).ne.dblesol(i)) then
            if (world_rank .eq. 0) print *, i, dbleout(i), ' <> ',
     *      dblesol(i)
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23013	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_DOUBLE_PRECISION and op MPC_PROD'
	endif


       fnderr = .false.
       do 23014 i=1,count
	cplxin(i) = i
        cplxsol(i) = (i)**(size)
	cplxout(i) = 0
23014	continue
       call MPC_Allreduce( cplxin, cplxout, count, 
     *      MPC_COMPLEX, MPC_PROD, comm, ierr )
              do 23015 i=1,count
        if (cplxout(i).ne.cplxsol(i)) then
            if (world_rank .eq. 0) print *, i, cplxout(i) ,' <> ',
     *      cplxsol(i)
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23015	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_COMPLEX and op MPC_PROD'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *, 'Found ', 
     *       errcnt, ' errors on ', rank, ' for MPC_PROD'
	endif
	errcnt = 0

C  Test max
	if (world_rank .eq. 0) print *, ' MPC_MAX'

       fnderr = .false.
       do 23016 i=1,count
	intin(i) = (rank + i)
        intsol(i) = (size - 1 + i)
	intout(i) = 0
23016	continue
       call MPC_Allreduce( intin, intout, count, 
     *      MPC_INTEGER, MPC_MAX, comm, ierr )
              do 23017 i=1,count
        if (intout(i).ne.intsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23017	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_INTEGER and op MPC_MAX'
	endif


       fnderr = .false.
       do 23018 i=1,count
	realin(i) = (rank + i)
        realsol(i) = (size - 1 + i)
	realout(i) = 0
23018	continue
       call MPC_Allreduce( realin, realout, count, 
     *      MPC_REAL, MPC_MAX, comm, ierr )
              do 23019 i=1,count
        if (realout(i).ne.realsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23019	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_REAL and op MPC_MAX'
	endif


       fnderr = .false.
       do 23020 i=1,count
	dblein(i) = (rank + i)
        dblesol(i) = (size - 1 + i)
	dbleout(i) = 0
23020	continue
       call MPC_Allreduce( dblein, dbleout, count, 
     *      MPC_DOUBLE_PRECISION, MPC_MAX, comm, ierr )
              do 23021 i=1,count
        if (dbleout(i).ne.dblesol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23021	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_DOUBLE_PRECISION and op MPC_MAX'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *, 'Found ',  
     *      errcnt, ' errors on ', rank, ' for MPC_MAX'
	endif
	errcnt = 0

C Test min 
	if (world_rank .eq. 0) print *, ' MPC_MIN'

       fnderr = .false.
       do 23022 i=1,count
	intin(i) = (rank + i)
        intsol(i) = i
	intout(i) = 0
23022	continue
       call MPC_Allreduce( intin, intout, count, 
     *      MPC_INTEGER, MPC_MIN, comm, ierr )
              do 23023 i=1,count
        if (intout(i).ne.intsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23023	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_INTEGER and op MPC_MIN'
	endif


       fnderr = .false.
       do 23024 i=1,count
	realin(i) = (rank + i)
        realsol(i) = i
	realout(i) = 0
23024	continue
       call MPC_Allreduce( realin, realout, count, 
     *      MPC_REAL, MPC_MIN, comm, ierr )
              do 23025 i=1,count
        if (realout(i).ne.realsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23025	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_REAL and op MPC_MIN'
	endif


       fnderr = .false.
       do 23026 i=1,count
	dblein(i) = (rank + i)
        dblesol(i) = i
	dbleout(i) = 0
23026	continue
       call MPC_Allreduce( dblein, dbleout, count, 
     *      MPC_DOUBLE_PRECISION, MPC_MIN, comm, ierr )
              do 23027 i=1,count
        if (dbleout(i).ne.dblesol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23027	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_DOUBLE_PRECISION and op MPC_MIN'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *, 'Found ',  
     *      errcnt, ' errors on ', rank, ' for MPC_MIN'
	endif
	errcnt = 0

C Test LOR
	if (world_rank .eq. 0) print *, ' MPC_LOR'

       fnderr = .false.
       do 23028 i=1,count
	login(i) = (mod(rank,2) .eq. 1)
        logsol(i) = (size .gt. 1)
	logout(i) = .FALSE.
23028	continue
       call MPC_Allreduce( login, logout, count, 
     *      MPC_LOGICAL, MPC_LOR, comm, ierr )
              do 23029 i=1,count
        if (logout(i).neqv.logsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23029	continue
       	if (fnderr) then
       	if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_LOGICAL and op MPC_LOR'
	endif


	if (errcnt .gt. 0) then
	   if (world_rank .eq. 0) print *,  
     *      'Found ', errcnt, ' errors on ', rank,
     *  	' for MPC_LOR(0)' 
	endif
	errcnt = 0



       fnderr = .false.
       do 23030 i=1,count
	login(i) = .false.
        logsol(i) = .false.
	logout(i) = .FALSE.
23030	continue
       call MPC_Allreduce( login, logout, count, 
     *      MPC_LOGICAL, MPC_LOR, comm, ierr )
              do 23031 i=1,count
        if (logout(i).neqv.logsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23031	continue
       	if (fnderr) then
       	if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_LOGICAL and op MPC_LOR'
	endif


	if (errcnt .gt. 0) then
	   if (world_rank .eq. 0) print *,  
     *      'Found ', errcnt, ' errors on ', rank,
     *              ' for MPC_LOR(1)'
	endif
	errcnt = 0

C Test LXOR 
	if (world_rank .eq. 0) print *, ' MPC_LXOR'

       fnderr = .false.
       do 23032 i=1,count
	login(i) = (rank .eq. 1)
        logsol(i) = (size .gt. 1)
	logout(i) = .FALSE.
23032	continue
       call MPC_Allreduce( login, logout, count, 
     *      MPC_LOGICAL, MPC_LXOR, comm, ierr )
              do 23033 i=1,count
        if (logout(i).neqv.logsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23033	continue
       	if (fnderr) then
       	if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_LOGICAL and op MPC_LXOR'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *, 'Found ', 
     *      errcnt,' errors on ', rank, ' for MPC_LXOR'
	endif
	errcnt = 0


       fnderr = .false.
       do 23034 i=1,count
	login(i) = .false.
        logsol(i) = .false.
	logout(i) = .FALSE.
23034	continue
       call MPC_Allreduce( login, logout, count, 
     *      MPC_LOGICAL, MPC_LXOR, comm, ierr )
              do 23035 i=1,count
        if (logout(i).neqv.logsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23035	continue
       	if (fnderr) then
       	if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_LOGICAL and op MPC_LXOR'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *,  
     *      'Found ',errcnt,' errors on ',rank,' for MPC_LXOR(0)'
	endif
	errcnt = 0


       fnderr = .false.
       do 23036 i=1,count
	login(i) = .true.
        logsol(i) = mod(size,2) .ne. 0 
	logout(i) = .FALSE.
23036	continue
       call MPC_Allreduce( login, logout, count, 
     *      MPC_LOGICAL, MPC_LXOR, comm, ierr )
              do 23037 i=1,count
        if (logout(i).neqv.logsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23037	continue
       	if (fnderr) then
       	if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_LOGICAL and op MPC_LXOR'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *,  
     *      'Found ',errcnt,' errors on ',rank,' for MPC_LXOR(1-0)'
	endif
	errcnt = 0

C Test LAND 
	if (world_rank .eq. 0) print *, ' MPC_LAND'

       fnderr = .false.
       do 23038 i=1,count
	login(i) = (mod(rank,2) .eq. 1)
        logsol(i) = .false.
	logout(i) = .FALSE.
23038	continue
       call MPC_Allreduce( login, logout, count, 
     *      MPC_LOGICAL, MPC_LAND, comm, ierr )
              do 23039 i=1,count
        if (logout(i).neqv.logsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23039	continue
       	if (fnderr) then
       	if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_LOGICAL and op MPC_LAND'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *, 'Found ', 
     *       errcnt, ' errors on ', rank, ' for MPC_LAND'
	endif
	errcnt = 0




       fnderr = .false.
       do 23040 i=1,count
	login(i) = .true.
        logsol(i) = .true.
	logout(i) = .FALSE.
23040	continue
       call MPC_Allreduce( login, logout, count, 
     *      MPC_LOGICAL, MPC_LAND, comm, ierr )
              do 23041 i=1,count
        if (logout(i).neqv.logsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23041	continue
       	if (fnderr) then
       	if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_LOGICAL and op MPC_LAND'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *,  
     *      'Found ',errcnt,' errors on ',rank,
     *      ' for MPC_LAND(true)'
	endif
	errcnt = 0
	
C Test BOR
	if (world_rank .eq. 0) print *, ' MPC_BOR'
        if (size .lt. 3) then

       fnderr = .false.
       do 23042 i=1,count
	intin(i) = mod(rank,4)
        intsol(i) = size - 1
	intout(i) = 0
23042	continue
       call MPC_Allreduce( intin, intout, count, 
     *      MPC_INTEGER, MPC_BOR, comm, ierr )
              do 23043 i=1,count
        if (intout(i).ne.intsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23043	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_INTEGER and op MPC_BOR'
	endif

	else

       fnderr = .false.
       do 23044 i=1,count
	intin(i) = mod(rank,4)
        intsol(i) = 3
	intout(i) = 0
23044	continue
       call MPC_Allreduce( intin, intout, count, 
     *      MPC_INTEGER, MPC_BOR, comm, ierr )
              do 23045 i=1,count
        if (intout(i).ne.intsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23045	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_INTEGER and op MPC_BOR'
	endif

	endif
	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *, 'Found ',  
     *      errcnt, ' errors on ', rank,
     *      	 ' for MPC_BOR(1)'
	endif
	errcnt = 0

C Test BAND 
	if (world_rank .eq. 0) print *, ' MPC_BAND'
C See bottom for function definitions

       fnderr = .false.
       do 23046 i=1,count
	intin(i) = ibxandval(rank,size,i)
        intsol(i) = i
	intout(i) = 0
23046	continue
       call MPC_Allreduce( intin, intout, count, 
     *      MPC_INTEGER, MPC_BAND, comm, ierr )
              do 23047 i=1,count
        if (intout(i).ne.intsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23047	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_INTEGER and op MPC_BAND'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *, 'Found ',  
     *      errcnt, ' errors on ', rank, 
     *		' for MPC_BAND(1)'
	endif
	errcnt = 0


       fnderr = .false.
       do 23048 i=1,count
	intin(i) = ibxandval1(rank,size,i)
        intsol(i) = 0
	intout(i) = 0
23048	continue
       call MPC_Allreduce( intin, intout, count, 
     *      MPC_INTEGER, MPC_BAND, comm, ierr )
              do 23049 i=1,count
        if (intout(i).ne.intsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23049	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_INTEGER and op MPC_BAND'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *, 'Found ',  
     *      errcnt, ' errors on ', rank, 
     *		' for MPC_BAND(0)'
	endif
	errcnt = 0

C Test BXOR 
	if (world_rank .eq. 0) print *, ' MPC_BXOR'
C See below for function definitions

       fnderr = .false.
       do 23050 i=1,count
	intin(i) = ibxorval1(rank)
        intsol(i) = ibxorsol1(size)
	intout(i) = 0
23050	continue
       call MPC_Allreduce( intin, intout, count, 
     *      MPC_INTEGER, MPC_BXOR, comm, ierr )
              do 23051 i=1,count
        if (intout(i).ne.intsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23051	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_INTEGER and op MPC_BXOR'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *, 'Found ',  
     *      errcnt, ' errors on ', rank, 
     *		' for MPC_BXOR(0)'
	endif
	errcnt = 0


       fnderr = .false.
       do 23052 i=1,count
	intin(i) = 0
        intsol(i) = 0
	intout(i) = 0
23052	continue
       call MPC_Allreduce( intin, intout, count, 
     *      MPC_INTEGER, MPC_BXOR, comm, ierr )
              do 23053 i=1,count
        if (intout(i).ne.intsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23053	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_INTEGER and op MPC_BXOR'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *, 'Found ',  
     *      errcnt, ' errors on ', rank, 
     *		' for MPC_BXOR(0)'
	endif
	errcnt = 0

C Assumes -1 == all bits set

       fnderr = .false.
       do 23054 i=1,count
	intin(i) = (-1)
	if (mod(size,2) .eq. 0) then
            intsol(i) = 0
        else
            intsol(i) = -1
        endif
	intout(i) = 0
23054	continue
       call MPC_Allreduce( intin, intout, count, 
     *      MPC_INTEGER, MPC_BXOR, comm, ierr )
              do 23055 i=1,count
        if (intout(i).ne.intsol(i)) then
            errcnt = errcnt + 1
            fnderr = .true. 
        endif
23055	continue
       	if (fnderr) then
	  if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_INTEGER and op MPC_BXOR'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *,  
     *      'Found ', errcnt, ' errors on ', rank, 
     *		' for MPC_BXOR(1-0)'
	endif
	errcnt = 0

C Test Maxloc 
	if (world_rank .eq. 0) print *, ' MPC_MAXLOC'

	fnderr = .false.
	do 23056 i=1, count
	   intin(2*i-1) = (rank + i)
	   intin(2*i)   = rank
	   intsol(2*i-1) = (size - 1 + i)
	   intsol(2*i) = (size-1)
	   intout(2*i-1) = 0
	   intout(2*i)   = 0
23056	continue
         	call MPC_Allreduce( intin, intout, count, 
     *      MPC_2INTEGER, MPC_MAXLOC, comm, ierr )
	do 23057 i=1, count
	if (intout(2*i-1) .ne. intsol(2*i-1) .or.
     *      intout(2*i) .ne. intsol(2*i)) then
	    errcnt = errcnt + 1
	    fnderr = .true. 
	endif
23057	continue
       	if (fnderr) then
	if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_2INTEGER and op MPC_MAXLOC'
	endif


	fnderr = .false.
	do 23058 i=1, count
	   realin(2*i-1) = (rank + i)
	   realin(2*i)   = rank
	   realsol(2*i-1) = (size - 1 + i)
	   realsol(2*i) = (size-1)
	   realout(2*i-1) = 0
	   realout(2*i)   = 0
23058	continue
         	call MPC_Allreduce( realin, realout, count, 
     *      MPC_2REAL, MPC_MAXLOC, comm, ierr )
	do 23059 i=1, count
	if (realout(2*i-1) .ne. realsol(2*i-1) .or.
     *      realout(2*i) .ne. realsol(2*i)) then
	    errcnt = errcnt + 1
	    fnderr = .true. 
	endif
23059	continue
       	if (fnderr) then
	if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_2REAL and op MPC_MAXLOC'
	endif


	fnderr = .false.
	do 23060 i=1, count
	   dblein(2*i-1) = (rank + i)
	   dblein(2*i)   = rank
	   dblesol(2*i-1) = (size - 1 + i)
	   dblesol(2*i) = (size-1)
	   dbleout(2*i-1) = 0
	   dbleout(2*i)   = 0
23060	continue
         	call MPC_Allreduce( dblein, dbleout, count, 
     *      MPC_2DOUBLE_PRECISION, MPC_MAXLOC, comm, ierr )
	do 23061 i=1, count
	if (dbleout(2*i-1) .ne. dblesol(2*i-1) .or.
     *      dbleout(2*i) .ne. dblesol(2*i)) then
	    errcnt = errcnt + 1
	    fnderr = .true. 
	endif
23061	continue
       	if (fnderr) then
	   if (world_rank .eq. 0) print *,
     *     'Error for type MPC_2DOUBLE_PRECISION and op MPC_MAXLOC'

	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *, 'Found ', 
     *       errcnt, ' errors on ', rank, 
     *		' for MPC_MAXLOC'
	endif
	errcnt = 0

C Test minloc 
	if (world_rank .eq. 0) print *, ' MPC_MINLOC'


	fnderr = .false.
	do 23062 i=1, count
	   intin(2*i-1) = (rank + i)
	   intin(2*i)   = rank
	   intsol(2*i-1) = i
	   intsol(2*i) = 0
	   intout(2*i-1) = 0
	   intout(2*i)   = 0
23062	continue
         	call MPC_Allreduce( intin, intout, count, 
     *      MPC_2INTEGER, MPC_MINLOC, comm, ierr )
	do 23063 i=1, count
	if (intout(2*i-1) .ne. intsol(2*i-1) .or.
     *      intout(2*i) .ne. intsol(2*i)) then
	    errcnt = errcnt + 1
	    fnderr = .true. 
	endif
23063	continue
       	if (fnderr) then
	if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_2INTEGER and op MPC_MINLOC'
	endif


	fnderr = .false.
	do 23064 i=1, count
	   realin(2*i-1) = (rank + i)
	   realin(2*i)   = rank
	   realsol(2*i-1) = i
	   realsol(2*i) = 0
	   realout(2*i-1) = 0
	   realout(2*i)   = 0
23064	continue
         	call MPC_Allreduce( realin, realout, count, 
     *      MPC_2REAL, MPC_MINLOC, comm, ierr )
	do 23065 i=1, count
	if (realout(2*i-1) .ne. realsol(2*i-1) .or.
     *      realout(2*i) .ne. realsol(2*i)) then
	    errcnt = errcnt + 1
	    fnderr = .true. 
	endif
23065	continue
       	if (fnderr) then
	if (world_rank .eq. 0) print *,  
     *      'Error for type MPC_2REAL and op MPC_MINLOC'
	endif


	fnderr = .false.
	do 23066 i=1, count
	   dblein(2*i-1) = (rank + i)
	   dblein(2*i)   = rank
	   dblesol(2*i-1) = i
	   dblesol(2*i) = 0
	   dbleout(2*i-1) = 0
	   dbleout(2*i)   = 0
23066	continue
         	call MPC_Allreduce( dblein, dbleout, count, 
     *      MPC_2DOUBLE_PRECISION, MPC_MINLOC, comm, ierr )
	do 23067 i=1, count
	if (dbleout(2*i-1) .ne. dblesol(2*i-1) .or.
     *      dbleout(2*i) .ne. dblesol(2*i)) then
	    errcnt = errcnt + 1
	    fnderr = .true. 
	endif
23067	continue
       	if (fnderr) then
	   if (world_rank .eq. 0) print *,
     *      'Error for type MPC_2DOUBLE_PRECISION and op MPC_MINLOC'
	endif


	if (errcnt .gt. 0) then
	if (world_rank .eq. 0) print *,  
     *      'Found ', errcnt, ' errors on ', rank, 
     *		' for MPC_MINLOC'
	endif
	errcnt = 0

	call MPC_Finalize( ierr )
	end

	integer function ibxorval1( ir )
	ibxorval1 = 0
	if (ir .eq. 1) ibxorval1 = 16+32+64+128
	return
	end

	integer function ibxorsol1( is )
	ibxorsol1 = 0
	if (is .gt. 1) ibxorsol1 = 16+32+64+128
	return
	end

C
C	Assumes -1 == all bits set
	integer function ibxandval( ir, is, i )
	integer ir, is, i
	ibxandval = -1
	if (ir .eq. is - 1) ibxandval = i
	return
	end
C
	integer function ibxandval1( ir, is, i )
	integer ir, is, i
	ibxandval1 = 0
	if (ir .eq. is - 1) ibxandval1 = i
	return
	end
 
