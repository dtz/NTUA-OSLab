
CC  = gcc

CRYPTODEVDIR=$(HOME)/cryptodev/cryptodev-linux-1.6

CFLAGS = -Wall -I$(CRYPTODEVDIR)
CFLAGS += -g
# CFLAGS += -O2 -fomit-frame-pointer -finline-functions


BINS = irc

all: $(BINS)

io_ops.o: io_ops.c io_ops.h
	$(CC) $(CFLAGS) -c $<

client.o: client.c client.h
	$(CC) $(CFLAGS) -c $<

server_ops.o: server_ops.c server.h
	$(CC) $(CFLAGS) -c $<

server.o: server.c server.h
	$(CC) $(CFLAGS) -c $<

chat.o: chat.c
	$(CC) $(CFLAGS) -c $<

irc: chat.o io_ops.o server.o client.o server_ops.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o *.~

distclean: 
	rm $(BINS)
	
