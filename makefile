COMPILER = gcc
CFLAGS = -Wall -lcrypto
EXES = assignment1

all: ${EXES}

assignment1: assignment1.c
	${COMPILER} ${CFLAGS}  assignment1.c -o assignment1

clean: 
	rm -f *~ *.o ${EXES}