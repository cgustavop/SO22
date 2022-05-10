CC = gcc
CCFLAGS = -g -Wall -Werror -Wextra -pedantic -fsanitize=undefined -fsanitize=address

all: sdstore sdstored

sdstore :  obj/sdstore.o 
	${CC} $^ -o $@ ${CCFLAGS}

sdstored : obj/sdstored.o obj/priority_queue.o obj/process_pipeline.o
	${CC} $^ -o $@ ${CCFLAGS}

obj/sdstore.o : src/sdstore.c
	${CC} $^ -c -o $@ ${CCFLAGS}

obj/sdstored.o : src/sdstored.c 
	${CC} $^ -c -o $@ ${CCFLAGS}

obj/priority_queue.o : src/priority_queue.c 
	${CC} $^ -c -o $@ ${CCFLAGS}

obj/process_pipeline.o : src/process_pipeline.c
	${CC} $^ -c -o $@ ${CCFLAGS}

clean :
	@echo "Cleaning..."
	rm sdstored
	rm sdstore
	rm -rf obj/*.o
	

	
