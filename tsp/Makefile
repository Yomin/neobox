
.PHONY: all, debug, clean, touch, exec

NAME = tsp
CFLAGS := $(CFLAGS) -Wall -pedantic -std=gnu99

all: DEBUG =
all: CFLAGS := $(CFLAGS) -D NDEBUG
all: touch $(NAME)

debug: DEBUG = yes
debug: CFLAGS := $(CFLAGS) -ggdb
debug: touch $(NAME)

clean:
	find . ! -type d \( -perm -111 -or -name "*\.o" \) -exec rm {} \;


$(NAME): $(NAME).c
	gcc $(CFLAGS) -o $@ $<


touch:
	$(shell [ -f debug -a -z "$(DEBUG)" ] && { touch $(NAME).c; rm debug; })
	$(shell [ ! -f debug -a -n "$(DEBUG)" ] && { touch $(NAME).c; touch debug; })

exec: debug
	./$(NAME) -f -d ../sim/ config_sim
