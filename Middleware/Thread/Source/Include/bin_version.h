#ifndef __BIN_VERSION_H
#define __BIN_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

#define PREFIX_LEN                              7
#define FW_INFO_LEN                             16

typedef struct
{
    uint8_t prefix[PREFIX_LEN];
    uint8_t sysinfo[FW_INFO_LEN];
    uint8_t feature_list;
} sys_information_t;

#define BULID_YEAR (((__DATE__ [9] - '0') * 10) + (__DATE__ [10] - '0'))

#define BULID_MONTH (\
  __DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 1 : 6) \
: __DATE__ [2] == 'b' ? 2 \
: __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 3 : 4) \
: __DATE__ [2] == 'y' ? 5 \
: __DATE__ [2] == 'l' ? 7 \
: __DATE__ [2] == 'g' ? 8 \
: __DATE__ [2] == 'p' ? 9 \
: __DATE__ [2] == 't' ? 10 \
: __DATE__ [2] == 'v' ? 11 \
: 12)

#define BULID_DAY (((__DATE__ [4] - '0') * 10) + (__DATE__ [5] - '0'))

#define BULID_HOUR (((__TIME__ [0] - '0') * 10) + (__TIME__ [1] - '0'))

#define BULID_MINUTE (((__TIME__ [3] - '0') * 10) + (__TIME__ [4] - '0'))

#define BULID_SECOND (((__TIME__ [6] - '0') * 10) + (__TIME__ [7] - '0'))

#define SYSTEMINFO_INIT(BIN_TYPE_ARR) {\
  "VerGet", \
  {(uint8_t)BULID_YEAR, (uint8_t)BULID_MONTH, (uint8_t)BULID_DAY, (uint8_t)(BULID_HOUR+BULID_MINUTE+BULID_SECOND), \
  BIN_TYPE_ARR}, \
  0x1 \
}

#define GET_BIN_VERSION(sysinfo) ( ((uint32_t)(sysinfo)[0] << 24) | \
                                       ((uint32_t)(sysinfo)[1] << 16) | \
                                       ((uint32_t)(sysinfo)[2] << 8)  | \
                                       ((uint32_t)(sysinfo)[3]) )

#define GET_BIN_TYPE_PTR(sysinfo) ((const uint8_t*)(&(sysinfo)[4]))

extern const sys_information_t systeminfo;

#ifdef __cplusplus
};
#endif
#endif