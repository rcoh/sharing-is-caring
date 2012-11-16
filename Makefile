# The sources we're building
SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)

# What we're building

PRODUCT = test-barriers
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

all: demo $(PRODUCT)


sic: test-barriers.o

# How to link the product
$(PRODUCT): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@

%.o:    %.c $(HEADERS)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<


#demo: sighandlerdemo.o mprotectdemo.o exec-mpdemo.o
#	$(CC) demos/sighandlerdemo.o $(CFLAGS) $(EXTRA_CFLAGS) -o sigdemo 
#	$(CC) demos/mprotectdemo.o $(CFLAGS) $(EXTRA_CFLAGS) -o mpdemo 
#	$(CC) demos/exec-mpdemo.o $(CFLAGS)  $(EXTRA_CFLAGS) -o execdemo

clean:
	$(RM) sigdemo mpdemo execdemo *.o 	
