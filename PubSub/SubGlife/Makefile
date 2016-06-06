CFLAGS = -fopenmp -Wall
OPTFLAG = -O0 -g
CC = cc

AE_DIR=../redis-3.2.0
AE_OBJS=$(AE_DIR)/src/ae.o $(AE_DIR)/src/zmalloc.o
AE_LIBS=$(AE_DIR)/deps/jemalloc/lib/libjemalloc.a

# Adjust these as needed to find the X11 and adevs libs and headers
PREFIX = ${AE_DIR}
LIBS = -lglut -lGL
LIBPATH = -L/usr/X11R6/lib
INCLUDE = -I${PREFIX}/src -I${PREFIX}/deps/hiredis

LIBNAME=$(AE_DIR)/deps/hiredis/libhiredis
DYLIBSUFFIX=so
STLIBSUFFIX=a
DYLIBNAME=$(LIBNAME).$(DYLIBSUFFIX)
STLIBNAME=$(LIBNAME).$(STLIBSUFFIX)

##
# Should not need to edit below this line
##

OBJS = SubGlife.o

.SUFFIXES: .c
.c.o:
	${CC} ${CFLAGS} ${OPTFLAG} ${INCLUDE} -c $<

SubGlife: ${OBJS}
	${CC} -o $@ ${CFLAGS} ${OPTFLAG} ${OBJS} $(AE_OBJS) $(AE_LIBS) ${LIBS} ${LIBPATH} ${INCLUDE} $(STLIBNAME)

clean:
	rm -f *.o *~ core output SubGlife
