NAME := lms
KVER := 2.6.10

.PHONY: all dist distopt package patch user

USE_EXIT_IN_CONF=1
export USE_EXIT_IN_CONF

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
	cp -r userspace README INSTALL LICENSE $(NAME)
	cd $(NAME)/userspace && make clean
	rm -rf $(NAME)/userspace/.svn
	mv linux-$(KVER)-lms.patch $(NAME)
	tar czvf $(NAME).tar.gz $(NAME)
	rm -rf $(NAME)

patch:
	diff -Pru --exclude .svn --exclude .config linux-$(KVER).orig linux-$(KVER) | grep -v ^Only > linux-$(KVER)-lms.patch

user:
	cd userspace && make
