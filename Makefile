# The sources we're building
# Sources without mains
SERVER_SOURCES = sic-server.c
GENERIC_SOURCES = sic.c sic-internals.c sic-util.c network.c sic-message.pb-c.c
HEADERS = sic.h sic-internals.h network.h sic-util.h sic-server.h sic-message.pb-c.h google/protobuf-c/protobuf-c.h google/protobuf-c/protobuf-c-rpc.h google/protobuf-c/protobuf-c-dispatch.h google/protobuf-c/protobuf-c-private.h

TEST_SOURCES = $(wildcard tests/test*.c)
TEST_OBJECTS = $(patsubst tests/%.c,tests/%.o,$(TEST_SOURCES))

# What we're building

PRODUCT = protos server
TEST_PRODUCTS = $(patsubst tests/%.c,%,$(TEST_SOURCES))
GENERIC_OBJECTS = $(patsubst %.c,%.o,$(GENERIC_SOURCES))
SERVER_OBJECTS = $(patsubst %.c,%.o,$(SERVER_SOURCES))

# What we're building with
CC = gcc
CFLAGS = -Wall
LDFLAGS = -lpthread -lprotobuf-c -lcrypt

ifeq ($(DEBUG),1)
# We want debug mode.
CFLAGS += -gdwarf-3 -O0
else
# We want release mode.
CFLAGS += -O3 -DNDEBUG -gdwarf-3
endif

all: $(PRODUCT) $(TEST_PRODUCTS)

protos: sic-message.proto
	protoc-c --c_out=. sic-message.proto

server: $(SERVER_OBJECTS) $(GENERIC_OBJECTS)
	$(CC) $(GENERIC_OBJECTS) $(SERVER_OBJECTS) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@

test-%: tests/test-%.o
	$(CC) $(GENERIC_OBJECTS) $< $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@

%.o:    %.c $(HEADERS)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<

tests/%.o:  tests/%.c $(HEADERS)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<


clean:
	$(RM) $(PRODUCT) $(TEST_PRODUCTS) $(GENERIC_OBJECTS) $(TEST_OBJECTS) $(SERVER_OBJECTS)
