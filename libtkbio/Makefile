
.PHONY: all, debug, sim, clean, touch, libs, sync

LIBALGPATH = lib/libalg
LIBALG = $(LIBALGPATH)/libalg.a
TSP = include/tsp

NAME = tkbio
SOURCES = $(shell find . -name "*.c")
OBJECTS = $(SOURCES:%.c=%.o)
INCLUDES = -I$(LIBALGPATH) -I$(TSP)
CFLAGS := $(CFLAGS) -Wall -pedantic -std=gnu99

all: DEBUG =
all: CFLAGS := $(CFLAGS) -D NDEBUG
all: touch $(LIBALG) lib$(NAME).a

debug: DEBUG = debug
debug: CFLAGS := $(CFLAGS) -ggdb
debug: touch $(LIBALG) lib$(NAME).a

sim: DEBUG = debug
sim: CFLAGS := $(CFLAGS) -ggdb -D SIM
sim: touch $(LIBALG) lib$(NAME).a

clean:
	find . ! -type d ! -type l \( -perm -111 -or -name "*\.a" -or -name "*\.o" \) -exec rm {} \;

libs:
	@for l in $(shell ls lib); do echo -n "-L../lib$(NAME)/lib/$$l -l$${l:3} "; done
	@echo "-lrt" # needed by shared memory


%.o: %.c
	gcc $(CFLAGS) $(INCLUDES) -c -o $@ $<

lib$(NAME).a: $(OBJECTS)
	ar rcs $@ $(OBJECTS)

$(LIBALG):
	@make -C $(LIBALGPATH) $(DEBUG)

touch:
	$(shell [ -f debug -a -z "$(DEBUG)" ] && { touch $(NAME).c; rm debug; })
	$(shell [ ! -f debug -a -n "$(DEBUG)" ] && { touch $(NAME).c; touch debug; })

sync:
	@make -C .. sync
