TOP_DIR = .

include $(TOP_DIR)/scripts/Makefile.variables

SUBDIRS = $(TOP_DIR)/include \
	  $(TOP_DIR)/cli     \
	  $(TOP_DIR)/tools   \
	  $(TOP_DIR)/swctl   \
	  $(TOP_DIR)/cdpd    \
	  $(TOP_DIR)/test

all:
	$(Q) for subdir in $(SUBDIRS) ; \
	do \
		make -C $${subdir}; \
	done

clean:
	$(Q) for subdir in $(SUBDIRS) ; \
	do \
		make -C $${subdir} $@ ; \
	done

install: all
	install -m 755 -D cdpd/cdpd $(ROOT)/usr/sbin/cdpd
	install -m 755 -D cli/filter $(ROOT)/bin/filter
ifneq ($(ROOT),)
	install -m 755 -D cli/reboot $(ROOT)/bin/reboot
endif
	install -m 755 -D cli/swcfgload $(ROOT)/usr/sbin/swcfgload
	install -m 755 -D cli/swcli $(ROOT)/usr/sbin/swcli
	install -m 755 -D cli/swclid $(ROOT)/usr/sbin/swclid
	install -m 755 -D cli/swcon $(ROOT)/usr/sbin/swcon
	install -m 755 -D cli/swlogin $(ROOT)/usr/sbin/swlogin
	install -m 755 -D cli/libswcli.so $(ROOT)/usr/lib/libswcli.so
ifneq ($(ROOT),)
	install -m 755 -D login/login $(ROOT)/bin/login
endif
	install -m 755 -D swctl/swctl $(ROOT)/usr/sbin/swctl
ifeq ($(ROOT),)
	/sbin/ldconfig
endif
