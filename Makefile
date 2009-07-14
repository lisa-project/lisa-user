NAME := lisa

MAJOR := 2
MINOR := 0
PATCH := 1

VERSION := $(MAJOR).$(MINOR).$(PATCH)

PWD := $(shell pwd)
PATCH := $(PWD)/linux-2.6-lisa.patch
RPMDIR := $(shell rpm --eval %{_topdir})

.PHONY: all dist distopt package patch user rpm install tarball

all:
	@echo "This makefile accepts the following targets:"
	@echo "dist		Make a binary distribution"
	@echo "distopt		Make a binary distribution with optional binaries"
	@echo "tarball		Make a source distribution for the whole project"
	@echo "patch		Make a patch against the original kernel source"
	@echo "user		Build userspace programs"
	@echo "install		Install userspace programs to appropriate paths"

dist:
ifndef DST
	@echo "You must specify DST"
else
	cd userspace && make DIST=1
	./mkdist.sh $(DST)
endif

distopt:
ifndef DST
	@echo "You must specify DST"
else
	cd userspace && make DIST=1
	OPT=true ./mkdist.sh $(DST)
endif

tarball:
	mkdir $(NAME)
	cd userspace && make clean
	./depend.sh $(NAME)
	cp -r userspace scripts dist README* INSTALL LICENSE Makefile *.sh rpm $(NAME)
	tar czvf $(NAME).tar.gz $(NAME)
	rm -rf $(NAME)

patch:
	./mkpatch.sh -o $(PATCH)

user:
	cd userspace && make

install:
	cd userspace && make install
	install -m 755 -D scripts/rc.fedora $(ROOT)/etc/rc.d/init.d/lisa
	install -m 755 -D scripts/xen/network-lisa $(ROOT)/etc/xen/scripts/network-lisa
	install -m 755 -D scripts/xen/vif-lisa $(ROOT)/etc/xen/scripts/vif-lisa
	mkdir -p $(ROOT)/etc/lisa/tags
	touch $(ROOT)/etc/lisa/config.text

rpm:
	cp -f rpm/$(NAME).spec $(RPMDIR)/SPECS
	cp -f $(NAME).tar.gz $(RPMDIR)/SOURCES/$(NAME)-$(VERSION).tar.gz
	rpmbuild --define "lisa_version $(VERSION)" --define "lisa_kapi_version $(shell ./kver.sh)" -ba $(RPMDIR)/SPECS/$(NAME).spec
