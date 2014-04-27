.PHONY: all, debug, clean, touch

LIBNEOBOXPATH = ../../libneobox
LIBNEOBOX = $(LIBNEOBOXPATH)/libneobox.a

CFLAGS = -Wall
INCLUDES = -I$(LIBNEOBOXPATH)
SOURCES = *.c
LIBS = -L$(LIBNEOBOXPATH) -lneobox $(shell make -sC $(LIBNEOBOXPATH) libs)
NAME = gps

all: debug1 = -DNDEBUG
all: debug2 =
all: touch $(NAME)

debug: debug1 = -ggdb
debug: debug2 = debug
debug: touch $(NAME)

$(NAME): $(LIBNEOBOX) $(SOURCES)
	gcc $(SOURCES) -o $@ $(CFLAGS) $(debug1) $(INCLUDES) $(LIBS)

$(LIBNEOBOX): $(LIBNEOBOXPATH)/neobox.c
	@make -C $(LIBNEOBOXPATH) $(debug2)

clean:
	find . ! -type d \( -perm -111 -or -name "*\.o" \) -exec rm {} \;

touch:
	$(shell [ -f debug -a -z "$(debug2)" ] && { touch $(NAME).c; rm debug; rm -f $(LIBNEOBOX); })
	$(shell [ ! -f debug -a -n "$(debug2)" ] && { touch $(NAME).c; touch debug; rm -f $(LIBNEOBOX); })
