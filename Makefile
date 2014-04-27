
.PHONY: all, debug, sim, clean, exec, sync

all: cmd=all
all: exec

debug: cmd=debug
debug: exec

sim: cmd=sim
sim: exec

clean: cmd=clean
clean: exec

exec:
	@for d in $(shell ls -d app/*/); do [ -f $$d/Makefile ] && make -C $$d $(cmd); done
	@for d in $(shell ls -d daemon/*/); do [ -f $$d/Makefile ] && make -C $$d $(cmd); done

sync:
	rsync -rtvC --filter ":- .gitignore" --exclude ".gitignore" . chloe:neobox
