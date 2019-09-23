CC=gcc
CFLAGS=-Wall -g
BINS= memory_shim.so leakcount sctracer

all: $(BINS)
memory_shim.so:  memory_shim.c
	$(CC) $(CFLAGS) -fPIC -shared -o memory_shim.so memory_shim.c -ldl
%: %.c
		$(CC) $(CFLAGS) -o $@ $<
clean:
		rm $(BINS)
