CPP=g++
PB_PATH=$(pb_prefix)
PB_CFLAGS=-I$(PB_PATH)/include
PB_LDFLAGS=-L$(PB_PATH)/lib{,64} -Wl,-rpath=$(PB_PATH)/lib{,64} -lprotobuf -lpthread -lprotoc

all: arpc_cpp_plugin arpc_python_plugin

%.cpp: %.h

arpc_cpp_plugin: gen_generic.cpp gen_cpp.cpp
	@echo "CXX Protobuf C++ Plugin"
	@$(CPP) -o $(prefix)/$@ $^ $(PB_CFLAGS) $(PB_LDFLAGS) -DARPC_FOR_CPP
	

arpc_python_plugin: gen_generic.cpp gen_py.cpp
	@echo "CXX Protobuf Python Plugin"
	@$(CPP) -o $(prefix)/$@ $^ $(PB_CFLAGS) $(PB_LDFLAGS) -DARPC_FOR_PYTHON
