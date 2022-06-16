CC=gcc
CFLAGS=-O0 -g
LDFLAGS=-lpapi

all: libcounters.so test count

count: counting.c main.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

libcounters.so: counting.c
	$(CC) $(CFLAGS) -fPIC -shared $^ -o $@ $(LDFLAGS)

test: test.c libcounters.so
	$(CC) -g $< -o $@ -L. -lcounters

clean:
	rm -f libcounters.so test count
