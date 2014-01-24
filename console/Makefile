.PHONY: all, debug, clean, touch, sim

LIBTKBIOPATH = ../libtkbio
LIBTKBIO = $(LIBTKBIOPATH)/libtkbio.a

CFLAGS = -Wall
INCLUDES = -I$(LIBTKBIOPATH)
SOURCES = *.c
LIBS = -L$(LIBTKBIOPATH) -ltkbio $(shell make -sC $(LIBTKBIOPATH) libs)
NAME = console

all: debug1 = -DNDEBUG
all: debug2 =
all: touch $(NAME)

debug: debug1 = -ggdb
debug: debug2 = debug
debug: touch $(NAME)

$(NAME): $(LIBTKBIO) $(SOURCES)
	gcc $(SOURCES) -o $@ $(CFLAGS) $(debug1) $(INCLUDES) $(LIBS)

$(LIBTKBIO): $(LIBTKBIOPATH)/*.c
	@make -C $(LIBTKBIOPATH) $(debug2)

clean:
	find . ! -type d \( -perm -111 -or -name "*\.o" \) -exec rm {} \;

touch:
	$(shell [ -f debug -a -z "$(debug2)" ] && { touch $(NAME).c; rm debug; rm -f $(LIBTKBIO); })
	$(shell [ ! -f debug -a -n "$(debug2)" ] && { touch $(NAME).c; touch debug; rm -f $(LIBTKBIO); })

sim: debug
	./$(NAME) --tkbio-fb ../sim/fb.ipc --tkbio-tsp ../sim $(shell tty)
