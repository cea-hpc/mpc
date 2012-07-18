##################################################################
#check the presence of svUnitTest
find_package(svUnitTest)

if (NOT WIN32)
	find_package(OpenMP REQUIRED)
endif(NOT WIN32)

if(SVUNITTEST_FOUND)
	set(UNITTEST_INCLUDE_DIR ${SVUNITTEST_INCLUDE_DIR})
	set(UNITTEST_LIBRARY ${SVUNITTEST_LIBRARY})
else(SVUNITTEST_FOUND)
	#add_subdirectory(svUnitTest_fake)
	set(UNITTEST_INCLUDE_DIR ../tools/svUnitTest_fake/)
	set(UNITTEST_LIBRARY "")
endif(SVUNITTEST_FOUND)

#setup directories
include_directories(${UNITTEST_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src)
add_definitions(-DMPC_ALLOCATOR_DISABLE_STATIC=yes)

#test list
set(ALLOC_TEST_LIST test_allocated_chunk test_chunk_padding test_thread_pool test_user_alloc_chain
	test_default_alloc_chain test_mm_source_default test_full_alloc_chain
	test_regions test_posix_malloc test_rfq test_light_mm_src)

#setup OpenMP for parallel tests (not required for all, but simpler to setup once)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

#Now you can build your tests
foreach(i ${ALLOC_TEST_LIST})
	add_executable(${i} ${i}.cpp)
	target_link_libraries(${i} ${UNITTEST_LIBRARY} alloc_primitives)
	add_test(${i} ${EXECUTOR_PATH} ${i})
endforeach(i)

add_executable(simple_malloc simple_malloc.c)
target_link_libraries(simple_malloc alloc)

