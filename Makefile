NAME := lisa
PWD := $(shell pwd)
PATCH := $(PWD)/linux-2.6-lisa.patch
VERSION := $(shell grep '^%define date_version ' lisa.spec | sed 's/^%define date_version \(.*\)$$/\1/')
RPMDIR := $(shell rpm --eval %{_topdir})

.PHONY: all dist distopt package patch user

USE_EXIT_IN_CONF=1
export USE_EXIT_IN_CONF

all:
	@echo "This makefile accepts the following targets:"
	@echo "dist		Make a binary distribution"
	@echo "distopt		Make a binary distribution with optional binaries"
	@echo "tarball		Make a source distribution for the whole project"
	@echo "patch		Make a patch against the original kernel source"
	@echo "user		Build userspace programs"

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

tarball: patch
	mkdir $(NAME)
	./depend.sh | xargs cp -t $(NAME) --parents
	cp -r userspace scripts dist README* INSTALL LICENSE Makefile *.sh $(NAME)
	cd $(NAME)/userspace && make clean
	mv $(PATCH) $(NAME)
	tar czvf $(NAME).tar.gz $(NAME)
	rm -rf $(NAME)

patch:
	./mkpatch.sh $(PATCH)

user:
	cd userspace && make

rpm: tarball
	cp -f $(NAME).spec $(RPMDIR)/SPECS
	cp -f $(NAME).tar.gz $(RPMDIR)/SOURCES/$(NAME)-$(VERSION).tar.gz
	rpmbuild -ba $(RPMDIR)/SPECS/$(NAME).spec
