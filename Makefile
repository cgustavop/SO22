CC = gcc
CCFLAGS = -g -O2 -Wall -fstack-protector -fopenmp

all: sdstore sdstored

sdstore :  obj/sdstore.o src/process.h
	${CC} $^ -o $@ ${CCFLAGS}

sdstored : obj/sdstored.o src/process.h
	${CC} $^ -o $@ ${CCFLAGS}

obj/sdstore.o : src/sdstore.c
	${CC} src/sdstore.c -c -o obj/sdstore.o ${CCFLAGS}

obj/sdstored.o : src/sdstored.c
	${CC} src/sdstored.c -c -o obj/sdstored.o ${CCFLAGS}


clean :
	@echo "Cleaning..."
	rm sdstored
	rm sdstore
	rm -rf obj/*.o
	

	
