.PHONY: all, debug, clean, touch

CFLAGS = -Wall
INCLUDES = -I../libtkbio
SOURCES = *.c
LIBS = -L../libtkbio -ltkbio
NAME = console

all: debug1 = -DNDEBUG
all: debug2 =
all: touch $(NAME)

debug: debug1 = -ggdb
debug: debug2 = debug
debug: touch $(NAME)

$(NAME): ../libtkbio/libtkbio.a $(SOURCES)
	gcc $(SOURCES) -o $@ $(CFLAGS) $(debug1) $(INCLUDES) $(LIBS)

../libtkbio/libtkbio.a: ../libtkbio/tkbio.c
	make -C ../libtkbio $(debug2)

clean:
	find . ! -type d \( -perm -111 -or -name "*\.o" \) -exec rm {} \;

touch:
	$(shell [ -f debug -a -z "$(debug2)" ] && { touch $(NAME).c; rm debug; rm ../libtkbio/*.a; })
	$(shell [ ! -f debug -a -n "$(debug2)" ] && { touch $(NAME).c; touch debug; rm ../libtkbio/*.a; })
