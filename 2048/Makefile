.PHONY: all, debug, sim, clean, touch, exec, sync

LIBTKBIOPATH = ../libtkbio
LIBTKBIO = $(LIBTKBIOPATH)/libtkbio.a

CFLAGS := $(CFLAGS) -Wall -pedantic -std=c99
INCLUDES = -I$(LIBTKBIOPATH)
SOURCES = *.c
LIBS = -L$(LIBTKBIOPATH) -ltkbio $(shell make -sC $(LIBTKBIOPATH) libs)
NAME = 2048

all: CFLAGS := $(CFLAGS) -DNDEBUG
all: debug =
all: touch $(NAME)

debug: CFLAGS := $(CFLAGS) -ggdb
debug: debug = debug
debug: touch $(NAME)

sim: CFLAGS := $(CFLAGS) -ggdb
sim: debug = sim
sim: touch $(NAME)

$(NAME): $(LIBTKBIO) $(SOURCES)
	gcc $(SOURCES) -o $@ $(CFLAGS) $(INCLUDES) $(LIBS)

$(LIBTKBIO): $(LIBTKBIOPATH)/*.c
	@make -C $(LIBTKBIOPATH) $(debug)

clean:
	find . ! -type d \( -perm -111 -or -name "*\.o" \) -exec rm {} \;

touch:
	$(shell [ -f debug -a -z "$(debug)" ] && { touch $(NAME).c; rm debug; rm -f $(LIBTKBIO); })
	$(shell [ ! -f debug -a -n "$(debug)" ] && { touch $(NAME).c; touch debug; rm -f $(LIBTKBIO); })

exec: sim
	./$(NAME) --tkbio-fb ../sim/fb.ipc --tkbio-tsp ../sim --tkbio-verbose

sync:
	@make -C .. sync
