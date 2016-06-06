CFLAGS = -fopenmp -Wall
OPTFLAG = -O0 -g
CC = g++

# Adjust these as needed to find the X11 and adevs libs and headers
AD_PREFIX = ../adevs-code-323-trunk
HI_PREFIX = ../redis-3.2.0/deps
LIBS = -lglut -lGL
LIBPATH = -L/usr/X11R6/lib
INCLUDE = -I${AD_PREFIX}/include -I${HI_PREFIX}/hiredis

LIBNAME=../redis-3.2.0/deps/hiredis/libhiredis
DYLIBSUFFIX=so
STLIBSUFFIX=a
DYLIBNAME=$(LIBNAME).$(DYLIBSUFFIX)
STLIBNAME=$(LIBNAME).$(STLIBSUFFIX)

##
# Should not need to edit below this line
##

OBJS = Cell.o PubGlife.o

.SUFFIXES: .cpp
.cpp.o:
	${CC} ${CFLAGS} ${OPTFLAG} ${INCLUDE} -c $<

PubGlife: ${OBJS}
	${CC} -o $@ ${CFLAGS} ${OPTFLAG} ${OBJS} ${LIBS} ${LIBPATH} ${INCLUDE} $(STLIBNAME)

clean:
	rm -f *.o *~ core PubGlife
