/* ############################# MPC License ############################## */
/* # Tue Oct 12 12:25:58 CEST 2021                                        # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Julien Adam <adamj@paratools.com>                                  # */
/* # - Patrick Carribault <patrick.carribault@cea.fr>                     # */
/* #                                                                      # */
/* ######################################################################## */
! default integer type assumed below 
! default logical type assumed below 
! OpenMP API v3.1

         include 'omp_lib_kinds.h'
         integer openmp_version
         parameter ( openmp_version = 201107 )
         external omp_set_num_threads 
         external omp_get_num_threads 
         integer omp_get_num_threads 
         external omp_get_max_threads 
         integer omp_get_max_threads 
         external omp_get_thread_num 
         integer omp_get_thread_num 
         external omp_get_num_procs 
         integer omp_get_num_procs 
         external omp_in_parallel 
         logical omp_in_parallel 
         external omp_set_dynamic 
         external omp_get_dynamic 
         logical omp_get_dynamic 
         external omp_set_nested 
         external omp_get_nested 
         logical omp_get_nested 
         external omp_set_schedule 
         external omp_get_schedule 
         external omp_get_thread_limit
         integer omp_get_thread_limit
         external omp_set_max_active_levels
         external omp_get_max_active_levels
         integer omp_get_max_active_levels
         external omp_get_level
         integer omp_get_level
         external omp_get_ancestor_thread_num
         integer omp_get_ancestor_thread_num
         external omp_get_team_size
         integer omp_get_team_size
         external omp_get_active_level
         integer omp_get_active_level
         
         external omp_in_final
         logical omp_in_final

         external omp_control_tool
         integer omp_control_tool
         
         external omp_init_lock
         external omp_destroy_lock
         external omp_set_lock
         external omp_unset_lock
         external omp_test_lock
         logical omp_test_lock
         
         external omp_init_nest_lock
         external omp_destroy_nest_lock
         external omp_set_nest_lock
         external omp_unset_nest_lock
         external omp_test_nest_lock
         integer omp_test_nest_lock
         
         external omp_get_wtick
         double precision omp_get_wtick
         external omp_get_wtime
         double precision omp_get_wtime
