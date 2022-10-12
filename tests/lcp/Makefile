CC=gcc
CFLAGS=`pkg-config --cflags libmpclowcomm`
CFLAGS+=`pkg-config --cflags openpa`
LIBS=`pkg-config --libs libmpclowcomm`
LIBS+=-lmpcconfig

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

all: ep_init context_init ep_earlysend ep_earlyrecv ep_sendfrag ep_double

context_init: context_init.o
	$(CC) -o $@ $< $(LIBS)

ep_init: ep_init.o
	$(CC) -o $@ $< $(LIBS)

ep_double: ep_double.o
	$(CC) -o $@ $< $(LIBS)

ep_earlysend: ep_earlysend.o
	$(CC) -o $@ $< $(LIBS)

ep_earlyrecv: ep_earlyrecv.o
	$(CC) -o $@ $< $(LIBS)

ep_sendfrag: ep_sendfrag.o
	$(CC) -o $@ $< $(LIBS)
