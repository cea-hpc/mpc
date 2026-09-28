# MPC_Doc/CMakeLists.txt
# Generate and install man pages

include(GNUInstallDirs)

# Compute manpage date from generation timestamp
string(TIMESTAMP _MPC_MANPAGE_DATE "%m/%Y")

# MPC man pages (always built)
set(_MPC_MAN_PAGES
    mpc_cc.1
    mpc_cxx.1
    mpc_f77.1
    mpc_status.1
    mpc_f08.1
    mpc_f90.1
)

foreach(_MANPAGE IN LISTS _MPC_MAN_PAGES)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/man/${_MANPAGE}in" "${CMAKE_CURRENT_BINARY_DIR}/${_MANPAGE}" @ONLY)
    # Apply substitutions that configure_file can't handle
    file(READ "${CMAKE_CURRENT_BINARY_DIR}/${_MANPAGE}" _MANPAGE_CONTENT)
    string(REPLACE "#PACKAGE_DATE#" "${_MPC_MANPAGE_DATE}" _MANPAGE_CONTENT "${_MANPAGE_CONTENT}")
    string(REPLACE "#PACKAGE_VERSION#" "${MPC_VERSION_STRING}" _MANPAGE_CONTENT "${_MANPAGE_CONTENT}")
    string(REPLACE "#PACKAGE_NAME#" ${CMAKE_PROJECT_DESCRIPTION} _MANPAGE_CONTENT "${_MANPAGE_CONTENT}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${_MANPAGE}" "${_MANPAGE_CONTENT}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${_MANPAGE}" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
endforeach()

# MPI man pages (conditional on MPC_ENABLE_MPI)
if(MPC_ENABLE_MPI)
    file(GLOB _MPI_MAN_TEMPLATES "${CMAKE_CURRENT_SOURCE_DIR}/man/MPI_*.3in")
    foreach(_TEMPLATE IN LISTS _MPI_MAN_TEMPLATES)
        get_filename_component(_NAME "${_TEMPLATE}" NAME_WE)
        set(_OUTFILE "${CMAKE_CURRENT_BINARY_DIR}/${_NAME}.3")

        # Concatenate template + footer
        file(READ "${_TEMPLATE}" _MPI_CONTENT)
        file(READ "${CMAKE_CURRENT_SOURCE_DIR}/man/mpi_std_footer.3" _MPI_FOOTER)
        set(_MPI_CONTENT "${_MPI_CONTENT}${_MPI_FOOTER}")

        # Apply substitutions
        string(REPLACE "#PACKAGE_DATE#" "${_MPC_MANPAGE_DATE}" _MPI_CONTENT "${_MPI_CONTENT}")
        string(REPLACE "#PACKAGE_VERSION#" "${MPC_VERSION_STRING}" _MPI_CONTENT "${_MPI_CONTENT}")
        string(REPLACE "#PACKAGE_NAME#" ${CMAKE_PROJECT_DESCRIPTION} _MPI_CONTENT "${_MPI_CONTENT}")
        file(WRITE "${_OUTFILE}" "${_MPI_CONTENT}")

        install(FILES "${_OUTFILE}" DESTINATION "${CMAKE_INSTALL_MANDIR}/man3")
    endforeach()
endif()
