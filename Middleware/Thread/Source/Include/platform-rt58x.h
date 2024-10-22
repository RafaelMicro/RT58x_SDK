/*
 *  Copyright (c) 2021, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes the platform-specific initializers.
 *
 */

#ifndef PLATFORM_RT58X_H_
#define PLATFORM_RT58X_H_

#include <openthread/instance.h>
#ifdef __cplusplus
extern "C" {
#endif
/*radio.c*/
void rafael_rfb_init(void);
void platformRadioProcess(otInstance *aInstance);
void rafael_radio_short_addr_ctrl(uint8_t ctrl_type, uint8_t *short_addr);
void rafael_radio_extend_addr_ctrl(uint8_t ctrl_type, uint8_t *extend_addr);
void rafael_radio_phy_wake_on_period_set(uint32_t wake_on_period);
void rafael_radio_phy_wake_on_start();
void rafael_radio_phy_wake_on_stop();
void radio_mac_broadcast_send(otInstance *aInstance, uint8_t *data, uint16_t lens);
int radio_mac_unicast_send(otInstance *aInstance, uint8_t *dst_mac_addr, uint8_t *data, uint16_t lens);
void radio_mac_received_callback(void (*mac_rcb)(uint8_t *data, uint16_t lens, int8_t rssi, uint8_t *src_addr));
uint32_t Get_Wake_On_Period();
void rafael_radio_mac_read_config_set(uint8_t mode);

/*entropy.c*/
void random_number_init(void);

/*uart.c*/
void UartProcessReceive(void);

/*alarm.c*/
void rt58x_alarm_init();
void rt58x_alarm_process(otInstance *aInstance);
uint64_t otPlatTimeGet(void);
uint32_t otPlatAlarmMilliGetNow(void);

#ifdef __cplusplus
};
#endif
#endif // PLATFORM_RT58X_H_
