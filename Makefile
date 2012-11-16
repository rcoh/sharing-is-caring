# The sources we're building
SOURCES = sic.c test-barriers.c sic-internals.c sic-util.c
HEADERS = sic.h sic-internals.h network.h sic-util.h sic-server.h

NETWORK_SOURCES = network.c sic-util.c network-tester.c

SERVER_SOURCES = sic-server.c

# What we're building

PRODUCT = test-barriers network-tester server 
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))
NETWORK_OBJECTS = $(patsubst %.c,%.o,$(NETWORK_SOURCES))
SERVER_OBJECTS = $(patsubst %.c,%.o,$(SERVER_SOURCES))

# What we're building with
CC = gcc 
CFLAGS = -Wall -std=c99
LDFLAGS = 

ifeq ($(DEBUG),1)
# We want debug mode.
CFLAGS += -gdwarf-3 -O0
else
# We want release mode.
CFLAGS += -O3 -DNDEBUG -gdwarf-3
endif

all: $(PRODUCT) 

network-tester: $(NETWORK_OBJECTS)
	$(CC) $(NETWORK_OBJECTS) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@

server: $(SERVER_OBJECTS)
	$(CC) $(SERVER_OBJECTS) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@

test-barriers: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@

%.o:    %.c $(HEADERS)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<


#demo: sighandlerdemo.o mprotectdemo.o exec-mpdemo.o
#	$(CC) demos/sighandlerdemo.o $(CFLAGS) $(EXTRA_CFLAGS) -o sigdemo 
#	$(CC) demos/mprotectdemo.o $(CFLAGS) $(EXTRA_CFLAGS) -o mpdemo 
#	$(CC) demos/exec-mpdemo.o $(CFLAGS)  $(EXTRA_CFLAGS) -o execdemo

clean:
	$(RM) sigdemo mpdemo execdemo *.o 	
