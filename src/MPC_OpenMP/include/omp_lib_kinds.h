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
