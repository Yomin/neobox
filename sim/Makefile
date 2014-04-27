
.PHONY: all, debug, clean, touch

NAME = sim
CFLAGS := $(CFLAGS) -Wall -pedantic -std=gnu99 $$(pkg-config --cflags --libs gtk+-3.0) -lrt

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
