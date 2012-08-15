
.PHONY: all, debug, clean, touch

NAME = tkbio

all: DEBUG =
all: CFLAGS = -D NDEBUG
all: touch lib$(NAME).a

debug: DEBUG = yes
debug: CFLAGS = -ggdb
debug: touch lib$(NAME).a

clean:
	find . ! -type d \( -perm -111 -or -name "*\.a" -or -name "*\.o" \) -exec rm {} \;


$(NAME).o: $(NAME).c
	gcc -Wall -c $(CFLAGS) -o $@ $<

lib$(NAME).a: $(NAME).o
	ar rcs $@ $<


touch:
	$(shell [ -f debug -a -z "$(DEBUG)" ] && { touch $(NAME).c; rm debug; })
	$(shell [ ! -f debug -a -n "$(DEBUG)" ] && { touch $(NAME).c; touch debug; })
