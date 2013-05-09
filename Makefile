
.PHONY: all, debug, clean, exec

all: cmd=all
all: exec

debug: cmd=debug
debug: exec

clean: cmd=clean
clean: exec

exec:
	@for d in $(shell ls -d */); do [ -f $$d/Makefile ] && make -C $$d $(cmd); done
