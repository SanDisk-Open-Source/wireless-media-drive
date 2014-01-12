CC = $(CROSS_COMPILE)gcc
CFLAGS = -O2 -Wall
CFLAGS += -DCONFIG_LIBNL20 -I$(NAS_ROOTFS)/usr/include -I$(NAS_ROOTFS)/include

LDFLAGS += -L$(NAS_ROOTFS)/lib -L$(NAS_ROOTFS)/usr/lib
LIBS += -lnl -lnl-genl -lm

OBJS = nvs.o misc_cmds.o calibrator.o plt.o ini.o

%.o: %.c calibrator.h nl80211.h plt.h nvs_dual_band.h
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(OBJS) 
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o calibrator

uim:
	$(CC) $(CFLAGS) $(LDFLAGS) uim_rfkill/$@.c -o $@

static: $(OBJS) 
	$(CC) $(LDFLAGS) --static $(OBJS) $(LIBS) -o calibrator


install:
	@echo Copy files to $(DESTDIR)/sbin
	@cp -f ./calibrator $(DESTDIR)/sbin

clean:
	@rm -f *.o calibrator uim
