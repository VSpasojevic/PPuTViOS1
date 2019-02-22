CROSS_COMPILE=$(TOOLCHAIN_CROSS_COMPILE)

CC_PREFIX=$(CROSS_COMPILE)-
CC=$(CC_PREFIX)gcc
CXX=$(CC_PREFIX)g++
LD=$(CC_PREFIX)ld

CFLAGS= -I. \
        -I./include/ \
        -I.$(SDK_INCLUDE)
        
CFLAGS += -D__LINUX__ -O0 -Wno-psabi 

CXXFLAGS = $(CFLAGS)

LIBS_PATH = -L$(SYSROOT)/home/galois/lib/

#LIBS := $(LIBS_PATH) -lpthread

SRC= vezba_1.c

all: remote_sample

remote_sample:
	$(CC) -o vezba_1 $(INCS) $(SRC) $(CFLAGS) $(LIBS)
    
clean:
	rm -f vezba_1
