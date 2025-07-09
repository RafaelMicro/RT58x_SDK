#include "version_api.h"

int xxx;

void tt_(int a)
{
    xxx = a;
}

void lib_version_init(void)
{
    volatile const version_entry_t *pt_lib_ver;

    int a = 0x5678;

#ifdef BOOTLOADER
    bootloader_ver_get(pt_lib_ver);
#else
    //ble_lib_ver_get(pt_lib_ver);
    //ble_mesh_lib_ver_get(pt_lib_ver);
    //zigbee_lib_ver_get(pt_lib_ver);
    //thread_lib_ver_get(pt_lib_ver);
    //matter_lib_ver_get(pt_lib_ver);
    application_ver_get(pt_lib_ver);
#endif

    tt_(a);
}




