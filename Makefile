CC = gcc
CFLAGS = -O -Wall -ggdb
LDFLAGS = -lm

OBJS = main.o pnm.o golomb.o parameters.o codingvars.c predictivecoding.o bitstream.o

INCLUDES = -I.

all: jpegls

jpegls: ${OBJS}
	@echo 'Linking..'
	${CC} ${CFLAGS} ${LDFLAGS} -o jpegls ${INCLUDES} ${OBJS}
	@echo -e '..done.\n'

main.o: main.c pnm.h type_defs.h bitstream.h golomb.h predictivecoding.h codingvars.h
	@echo 'compiling main.c ..'
	${CC} ${CFLAGS} ${INCLUDES} -c main.c -o main.o
	@echo -e '..done.\n'

pnm.o: pnm.c pnm.h type_defs.h
	@echo 'compiling pnm.c ..'
	${CC} ${CFLAGS} ${INCLUDES} -c pnm.c -o pnm.o
	@echo -e '..done.\n'

golomb.o: golomb.c golomb.h type_defs.h bitstream.h
	@echo 'compiling golomb.c ..'
	${CC} ${CFLAGS} ${INCLUDES} -c golomb.c -o golomb.o
	@echo -e '..done.\n'

bitstream.o: bitstream.c bitstream.h type_defs.h
	@echo 'compiling bitstream.c ..'
	${CC} ${CFLAGS} ${INCLUDES} -c bitstream.c -o bitstream.o
	@echo -e '..done.\n'

parameters.o: parameters.c parameters.h type_defs.h
	@echo 'compiling parameters.c ..'
	${CC} ${CFLAGS} ${INCLUDES} -c parameters.c -o parameters.o
	@echo -e '..done.\n'

codingvars.o: codingvars.c codingvars.h type_defs.h
	@echo 'compiling codingvars.c ..'
	${CC} ${CFLAGS} ${INCLUDES} -c codingvars.c -o codingvars.o

predictivecoding.o: predictivecoding.c predictivecoding.h type_defs.h golomb.h bitstream.h
	@echo 'compiling codingvars.c ..'
	${CC} ${CFLAGS} ${INCLUDES} -c predictivecoding.c -o predictivecoding.o

install: jpegls
	install -m 755 editor /usr/local/bin/jpegls

clean:
	rm -f *.o jpegls

realclean:
	rm -f *.o *~ jpegls
