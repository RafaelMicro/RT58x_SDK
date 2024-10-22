/**
 * @file app.c
 * @author
 * @brief
 * @version 0.1
 * @date 20230824
 *
 * @copyright Copyright (c) 2022
 *
 */
//=============================================================================
//                Include (Better to prevent)
//=============================================================================
#include "main.h"
#include "thread_queue.h"

//=============================================================================
//                Public Definitions of const value
//=============================================================================
#define APP_RETENTION_INDEX 1
#define APP_COMMISSION_DATA_FLASH_START 0xF3000
//=============================================================================
//                Public ENUM
//=============================================================================

//=============================================================================
//                Public Struct
//=============================================================================

//=============================================================================
//                Public Function Declaration
//=============================================================================
void app_commission_erase()
{
    enter_critical_section();
    while (flash_check_busy());
    flash_erase(FLASH_ERASE_SECTOR, APP_COMMISSION_DATA_FLASH_START);
    while (flash_check_busy());
    flush_cache();
    leave_critical_section();
}

void app_commission_start()
{
    enter_critical_section();
    while (flash_check_busy());
    flash_write_byte(APP_COMMISSION_DATA_FLASH_START, 0x01);
    while (flash_check_busy());
    leave_critical_section();
}

void app_commission_data_write(app_commission_data_t *a_commission_data)
{
    static uint8_t program_data[0x100];
    memset(&program_data, 0xFF, 0x100);
    mem_memcpy(&program_data[1], a_commission_data, sizeof(app_commission_data_t));

    // util_log_mem(UTIL_LOG_INFO, "comm ", &program_data, 0x100, 0);
    enter_critical_section();
    while (flash_check_busy());
    flash_write_page((uint32_t)&program_data, APP_COMMISSION_DATA_FLASH_START);
    while (flash_check_busy());
    leave_critical_section();
}

bool app_commission_data_check()
{
    static uint8_t read_buf[0x100];
    flash_read_page((uint32_t)(read_buf), APP_COMMISSION_DATA_FLASH_START);
    app_commission_data_t a_commission_data = {0};
    app_commission_t *m_commission = (app_commission_t *)read_buf;

    memset(&a_commission_data, 0xff, sizeof(app_commission_data_t));
    // util_log_mem(UTIL_LOG_INFO, "a ", &a_commission_data, sizeof(app_commission_data_t), 0);
    // util_log_mem(UTIL_LOG_INFO, "m ", &m_commission->data, sizeof(app_commission_data_t), 0);
    if (memcmp(&a_commission_data, &m_commission->data, sizeof(app_commission_data_t)) == 0)
    {
        return false;
    }

    return true;
}

void app_commission_get(app_commission_t *a_commission)
{
    static uint8_t read_buf[0x100];
    flash_read_page((uint32_t)(read_buf), APP_COMMISSION_DATA_FLASH_START);
    app_commission_t *m_commission = (app_commission_t *)read_buf;
    mem_memcpy(a_commission, m_commission, sizeof(app_commission_t));
    // util_log_mem(UTIL_LOG_INFO, "1.get ", a_commission, sizeof(app_commission_t), 0);
    // util_log_mem(UTIL_LOG_INFO, "2.get ", m_commission, sizeof(app_commission_t), 0);
}