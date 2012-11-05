# The sources we're building
SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)

# What we're building
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

# What we're building with
CC = gcc 
CFLAGS = -Wall 
LDFLAGS = 

ifeq ($(DEBUG),1)
# We want debug mode.
CFLAGS += -gdwarf-3 -O0
else
# We want release mode.
CFLAGS += -O3 -DNDEBUG -gdwarf-3
endif

all: demo

%.o:    %.c $(HEADERS)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<

demo: sighandlerdemo.o mprotectdemo.o
	$(CC) sighandlerdemo.o $(CFLAGS) $(EXTRA_CFLAGS) -o sigdemo 
	$(CC) mprotectdemo.o $(CFLAGS) $(EXTRA_CFLAGS) -o mpdemo 

clean:
	$(RM) demo *.o 	
