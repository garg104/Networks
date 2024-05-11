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
default: rrunners rrunnerc

# To create the executable file count we need the object files
# countwords.o, counter.o, and scanner.o:
#
rrunners:  rrunners.o
	$(CC) $(CFLAGS) -o rrunners rrunners.o

rrunnerc:  rrunnerc.o
	$(CC) $(CFLAGS) -o rrunnerc rrunnerc.o


# To create the object file simsh1.o, we need the source
# files simsh1.c:
#
rrunners.o:  rrunners.c
	$(CC) $(CFLAGS) -c rrunners.c

# To create the object file simsh2.o, we need the source
# files simsh2.c:
#
rrunnerc.o:  rrunnerc.c
	$(CC) $(CFLAGS) -c rrunnerc.c


# To create the object file parser.o, we need the source files
# parser.c and parser.h:
#
parser.o:  parser.c parser.h
	$(CC) $(CFLAGS) -c parser.c

# To start over from scratch, type 'make clean'.  This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean:
	$(RM) *.bin *.o *~ rrunners rrunnerc NEW_*
