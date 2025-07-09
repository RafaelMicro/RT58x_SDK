/**
 * @file radio.h
 * @author
 * @date
 * @brief Brief single line description use for indexing
 *
 * More detailed description can go here
 *
 *
 * @see http://
 */
#ifndef _RADIO_H_
#define _RADIO_H_
#include <stdbool.h>
#include "rfb_comm.h"
#include "rfb.h"
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/


typedef struct rfb_ctrl_s
{
    /**
    * @brief Initiate RFB, and register interrupt event.
    *
    * @param Rfb interrupt event struct [rx_done, tx_done, rx_timeout]
    *
    * @return Rfb wakeup time
    * @date 14 Dec. 2020
    * @see
    * @image
    */
    void (*init)(rfb_interrupt_event_t *_rfb_interrupt_event);
    /**
    * @brief Get RFB current state.
    *
    * @param None
    *
    * @return Current state of RFB [IDLE = 0, TX=3, RX = 5]
    * @date 14 Dec. 2020
    * @see
    * @image
    */
    uint8_t (*rf_state_get)(void);
    /**
    * @brief Set RFB modem type.
    *
    * @param Modem [RFB_MODEM_FSK]
    *
    * @return None
    * @date 17 Dec. 2020
    * @see
    * @image
    */
    void (*modem_set)(rfb_modem_type_t modem);
    /**
    * @brief Set RF frequency.
    *
    * @param RF frequency [116250~930000 (kHz)]
    *
    * @return None
    * @date 14 Dec. 2020
    * @see
    * @image
    */
    void (*frequency_set)(uint32_t rf_frequency);
    /**
    * @brief Check channel is free or not.
    *
    * @param RF frequency [116250~930000 (kHz) ]
    * @param Rssi_threshold [0->0dbm, 1->-1dBm, 2->-2dBm, ..., etc.]
    * @return Is channel free [1: channel is free 0: channel is not free]
    * @date 14 Dec. 2020
    * @see
    * @image
    */
    bool (*is_channel_free)(uint32_t rf_frequency, uint8_t rssi_threshold);
    /**
    * @brief Set RX configurations.
    *
    * @param Data rate FSK [FSK_2M = 0, FSK_1M = 1, FSK_500K = 2, FSK_200K = 3, FSK_100K = 4,
                            FSK_50K = 5, FSK_300K = 6, FSK_150K = 7, FSK_75K = 8]
    * @param Preamble length
             FSK [1~63]
    * @param 32bits timeout [us]
    * @param Is RX continuous mode [1: continous 0: one time/timeout]
    * @return None
    * @date 17 Dec. 2020
    * @see
    * @image
    */
    void (*rx_config_set)(uint8_t data_rate, uint16_t preamble_len, fsk_mod_t mod_idx, crc_type_t crc_type,
                          whiten_enable_t whiten_enable, uint32_t rx_timeout, bool rx_continuous);

    /**
    * @brief Set TX configurations.
    *
    * @param TX power [0: TX_POWER_20dBm , 1:TX_POWER_14dBm, 2:TX_POWER_0dBm]
    * @param Modulation index(fdev) [MOD_0P5 = 0, MOD_1 = 1] (FSK only)
    * @param Data rate
                       FSK [FSK_2M = 0, FSK_1M = 1, FSK_500K = 2, FSK_200K = 3, FSK_100K = 4,
                            FSK_50K = 5, FSK_300K = 6, FSK_150K = 7, FSK_75K = 8]
    * @param Preamble length
          FSK [1~63]
    * @return None
    * @date 14 Dec. 2020
    * @see
    * @image
    */
    void (*tx_config_set)(tx_power_level_t tx_power, uint8_t data_rate, uint16_t preamble_len, fsk_mod_t mod_idx,
                          crc_type_t crc_type, whiten_enable_t whiten_enable);
    /**
     * @brief Send data to RFB buffer and RFB will transmit this data automatically
     *
     * @param Data address
     * @param Packet length
     *         FSK [1~2047] / OQPSK [1~127]
     * @param Set ack request(4bits) [0: false, 1:true] (Only apply for 15p4 mode)
     * @param Data sequence number (Only apply for 15p4 mode)
     * @return None
     * @date 20 Jan. 2021
     * @see
     * @image
     */
    void (*data_send)(uint8_t *tx_data_address, uint16_t packet_length, uint8_t InitialCwAckRequest, uint8_t Dsn);
    /**
     * @brief Set RFB to sleep state
     *
     * @param None
     * @return None
     * @date 14 Dec. 2020
     * @see
     * @image
     */
    void (*sleep_set)(void);
    /**
     * @brief Set RFB to idle state
     *
     * @param None
     * @return None
     * @date 14 Dec. 2020
     * @see
     * @image
     */
    void (*idle_set)(void);
    /**
     * @brief Set RFB to RX state
     *
     * @param None
     * @return None
     * @date 14 Dec. 2020
     * @see
     * @image
     */
    void (*rx_start)(void);
    /**
     * @brief Set TX continuous wave (for testing, tx timeout is not supported)
     *
     * @param tx_enable [1: start, 0: stop]
     * @return None
     * @date 14 Dec. 2020
     * @see
     * @image
     */
    void (*tx_continuous_wave_set)(bool tx_enable);
    /**
     * @brief Read RSSI
     *
     * @param Modem [RFB_MODEM_SLINK, RFB_MODEM_FSK]
     * @return RSSI value [0->0dbm, 1->-1dBm, 2->-2dBm, ..., etc.]
     * @date 14 Dec. 2020
     * @see
     * @image
     */
    uint8_t (*rssi_read)(rfb_modem_type_t modem);
    /**
     * @brief Write HW register
     *
     * @param 16-bits register address
     * @param 32bits register value
     * @date 14 Dec. 2020
     * @see
     * @image
     */
    void (*register_write)(uint16_t reg_addr, uint32_t reg_val);
    /**
     * @brief Read HW register
     *
     * @param 16-bits register address
     * @return 32bits register value
     * @date 14 Dec. 2020
     * @see
     * @image
     */
    uint32_t (*register_read)(uint16_t reg_addr);
    /**
    * @brief Get RFB firmware version
    *
    * @param None
    * @return None
    * @date 20 Jan. 2021
    * @see
    * @image
    */
    uint32_t (*fw_version_get)(void);
} rfb_ctrl_t;

/**************************************************************************************************
 *    Global Prototypes
 *************************************************************************************************/
extern rfb_ctrl_t rfb_ctrl;
#endif

