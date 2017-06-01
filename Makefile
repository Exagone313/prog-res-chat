CFLAGS ?= -O2 -Wall -Wextra -pedantic
BIN ?= server

SRCDIR := src
SRC := main.c \
	usage.c \
	state.c \
	thread.c

OBJ := $(addprefix $(SRCDIR)/,$(SRC:.c=.o))
CFLAGS += -std=gnu11 -MMD -MP

all: $(BIN)

debug: CFLAGS += -DDEBUG
debug: all

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -pthread

-include $(OBJ:.o=.d)

clean:
	$(RM) $(OBJ) $(OBJ:.o=.d)

.PHONY: all debug clean

