# ==================================================================
# RuntimeConstants.cmake — Runtime constant extraction and feature detection
# ==================================================================

# ------------------------------------------------------------------
# Helper: extract C preprocessor define values at runtime
# ------------------------------------------------------------------
function(mpc_extract_define HEADER DEFINE CMAKE_VAR)
    set(_TEST_SRC
        "
#include <${HEADER}>
#include <stdio.h>
int main() {
    long long val = (long long) ${DEFINE};
    FILE *f = fopen(\"${CMAKE_CURRENT_BINARY_DIR}/define_val.out\", \"w\");
    if (!f) return 1;
    fprintf(f, \"%lld\\n\", val);
    fclose(f);
    return 0;
}
"
    )
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/define_test.c" "${_TEST_SRC}")
    try_run(
        _RUN_RES
        _COMPILE_RES
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/define_test.c"
        COMPILE_OUTPUT_VARIABLE _COMPILE_OUT
        RUN_OUTPUT_VARIABLE _RUN_OUT
    )
    if(_COMPILE_RES AND _RUN_RES EQUAL 0)
        file(READ "${CMAKE_CURRENT_BINARY_DIR}/define_val.out" _VAL)
        string(STRIP "${_VAL}" _VAL)
        set(${CMAKE_VAR} "${_VAL}" PARENT_SCOPE)
    else()
        set(${CMAKE_VAR} "0" PARENT_SCOPE)
    endif()
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/define_test.c" "${CMAKE_CURRENT_BINARY_DIR}/define_val.out")
endfunction()

# ------------------------------------------------------------------
# Runtime constants - signal
# ------------------------------------------------------------------
# NSIG / _NSIG
mpc_extract_define("signal.h" "NSIG" MPC_CONFIG_NSIG)
if(NOT MPC_CONFIG_NSIG OR MPC_CONFIG_NSIG STREQUAL "0")
    mpc_extract_define("signal.h" "_NSIG" MPC_CONFIG_NSIG)
    if(NOT MPC_CONFIG_NSIG OR MPC_CONFIG_NSIG STREQUAL "0")
        set(MPC_CONFIG_NSIG "32")
    endif()
endif()

# ------------------------------------------------------------------
# Runtime constants - pthreads
# ------------------------------------------------------------------
# Mutex types
mpc_extract_define("pthread.h" "PTHREAD_MUTEX_NORMAL" MPC_CONFIG_PTHREAD_MUTEX_NORMAL)
mpc_extract_define("pthread.h" "PTHREAD_MUTEX_RECURSIVE" MPC_CONFIG_PTHREAD_MUTEX_RECURSIVE)
mpc_extract_define("pthread.h" "PTHREAD_MUTEX_ERRORCHECK" MPC_CONFIG_PTHREAD_MUTEX_ERRORCHECK)
mpc_extract_define("pthread.h" "PTHREAD_MUTEX_DEFAULT" MPC_CONFIG_PTHREAD_MUTEX_DEFAULT)

# Priority inheritance
mpc_extract_define("pthread.h" "PTHREAD_PRIO_NONE" MPC_CONFIG_PTHREAD_PRIO_NONE)
mpc_extract_define("pthread.h" "PTHREAD_PRIO_INHERIT" MPC_CONFIG_PTHREAD_PRIO_INHERIT)
mpc_extract_define("pthread.h" "PTHREAD_PRIO_PROTECT" MPC_CONFIG_PTHREAD_PRIO_PROTECT)

# Thread creation attributes
mpc_extract_define("pthread.h" "PTHREAD_CREATE_JOINABLE" MPC_CONFIG_PTHREAD_CREATE_JOINABLE)
mpc_extract_define("pthread.h" "PTHREAD_CREATE_DETACHED" MPC_CONFIG_PTHREAD_CREATE_DETACHED)

# Scheduling attributes
mpc_extract_define("pthread.h" "PTHREAD_INHERIT_SCHED" MPC_CONFIG_PTHREAD_INHERIT_SCHED)
mpc_extract_define("pthread.h" "PTHREAD_EXPLICIT_SCHED" MPC_CONFIG_PTHREAD_EXPLICIT_SCHED)
mpc_extract_define("pthread.h" "PTHREAD_SCOPE_SYSTEM" MPC_CONFIG_PTHREAD_SCOPE_SYSTEM)
mpc_extract_define("pthread.h" "PTHREAD_SCOPE_PROCESS" MPC_CONFIG_PTHREAD_SCOPE_PROCESS)

# Shared/private attributes
mpc_extract_define("pthread.h" "PTHREAD_PROCESS_PRIVATE" MPC_CONFIG_PTHREAD_PROCESS_PRIVATE)
mpc_extract_define("pthread.h" "PTHREAD_PROCESS_SHARED" MPC_CONFIG_PTHREAD_PROCESS_SHARED)

# Cancellation
mpc_extract_define("pthread.h" "PTHREAD_CANCEL_ENABLE" MPC_CONFIG_PTHREAD_CANCEL_ENABLE)
mpc_extract_define("pthread.h" "PTHREAD_CANCEL_DISABLE" MPC_CONFIG_PTHREAD_CANCEL_DISABLE)
mpc_extract_define("pthread.h" "PTHREAD_CANCEL_DEFERRED" MPC_CONFIG_PTHREAD_CANCEL_DEFERRED)
mpc_extract_define("pthread.h" "PTHREAD_BARRIER_SERIAL_THREAD" MPC_CONFIG_PTHREAD_BARRIER_SERIAL_THREAD)
mpc_extract_define("pthread.h" "PTHREAD_CANCEL_ASYNCHRONOUS" MPC_CONFIG_PTHREAD_CANCEL_ASYNCHRONOUS)
mpc_extract_define("pthread.h" "PTHREAD_CANCELED" MPC_CONFIG_PTHREAD_CANCELED)

# Thread keys limit
mpc_extract_define("pthread.h" "PTHREAD_KEYS_MAX" MPC_THREAD_KEYS_MAX)
if(NOT MPC_THREAD_KEYS_MAX OR MPC_THREAD_KEYS_MAX STREQUAL "0")
    set(MPC_THREAD_KEYS_MAX 1024)
endif()

# PTHREAD_STACK_MIN
mpc_extract_define("pthread.h" "PTHREAD_STACK_MIN" MPC_CONFIG_PTHREAD_STACK_MIN)
if(NOT MPC_CONFIG_PTHREAD_STACK_MIN OR MPC_CONFIG_PTHREAD_STACK_MIN STREQUAL "0")
    mpc_extract_define("limits.h" "PTHREAD_STACK_MIN" MPC_CONFIG_PTHREAD_STACK_MIN)
    if(NOT MPC_CONFIG_PTHREAD_STACK_MIN OR MPC_CONFIG_PTHREAD_STACK_MIN STREQUAL "0")
        set(MPC_CONFIG_PTHREAD_STACK_MIN "8192")
    endif()
endif()

# ------------------------------------------------------------------
# Runtime constants - scheduling
# ------------------------------------------------------------------
mpc_extract_define("sched.h" "SCHED_OTHER" MPC_CONFIG_SCHED_OTHER)
mpc_extract_define("sched.h" "SCHED_RR" MPC_CONFIG_SCHED_RR)
mpc_extract_define("sched.h" "SCHED_FIFO" MPC_CONFIG_SCHED_FIFO)

# ------------------------------------------------------------------
# Runtime constants - semaphores
# ------------------------------------------------------------------
mpc_extract_define("semaphore.h" "SEM_VALUE_MAX" MPC_CONFIG_SEM_VALUE_MAX)
if(NOT MPC_CONFIG_SEM_VALUE_MAX OR MPC_CONFIG_SEM_VALUE_MAX STREQUAL "0")
    set(MPC_CONFIG_SEM_VALUE_MAX "2147483647")
endif()
mpc_extract_define("semaphore.h" "SEM_FAILED" MPC_CONFIG_SEM_FAILED)

# ------------------------------------------------------------------
# Compiler feature detection - compiler ID
# ------------------------------------------------------------------
check_c_source_compiles(
    "
#ifndef __GNUC__
#error
#endif
int main() { return 0; }
"
    MPC_COMPILER_IS_GCC
)

check_c_source_compiles(
    "
#ifndef __clang__
#error
#endif
int main() { return 0; }
"
    MPC_COMPILER_IS_CLANG
)

# ------------------------------------------------------------------
# Compiler feature detection - language features
# ------------------------------------------------------------------
# TLS support
set(CMAKE_REQUIRED_FLAGS "")
check_c_source_compiles(
    "
int main() {
    static _Thread_local int x = 0;
    return x;
}
"
    TLS_SUPPORT
)
if(TLS_SUPPORT)
    set(TLS_SUPPORT 1)
else()
    set(TLS_SUPPORT 0)
endif()

# pragma message support
set(_OLD_CFLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
check_c_source_compiles(
    "
int main() {
    #pragma message \"test\"
    return 0;
}
"
    HAVE_PRAGMA_MESSAGE
)
set(CMAKE_C_FLAGS "${_OLD_CFLAGS}")

# visibility attribute
check_c_source_compiles(
    "
__attribute__((visibility(\"default\"))) int foo(void);
int main() { return 0; }
"
    HAVE_VISIBILITY
)
if(HAVE_VISIBILITY)
    set(HAVE_VISIBILITY 1)
else()
    set(HAVE_VISIBILITY 0)
endif()

# __builtin_expect support
check_c_source_compiles(
    "
int main() {
    int x = 1;
    return __builtin_expect(x, 0);
}
"
    HAVE_BUILTIN_EXPECT
)
if(HAVE_BUILTIN_EXPECT)
    set(HAVE_BUILTIN_EXPECT 1)
else()
    set(HAVE_BUILTIN_EXPECT 0)
endif()

# ------------------------------------------------------------------
# Compiler feature detection - macros and variables
# ------------------------------------------------------------------
# __func__ support
check_c_source_compiles(
    "
#include <stdio.h>
int main() {
    printf(\"%s\", __func__);
    return 0;
}
"
    HAVE___FUNC__
)
if(HAVE___FUNC__)
    set(MPC_ARCH_DETECTED_FUNCTION "__func__")
else()
    message(FATAL_ERROR "Unable to detect function name macro")
endif()

# __STRING support - GCC and Clang both support this built-in preprocessor macro
if(MPC_COMPILER_IS_GCC OR MPC_COMPILER_IS_CLANG OR CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(MPC_ARCH_DETECTED_STRING "__STRING")
else()
    message(FATAL_ERROR "Missing __STRING macro (GCC/Clang required)")
endif()

# environ variable
check_c_source_compiles(
    "
#include <stdlib.h>
extern char **environ;
int main() { return (int)(long)environ; }
"
    HAVE_ENVIRON_VAR
)

# ------------------------------------------------------------------
# Process mode and extls detection
# ------------------------------------------------------------------
option(MPC_ENABLE_PROCESS_MODE "Compile MPC in process mode (OFF=try extls first)" OFF)

# Detect extls (autopriv) for TLS privatization
pkg_check_modules(EXTLS autopriv)
if(EXTLS_FOUND)
    set(HAVE_EXTLS YES)
    set(MPC_USE_EXTLS YES)
    set(EXTLS_CFLAGS "${EXTLS_CFLAGS}")
else()
    set(HAVE_EXTLS NO)
    unset(MPC_USE_EXTLS)
    set(EXTLS_CFLAGS "")
endif()

if(MPC_ENABLE_PROCESS_MODE)
    set(MPC_IN_PROCESS_MODE "yes")
    set(MPC_IN_PROCESS_MODE_INT 1)
elseif(NOT HAVE_EXTLS)
    # No extls available, fall back to process mode (matches autotools)
    message(WARNING "No extls available, fall back to process mode.")
    set(MPC_IN_PROCESS_MODE "yes")
    set(MPC_IN_PROCESS_MODE_INT 1)
else()
    unset(MPC_IN_PROCESS_MODE)
    set(MPC_IN_PROCESS_MODE_INT 0)
endif()

# ------------------------------------------------------------------
# Launcher program detection
# ------------------------------------------------------------------
set(MPC_LAUNCHER_PROGRAM "none")
if(MPC_LAUNCHER STREQUAL "HYDRA")
    set(MPC_LAUNCHER_PROGRAM "HYDRA")
endif()

# SRUN support
find_program(SRUN_IN_PATH srun)
set(MPC_HAS_SRUN_SUPPORT NO)
if(SRUN_IN_PATH AND (MPC_LAUNCHER STREQUAL "PMIX" OR MPC_LAUNCHER STREQUAL "PMI1"))
    set(MPC_HAS_SRUN_SUPPORT YES)
endif()

# PRRTE launcher program
if(MPC_LAUNCHER STREQUAL "PMIX")
    find_program(PRTERUN prterun)
    if(NOT PRTERUN)
        find_program(PRTERUN prun)
    endif()
    if(PRTERUN)
        set(MPC_LAUNCHER_PROGRAM "PRRTE")
        set(MPC_LAUNCHER_PROGRAM_PRRTE YES)
    endif()
endif()
