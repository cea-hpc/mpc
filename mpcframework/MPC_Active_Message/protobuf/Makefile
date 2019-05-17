prefix?=build/
CPP=g++
PB_PATH?=$(shell spack location -i protobuf)/lib64/pkgconfig/
PB_CFLAGS?=$(shell pkg-config --cflags protobuf --with-path=${PB_PATH})
PB_LDFLAGS?=$(shell pkg-config --libs protobuf --with-path=${PB_PATH})

all: arpc_cpp_plugin arpc_python_plugin

%.cpp: %.h

arpc_cpp_plugin: gen_generic.cpp gen_cpp.cpp
	$(CPP) -o $(prefix)/bin/$@ $^ $(PB_CFLAGS) $(PB_LDFLAGS) -lprotoc -DARPC_FOR_CPP
	

arpc_python_plugin: gen_generic.cpp gen_py.cpp
	$(CPP) -o $(prefix)/bin/$@ $^ $(PB_CFLAGS) $(PB_LDFLAGS) -lprotoc -DARPC_FOR_PYTHON
