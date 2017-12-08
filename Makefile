MAIN_DIR = src

.PHONY: main

all:
	$(MAKE) -C $(MAIN_DIR)
main:
	$(MAKE) -C $(MAIN_DIR) main
test:
	$(MAKE) -C $(MAIN_DIR) test

clean:
	$(MAKE) -C $(MAIN_DIR) clean

memcheck:
	$(MAKE) -C $(MAIN_DIR) memcheck
