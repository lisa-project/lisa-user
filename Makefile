NAME := lms
KVER := 2.6.10

.PHONY: all package path

all: package

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
