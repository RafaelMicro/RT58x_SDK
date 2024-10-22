/** @file user.c
 *
 * @brief BLE TRSP peripheral role demo.
 *
 */

/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "task_hci.h"
#include "lpm.h"
#include "ble_event.h"
#include "ble_printf.h"
#include "ble_profile.h"
#include "main.h"

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define APP_STACK_SIZE                  256
#define BLE_APP_CB_QUEUE_SIZE           16
#define APP_ISR_QUEUE_SIZE              2
#define APP_REQ_QUEUE_SIZE              6
#define APP_QUEUE_SIZE                  (BLE_APP_CB_QUEUE_SIZE + APP_ISR_QUEUE_SIZE + APP_REQ_QUEUE_SIZE)

#define APP_TRSP_P_HOST_ID              0

// MTU size
#define DEFAULT_MTU                     BLE_GATT_ATT_MTU_MAX
#define UART_BUFF_DEPTH                 2

// Advertising device name
#define DEVICE_NAME                     'T', 'H', 'R', 'E', 'A', 'D', 'S', 'U', 'B', 'G'

// Advertising parameters
#define APP_ADV_INTERVAL_MIN            160U      // 160*0.625ms=100ms
#define APP_ADV_INTERVAL_MAX            160U      // 160*0.625ms=100ms

// GAP device name
static const uint8_t         DEVICE_NAME_STR[] = {DEVICE_NAME};

// Device BLE Address
static const ble_gap_addr_t  DEVICE_ADDR = {.addr_type = RANDOM_STATIC_ADDR,
                                            .addr = {0x21, 0x22, 0x23, 0x22, 0x11, 0xC0 }
                                           };

/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/
static xQueueHandle g_app_msg_q;
static SemaphoreHandle_t semaphore_cb;
static SemaphoreHandle_t semaphore_isr;
static SemaphoreHandle_t semaphore_app;

static ble_cfg_t gt_app_cfg;

static uint8_t g_advertising_host_id = BLE_HOSTID_RESERVED;
static uint8_t g_rx_buffer[UART_BUFF_DEPTH][DEFAULT_MTU];
static uint8_t g_rx_buffer_length[UART_BUFF_DEPTH];
static uint8_t g_uart_index = 0;
static uint8_t g_uart_transmit_index = 0;
static uint8_t g_trsp_mtu = BLE_GATT_ATT_MTU_MIN;
static bool g_data_send_pending   = false;    // true: indicates that there is data pending, still send old data and skip new data from UART.

static TimerHandle_t BLE_Time_Tick;
static uint16_t networking_tmr = 0;

#define NETWORKING_TIMEOUT 1*60

#define NETWORKING_TMR_START() (networking_tmr = NETWORKING_TIMEOUT)

#define NETWORKING_TMR_STOP() (networking_tmr = 0)

/**************************************************************************************************
 *    FUNCTION DECLARATION
 *************************************************************************************************/
static void ble_app_main(app_req_param_t *p_param);
static ble_err_t ble_init(void);
static void svcs_trsps_data_init(ble_svcs_trsps_data_t *p_data);
static void svcs_gatts_data_init(ble_svcs_gatts_data_t *p_data);
static ble_err_t adv_init(void);
static ble_err_t adv_enable(uint8_t host_id);
static bool app_request_set(uint8_t host_id, app_request_t request, bool from_isr);
static void trsp_data_send_from_isr(uint8_t *p_data, uint8_t length);

/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
/* ------------------------------
 *  Handler
 * ------------------------------
 */
static void commission_data_ack(uint8_t host_id, uint16_t handle_num)
{
    uint8_t data_ack[8] = {0x7E, 0x00, 0x03, 0x61, 0x63, 0x6B, 0xB0, 0x7E};
    ble_info_link0_t *p_profile_info = (ble_info_link0_t *)ble_app_link_info[host_id].profile_info;
    ble_err_t status = ble_svcs_trsps_server_send(host_id,
                       BLEGATT_CCCD_INDICATION,
                       p_profile_info->svcs_info_trsps.server_info.handles.hdl_udatni01,
                       (uint8_t *)&data_ack,
                       sizeof(data_ack));
    // info("status %d %d %d \n", status, host_id, p_profile_info->svcs_info_trsps.server_info.handles.hdl_udatni01);
}

static void commission_data_process(uint8_t host_id, uint16_t handle_num, uint8_t *data, uint16_t lens)
{
    if (lens != sizeof(ble_commission_data_t))
    {
        info("commission_data_parse lens %u %u fail \n", lens, sizeof(ble_commission_data_t));
        return;
    }
    if (data[0] != 0x7E &&  data[lens - 1] != 0x7E)
    {
        info("commission_data_parse mask %02X %02X fail \n", data[0], data[lens - 1]);
        return;
    }
    ble_commission_data_t *a_ble_commission_data = (ble_commission_data_t *)data;
    uint8_t crc_8 = 0;
    for (uint16_t i = 0; i < (lens - 2); i++)
    {
        crc_8 += data[i];
    }
    if (a_ble_commission_data->crc != crc_8)
    {
        info("commission_data_parse crc %02X  %02X fail \n", a_ble_commission_data->crc, crc_8);
        return;
    }
    app_commission_data_write(&a_ble_commission_data->data);

    commission_data_ack(host_id, handle_num);

}

static void ble_svcs_trsps_evt_handler(ble_evt_att_param_t *p_param)
{
    ble_err_t status;
    if (p_param->gatt_role == BLE_GATT_ROLE_SERVER)
    {
        /* ----------------- Handle event from client ----------------- */
        switch (p_param->event)
        {
        case BLESERVICE_TRSPS_UDATRW01_WRITE_EVENT:
        case BLESERVICE_TRSPS_UDATRW01_WRITE_WITHOUT_RSP_EVENT:
        {
            uint8_t *p_data;

            p_data = pvPortMalloc(p_param->length + 1);
            if (p_data != NULL)
            {
                memcpy(p_data, p_param->data, p_param->length);
                *(p_data + p_param->length) = '\0';
                // info("received : ");
                // for (int i = 0; i < p_param->length ; i++ )
                // {
                //     info("%02x", p_data[i]);
                // }
                // info("\n");

                /*commission data set*/
                commission_data_process(p_param->host_id, p_param->handle_num, p_data, p_param->length);
                /*change thread*/
                vPortFree(p_data);
            }
        }
        break;

        case BLESERVICE_TRSPS_UDATNI01_INDICATE_CONFIRM_EVENT:
            info_color(LOG_DEFAULT, "------------------------------------------\n");
            info_color(LOG_DEFAULT, "  setting success change to thread...\n");
            info_color(LOG_DEFAULT, "------------------------------------------\n");
            NVIC_SystemReset();
            break;

        case BLESERVICE_TRSPS_UDATRW01_READ_EVENT:
        {
            const uint8_t readData[] = "UDATRW01 data";
            ble_gatt_data_param_t gatt_data_param;

            gatt_data_param.host_id = p_param->host_id;
            gatt_data_param.handle_num = p_param->handle_num;
            gatt_data_param.length = SIZE_STRING(readData);
            gatt_data_param.p_data = (uint8_t *)readData;

            status = ble_svcs_data_send(TYPE_BLE_GATT_READ_RSP, &gatt_data_param);
            if (status != BLE_ERR_OK)
            {
                info_color(LOG_RED, "ble_gatt_read_rsp status: %d\n", status);
            }
        }
        break;

        default:
            break;
        }
    }
}


// TRSPS Peripheral
static void app_peripheral_handler(app_req_param_t *p_param)
{
    ble_err_t status;
    uint8_t host_id;
    ble_info_link0_t *p_profile_info;

    host_id = p_param->host_id;
    p_profile_info = (ble_info_link0_t *)ble_app_link_info[host_id].profile_info;

    switch (p_param->app_req)
    {
    case APP_REQUEST_ADV_START:
        do
        {
            // service data init
            svcs_gatts_data_init(&p_profile_info->svcs_info_gatts.server_info.data);
            svcs_trsps_data_init(&p_profile_info->svcs_info_trsps.server_info.data);

            // set preferred MTU size and data length
            status = ble_cmd_default_mtu_size_set(host_id, DEFAULT_MTU);
            if (status != BLE_ERR_OK)
            {
                info_color(LOG_RED, "ble_cmd_default_mtu_size_set() status = %d\n", status);
                break;
            }

            // enable advertising
            status = adv_init();
            if (status != BLE_ERR_OK)
            {
                info_color(LOG_RED, "adv_init() status = %d\n", status);
                break;
            }

            status = adv_enable(host_id);
            if (status != BLE_ERR_OK)
            {
                info_color(LOG_RED, "adv_enable() status = %d\n", status);
                break;
            }
        } while (0);

        g_trsp_mtu = BLE_GATT_ATT_MTU_MIN;
        break;

    case APP_REQUEST_TRSPS_DATA_SEND:
        // send data to client
        if ((p_profile_info->svcs_info_trsps.server_info.data.udatni01_cccd & BLEGATT_CCCD_NOTIFICATION) != 0)
        {
            status = ble_svcs_trsps_server_send(  host_id,
                                                  BLEGATT_CCCD_NOTIFICATION,
                                                  p_profile_info->svcs_info_trsps.server_info.handles.hdl_udatni01,
                                                  &g_rx_buffer[g_uart_transmit_index][0],
                                                  g_rx_buffer_length[g_uart_transmit_index]);
        }
        else if ((p_profile_info->svcs_info_trsps.server_info.data.udatni01_cccd & BLEGATT_CCCD_INDICATION) != 0)
        {
            status = ble_svcs_trsps_server_send(  host_id,
                                                  BLEGATT_CCCD_INDICATION,
                                                  p_profile_info->svcs_info_trsps.server_info.handles.hdl_udatni01,
                                                  &g_rx_buffer[g_uart_transmit_index][0],
                                                  g_rx_buffer_length[g_uart_transmit_index]);
        }

        if (status == BLE_ERR_OK)
        {
            g_data_send_pending = false;

            g_uart_transmit_index++;
            if (g_uart_transmit_index == UART_BUFF_DEPTH)
            {
                g_uart_transmit_index = 0;
            }
        }
        else
        {
            g_data_send_pending = true;

            // re-send
            if (app_request_set(host_id, APP_REQUEST_TRSPS_DATA_SEND, false) == false)
            {
                // No Application queue buffer. Error.
            }
        }
        break;

    default:
        break;
    }
}

static void ble_evt_handler(ble_evt_param_t *p_param)
{
    switch (p_param->event)
    {
    case BLE_ADV_EVT_SET_ENABLE:
    {
        ble_evt_adv_set_adv_enable_t *p_adv_enable = (ble_evt_adv_set_adv_enable_t *)&p_param->event_param.ble_evt_adv.param.evt_set_adv_enable;

        if (p_adv_enable->status == BLE_HCI_ERR_CODE_SUCCESS)
        {
            if (p_adv_enable->adv_enabled == true)
            {
                if (g_advertising_host_id != BLE_HOSTID_RESERVED)
                {
                    ble_app_link_info[g_advertising_host_id].state = STATE_ADVERTISING;
                }
                info_color(LOG_GREEN, "Advertising...\n");
            }
            else
            {
                if (g_advertising_host_id != BLE_HOSTID_RESERVED)
                {
                    ble_app_link_info[g_advertising_host_id].state = STATE_STANDBY;
                }
                info_color(LOG_GREEN, "Idle.\n");
            }
        }
        else
        {
            info_color(LOG_RED, "Advertising enable failed.\n");
        }
    }
    break;

    case BLE_GAP_EVT_CONN_COMPLETE:
    {
        ble_evt_gap_conn_complete_t *p_conn_param = (ble_evt_gap_conn_complete_t *)&p_param->event_param.ble_evt_gap.param.evt_conn_complete;

        if (p_conn_param->status != BLE_HCI_ERR_CODE_SUCCESS)
        {
            info_color(LOG_RED, "Connect failed, error code = 0x%02x\n", p_conn_param->status);
        }
        else
        {
            ble_app_link_info[p_conn_param->host_id].state = STATE_CONNECTED;
            info_color(LOG_GREEN, "Connected, ID=%d, Connected to %02x:%02x:%02x:%02x:%02x:%02x\n",
                       p_conn_param->host_id,
                       p_conn_param->peer_addr.addr[5],
                       p_conn_param->peer_addr.addr[4],
                       p_conn_param->peer_addr.addr[3],
                       p_conn_param->peer_addr.addr[2],
                       p_conn_param->peer_addr.addr[1],
                       p_conn_param->peer_addr.addr[0]);
        }
    }
    break;

    case BLE_GAP_EVT_CONN_PARAM_UPDATE:
    {
        ble_evt_gap_conn_param_update_t *p_conn_param = (ble_evt_gap_conn_param_update_t *)&p_param->event_param.ble_evt_gap.param.evt_conn_param_update;

        if (p_conn_param->status != BLE_HCI_ERR_CODE_SUCCESS)
        {
            info_color(LOG_RED, "Connection update failed, error code = 0x%02x\n", p_conn_param->status);
        }
        else
        {
            info_color(LOG_DEFAULT, "Connection updated\n");
            info_color(LOG_DEFAULT, "ID: %d, ", p_conn_param->host_id);
            info_color(LOG_DEFAULT, "Interval: %d, ", p_conn_param->conn_interval);
            info_color(LOG_DEFAULT, "Latency: %d, ", p_conn_param->periph_latency);
            info_color(LOG_DEFAULT, "Supervision Timeout: %d\n", p_conn_param->supv_timeout);
        }
    }
    break;

    case BLE_GAP_EVT_PHY_READ:
    case BLE_GAP_EVT_PHY_UPDATE:
    {
        ble_evt_gap_phy_t *p_phy_param = (ble_evt_gap_phy_t *)&p_param->event_param.ble_evt_gap.param.evt_phy;
        if (p_phy_param->status != BLE_HCI_ERR_CODE_SUCCESS)
        {
            info_color(LOG_RED, "PHY update/read failed, error code = 0x%02x\n", p_phy_param->status);
        }
        else
        {
            info_color(LOG_DEFAULT, "PHY updated/read, ID: %d, TX PHY: %d, RX PHY: %d\n", p_phy_param->host_id, p_phy_param->tx_phy, p_phy_param->rx_phy);
        }
    }
    break;

    case BLE_ATT_GATT_EVT_MTU_EXCHANGE:
    {
        ble_evt_mtu_t *p_mtu_param = (ble_evt_mtu_t *)&p_param->event_param.ble_evt_att_gatt.param.ble_evt_mtu;

        g_trsp_mtu = p_mtu_param->mtu; // update to real mtu size

        info_color(LOG_DEFAULT, "MTU Exchanged, ID:%d, size: %d\n", p_mtu_param->host_id, p_mtu_param->mtu);
    }
    break;

    case BLE_ATT_GATT_EVT_WRITE_SUGGESTED_DEFAULT_DATA_LENGTH:
    {
        ble_evt_suggest_data_length_set_t *p_data_len_param = (ble_evt_suggest_data_length_set_t *)&p_param->event_param.ble_evt_att_gatt.param.ble_evt_suggest_data_length_set;

        if (p_data_len_param->status == BLE_HCI_ERR_CODE_SUCCESS)
        {
            info_color(LOG_DEFAULT, "Write default data length, status: %d\n", p_data_len_param->status);
        }
        else
        {
            info_color(LOG_RED, "Write default data length, status: %d\n", p_data_len_param->status);
        }
    }
    break;

    case BLE_ATT_GATT_EVT_DATA_LENGTH_CHANGE:
    {
        ble_evt_data_length_change_t *p_data_len_param = (ble_evt_data_length_change_t *)&p_param->event_param.ble_evt_att_gatt.param.ble_evt_data_length_change;
        info_color(LOG_DEFAULT, "Data length changed, ID: %d\n", p_data_len_param->host_id);
        info_color(LOG_DEFAULT, "MaxTxOctets: %d  MaxTxTime:%d\n", p_data_len_param->max_tx_octets, p_data_len_param->max_tx_time);
        info_color(LOG_DEFAULT, "MaxRxOctets: %d  MaxRxTime:%d\n", p_data_len_param->max_rx_octets, p_data_len_param->max_rx_time);
    }
    break;

    case BLE_GAP_EVT_DISCONN_COMPLETE:
    {
        ble_evt_gap_disconn_complete_t *p_disconn_param = (ble_evt_gap_disconn_complete_t *)&p_param->event_param.ble_evt_gap.param.evt_disconn_complete;
        if (p_disconn_param->status != BLE_HCI_ERR_CODE_SUCCESS)
        {
            info_color(LOG_RED, "Disconnect failed, error code = 0x%02x\n", p_disconn_param->status);
        }
        else
        {
            ble_app_link_info[p_disconn_param->host_id].state = STATE_STANDBY;

            // re-start adv
            if (app_request_set(p_disconn_param->host_id, APP_REQUEST_ADV_START, false) == false)
            {
                // No Application queue buffer. Error.
            }

            info_color(LOG_GREEN, "Disconnect, ID:%d, Reason:0x%02x\n", p_disconn_param->host_id, p_disconn_param->reason);
        }
    }
    break;

    default:
        break;
    }
}

/* ------------------------------
 *  Methods
 * ------------------------------
 */
static void trsp_data_send_from_isr(uint8_t *p_data, uint8_t length)
{
    // if there is data pending, still send old data and skip new data
    if (g_data_send_pending == false)
    {
        g_rx_buffer_length[g_uart_index] = length;
        memcpy(&g_rx_buffer[g_uart_index][0], p_data, length);

        g_uart_index++;
        if (g_uart_index == UART_BUFF_DEPTH)
        {
            g_uart_index = 0;
        }
        // send queue to task_bla_app
        if (app_request_set(APP_TRSP_P_HOST_ID, APP_REQUEST_TRSPS_DATA_SEND, true) == false)
        {
            // No Application queue buffer. Error.
        }
    }
}

static ble_err_t adv_init(void)
{
    ble_err_t status;
    ble_adv_param_t adv_param;
    ble_adv_data_param_t adv_data_param;
    ble_adv_data_param_t adv_scan_data_param;
    ble_gap_addr_t addr_param;
    const uint8_t   SCANRSP_ADLENGTH  = (1) + sizeof(DEVICE_NAME_STR); //  1 byte data type

    // adv data
    uint8_t adv_data[] =
    {
        0x02, GAP_AD_TYPE_FLAGS, BLE_GAP_FLAGS_LIMITED_DISCOVERABLE_MODE,
    };

    // scan response data
    uint8_t adv_scan_rsp_data[] =
    {
        SCANRSP_ADLENGTH,                   // AD length
        GAP_AD_TYPE_LOCAL_NAME_COMPLETE,    // AD data type
        DEVICE_NAME,                        // the name is shown on scan list
    };

    ble_cmd_device_addr_get(&addr_param);
    do
    {
        adv_param.adv_type = ADV_TYPE_ADV_IND;
        adv_param.own_addr_type = addr_param.addr_type;
        adv_param.adv_interval_min = APP_ADV_INTERVAL_MIN;
        adv_param.adv_interval_max = APP_ADV_INTERVAL_MAX;
        adv_param.adv_channel_map = ADV_CHANNEL_ALL;
        adv_param.adv_filter_policy = ADV_FILTER_POLICY_ACCEPT_ALL;

        // set adv parameter
        status = ble_cmd_adv_param_set(&adv_param);
        if (status != BLE_ERR_OK)
        {
            info_color(LOG_RED, "ble_cmd_adv_param_set() status = %d\n", status);
            break;
        }

        // set adv data
        adv_data_param.length = sizeof(adv_data);
        memcpy(&adv_data_param.data, &adv_data, sizeof(adv_data));
        status = ble_cmd_adv_data_set(&adv_data_param);
        if (status != BLE_ERR_OK)
        {
            info_color(LOG_RED, "ble_cmd_adv_data_set() status = %d\n", status);
            break;
        }

        // set scan rsp data
        adv_scan_data_param.length = sizeof(adv_scan_rsp_data);
        memcpy(&adv_scan_data_param.data, &adv_scan_rsp_data, sizeof(adv_scan_rsp_data));
        status = ble_cmd_adv_scan_rsp_set(&adv_scan_data_param);
        if (status != BLE_ERR_OK)
        {
            info_color(LOG_RED, "ble_cmd_adv_scan_rsp_set() status = %d\n", status);
            break;
        }
    } while (0);

    return status;
}

static ble_err_t adv_enable(uint8_t host_id)
{
    ble_err_t status;

    status = ble_cmd_adv_enable(host_id);
    if (status != BLE_ERR_OK)
    {
        info_color(LOG_RED, "adv_enable() status = %d\n", status);
    }

    return status;
}

static bool app_request_set(uint8_t host_id, app_request_t request, bool from_isr)
{
    app_queue_t p_app_q;

    p_app_q.event = 0; // from BLE
    p_app_q.param_type = QUEUE_TYPE_APP_REQ;
    p_app_q.param.app_req.host_id = host_id;
    p_app_q.param.app_req.app_req = request;

    if (from_isr == false)
    {
        if (xSemaphoreTake(semaphore_app, 0) == pdTRUE)
        {
            p_app_q.from_isr = false;
            if (xQueueSendToBack(g_app_msg_q, &p_app_q, 0) != pdTRUE)
            {
                // send error
                xSemaphoreGive(semaphore_app);
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        BaseType_t context_switch = pdFALSE;

        if (xSemaphoreTakeFromISR(semaphore_isr, &context_switch) == pdTRUE)
        {
            p_app_q.from_isr = true;
            context_switch = pdFALSE;
            if (xQueueSendToBackFromISR(g_app_msg_q, &p_app_q, &context_switch) != pdTRUE)
            {
                context_switch = pdFALSE;
                xSemaphoreGiveFromISR(semaphore_isr, &context_switch);
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    return true;
}

/* ------------------------------
 *  Application Task
 * ------------------------------
 */
static void app_evt_indication_cb(uint32_t data_len)
{
    int i32_err;
    uint8_t *p_buf;
    app_queue_t p_app_q;

    if (xSemaphoreTake(semaphore_cb, 0) == pdPASS)
    {
        do
        {
            p_buf = pvPortMalloc(data_len);
            if (!p_buf)
            {
                xSemaphoreGive(semaphore_cb);
                break;
            }

            p_app_q.event = 0;
            i32_err = ble_event_msg_recvfrom(p_buf, &data_len);
            p_app_q.param_type = QUEUE_TYPE_OTHERS;
            p_app_q.param.pt_tlv = (ble_tlv_t *)p_buf;

            if (i32_err == 0)
            {
                while (xQueueSendToBack(g_app_msg_q, &p_app_q, 1) != pdTRUE);
            }
            else
            {
                info_color(LOG_RED, "[%s] err = %d !\n", __func__, (ble_err_t)i32_err);
                vPortFree(p_buf);
                xSemaphoreGive(semaphore_cb);
            }
        } while (0);
    }
}

static void sys_init()
{
    /* delay init */
    Delay_Init();

    /* low power mode init */
    Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP0);
    Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_UART_RX);
}

static void ble_network_timeout_handler()
{
    info_color(LOG_DEFAULT, "------------------------------------------\n");
    info_color(LOG_DEFAULT, "  timeout change to thread...\n");
    info_color(LOG_DEFAULT, "------------------------------------------\n");
#if 0 //for test
    app_commission_data_t *a_commission_data = pvPortMalloc(sizeof(app_commission_data_t));;

    uint8_t br_mac[8] = {0x50, 0x38, 0x32, 0x33, 0x32, 0x45, 0x84, 0x38};
    uint8_t br_key[16] = {0xfe, 0x83, 0x44, 0x8a, 0x67, 0x29, 0xfe, 0xab,
                          0xab, 0xfe, 0x29, 0x67, 0x8a, 0x44, 0x83, 0xfe
                         };
    memcpy(a_commission_data->br_mac, br_mac, 8);
    memcpy(a_commission_data->br_networkkey, br_key, 16);
    a_commission_data->br_channel = 3;
    a_commission_data->br_panid = 0xcdab;

    app_commission_data_write(a_commission_data);
    vPortFree(a_commission_data);
#endif
    NVIC_SystemReset();
}

static void ble_time_tick_handler(TimerHandle_t xTimer)
{
    if (0 < networking_tmr )
    {
        networking_tmr % 2 == 0 ? gpio_pin_write(20, 0) : gpio_pin_write(20, 1);
        if (0 == --networking_tmr)
        {
            ble_network_timeout_handler();
        }
    }
    xTimerStart(BLE_Time_Tick, 0);
}

static void app_main_task(void)
{
    ble_err_t status;
    app_queue_t p_app_q;

    status = BLE_ERR_OK;

    sys_init();

    // BLE default setting and profile init
    status = ble_init();
    if (status != BLE_ERR_OK)
    {
        info_color(LOG_RED, "[DEBUG_ERR] ble_init() fail: %d\n", status);
        while (1);
    }

    // Initial parameters
    g_uart_index = 0;
    g_uart_transmit_index = 0;

    // start adv
    if (app_request_set(APP_TRSP_P_HOST_ID, APP_REQUEST_ADV_START, false) == false)
    {
        // No Application queue buffer. Error.
    }

    app_commission_start();
    BLE_Time_Tick = xTimerCreate("BLE_Time_Tick_T", pdMS_TO_TICKS(1 * 1000), false, NULL, ble_time_tick_handler);
    xTimerStart(BLE_Time_Tick, 0);
    NETWORKING_TMR_START();

    for (;;)
    {
        if (xQueueReceive(g_app_msg_q, &p_app_q, portMAX_DELAY) == pdTRUE)
        {
            switch (p_app_q.param_type)
            {
            case QUEUE_TYPE_APP_REQ:
            {
                if (p_app_q.from_isr == true)
                {
                    xSemaphoreGive(semaphore_isr);
                }
                else
                {
                    xSemaphoreGive(semaphore_app);
                }
                ble_app_main(&p_app_q.param.app_req);
            }
            break;

            case QUEUE_TYPE_OTHERS:
            {
                if (p_app_q.param.pt_tlv != NULL)
                {
                    switch (p_app_q.param.pt_tlv->type)
                    {
                    case BLE_APP_GENERAL_EVENT:
                        ble_evt_handler((ble_evt_param_t *)p_app_q.param.pt_tlv->value);
                        break;

                    case BLE_APP_SERVICE_EVENT:
                    {
                        ble_evt_att_param_t *p_svcs_param = (ble_evt_att_param_t *)p_app_q.param.pt_tlv->value;

                        switch (p_svcs_param->gatt_role)
                        {
                        case BLE_GATT_ROLE_CLIENT:
                            att_db_link[p_svcs_param->host_id].p_client_db[p_svcs_param->cb_index]->att_handler(p_svcs_param);
                            break;

                        case BLE_GATT_ROLE_SERVER:
                            att_db_link[p_svcs_param->host_id].p_server_db[p_svcs_param->cb_index]->att_handler(p_svcs_param);
                            break;

                        default:
                            break;
                        }
                    }
                    break;

                    default:
                        break;
                    }

                    // free
                    vPortFree(p_app_q.param.pt_tlv);
                    xSemaphoreGive(semaphore_cb);
                }
            }
            break;

            default:
                break;
            }
        }
    }
}

static void ble_app_main(app_req_param_t *p_param)
{
    // Link - Peripheral
    app_peripheral_handler(p_param);
}

/* ------------------------------
 *  Application Initializations
 * ------------------------------
 */
static void svcs_gatts_data_init(ble_svcs_gatts_data_t *p_data)
{
    p_data->service_changed_cccd = 0;
}

static void svcs_trsps_data_init(ble_svcs_trsps_data_t *p_data)
{
    p_data->udatni01_cccd = 0;
}

static ble_err_t server_profile_init(uint8_t host_id)
{
    ble_err_t status = BLE_ERR_OK;
    ble_info_link0_t *p_profile_info = (ble_info_link0_t *)ble_app_link_info[host_id].profile_info;

    // set link's state
    ble_app_link_info[host_id].state = STATE_STANDBY;

    do
    {
        // GAP Related
        // -------------------------------------
        status = ble_svcs_gaps_init(host_id, BLE_GATT_ROLE_SERVER, &(p_profile_info->svcs_info_gaps), NULL);
        if (status != BLE_ERR_OK)
        {
            break;
        }

        // set GAP device name
        status = ble_svcs_gaps_device_name_set((uint8_t *)DEVICE_NAME_STR, sizeof(DEVICE_NAME_STR));
        if (status != BLE_ERR_OK)
        {
            break;
        }

        // GATT Related
        // -------------------------------------
        //        status = ble_svcs_gatts_init(host_id, BLE_GATT_ROLE_SERVER, &(p_profile_info->svcs_info_gatts), NULL);
        //        if (status != BLE_ERR_OK)
        //        {
        //            break;
        //        }

        // DIS Related
        // -------------------------------------
        //        status = ble_svcs_dis_init(host_id, BLE_GATT_ROLE_SERVER, &(p_profile_info->svcs_info_dis), NULL);
        //        if (status != BLE_ERR_OK)
        //        {
        //            break;
        //        }

        // TRSPS Related
        // -------------------------------------
        status = ble_svcs_trsps_init(host_id, BLE_GATT_ROLE_SERVER, &(p_profile_info->svcs_info_trsps), ble_svcs_trsps_evt_handler);
        if (status != BLE_ERR_OK)
        {
            break;
        }
    } while (0);

    return status;
}

static ble_err_t ble_init(void)
{
    ble_err_t status;
    ble_unique_code_format_t unique_code_param;
    ble_gap_addr_t device_addr_param;

    status = BLE_ERR_OK;
    do
    {
        status = ble_cmd_phy_controller_init();
        if (status != BLE_ERR_OK)
        {
            break;
        }

        status = ble_cmd_read_unique_code(&unique_code_param);
        if (status == BLE_ERR_OK)
        {
            device_addr_param.addr_type = unique_code_param.addr_type;
            memcpy(&device_addr_param.addr, &unique_code_param.ble_addr, 6);
            status = ble_cmd_device_addr_set((ble_gap_addr_t *)&device_addr_param);
            if (status != BLE_ERR_OK)
            {
                break;
            }
            status = ble_cmd_write_identity_resolving_key((ble_sm_irk_param_t *)&unique_code_param.ble_irk[0]);
            if (status != BLE_ERR_OK)
            {
                break;
            }
        }
        else
        {
            status = ble_cmd_device_addr_set((ble_gap_addr_t *)&DEVICE_ADDR);
            if (status != BLE_ERR_OK)
            {
                break;
            }
        }

        status = ble_cmd_resolvable_address_init();
        if (status != BLE_ERR_OK)
        {
            break;
        }

        status = ble_cmd_suggest_data_len_set(BLE_GATT_DATA_LENGTH_MAX);
        if (status != BLE_ERR_OK)
        {
            break;
        }

        // BLE profile init --> only 1 link (host id = 0)
        status = server_profile_init(APP_TRSP_P_HOST_ID);
        if (status != BLE_ERR_OK)
        {
            break;
        }

    } while (0);

    return status;
}


/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/
bool uart_data_handler(char ch)
{
    bool status = false;
    static uint8_t rx_buffer[DEFAULT_MTU];
    static uint8_t index = 0;

    rx_buffer[index++] = ch;

    if (index >= (g_trsp_mtu - 3)) // 3 bytes header
    {
        // send data via TRSP service
        trsp_data_send_from_isr(rx_buffer, index);

        // reset index
        index = 0;
    }
    else if ((ch == '\n') || (ch == '\r'))
    {
        // send data via TRSP service
        trsp_data_send_from_isr(rx_buffer, (index - 1)); // (-1) removed '\n' or '\r'

        // reset index
        index = 0;

        // set status to true indicates the system can enable sleep mode
        status = true;
    }
    return status;
}

void ble_app_init(void)
{
    // banner
    info_color(LOG_DEFAULT, "------------------------------------------\n");
    info_color(LOG_DEFAULT, "  BLE start...\n");
    info_color(LOG_DEFAULT, "------------------------------------------\n");

    // BLE Stack init
    gt_app_cfg.pf_evt_indication = app_evt_indication_cb;
    task_hci_init();
    ble_host_stack_init(&gt_app_cfg);
    info_color(LOG_DEFAULT, "BLE stack initial...\n");

    // application task
    g_app_msg_q = xQueueCreate(APP_QUEUE_SIZE, sizeof(app_queue_t));
    xTaskCreate((TaskFunction_t)app_main_task, "app", APP_STACK_SIZE, NULL, TASK_PRIORITY_APP, NULL);

    semaphore_cb = xSemaphoreCreateCounting(BLE_APP_CB_QUEUE_SIZE, BLE_APP_CB_QUEUE_SIZE);
    semaphore_isr = xSemaphoreCreateCounting(APP_ISR_QUEUE_SIZE, APP_ISR_QUEUE_SIZE);
    semaphore_app = xSemaphoreCreateCounting(APP_REQ_QUEUE_SIZE, APP_REQ_QUEUE_SIZE);

}

