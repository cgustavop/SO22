CC = gcc
CCFLAGS = -g -Wall -Werror -Wextra -pedantic -fsanitize=undefined -fsanitize=address

all: sdstore sdstored

sdstore :  obj/sdstore.o src/request.h 
	${CC} $^ -o $@ ${CCFLAGS}

sdstored : obj/sdstored.o obj/priority_queue.o src/request.h src/priority_queue.h
	${CC} $^ -o $@ ${CCFLAGS}

obj/sdstore.o : src/sdstore.c
	${CC} $^ -c -o $@ ${CCFLAGS}

obj/sdstored.o : src/sdstored.c 
	${CC} $^ -c -o $@ ${CCFLAGS}

obj/priority_queue.o : src/priority_queue.c 
	${CC} $^ -c -o $@ ${CCFLAGS}

clean :
	@echo "Cleaning..."
	rm sdstored
	rm sdstore
	rm -rf obj/*.o
	

	
