# Fortran setup: -fallow-argument-mismatch check and symbol naming convention detection

# Fortran -fallow-argument-mismatch check (matches autotools)
if(MPC_ENABLE_FORTRAN)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/fortran_test_check.f90" "PROGRAM foo\nEND PROGRAM\n")
    try_compile(FORTRAN_ARG_MISMATCH_SUPPORTED
        "${CMAKE_CURRENT_BINARY_DIR}/fortran_test_compile"
        "${CMAKE_CURRENT_BINARY_DIR}/fortran_test_check.f90"
        CMAKE_FLAGS "CMAKE_TRY_COMPILE_TARGET_TYPE:STATIC_LIBRARY"
        COMPILE_DEFINITIONS "-fallow-argument-mismatch"
    )
    if(FORTRAN_ARG_MISMATCH_SUPPORTED)
        set(FORTRAN_ARG_MISMATCH_ARG "-fallow-argument-mismatch")
    else()
        set(FORTRAN_ARG_MISMATCH_ARG "")
    endif()
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/fortran_test_check.f90")
else()
    set(FORTRAN_ARG_MISMATCH_ARG "")
endif()

# Fortran symbol naming convention detection (matches autotools)
if(MPC_ENABLE_FORTRAN)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/fortran_name_test.f90" "      subroutine fortran_name()\n      end subroutine\n")
    try_compile(FORTRAN_NAME_COMPILE
        "${CMAKE_CURRENT_BINARY_DIR}/fortran_name_test"
        "${CMAKE_CURRENT_BINARY_DIR}/fortran_name_test.f90"
        CMAKE_FLAGS "CMAKE_TRY_COMPILE_TARGET_TYPE:STATIC_LIBRARY"
    )
    if(FORTRAN_NAME_COMPILE)
        execute_process(
            COMMAND nm "${CMAKE_CURRENT_BINARY_DIR}/fortran_name_test/libfortran_name_test.a"
            OUTPUT_VARIABLE FORTRAN_NAME_SYMBOLS
            ERROR_QUIET
        )
        if(FORTRAN_NAME_SYMBOLS MATCHES "fortran_name__")
            set(FORTRANDOUBLEUNDERSCORE 1)
            message(STATUS "Fortran symbol format: __")
        elseif(FORTRAN_NAME_SYMBOLS MATCHES "fortran_name_")
            set(FORTRANUNDERSCORE 1)
            message(STATUS "Fortran symbol format: _")
        elseif(FORTRAN_NAME_SYMBOLS MATCHES "fortran_name")
            set(FORTRANLOWER 1)
            message(STATUS "Fortran symbol format: lowercase")
        elseif(FORTRAN_NAME_SYMBOLS MATCHES "FORTRAN_NAME")
            set(FORTRANCAPS 1)
            message(STATUS "Fortran symbol format: uppercase")
        else()
            # Default to underscore convention (most common for GCC)
            set(FORTRANUNDERSCORE 1)
            message(STATUS "Fortran symbol format: _ (default)")
        endif()
    else()
        set(FORTRANUNDERSCORE 1)
        message(STATUS "Fortran symbol format: _ (default, compile test failed)")
    endif()
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/fortran_name_test.f90")
endif()
