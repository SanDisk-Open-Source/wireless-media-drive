# A long time ago, far, far away, under Solaris, you needed to
#    CFLAGS += -xO2 -Xc
#    LDLIBS += -lnsl -lsocket
# To cross-compile
#    CC = arm-linux-gcc
# To check for lint
#    CFLAGS += -Wpointer-arith -Wcast-align -Wcast-qual -Wshadow -Wundef \
#     -Waggregate-return -Wnested-externs -Winline -Wwrite-strings -Wstrict-prototypes

# This is old-school networking code, making the traditional cast between
# struct sockaddr* and struct sockaddr_in*.  Thus a modern gcc needs:
CFLAGS += -fno-strict-aliasing

CFLAGS += -std=c89
CFLAGS += -W -Wall
CFLAGS += -O2
# CFLAGS += -DPRECISION_SIOCGSTAMP
CFLAGS += -DENABLE_DEBUG
CFLAGS += -DENABLE_REPLAY
# CFLAGS += -DUSE_OBSOLETE_GETTIMEOFDAY

LDFLAGS += -lrt

all: ntpclient

test: ntpclient
	./ntpclient -d -r <test.dat

ntpclient: ntpclient.o phaselock.o

ntpclient.o phaselock.o: ntpclient.h

adjtimex: adjtimex.o

clean:
	rm -f ntpclient adjtimex *.o
