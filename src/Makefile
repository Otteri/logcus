SRC_FILES=main.c logcus.c
SRC_FILES2=test.c logcus.c

all: main

main: $(SRC_FILES)
	gcc -g -Wall -Wextra -pedantic -std=c99 -o $@ $(SRC_FILES) -pthread -lrt -lm

clean:
	rm -f main
	> msg.log

memcheck:
	valgrind --leak-check=full ./main example.c

test: $(SRC_FILES2)
	gcc -g -Wall -Wextra -pedantic -std=c99 -o $@ $(SRC_FILES2) -lm
