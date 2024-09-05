CC ?= gcc
CFLAGS += -std=gnu11 -Os

all: monitor
monitor: monitor.c
	@${CC} monitor.c ${CFLAGS} -o monitor

clean:
	@rm -rf monitor
