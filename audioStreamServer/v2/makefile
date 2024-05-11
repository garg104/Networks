CC = gcc
CFLAGS  = -g -Wall

all: audiosrv audiocli 

# To create the executable file count we need the object files
# countwords.o, counter.o, and scanner.o:
#
audiosrv: endian_convertor.o server.o 
	$(CC) $(CFLAGS) -o audiosrv endian_convertor.o server.o -lpthread

audiocli: endian_convertor.o client.o
	$(CC) $(CFLAGS) -o audiocli endian_convertor.o client.o -lasound -lpthread

client.o: client.c
	$(CC) $(CFLAGS) -c client.c
# To create the object file scanner.o, we need the source files
# scanner.c and scanner.h:
#
server.o:  server.c 
	$(CC) $(CFLAGS) -c server.c

endian_convertor.o: endian_convertor.c endian_convertor.h
	$(CC) $(CFLAGS) -c endian_convertor.c

# To start over from scratch, type 'make clean'.  This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean: 
	$(RM) audiocli audiosrv *.o *~ recv_*
