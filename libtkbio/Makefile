
.PHONY: all, debug, clean, touch, libs

LIBALGPATH = lib/libalg
LIBALG = $(LIBALGPATH)/libalg.a
TSP = include/tsp

NAME = tkbio
SOURCES = $(shell find . -name "*.c")
OBJECTS = $(SOURCES:%.c=%.o)
INCLUDES = -I$(LIBALGPATH) -I$(TSP)

all: DEBUG =
all: CFLAGS = -D NDEBUG
all: touch $(LIBALG) lib$(NAME).a

debug: DEBUG = debug
debug: CFLAGS = -ggdb
debug: touch $(LIBALG) lib$(NAME).a

clean:
	find . ! -type d ! -type l \( -perm -111 -or -name "*\.a" -or -name "*\.o" \) -exec rm {} \;

libs:
	@for l in $(shell ls lib); do echo -n "-L../lib$(NAME)/lib/$$l -l$${l:3} "; done
	@echo "-lrt"


%.o: %.c
	gcc -Wall -c $(CFLAGS) $(INCLUDES) -o $@ $<

lib$(NAME).a: $(OBJECTS)
	ar rcs $@ $(OBJECTS)

$(LIBALG):
	@make -C $(LIBALGPATH) $(DEBUG)

touch:
	$(shell [ -f debug -a -z "$(DEBUG)" ] && { touch $(NAME).c; rm debug; })
	$(shell [ ! -f debug -a -n "$(DEBUG)" ] && { touch $(NAME).c; touch debug; })
