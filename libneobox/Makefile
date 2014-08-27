
.PHONY: all, debug, fulldebug, sim, clean, touch, libs, sync

LIBALGPATH = lib/libalg
LIBALG = $(LIBALGPATH)/libalg.a
LIBRJPATH = lib/librj
LIBRJ = $(LIBALGPATH)/librj.a
IOD = include/iod

NAME = neobox
SOURCES = $(shell find . -name "*.c")
OBJECTS = $(SOURCES:%.c=%.o)
INCLUDES = -I$(LIBALGPATH) -I$(LIBRJPATH) -I$(IOD)
CFLAGS := $(CFLAGS) -Wall -pedantic -std=gnu99 -D NEOBOX

all: DEBUG =
all: FULLDEBUG =
all: CFLAGS := $(CFLAGS) -D NDEBUG
all: touch $(LIBALG) $(LIBRJ) lib$(NAME).a

debug: DEBUG = debug
debug: FULLDEBUG =
debug: CFLAGS := $(CFLAGS) -ggdb
debug: touch $(LIBALG) $(LIBRJ) lib$(NAME).a

fulldebug: DEBUG = debug
fulldebug: FULLDEBUG = debug
fulldebug: CFLAGS := $(CFLAGS) -ggdb
fulldebug: touch $(LIBALG) $(LIBRJ) lib$(NAME).a

sim: DEBUG = debug
sim: FULLDEBUG =
sim: CFLAGS := $(CFLAGS) -ggdb -D SIM
sim: touch $(LIBALG) $(LIBRJ) lib$(NAME).a

clean:
	find . ! -type d ! -type l \( -perm -111 -or -name "*\.a" -or -name "*\.o" \) -exec rm {} \;

libs:
	@for l in $(shell ls lib); do echo -n "-L../../lib$(NAME)/lib/$$l -l$${l:3} "; done
	@echo -n " -lrt" # needed by shared memory
	@echo -n " -lasound" # needed neobox_sound
	@echo -n " -lpthread" # needed by neobox_sound


%.o: %.c
	gcc $(CFLAGS) $(INCLUDES) -c -o $@ $<

lib$(NAME).a: $(OBJECTS)
	ar rcs $@ $(OBJECTS)

$(LIBALG):
	@make -C $(LIBALGPATH) $(FULLDEBUG)

$(LIBRJ):
	@make -C $(LIBRJPATH) $(FULLDEBUG)

touch:
	$(shell [ -f debug -a -z "$(DEBUG)" ] && { touch $(NAME).c; rm debug; })
	$(shell [ ! -f debug -a -n "$(DEBUG)" ] && { touch $(NAME).c; touch debug; })

sync:
	@make -C .. sync
