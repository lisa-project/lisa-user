TOP_DIR = .

include $(TOP_DIR)/scripts/Makefile.variables

SUBDIRS = \
	$(TOP_DIR)/include \
	$(TOP_DIR)/switch \
	$(TOP_DIR)/lib     \
	$(TOP_DIR)/cli     \
	$(TOP_DIR)/tools   \
	$(TOP_DIR)/test

ifeq ($(IMPL), lisa)
SUBDIRS += \
	$(TOP_DIR)/cdpd    \
	$(TOP_DIR)/rstp
endif

DEPFILE = lisa.dep

all: build
build:
	$(Q) for subdir in $(SUBDIRS) ; \
	do \
		make -C $${subdir}; \
	done

clean:
	$(Q) for subdir in $(SUBDIRS) ; \
	do \
		make -C $${subdir} $@ ; \
	done
	$(Q)-rm -f $(DEPFILE)

dep:
	$(Q) for subdir in $(SUBDIRS) ; \
	do \
		make -C $${subdir} $@ ; \
	done
	$(Q)-touch $(DEPFILE)
	$(Q)-find . -name '*.d' -exec cat {} >> $(DEPFILE) \;

install: build
	install -m 755 -D cdpd/cdpd $(ROOT)/usr/sbin/cdpd
	install -m 755 -D tools/filter $(ROOT)/bin/filter
	install -m 755 -D tools/swclid $(ROOT)/usr/sbin/swclid
ifneq ($(ROOT),)
	install -m 755 -D tools/reboot $(ROOT)/bin/reboot
endif
	install -m 755 -D cli/swcfgload $(ROOT)/usr/sbin/swcfgload
	install -m 755 -D cli/swcli $(ROOT)/usr/sbin/swcli
	install -m 755 -D cli/swcon $(ROOT)/usr/sbin/swcon
	install -m 755 -D cli/swlogin $(ROOT)/usr/sbin/swlogin
	install -m 755 -D cli/libswcli.so $(ROOT)/usr/lib/libswcli.so
	install -m 755 -D switch/libswitch.so $(ROOT)/usr/lib/libswitch.so
ifneq ($(ROOT),)
	install -m 755 -D tools/login $(ROOT)/bin/login
endif
	install -m 755 -D tools/swctl $(ROOT)/usr/sbin/swctl
ifeq ($(ROOT),)
	/sbin/ldconfig
endif
