all: install
install: /usr/bin/zfs_mount
	mkinitcpio -P

.PHONY: all install

CFLAGS=-Wall -Wextra -Werror -isystem /usr/include/libspl -isystem /usr/include/libzfs
LDFLAGS=-lzfs -luutil

zfs_mount: zfs_mount.c

/usr/bin/zfs_mount: zfs_mount
	install -m755 $< $@
	strip $@
