PROG=	dynamicprefixvici
SRC=	dynamicprefixvici.c

CC?=			cc
INSTALL?=		install
MAKE?=			make
PREFIX?=		/usr/local
RM?=			rm

CFLAGS?=		-Ofast -march=native
INCLUDEDIR?=	${PREFIX}/include
INSTALLDIR?=	${PREFIX}/sbin
LIBS?=			${PREFIX}/lib/ipsec/libvici.so

build:
	${CC} ${SRC} ${LIBS} -I${INCLUDEDIR} ${CFLAGS} -o${PROG}

clean:
	${RM} -f ${PROG} ${PROG}.core

install:
	${INSTALL} -g wheel -o root -s ${PROG} ${INSTALLDIR}