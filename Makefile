CC=gcc -m486 -g -Wall -I. -DVERSION=\"V0.5\"

all: lcd4linux

system: system.c system.h config.c config.h filter.c filter.h
	${CC} -DSTANDALONE -lm -o system system.c config.c filter.c

#lcd4linux: lcd4linux.c config.c lcd2041.c system.c isdn.c filter.c Makefile
#	${CC} -lm -o lcd4linux lcd4linux.c config.c lcd2041.c system.c isdn.c filter.c

lcd4linux: display.c MatrixOrbital.c
	${CC} -lm -o lcd4linux display.c MatrixOrbital.c

clean:
	rm -f lcd4linux *.o *~
