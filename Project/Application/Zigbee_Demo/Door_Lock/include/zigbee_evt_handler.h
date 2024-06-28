// ---------------------------------------------------------------------------
//
// ---------------------------------------------------------------------------
#ifndef __ZIGBEE_EVT_HANDLER_H__
#define __ZIGBEE_EVT_HANDLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "zigbee_stack_api.h"

void zigbee_evt_handler(sys_tlv_t *pt_tlv);
void zigbee_app_dump_device_table(void);

#ifdef __cplusplus
};
#endif
#endif /* __ZIGBEE_EVT_HANDLER_H__ */
