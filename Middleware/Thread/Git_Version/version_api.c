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
void thread_lib_ver_get(volatile const version_entry_t *pt_ver_info)
{
    (void) *pt_ver_info;

    volatile const       version_entry_t thread_lib_version_info =
    {
        .version_filed_name = "VERSION_NAME",
        .version_name = "thread_lib_ver@$GIT_TAG_NAME4",
        .version_info_filed_name = "VERSION_INFO",
        .version_info = LIB_VERSION,
        .build_date_filed_name = "BUILD_DATE",
        .build_date = BUILD_DATE
    };
    pt_ver_info = &thread_lib_version_info;
}
#endif
