NAME := lisa
KVER := 2.6.19

.PHONY: all dist distopt package patch user site

USE_EXIT_IN_CONF=1
export USE_EXIT_IN_CONF

prune_patch_paths := $(shell for i in linux-$(KVER)/config/*; do echo `basename $$i` | sed 's/^/--exclude /' ; done | xargs)

all:
	@echo "This makefile accepts the following targets:"
	@echo "dist		Make a binary distribution"
	@echo "distopt		Make a binary distribution with optional binaries"
	@echo "package		Make a source distribution for the whole project"
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

package: patch
	mkdir $(NAME)
	cp -r userspace dist README.dev README INSTALL LICENSE Makefile $(NAME)
	cd $(NAME)/userspace && make clean
	rm -rf `find $(NAME) -name .svn`
	mv linux-$(KVER)-lms.patch $(NAME)
	tar czvf $(NAME).tar.gz $(NAME)
	rm -rf $(NAME)

patch:
	diff -Pru --exclude .svn --exclude .config $(prune_patch_paths) linux-$(KVER).orig linux-$(KVER) | grep -v ^Only > linux-$(KVER)-lms.patch

user:
	cd userspace && make

site:
	tar zcvf site.tgz site --exclude site/.svn
