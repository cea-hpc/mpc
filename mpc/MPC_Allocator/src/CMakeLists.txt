######################### COPYING FUNCTION #######################
function(cm_export_file FILE DEST)
        if(NOT TARGET export-files)
                add_custom_target(export-files ALL COMMENT "Copying libraries into tests folder")
        endif (NOT TARGET export-files)
        get_filename_component(FILENAME "${FILE}" NAME)
        add_custom_command(TARGET export-files COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FILE}" "${DEST}/${FILENAME}")
        install(FILES "${FILE}" DESTINATION "${DEST}")
endfunction (cm_export_file)

#################### DEFINE SOURCES #####################
Message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	set(alloc_SRC sctk_alloc_debug.c sctk_allocator.c sctk_alloc_stats.c sctk_alloc_spy.c sctk_alloc_posix.c sctk_alloc_topology.c sctk_alloc_light_mm_source.c sctk_alloc_common.c sctk_alloc_numa_stat.c)
else()
	set(alloc_SRC sctk_allocator.c sctk_alloc_posix.c sctk_alloc_stats.c sctk_alloc_spy.c sctk_alloc_topology.c sctk_alloc_light_mm_source.c sctk_alloc_common.c sctk_alloc_numa_stat.c)
endif()

################# CHECK SOME INCLUDE FILES #############
include(CheckIncludeFile)
CHECK_INCLUDE_FILE (numaif.h HAVE_NUMAIF_H)
if(HAVE_NUMAIF_H)
	add_definitions(-DHAVE_NUMAIF_H)
endif(HAVE_NUMAIF_H)

#################### CHECK PAGEMAP ######################
if (EXISTS /proc/self/pagemap)
	add_definitions(-DHAVE_LINUX_PAGEMAP_H)
endif(EXISTS /proc/self/pagemap)

#################### ADD INCLUDE DIRS ###################
include_directories(${HWLOC_INCLUDE_DIRS})
if(NOT WIN32)
	set(INCLUDE_PTHREAD "pthread")
else()
	set(INCLUDE_PTHREAD "")
endif()
#################### SETUP LIBS #########################
add_library(alloc_primitives SHARED ${alloc_SRC})

if(NOT WIN32)
	set(alloc_SRC ${alloc_SRC} sctk_alloc_posix_export.c)
	add_library(alloc_spy SHARED ${alloc_SRC})	
else()
add_library(mpc_win_allocator SHARED ${alloc_SRC} win_export/disasm-lib/cpu.c win_export/disasm-lib/disasm.c win_export/disasm-lib/disasm_x86.c win_export/disasm-lib/misc.c win_export/sctk_alloc_win_export.cpp win_export/mhook-lib/mhook.cpp)
endif()
add_library(alloc SHARED ${alloc_SRC})

add_library(alloc_trace SHARED ${alloc_SRC})

#################### LINK LIBS ##########################
target_link_libraries(alloc ${HWLOC_LIBRARY} ${INCLUDE_PTHREAD} )
if (NOT WIN32)
target_link_libraries(alloc_spy ${HWLOC_LIBRARY} ${INCLUDE_PTHREAD})
else()
target_link_libraries(mpc_win_allocator ${HWLOC_LIBRARY})
endif()
target_link_libraries(alloc_trace ${HWLOC_LIBRARY} ${INCLUDE_PTHREAD})
target_link_libraries(alloc_primitives ${HWLOC_LIBRARY} ${INCLUDE_PTHREAD})

################### EXTRA FLAGS #########################
set_target_properties(alloc PROPERTIES COMPILE_FLAGS "-DENABLE_STATIC=yes")
if(NOT WIN32)
set_target_properties(alloc_spy PROPERTIES COMPILE_FLAGS "-DENABLE_STATIC=yes -DSCTK_ALLOC_SPY=yes")
endif()
set_target_properties(alloc_trace PROPERTIES COMPILE_FLAGS "-DENABLE_STATIC=yes -DENABLE_TRACE=yes")
set_target_properties(alloc_primitives PROPERTIES COMPILE_FLAGS "-DMPC_ALLOCATOR_DISABLE_STATIC=yes")

if(WIN32)
	#cm_export_file("${CMAKE_BINARY_DIR}/src/alloc_primitives.dll" "${CMAKE_BINARY_DIR}/tests/")
	#cm_export_file("${CMAKE_BINARY_DIR}/src/alloc.dll" "${CMAKE_BINARY_DIR}/tests/")
	#cm_export_file("${CMAKE_BINARY_DIR}/src/alloc_spy.dll" "${CMAKE_BINARY_DIR}/tests/")
	#cm_export_file("${CMAKE_BINARY_DIR}/src/alloc_trace.dll" "${CMAKE_BINARY_DIR}/tests/")
endif(WIN32)


IF(WIN32)
IF(EXISTS ${CMAKE_SOURCE_DIR}/tests/simple_malloc.cpp)
find_package(OpenMP REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(UNITTEST_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/tools/svUnitTest_fake/)
include_directories(${UNITTEST_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src)

add_executable(toto1 ../tests/simple_malloc.cpp win_export/mpc_alloc_win_load.c)
target_link_libraries(toto1 mpc_win_allocator)
ENDIF(EXISTS ${CMAKE_SOURCE_DIR}/tests/simple_malloc.cpp)
ENDIF(WIN32)
