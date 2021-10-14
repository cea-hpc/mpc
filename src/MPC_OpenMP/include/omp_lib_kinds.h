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
/* # - Ricardo Bispo vieira <ricardo.bispo-vieira@exascale-computing.eu>  # */
/* #                                                                      # */
/* ######################################################################## */
!Omp lib kinds header for locks and scheduling policies
        integer omp_lock_kind
        integer omp_nest_lock_kind

!this selects an integer that is large enough to hold a 64 bit integer
        parameter(omp_lock_kind = selected_int_kind(8))
        parameter(omp_nest_lock_kind = selected_int_kind(8))

        integer omp_sched_kind

!this selects an integer that is large enough to hold a 32 bit integer
        parameter(omp_sched_kind = selected_int_kind(8))
        integer(omp_sched_kind) omp_sched_static
        parameter(omp_sched_static = 1)
        integer(omp_sched_kind) omp_sched_dynamic
        parameter(omp_sched_dynamic = 2)
        integer(omp_sched_kind) omp_sched_guided
        parameter(omp_sched_guided = 3)
        integer(omp_sched_kind) omp_sched_auto
        parameter(omp_sched_auto = 4)

        integer omp_control_tool_result_kind

!this selects an integer that is large enough to hold a 64 bit integer
        parameter(omp_control_tool_result_kind = selected_int_kind(8))
        INTEGER, PARAMETER :: omp_control_tool_notool = -2
        INTEGER, PARAMETER :: omp_control_tool_nocallback = -1
        INTEGER, PARAMETER :: omp_control_tool_success = 0
        INTEGER, PARAMETER :: omp_control_tool_ignored = 1

        integer omp_control_tool_kind

!this selects an integer that is large enough to hold a 64 bit integer
        parameter(omp_control_tool_kind = selected_int_kind(8))
        INTEGER, PARAMETER :: omp_control_tool_start = 1
        INTEGER, PARAMETER :: omp_control_tool_pause = 2
        INTEGER, PARAMETER :: omp_control_tool_flush = 3
        INTEGER, PARAMETER :: omp_control_tool_end = 4
