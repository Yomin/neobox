
.PHONY: all, debug, clean, touch

NAME = tkbio
SOURCES = $(shell find . -name "*.c")
OBJECTS = $(SOURCES:%.c=%.o)
INCLUDES = -Ilibs/alg

all: DEBUG =
all: CFLAGS = -D NDEBUG
all: touch lib$(NAME).a

debug: DEBUG = yes
debug: CFLAGS = -ggdb
debug: touch lib$(NAME).a

clean:
	find . ! -type d \( -perm -111 -or -name "*\.a" -or -name "*\.o" \) -exec rm {} \;


%.o: %.c
	gcc -Wall -c $(CFLAGS) $(INCLUDES) -o $@ $<

lib$(NAME).a: $(OBJECTS)
	ar rcs $@ $(OBJECTS)


touch:
	$(shell [ -f debug -a -z "$(DEBUG)" ] && { touch $(NAME).c; rm debug; })
	$(shell [ ! -f debug -a -n "$(DEBUG)" ] && { touch $(NAME).c; touch debug; })
