GTEST_PREFIX=$(PWD)/BUILD/gtest
CXX_FLAGS=-std=c++11 -I$(GTEST_PREFIX)/include/ -I$(PWD)/../ -fno-mpc-privatize
LD_FLAGS=-L$(GTEST_PREFIX)/lib/ -lgtest
CXX=mpc_cxx

TESTS_FILES=$(wildcard MPC_Common/*.cpp)

all: BUILD run_tests

mpc_unit_tests: main.cpp $(TESTS_FILES)
	$(CXX) $(CXX_FLAGS)  $^ $(LD_FLAGS) -o mpc_unit_tests


run_tests: mpc_unit_tests
	MPC_CALL_ORIGINAL_MAIN=1 ./mpc_unit_tests

BUILD:
	mkdir -p BUILD
	cd BUILD; cmake ../googletest/ -DBUILD_GTEST=1 -DCMAKE_INSTALL_PREFIX=$(GTEST_PREFIX)
	cd BUILD; make -j8 install


clean:
	rm -fr BUILD mpc_unit_tests