MAIN_DIR = src

.PHONY: main

all: main

main:
	$(MAKE) -C $(MAIN_DIR)

clean:
	$(MAKE) -C $(MAIN_DIR) clean

memcheck:
	$(MAKE) -C $(MAIN_DIR) memcheck
