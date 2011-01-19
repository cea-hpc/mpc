MPC_CFLAGS += -DSTAND_ALONE_MPC 

compile: compile-advection compile-conduction

compile-advection:
	@echo "Compiling advection "
	@mpc_cxx -I. -Iinclude -Iparallelpp/ -Iutilspp/ -o advection -DCONV -O2 -g *.C  

compile-conduction:
	@echo "Compiling conduction"
	@mpc_cxx -I. -Iinclude -Iparallelpp/ -Iutilspp/ -o conduction -O2 -g *.C  

clean: 
	@rm conduction advection 
