# define some Makefile variables for the compiler and compiler flags
# to use Makefile variables later in the Makefile: $()
#
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#
# for C++ define  CC = g++
CC = gcc
CFLAGS  = -g -Wall

# typing 'make' will invoke the first target entry in the file 
# (in this case the default target entry)
# you can name this target entry anything, but "default" or "all"
# are the most commonly used names by convention
#
default: server

# To create the executable file count we need the object files
# countwords.o, counter.o, and scanner.o:
#
server:  simsh1.o parser.o
	$(CC) $(CFLAGS) -o simsh1.bin simsh1.o parser.o

# To create the object file countwords.o, we need the source
# files countwords.c, scanner.h, and counter.h:
#
simsh1.o:  simsh1.c
	$(CC) $(CFLAGS) -c simsh1.c

# To create the object file counter.o, we need the source files
# counter.c and counter.h:
#
parser.o:  parser.c parser.h
	$(CC) $(CFLAGS) -c parser.c

# To start over from scratch, type 'make clean'.  This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean:
	$(RM) server *.o *~
