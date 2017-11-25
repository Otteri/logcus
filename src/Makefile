SRC_FILES=main.c

all: main

main: $(SRC_FILES)
	gcc -g -Wall -Wextra -pedantic -std=c99 -o $@ $(SRC_FILES) -lm

clean:
	rm -f main
	> msg.log

memcheck:
	valgrind --leak-check=full ./main example.c
