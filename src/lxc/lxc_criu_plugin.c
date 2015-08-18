#include <errno.h>
#include <criu/criu-log.h>
#include <criu/criu-plugin.h>

static int
lxcCriuDumpExtMount(char *mountpoint, int id) {

    /* Things we'll encounter:
     *   /proc/meminfo -> FUSE: no need to dump, just restore
     *   /sys/fs/cgroup -> nothing to do: just restore
     *   All bind mounts from the guest definition, just restore later
     *
     * Say amen to all ext mount dumps but do nothing, just log them in case
     * we missed something
     */
    return 0;
}

static int
lxcCriuRestoreExtMount(int id,
                       char *mountpoint,
                       char *old_root,
                       int *isFile) {

    return -ENOTSUP;
}


#ifdef CR_PLUGIN_REGISTER_DUMMY

CR_PLUGIN_REGISTER_DUMMY("libvirt_lxc")
CR_PLUGIN_REGISTER_HOOK(CR_PLUGIN_HOOK__DUMP_EXT_MOUNT, lxcCriuDumpExtMount)
CR_PLUGIN_REGISTER_HOOK(CR_PLUGIN_HOOK__RESTORE_EXT_MOUNT, lxcCriuRestoreExtMount)

#else /* CR_PLUGIN_REGISTER_DUMMY */

/* These are simply wrapper functions for the old plugin API.
 * This API is deprecated, but we may want to build against criu
 * post-1.1 and pre-1.4 */
int cr_plugin_dump_ext_mount(char *mountpoint, int id) {
    return lxcCriuDumpExtMount(mountpoint, id);
}

int cd_plugin_restore_ext_mount(int id,
                                char *mountpoint,
                                char *old_root,
                                int *isFile) {
    return lxcCriuRestoreExtMount(id, mountpoint, old_root, isFile);
}
#endif /* CR_PLUGIN_REGISTER_DUMMY */
