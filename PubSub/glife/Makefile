CFLAGS = -fopenmp -Wall
OPTFLAG = -g -O0 
CC = g++

# Adjust these as needed to find the X11 and adevs libs and headers
PREFIX = /usr/local/adevs
LIBS = -lglut -lGL
LIBPATH = -L/usr/X11R6/lib
INCLUDE = -I${PREFIX}/include

##
# Should not need to edit below this line
##

OBJS = Cell.o main.o

.SUFFIXES: .cpp
.cpp.o:
	${CC} ${CFLAGS} ${OPTFLAG} ${INCLUDE} -c $<

glife: ${OBJS}
	${CC} -o glife ${CFLAGS} ${OPTFLAG} ${OBJS} ${LIBS} ${LIBPATH} ${INCLUDE}

clean:
	rm -f *.o core glife
