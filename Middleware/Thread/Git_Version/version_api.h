#include "cm3_mcu.h"


typedef struct __version_entry_s
{
    char version_filed_name[13];
    char version_name[40];
    char version_info_filed_name[13];
    char version_info[40];
    char build_date_filed_name[11];
    char build_date[30];
} version_entry_t;


// void ble_lib_ver_get(volatile const version_entry_t *pt_ver_info);
// void ble_mesh_lib_ver_get(volatile const version_entry_t *pt_ver_info);
// void zigbee_lib_ver_get(volatile const version_entry_t *pt_ver_info);
void thread_lib_ver_get(volatile const version_entry_t *pt_ver_info);
// void matter_lib_ver_get(volatile const version_entry_t *pt_ver_info);
// void bootloader_ver_get(volatile const version_entry_t *pt_ver_info);
// void application_ver_get(volatile const version_entry_t *pt_ver_info);

