NAME := linux-multilayer-switch

.PHONY: all package path

all: package

package: patch
	mkdir $(NAME)
	cp -r userspace README INSTALL LICENSE $(NAME)
	cd $(NAME)/userspace && make clean
	rm -rf $(NAME)/userspace/.svn
	mv linux-2.6.10-multilayer-switch.patch $(NAME)
	tar czvf $(NAME).tar.gz $(NAME)
	rm -rf $(NAME)

patch:
	diff -Pru --exclude .svn --exclude .config linux-2.6.10.orig linux-2.6.10 | grep -v ^Only > linux-2.6.10-multilayer-switch.patch
