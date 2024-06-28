/**
 * @file rfb.h
 * @author
 * @date
 * @brief Brief single line description use for indexing
 *
 * More detailed description can go here
 *
 *
 * @see http://
 */
#ifndef _RFB_H_
#define _RFB_H_
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/


/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
typedef enum
{
    RFB_EVENT_SUCCESS = 0,
    RFB_CNF_EVENT_INVALID_CMD,
    RFB_CNF_EVENT_RX_BUSY,
    RFB_CNF_EVENT_TX_BUSY,
    RFB_CNF_EVENT_UNSUPPORT_CMD,
    RFB_CNF_EVENT_NOT_AVAILABLE,
    RFB_CNF_EVENT_CONTENT_ERROR,
    RFB_RSP_EVENT_NOT_AVAILABLE,
    RFB_RSP_EVENT_CONTENT_ERROR
} RFB_EVENT_STATUS;

typedef enum
{
    RFB_CMN_EVENT_SUCCESS = 0,
    RFB_CMN_CNF_EVENT_INVALID_CMD,
    RFB_CMN_CNF_EVENT_RX_BUSY,
    RFB_CMN_CNF_EVENT_TX_BUSY,
    RFB_CMN_CNF_EVENT_UNSUPPORT_CMD,
    RFB_CMN_CNF_EVENT_NOT_AVAILABLE,
    RFB_CMN_CNF_EVENT_CONTENT_ERROR,
    RFB_CMN_RSP_EVENT_NOT_AVAILABLE,
    RFB_CMN_RSP_EVENT_CONTENT_ERROR
} RFB_CMN_EVENT_STATUS;

typedef enum
{
    RFB_WRITE_TXQ_SUCCESS = 0,
    RFB_WRITE_TXQ_FULL
} RFB_WRITE_TXQ_STATUS;

typedef enum
{
    RFB_KEYING_FSK               = 1,
    RFB_KEYING_OQPSK             = 2,
} rfb_keying_type_t;

typedef enum
{
    RFB_MODEM_FSK                = 1,
    RFB_MODEM_ZIGBEE             = 2,
    RFB_MODEM_BLE                = 3,
    RFB_MODEM_OQPSK              = 4,
} rfb_modem_type_t;

typedef struct rfb_interrupt_event_s
{
    void (*rx_done)(uint16_t packet_length, uint8_t *rx_data_address, uint8_t crc_status, uint8_t rssi, uint8_t snr);
    void (*tx_done)(uint8_t tx_status);
    void (*rx_timeout)(void);
    void (*rtc)(void);
} rfb_interrupt_event_t;

typedef enum
{
    BAND_SUBG_915M   = 0,
    BAND_2P4G        = 1,
    BAND_SUBG_868M   = 2,
    BAND_SUBG_433M   = 3,
    BAND_SUBG_315M   = 4
} band_type_t;

typedef enum
{
    BAND_OQPSK_SUBG_915M   = 0,
    BAND_OQPSK_SUBG_868M   = 1,
    BAND_OQPSK_SUBG_433M   = 2,
    BAND_OQPSK_SUBG_315M   = 3
} oqpsk_band_type_t;

typedef enum
{
    TX_POWER_20dBm     = 0,
    TX_POWER_14dBm     = 1,
    TX_POWER_0dBm      = 2
} tx_power_level_t;

typedef enum
{
    FSK_PREAMBLE_TYPE_0     = 0,  /*01010101*/
    FSK_PREAMBLE_TYPE_1     = 1   /*10101010*/
} fsk_preamble_type_t;

typedef enum
{
    SFD_4BYTE           = 0,
    SFD_2BYTE           = 1
} sfd_type_t;

typedef enum
{
    FSK_200K             = 3,
    FSK_100K             = 4,
    FSK_50K              = 5,
    FSK_300K             = 6,
    FSK_150K             = 7
} fsk_bw_t;

typedef enum
{
    OQPSK_25K            = 3,
    OQPSK_12P5K          = 4,
    OQPSK_6P25K          = 5,
} oqpsk_bw_t;

typedef enum
{
    MOD_0P5             = 0,
    MOD_1               = 1,
    MOD_UNDEF           = 2,
} fsk_mod_t;

typedef enum
{
    CRC_16              = 0,
    CRC_32              = 1
} crc_type_t;

typedef enum
{
    FSK              = 0,
    GFSK             = 1,
    OQPSK            = 2,
} fsk_filter_type_t;

typedef enum
{
    WHITEN_DISABLE     = 0,
    WHITEN_ENABLE      = 1
} whiten_enable_t;

typedef enum
{
    CLEAR_ALL          = 0,
    ADD_AN_ADDRESS     = 1,
    REMOVE_AN_ADDRESS  = 2,
} src_match_ctrl_t;

#if (defined RFB_ZIGBEE_ENABLED && RFB_ZIGBEE_ENABLED == 1)
typedef struct rfb_zb_ctrl_s
{
    /**
    * @brief Initiate RFB, and register interrupt event.
    *
    * @param Rfb interrupt event struct [rx_done, tx_done, rx_timeout]
    *
    * @return command setting result
    * @date 14 Dec. 2020
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*init)(rfb_interrupt_event_t *_rfb_interrupt_event);

    /**
    * @brief Set RF frequency.
    *
    * @param RF frequency [2402~2480 (MHz)]
    *
    * @return command setting result
    * @date 14 Dec. 2020
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*frequency_set)(uint32_t rf_frequency);

    /**
    * @brief Check channel is free or not.
    *
    * @param RF frequency [2402~2480 (MHz)]
    * @param Rssi_threshold [0->0dbm, 1->-1dBm, 2->-2dBm, ..., etc.]
    * @return Is channel free [1: channel is free 0: channel is not free]
    * @date 14 Dec. 2020
    * @see
    * @image
    */
    bool (*is_channel_free)(uint32_t rf_frequency, uint8_t rssi_threshold);

    /**
    * @brief Send data to RFB buffer and RFB will transmit this data automatically
    *
    * @param Data address
    * @param Packet length
    *         Zigbee 1~127
    * @param TX control
    *        BIT1: [0: DIRECT_TRANSMISSION, 1:NONBEACON_MODE_CSMACA]
    *        BIT0: [0: ACK request = false, ACK request = true] (auto ack feature must enable)
    * @param Data sequence number (auto ack feature must enable)
    * @return Write Tx Queue result
    * @date 20 Jan. 2021
    * @see
    * @image
    */
    RFB_WRITE_TXQ_STATUS (*data_send)(uint8_t *tx_data_address, uint16_t packet_length, uint8_t tx_control, uint8_t dsn);

    /**
    * @brief Set TX continuous wave (for testing, tx timeout is not supported)
    *
    * @param tx_enable [1: start, 0: stop]
    * @return command setting result
    * @date 14 Dec. 2020
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*tx_continuous_wave_set)(bool tx_enable);

    /**
    * @brief Read RSSI
    *
    * @param Modem [RFB_MODEM_ZIGBEE]
    * @return RSSI value [0->0dbm, 1->-1dBm, 2->-2dBm, ..., etc.]
    * @date 14 Dec. 2020
    * @see
    * @image
    */
    uint8_t (*rssi_read)(rfb_modem_type_t modem);

    /**
    * @brief Set 15.4 address filter
    * @param Activate peomiscuous mode by set mac_promiscuous_mode to true
    * @param 16bits short address.
    * @param 32bits long address[0]
    * @param 32bits long address[1]
    *        long address[0] and [1] contain 64 bits extended address
    * @param 16bits PAN ID.
    * @param Whether the device id coordinator or mot [0:false, 1:true]
    * @return command setting result
    * @date 17 Dec. 2021
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*address_filter_set)(uint8_t mac_promiscuous_mode, uint16_t short_source_address, uint32_t long_source_address_0, uint32_t long_source_address_1, uint16_t pan_id, uint8_t isCoordinator);

    /**
    * @brief Set 15.4 MAC PIB
    * @param The time forming the basic time period used by the CSMA-CA algorithm, specified in us.
    * @param The maximum time to wait for an acknowledgment frame to arrive following a transmitted data frame, specified in us.
    * @param The maximum value of the backoff exponent, BE, in the CSMA-CA algorithm
    * @param The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel access failure
    * @param The maximum time to wait either for a frame intended as a response to a data request frame, specified in us.
    * @param The maximum number of retries allowed after a transmission failure.
    * @param The minimum value of the backoff exponent (BE) in the CSMA-CA algorithm
    * @return command setting result
    * @date 17 Dec. 2021
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*mac_pib_set)(uint32_t a_unit_backoff_period, uint32_t mac_ack_wait_duration, uint8_t mac_max_BE, uint8_t mac_max_CSMA_backoffs,
                                    uint32_t mac_max_frame_total_wait_time, uint8_t mac_max_frame_retries, uint8_t mac_min_BE);

    /**
    * @brief Set 15.4 PHY PIB
    * @param RX-to-TX or TX-to-RX turnaround time, specified in us.
    * @param 0: Energy above threshold, 1: Carrier sense only, 2: Carrier sense with energy above threshold, where the logical operator is AND.
    *        3: Carrier sense with energy above threshold, where the logical operator is OR.
    * @param The received power threshold of the, energy above threshold, algorithm.
    * @param The duration for CCA, specified in us.
    * @return command setting result
    * @date 17 Dec. 2021
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*phy_pib_set)(uint16_t a_turnaround_time, uint8_t phy_cca_mode, uint8_t phy_cca_threshold, uint16_t phy_cca_duration);

    /**
    * @brief Enable auto ACK
    *
    * @param Enable auto ACK flag
    * @return command setting result
    * @date 20 Jan. 2021
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*auto_ack_set)(uint8_t auto_ack);

    /**
    * @brief Set frame pending bit
    *
    * @param frame pending bit flag [0:false, 1:true]
    * @return command setting result
    * @date 20 Jan. 2021
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*frame_pending_set)(uint8_t frame_pending);

    /**
    * @brief Set RX on when IDLW
    *
    * @param the flag for transfering to RX state automatically when RFB is idle [0:false, 1:true]; Th
    * @return command setting result
    * @date 20 Jan. 2021
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*auto_state_set)(bool rx_on_when_idle);

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

    /**
    * @brief Enable/ disable source address match feature
    *
    * @param Enable flag [0:disable, 1:enable]
    * @return command setting result
    * @date 17 Aug. 2022
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*src_addr_match_ctrl)(uint8_t ctrl_type);

    /**
    * @brief Control the short source address table
    *
    * @param Control type [0: clear src matching table,
    *                      1: add/set address entry - ack pending bit,
    *                      2: clear address entry   - ack pending bit,
    *                      3: add/set address entry - link metrics LQI (ehn-ack),
    *                      4: clear address entry   - link metrics LQI (ehn-ack),
    *                      5: add/set address entry - link metrics link margin (ehn-ack),
    *                      6: clear address entry   - link metrics link margin (ehn-ack),
    *                      7: add/set address entry - link metrics RSSI (ehn-ack),
    *                      8: clear address entry   - link metrics RSSI (ehn-ack),
    *                      9: find address entry]
    * @param The pointer for short address
    * @return None
    * @date 17 Aug. 2022
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*short_addr_ctrl)(uint8_t ctrl_type, uint8_t *short_addr);

    /**
    * @brief Control the short source address table
    *
    * @param Control type [0: clear src matching table,
    *                      1: add/set address entry - ack pending bit,
    *                      2: clear address entry   - ack pending bit,
    *                      3: add/set address entry - link metrics LQI (ehn-ack),
    *                      4: clear address entry   - link metrics LQI (ehn-ack),
    *                      5: add/set address entry - link metrics link margin (ehn-ack),
    *                      6: clear address entry   - link metrics link margin (ehn-ack),
    *                      7: add/set address entry - link metrics RSSI (ehn-ack),
    *                      8: clear address entry   - link metrics RSSI (ehn-ack),
    *                      9: find address entry]
    * @param The pointer for short address
    * @return None
    * @date 17 Aug. 2022
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*extend_addr_ctrl)(uint8_t ctrl_type, uint8_t *extend_addr);

    /**
    * @brief Configure 128bits encryption key
    *
    * @param The pointer for key
    * @return command setting result
    * @date 30 Aug. 2022
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*key_set)(uint8_t *key_addr);
    /**
    * @brief Enable/ disable the CSL receiver
    *
    * @param Enable flag [0:disable, 1:enable]
    * @param CSL period
    * @return command setting result
    * @date 8 Sep. 2022
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*csl_receiver_ctrl)(uint8_t csl_receiver_ctrl, uint16_t csl_period);
    /**
    * @brief Get the current accuracy
    *
    * @param None
    * @return The current accuracy
    * @date 8 Sep. 2022
    * @see
    * @image
    */
    uint8_t (*csl_accuracy_get)(void);
    /**
    * @brief Get the current uncertainty
    *
    * @param None
    * @return The current uncertainty
    * @date 8 Sep. 2022
    * @see
    * @image
    */
    uint8_t (*csl_uncertainty_get)(void);
    /**
    * @brief Update CSL sample time
    *
    * @param None
    * @return command setting result
    * @date 13 Sep. 2022
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*csl_sample_time_update)(uint32_t csl_sample_time);
    /**
    * @brief Read RFB RTC time
    *
    * @param None
    * @return The current RTC time
    * @date 20 Sep. 2022
    * @see
    * @image
    */
    uint32_t (*rtc_time_read)(void);
    /**
    * @brief Read ack pkt data
    *
    * @param rx_data_address
    * @param rx_time_address
    * @return The length of ack pkt
    * @date 18 Dec. 2022
    * @see
    * @image
    */
    uint8_t (*ack_packet_read)(uint8_t *rx_data_address, uint8_t *rx_time_address, bool is2bytephr);
    /**
    * @brief Read RFB RX RTC time
    *
    * @param rx_cnt: total_rx_done_cnt%5 (start from 0)
    * @return The RX RTC time stored in local data q
    * @date 18 Dec. 2022
    * @see
    * @image
    */
    uint32_t (*rx_rtc_time_get)(uint8_t rx_cnt);
    /**
    * @brief Read RFB current channel
    *
    * @param rx_cnt: total_rx_done_cnt%5 (start from 0)
    * @return The channel number (for 2.4G)
    * @date 6 Mar. 2023
    * @see
    * @image
    */
    uint8_t (*current_channel_get)(uint8_t rx_cnt);
    /**
    * @brief Read RFB current frame counter
    *
    * @param None
    * @return The frame counter value
    * @date 6 Mar. 2023
    * @see
    * @image
    */
    uint32_t (*frame_counter_get)(void);
} rfb_zb_ctrl_t;
#endif

#if (defined RFB_SUBG_ENABLED && RFB_SUBG_ENABLED == 1)
typedef struct rfb_subg_ctrl_s
{
    /**
    * @brief Initiate RFB, and register interrupt event.
    *
    * @param Rfb interrupt event struct [rx_done, tx_done, rx_timeout]
    * @param keying_mode [RFB_KEYING_FSK / RFB_KEYING_OQPSK]
    * @param band_type [BAND_SUBG_915M / BAND_2P4G / BAND_SUBG_868M / BAND_SUBG_433M / BAND_SUBG_315M] for FSK
    *                  [BAND_OQPSK_SUBG_915M / BAND_OQPSK_SUBG_868M / BAND_OQPSK_SUBG_433M / BAND_OQPSK_SUBG_315M] for OQPSK
    *
    * @return Rfb setting result
    * @date 07 Dec. 2022
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*init)(rfb_interrupt_event_t *_rfb_interrupt_event, rfb_keying_type_t keying_mode, uint8_t band_type);

    /**
    * @brief Set RFB modem type.
    *
    * @param Modem [RFB_MODEM_FSK]
    * @param band_type [BAND_SUBG_915M / BAND_2P4G / BAND_SUBG_868M / BAND_SUBG_433M / BAND_SUBG_315M] for FSK
    *                  [BAND_OQPSK_SUBG_915M / BAND_OQPSK_SUBG_868M / BAND_OQPSK_SUBG_433M / BAND_OQPSK_SUBG_315M] for OQPSK
    *
    * @return command setting result
    * @date 17 Dec. 2020
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*modem_set)(rfb_modem_type_t modem, uint8_t band_type);

    /**
    * @brief Set RF frequency.
    *
    * @param RF frequency [116250~930000 (kHz)]
    *
    * @return command setting result
    * @date 14 Dec. 2020
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*frequency_set)(uint32_t rf_frequency);

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
    * @return command setting result
    * @date 17 Dec. 2020
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*rx_config_set)(uint8_t data_rate, uint16_t preamble_len, fsk_mod_t mod_idx, crc_type_t crc_type,
                                      whiten_enable_t whiten_enable, uint32_t rx_timeout, bool rx_continuous, uint8_t filter_type);

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
    * @return command setting result
    * @date 14 Dec. 2020
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*tx_config_set)(tx_power_level_t tx_power, uint8_t data_rate, uint16_t preamble_len, fsk_mod_t mod_idx,
                                      crc_type_t crc_type, whiten_enable_t whiten_enable, uint8_t filter_type);

    /**
     * @brief Send data to RFB buffer and RFB will transmit this data automatically
     *
     * @param Data address
     * @param Packet length
     *         FSK [1~2047] / OQPSK [1~127]
     * @param TX control [0](Not apply for 15p4g mode)
     * @param Data sequence number [0] (Not apply for 15.4g mode)
     * @return Write Tx queue result
     * @date 20 Jan. 2021
     * @see
     * @image
     */
    RFB_WRITE_TXQ_STATUS (*data_send)(uint8_t *tx_data_address, uint16_t packet_length, uint8_t tx_control, uint8_t dsn);

    /**
     * @brief Set RFB to sleep state
     *
     * @param None
     * @return command setting result
     * @date 14 Dec. 2020
     * @see
     * @image
     */
    RFB_EVENT_STATUS (*sleep_set)(void);

    /**
     * @brief Set RFB to idle state
     *
     * @param None
     * @return command setting result
     * @date 14 Dec. 2020
     * @see
     * @image
     */
    RFB_EVENT_STATUS (*idle_set)(void);

    /**
     * @brief Set RFB to RX state
     *
     * @param None
     * @return command setting result
     * @date 14 Dec. 2020
     * @see
     * @image
     */
    RFB_EVENT_STATUS (*rx_start)(void);

    /**
     * @brief Set TX continuous wave (for testing, tx timeout is not supported)
     *
     * @param tx_enable [1: start, 0: stop]
     * @return command setting result
     * @date 14 Dec. 2020
     * @see
     * @image
     */
    RFB_EVENT_STATUS (*tx_continuous_wave_set)(bool tx_enable);

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
    * @brief Get RFB firmware version
    *
    * @param None
    * @return None
    * @date 20 Jan. 2021
    * @see
    * @image
    */
    uint32_t (*fw_version_get)(void);

    /**
     * @brief Set 15.4 address filter
     * @param Activate peomiscuous mode by set mac_promiscuous_mode to true
     * @param 16bits short address.
     * @param 32bits long address[0]
     * @param 32bits long address[1]
     *        long address[0] and [1] contain 64 bits extended address
     * @param 16bits PAN ID.
     * @param Whether the device id coordinator or mot [0:false, 1:true]
     * @return command setting result
     * @date 17 Dec. 2021
     * @see
     * @image
     */
    RFB_EVENT_STATUS (*address_filter_set)(uint8_t mac_promiscuous_mode, uint16_t short_source_address, uint32_t long_source_address_0, uint32_t long_source_address_1, uint16_t pan_id, uint8_t isCoordinator);

    /**
    * @brief Set 15.4 MAC PIB
    * @param The time forming the basic time period used by the CSMA-CA algorithm, specified in us.
    * @param The maximum time to wait for an acknowledgment frame to arrive following a transmitted data frame, specified in us.
    * @param The maximum value of the backoff exponent, BE, in the CSMA-CA algorithm
    * @param The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel access failure
    * @param The maximum time to wait either for a frame intended as a response to a data request frame, specified in us.
    * @param The maximum number of retries allowed after a transmission failure.
    * @param The minimum value of the backoff exponent (BE) in the CSMA-CA algorithm
    * @return command setting result
    * @date 17 Dec. 2021
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*mac_pib_set)(uint32_t a_unit_backoff_period, uint32_t mac_ack_wait_duration, uint8_t mac_max_BE, uint8_t mac_max_CSMA_backoffs,
                                    uint32_t mac_max_frame_total_wait_time, uint8_t mac_max_frame_retries, uint8_t mac_min_BE);

    /**
    * @brief Set 15.4 PHY PIB
    * @param RX-to-TX or TX-to-RX turnaround time, specified in us.
    * @param 0: Energy above threshold, 1: Carrier sense only, 2: Carrier sense with energy above threshold, where the logical operator is AND.
    *        3: Carrier sense with energy above threshold, where the logical operator is OR.
    * @param The received power threshold of the, energy above threshold, algorithm.
    * @param The duration for CCA, specified in us.
    * @return command setting result
    * @date 17 Dec. 2021
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*phy_pib_set)(uint16_t a_turnaround_time, uint8_t phy_cca_mode, uint8_t phy_cca_threshold, uint16_t phy_cca_duration);

    /**
    * @brief Enable auto ACK
    *
    * @param Enable auto ACK flag
    * @return command setting result
    * @date 20 Jan. 2021
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*auto_ack_set)(uint8_t auto_ack);

    /**
    * @brief Set frame pending bit
    *
    * @param frame pending bit flag [0:false, 1:true]
    * @return command setting result
    * @date 20 Jan. 2021
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*frame_pending_set)(uint8_t frame_pending);

    /**
    * @brief Set RX on when IDLW
    *
    * @param the flag for transfering to RX state automatically when RFB is idle [0:false, 1:true];
    * @return command setting result
    * @date 20 Jan. 2021
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*auto_state_set)(bool rx_on_when_idle);

    /**
    * @brief Enable/ disable source address match feature
    *
    * @param Enable flag [0:disable, 1:enable]
    * @return None
    * @date 17 Aug. 2022
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*src_addr_match_ctrl)(uint8_t ctrl_type);

    /**
    * @brief Control the short source address table
    *
    * @param Control type [0: clear src matching table,
    *                      1: add/set address entry - ack pending bit,
    *                      2: clear address entry   - ack pending bit,
    *                      3: add/set address entry - link metrics LQI (ehn-ack),
    *                      4: clear address entry   - link metrics LQI (ehn-ack),
    *                      5: add/set address entry - link metrics link margin (ehn-ack),
    *                      6: clear address entry   - link metrics link margin (ehn-ack),
    *                      7: add/set address entry - link metrics RSSI (ehn-ack),
    *                      8: clear address entry   - link metrics RSSI (ehn-ack),
    *                      9: find address entry]
    * @param The pointer for short address
    * @return None
    * @date 17 Aug. 2022
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*short_addr_ctrl)(uint8_t ctrl_type, uint8_t *short_addr);

    /**
    * @brief Control the extended source address table
    *
    * @param Control type [0: clear src matching table,
    *                      1: add/set address entry - ack pending bit,
    *                      2: clear address entry   - ack pending bit,
    *                      3: add/set address entry - link metrics LQI (ehn-ack),
    *                      4: clear address entry   - link metrics LQI (ehn-ack),
    *                      5: add/set address entry - link metrics link margin (ehn-ack),
    *                      6: clear address entry   - link metrics link margin (ehn-ack),
    *                      7: add/set address entry - link metrics RSSI (ehn-ack),
    *                      8: clear address entry   - link metrics RSSI (ehn-ack),
    *                      9: find address entry]
    * @param The pointer for extended address
    * @return None
    * @date 17 Aug. 2022
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*extend_addr_ctrl)(uint8_t ctrl_type, uint8_t *extend_addr);

    /**
    * @brief Configure 128bits encryption key
    *
    * @param The pointer for key
    * @return None
    * @date 30 Aug. 2022
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*key_set)(uint8_t *key_addr);

    /**
    * @brief Enable/ disable the CSL receiver
    *
    * @param Enable flag [0:disable, 1:enable]
    * @param CSL period
    * @return None
    * @date 8 Sep. 2022
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*csl_receiver_ctrl)(uint8_t csl_receiver_ctrl, uint16_t csl_period);

    /**
    * @brief Get the current accuracy
    *
    * @param None
    * @return The current accuracy
    * @date 8 Sep. 2022
    * @see
    * @image
    */
    uint8_t (*csl_accuracy_get)(void);

    /**
    * @brief Get the current uncertainty
    *
    * @param None
    * @return The current uncertainty
    * @date 8 Sep. 2022
    * @see
    * @image
    */
    uint8_t (*csl_uncertainty_get)(void);

    /**
    * @brief Update CSL sample time
    *
    * @param None
    * @return The current uncertainty
    * @date 13 Sep. 2022
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*csl_sample_time_update)(uint32_t csl_sample_time);

    /**
    * @brief Read RFB RTC time
    *
    * @param None
    * @return The current RTC time
    * @date 20 Sep. 2022
    * @see
    * @image
    */
    uint32_t (*rtc_time_read)(void);

    /**
    * @brief Read ack pkt data
    *
    * @param rx_data_address
    * @param rx_time_address
    * @return The length of ack pkt
    * @date 18 Dec. 2022
    * @see
    * @image
    */
    uint8_t (*ack_packet_read)(uint8_t *rx_data_address, uint8_t *rx_time_address, bool is2bytephr);

    /**
    * @brief Read RFB RX RTC time
    *
    * @param rx_cnt: total_rx_done_cnt%5 (start from 0)
    * @return The RX RTC time stored in local data q
    * @date 18 Dec. 2022
    * @see
    * @image
    */
    uint32_t (*rx_rtc_time_get)(uint8_t rx_cnt);

    /**
    * @brief Read RFB current channel
    *
    * @param rx_cnt: total_rx_done_cnt%5 (start from 0)
    * @return The channel number (for 2.4G)
    * @date 6 Mar. 2023
    * @see
    * @image
    */
    uint8_t (*current_channel_get)(uint8_t rx_cnt);

    /**
    * @brief Read RFB current frame counter
    *
    * @param None
    * @return The frame counter value
    * @date 6 Mar. 2023
    * @see
    * @image
    */
    uint32_t (*frame_counter_get)(void);
} rfb_subg_ctrl_t;
#endif

#if (defined RFB_BLE_ENABLED && RFB_BLE_ENABLED == 1)
typedef struct rfb_ble_ctrl_s
{
    /**
    * @brief Initiate RFB, and register interrupt event.
    *
    * @param Rfb interrupt event struct [rx_done, tx_done]
    *
    * @return Command setting result
    * @date 14 Dec. 2020
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*init)(rfb_interrupt_event_t *_rfb_interrupt_event);

    /**
    * @brief Set RFB modem type.
    *
    * @param Modem [RFB_MODEM_BLE]
    * @param band_type [BAND_2P4G]
    *
    * @return Command setting result
    * @date 17 Dec. 2020
    * @see
    * @image
    */
    RFB_EVENT_STATUS (*modem_set)(rfb_modem_type_t modem, uint8_t band_type);


    /**
    * @brief Set RF frequency.
    *
    * @param RF frequency [2400~2480]
    *
    * @return Command setting result
    * @date 14 Dec. 2020
    * @see
    * @image
    */

    RFB_EVENT_STATUS (*frequency_set)(uint32_t rf_frequency);

    /**
     * @brief Send data to RFB buffer and RFB will transmit this data automatically
     *
     * @param Data address
     * @param Packet length
     *         FSK [1~2047] / OQPSK [1~127]
     * @param TX control [0](Not apply for 15p4g mode)
     * @param Data sequence number [0] (Not apply for 15.4g mode)
     * @return Write Tx queue result
     * @date 20 Jan. 2021
     * @see
     * @image
     */
    RFB_WRITE_TXQ_STATUS (*data_send)(uint8_t *tx_data_address, uint16_t packet_length, uint8_t tx_control, uint8_t dsn);
    RFB_EVENT_STATUS (*ble_modem_set)(uint8_t data_rate, uint8_t coded_scheme);
    RFB_EVENT_STATUS (*ble_mac_set)(uint32_t sfd_content, uint8_t whitening_en, uint8_t whitening_init_value, uint32_t crc_init_value);

    /**
     * @brief Set RFB to RX state
     *
     * @param None
     * @return command setting result
     * @date 14 Dec. 2020
     * @see
     * @image
     */
    RFB_EVENT_STATUS (*rx_start)(void);

} rfb_ble_ctrl_t;
#endif
/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
#if (defined RFB_SUBG_ENABLED && RFB_SUBG_ENABLED == 1)
#define FSK_MAX_RF_LEN 2063 //2047+16
#define OQPSK_MAX_RF_LEN 142 //127+15
#define MAX_RF_LEN FSK_MAX_RF_LEN
#elif (defined RFB_BLE_ENABLED && RFB_BLE_ENABLED == 1)
#define MAX_RF_LEN 268 //255+13
#elif (defined RFB_ZIGBEE_ENABLED && RFB_ZIGBEE_ENABLED == 1)
#define MAX_RF_LEN 140 //127+13
#endif

/**************************************************************************************************
 *    Global Prototypes
 *************************************************************************************************/
#if (defined RFB_ZIGBEE_ENABLED && RFB_ZIGBEE_ENABLED == 1)
rfb_zb_ctrl_t *rfb_zb_init(void);
#endif
#endif
#if (defined RFB_SUBG_ENABLED && RFB_SUBG_ENABLED == 1)
rfb_subg_ctrl_t *rfb_subg_init(void);
#endif
#if (defined RFB_BLE_ENABLED && RFB_BLE_ENABLED == 1)
rfb_ble_ctrl_t *rfb_ble_init(void);
#endif
