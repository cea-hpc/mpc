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
#Message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
#if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	set(alloc_SRC sctk_alloc_debug.c sctk_allocator.c sctk_alloc_posix.c sctk_alloc_topology.c sctk_alloc_light_mm_source.c sctk_alloc_common.c sctk_alloc_numa_stat.c sctk_alloc_on_node.c sctk_alloc_config.c sctk_alloc_hooks.c)
#else()
#	set(alloc_SRC sctk_allocator.c sctk_alloc_posix.c sctk_alloc_topology.c sctk_alloc_light_mm_source.c sctk_alloc_common.c sctk_alloc_numa_stat.c)
#endif()

################# CHECK SOME INCLUDE FILES #############
include(CheckIncludeFile)
include(CheckSymbolExists)
CHECK_INCLUDE_FILE (numaif.h HAVE_NUMAIF_H)
CHECK_SYMBOL_EXISTS (move_pages numaif.h HAVE_MOVE_PAGES)
if(HAVE_NUMAIF_H)
	add_definitions(-DHAVE_NUMAIF_H)
endif(HAVE_NUMAIF_H)
if (HAVE_MOVE_PAGES)
	add_definitions(-DHAVE_MOVE_PAGES)
endif (HAVE_MOVE_PAGES)

CHECK_SYMBOL_EXISTS (__malloc_initialize_hook malloc.h HAVE_MALLOC_INITIALIZE_HOOK)
if (HAVE_MALLOC_INITIALIZE_HOOK)
	add_definitions(-DENABLE_GLIBC_ALLOC_HOOKS)
endif (HAVE_MALLOC_INITIALIZE_HOOK)	

#################### CHECK PAGEMAP ######################
if (EXISTS /proc/self/pagemap)
	add_definitions(-DHAVE_LINUX_PAGEMAP)
endif(EXISTS /proc/self/pagemap)

#################### ADD INCLUDE DIRS ###################
if(NOT WIN32)
	set(INCLUDE_PTHREAD "pthread")
else()
	set(INCLUDE_PTHREAD "")
endif()
#################### SETUP LIBS #########################
add_library(mpcalloc_primitives_for_test STATIC ${alloc_SRC})
add_library(mpcalloc_primitives SHARED ${alloc_SRC})

if(NOT WIN32)
	set(alloc_SRC ${alloc_SRC} sctk_alloc_posix_export.c)	
else()
	add_library(mpc_allocator SHARED ${alloc_SRC} win_export/disasm-lib/cpu.c win_export/disasm-lib/disasm.c win_export/disasm-lib/disasm_x86.c win_export/disasm-lib/misc.c win_export/sctk_alloc_win_export.cpp win_export/mhook-lib/mhook.cpp)
	add_library(mpc_allocator_import STATIC win_export/mpc_alloc_win_load.c)
endif()
add_library(mpcalloc SHARED ${alloc_SRC})

add_library(mpcalloc_trace SHARED ${alloc_SRC})

#################### LINK LIBS ##########################
target_link_libraries(mpcalloc ${HWLOC_LIBRARY} ${INCLUDE_PTHREAD} )
if (WIN32)
target_link_libraries(mpc_allocator ${HWLOC_LIBRARY})
endif()
target_link_libraries(mpcalloc_trace ${HWLOC_LIBRARY} ${INCLUDE_PTHREAD})
target_link_libraries(mpcalloc_primitives_for_test ${HWLOC_LIBRARY} ${INCLUDE_PTHREAD})
target_link_libraries(mpcalloc_primitives ${HWLOC_LIBRARY} ${INCLUDE_PTHREAD})

################### EXTRA FLAGS #########################
set_target_properties(mpcalloc PROPERTIES COMPILE_FLAGS "-DENABLE_STATIC=yes")
set_target_properties(mpcalloc_primitives PROPERTIES COMPILE_FLAGS "-DENABLE_STATIC=yes")
set_target_properties(mpcalloc_trace PROPERTIES COMPILE_FLAGS "-DENABLE_STATIC=yes -DENABLE_TRACE=yes")
set_target_properties(mpcalloc_primitives_for_test PROPERTIES COMPILE_FLAGS "-DMPC_ALLOCATOR_DISABLE_STATIC=yes")

if(WIN32)
	#cm_export_file("${CMAKE_BINARY_DIR}/src/alloc_primitives.dll" "${CMAKE_BINARY_DIR}/tests/")
	#cm_export_file("${CMAKE_BINARY_DIR}/src/alloc.dll" "${CMAKE_BINARY_DIR}/tests/")
	#cm_export_file("${CMAKE_BINARY_DIR}/src/alloc_spy.dll" "${CMAKE_BINARY_DIR}/tests/")
	#cm_export_file("${CMAKE_BINARY_DIR}/src/alloc_trace.dll" "${CMAKE_BINARY_DIR}/tests/")
endif(WIN32)

######################################################## 
#only for easier debug during windows port.
IF(WIN32)
	IF(EXISTS ${CMAKE_SOURCE_DIR}/tests/simple_malloc.cpp)
		find_package(OpenMP REQUIRED)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
		set(UNITTEST_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/tools/svUnitTest_fake/)
		include_directories(${UNITTEST_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src)

		add_executable(win_port_test ../tests/simple_malloc.cpp win_export/mpc_alloc_win_load.c)
		target_link_libraries(win_port_test mpc_allocator)
	ENDIF(EXISTS ${CMAKE_SOURCE_DIR}/tests/simple_malloc.cpp)
ENDIF(WIN32)

######################################################## 
INSTALL(TARGETS mpcalloc mpcalloc_trace mpcalloc_primitives
        ARCHIVE DESTINATION lib${LIB_SUFFIX}
        LIBRARY DESTINATION lib${LIB_SUFFIX})
