CC=mpc_cc -g

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

all: ep_am

ep_am: ep_am.o
	$(CC) -o $@ $< $(LIBS)
