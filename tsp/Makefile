
.PHONY: all, debug, clean, touch

NAME = tsp

all: DEBUG =
all: CFLAGS = -D NDEBUG
all: touch $(NAME)

debug: DEBUG = yes
debug: CFLAGS = -ggdb
debug: touch $(NAME)

clean:
	find . ! -type d \( -perm -111 -or -name "*\.o" \) -exec rm {} \;


$(NAME): $(NAME).c
	gcc -Wall $(CFLAGS) -o $@ $<


touch:
	$(shell [ -f debug -a -z "$(DEBUG)" ] && { touch $(NAME).c; rm debug; })
	$(shell [ ! -f debug -a -n "$(DEBUG)" ] && { touch $(NAME).c; touch debug; })
