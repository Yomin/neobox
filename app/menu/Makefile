.PHONY: all, debug, sim, clean, touch, exec, sync

LIBNEOBOXPATH = ../../libneobox
LIBNEOBOX = $(LIBNEOBOXPATH)/libneobox.a
SIMPATH = ../../sim

CFLAGS := $(CFLAGS) -Wall -pedantic -std=gnu99
INCLUDES = -I$(LIBNEOBOXPATH)
SOURCES = *.c
LIBS = -L$(LIBNEOBOXPATH) -lneobox $(shell make -sC $(LIBNEOBOXPATH) libs)
NAME = menu

all: CFLAGS := $(CFLAGS) -DNDEBUG
all: debug =
all: touch $(NAME)

debug: CFLAGS := $(CFLAGS) -ggdb
debug: debug = debug
debug: touch $(NAME)

sim: CFLAGS := $(CFLAGS) -ggdb -DSIM
sim: debug = sim
sim: touch $(NAME)

$(NAME): $(LIBNEOBOX) $(SOURCES)
	gcc $(SOURCES) -o $@ $(CFLAGS) $(INCLUDES) $(LIBS)

$(LIBNEOBOX): $(LIBNEOBOXPATH)/*.c
	@make -C $(LIBNEOBOXPATH) $(debug)

clean:
	find . ! -type d \( -perm -111 -or -name "*\.o" \) -exec rm {} \;

touch:
	$(shell [ -f debug -a -z "$(debug)" ] && { touch $(NAME).c; rm debug; rm -f $(LIBNEOBOX); })
	$(shell [ ! -f debug -a -n "$(debug)" ] && { touch $(NAME).c; touch debug; rm -f $(LIBNEOBOX); })

exec: sim
	./$(NAME) --neobox-fb $(SIMPATH)/fb.ipc --neobox-iod $(SIMPATH)/iod --neobox-verbose --neobox-config config_sim -v

sync:
	@make -C .. sync
