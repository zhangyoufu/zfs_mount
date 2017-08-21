#include <libzfs.h>
#include <string.h>
#include <sys/mntent.h>

static int
zfs_add_option(zfs_handle_t *zhp, char *options, int len,
    zfs_prop_t prop, char *on, char *off)
{
    char *source;
    uint64_t value;

    /* Skip adding duplicate default options */
    if ((strstr(options, on) != NULL) || (strstr(options, off) != NULL))
        return (0);

    /*
     * zfs_prop_get_int() is not used to ensure our mount options
     * are not influenced by the current /proc/self/mounts contents.
     */
    value = getprop_uint64(zhp, prop, &source);

    (void) strlcat(options, ",", len);
    (void) strlcat(options, value ? on : off, len);

    return (0);
}

static int
zfs_add_options(zfs_handle_t *zhp, char *options, int len)
{
    int error = 0;

    error = zfs_add_option(zhp, options, len,
        ZFS_PROP_ATIME, MNTOPT_ATIME, MNTOPT_NOATIME);
    /*
     * don't add relatime/strictatime when atime=off, otherwise strictatime
     * will force atime=on
     */
    if (strstr(options, MNTOPT_NOATIME) == NULL) {
        error = zfs_add_option(zhp, options, len,
            ZFS_PROP_RELATIME, MNTOPT_RELATIME, MNTOPT_STRICTATIME);
    }
    error = error ? error : zfs_add_option(zhp, options, len,
        ZFS_PROP_DEVICES, MNTOPT_DEVICES, MNTOPT_NODEVICES);
    error = error ? error : zfs_add_option(zhp, options, len,
        ZFS_PROP_EXEC, MNTOPT_EXEC, MNTOPT_NOEXEC);
    error = error ? error : zfs_add_option(zhp, options, len,
        ZFS_PROP_READONLY, MNTOPT_RO, MNTOPT_RW);
    error = error ? error : zfs_add_option(zhp, options, len,
        ZFS_PROP_SETUID, MNTOPT_SETUID, MNTOPT_NOSETUID);
    error = error ? error : zfs_add_option(zhp, options, len,
        ZFS_PROP_NBMAND, MNTOPT_NBMAND, MNTOPT_NONBMAND);

    return (error);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <mount options> <zfs dataset> <mountpoint>\n", argv[0]);
        return 1;
    }

    char *options = argv[1];
    char *dataset = argv[2];
    char *mountpoint = argv[3];

    libzfs_handle_t *libzfs_handle = libzfs_init();
    if (libzfs_handle == NULL) {
        fprintf(stderr, "%s", libzfs_error_init(errno));
        return 1;
    }
    libzfs_print_on_error(libzfs_handle, B_TRUE);

    zfs_handle_t *zfs_handle = zfs_open(libzfs_handle, dataset, ZFS_TYPE_FILESYSTEM);
    if (zfs_handle == NULL) {
        fputs("zfs_open failed\n", stderr);
        return 1;
    }

    char mntopts[MNT_LINE_MAX];
    strlcpy(mntopts, options, sizeof(mntopts));

    if (zfs_add_options(zfs_handle, mntopts, sizeof(mntopts))) {
        fputs("zfs_add_options failed\n", stderr);
        return 1;
    }

    zfs_close(zfs_handle);
    libzfs_fini(libzfs_handle);

    execl("/bin/mount", "", "-t", MNTTYPE_ZFS, "-o", mntopts, dataset, mountpoint, NULL);
    fputs("execl failed\n", stderr);
    return 1;
}
