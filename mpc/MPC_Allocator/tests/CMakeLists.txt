##################################################################
mpc_setup_testing()

##################################################################
#setup directories and flags
include_directories(${UNITTEST_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src)
#Disable static (SCTK_PRIVATE) to get access to internal symbols
add_definitions(-DMPC_ALLOCATOR_DISABLE_STATIC=yes)

##################################################################
#setup OpenMP for parallel tests (not required for all, but simpler to setup once)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")

##################################################################
#test list
set(ALLOC_TEST_LIST test_allocated_chunk test_chunk_padding test_thread_pool test_user_alloc_chain
	test_default_alloc_chain test_mm_source_default test_full_alloc_chain
	test_regions test_posix_malloc test_rfq test_light_mm_src test_malloc_on_node
	test_internal_spinlock)

##################################################################
#Now you can build your tests
foreach(i ${ALLOC_TEST_LIST})
	add_executable(${i} ${i}.cpp)
	target_link_libraries(${i} ${UNITTEST_LIBRARY} mpcalloc_primitives_for_test)
	add_test(${i} ${EXECUTOR_PATH} ${i})
endforeach(i)

##################################################################
#a simple exec with full allocator
add_executable(simple_malloc simple_malloc-1.cpp)
target_link_libraries(simple_malloc mpcalloc)

##################################################################
#A small bench of mpscf queues
add_executable(test_mpscf_queue test_mpscf_queue.c)
add_test(test_mpscf_queue test_mpscf_queue valid)
