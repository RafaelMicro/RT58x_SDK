#include "version_api.h"
#include "util_log.h"
#include "git_ver_info.h"

void lib_version_init(void)
{
    volatile const version_entry_t *pt_lib_ver;

#ifdef BOOTLOADER
    bootloader_ver_get(pt_lib_ver);
#else
    //ble_lib_ver_get(pt_lib_ver);
    //ble_mesh_lib_ver_get(pt_lib_ver);
    //zigbee_lib_ver_get(pt_lib_ver);
    thread_lib_ver_get(pt_lib_ver);
    //matter_lib_ver_get(pt_lib_ver);
    // application_ver_get(pt_lib_ver);
    info("Lib Version        : %s \r\n", LIB_VERSION);
#endif
}
