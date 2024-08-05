CC=gcc
CFLAGS=`pkg-config --cflags libmpclowcomm`
CFLAGS+=`pkg-config --cflags openpa`
CFLAGS+=-g
LIBS=`pkg-config --libs libmpclowcomm`
LIBS+=-lmpcconfig

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

all: context_init \
	manager_init \
	multi_manager_init \
	ep_init \
	ep_double \
	ep_earlysend \
	ep_earlyrecv \
	ep_sendfrag \
	ep_multi_manager_send \
	ep_am

context_init: context_init.o
	$(CC) -o $@ $< $(LIBS)

manager_init: manager_init.o
	$(CC) -o $@ $< $(LIBS)

multi_manager_init: multi_manager_init.o
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

ep_multi_manager_send: ep_multi_manager_send.o
	$(CC) -o $@ $< $(LIBS)

ep_am: ep_am.o
	$(CC) -o $@ $< $(LIBS)
