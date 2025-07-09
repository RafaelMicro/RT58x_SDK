#include "version_api.h"
#include "git_ver_info.h"

#ifdef BOOTLOADER
void bootloader_ver_get(volatile const version_entry_t *pt_ver_info)
{
    (void) *pt_ver_info;

    volatile const       version_entry_t bootloader_version_info =
    {
        .version_filed_name = "VERSION_NAME",
        .version_name = "bootloader_ver",
        .version_info_filed_name = "VERSION_INFO",
        .version_info = "194",
        .build_date_filed_name = "BUILD_DATE",
        .build_date = ""
    };
    pt_ver_info = &bootloader_version_info;
}

#else
#if 0
void ble_lib_ver_get(volatile const version_entry_t *pt_ver_info)
{

    (void) *pt_ver_info;

    volatile const       version_entry_t ble_lib_version_info =
    {
        .version_filed_name = "VERSION_NAME",
        .version_name = "ble_lib_ver@$GIT_TAG_NAME1",
        .version_info_filed_name = "VERSION_INFO",
        .version_info = "GIT_TAG_NAME1",
        .build_date_filed_name = "BUILD_DATE",
        .build_date = ""
    };

    pt_ver_info = &ble_lib_version_info;
}


void ble_mesh_lib_ver_get(volatile const version_entry_t *pt_ver_info)
{
    (void) *pt_ver_info;

    volatile const       version_entry_t ble_mesh_lib_version_info =
    {
        .version_filed_name = "VERSION_NAME",
        .version_name = "ble_mesh_lib_ver@$GIT_TAG_NAME2",
        .version_info_filed_name = "VERSION_INFO",
        .version_info = "GIT_TAG_NAME2",
        .build_date_filed_name = "BUILD_DATE",
        .build_date = ""
    };
    pt_ver_info = &ble_mesh_lib_version_info;

}

void zigbee_lib_ver_get(volatile const version_entry_t *pt_ver_info)
{
    (void) *pt_ver_info;

    volatile const       version_entry_t zigbee_lib_version_info =
    {
        .version_filed_name = "VERSION_NAME",
        .version_name = "zigbee_lib_ver@$GIT_TAG_NAME3",
        .version_info_filed_name = "VERSION_INFO",
        .version_info = LIB_VERSION,
        .build_date_filed_name = "BUILD_DATE",
        .build_date = BUILD_DATE
    };
    pt_ver_info = &zigbee_lib_version_info;
}


void thread_lib_ver_get(volatile const version_entry_t *pt_ver_info)
{
    (void) *pt_ver_info;

    volatile const       version_entry_t thread_lib_version_info =
    {
        .version_filed_name = "VERSION_NAME",
        .version_name = "thread_lib_ver@$GIT_TAG_NAME4",
        .version_info_filed_name = "VERSION_INFO",
        .version_info = "GIT_TAG_NAME4",
        .build_date_filed_name = "BUILD_DATE",
        .build_date = ""
    };
    pt_ver_info = &thread_lib_version_info;
}

void matter_lib_ver_get(volatile const version_entry_t *pt_ver_info)
{
    (void) *pt_ver_info;

    volatile const       version_entry_t matter_lib_version_info =
    {
        .version_filed_name = "VERSION_NAME",
        .version_name = "matter_lib_ver@$GIT_TAG_NAME5",
        .version_info_filed_name = "VERSION_INFO",
        .version_info = "GIT_TAG_NAME5",
        .build_date_filed_name = "BUILD_DATE",
        .build_date = ""
    };
    pt_ver_info = &matter_lib_version_info;
}
#endif
void application_ver_get(volatile const version_entry_t *pt_ver_info)
{
    (void) *pt_ver_info;

    volatile const       version_entry_t app_version_info =
    {
        .version_filed_name = "VERSION_NAME",
        .version_name = "RT200",
        .version_info_filed_name = "VERSION_INFO",
        .version_info = "20240827RFJP0004",
        .build_date_filed_name = "BUILD_DATE",
        .build_date = "20241101"
    };

    pt_ver_info = &app_version_info;

}
#endif
