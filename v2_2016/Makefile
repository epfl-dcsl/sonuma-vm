SUBDIRS = libsonuma softrmc_daemon softrmc_driver sample_code
CLEANDIRS = $(SUBDIRS:%=clean-%)

ODIR = bin
binaries := $(shell mkdir -p $(ODIR))

all: $(SUBDIRS)

#apps: libix

$(SUBDIRS):	
	$(MAKE) -C $@

clean: $(CLEANDIRS)
	rm -r bin
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

.PHONY: all clean $(SUBDIRS) $(CLEANDIRS)
