
#ifndef __THREAD_APP_H__
#define __THREAD_APP_H__

#ifdef __cplusplus
extern "C" {
#endif
#if PLAFFORM_CONFIG_ENABLE_SUBG == TRUE
#define DEF_CHANNEL 1
#else
#define DEF_CHANNEL 11
#endif

#define UDP_PORT 5678

#define SENSOR_COMMAND_LENS_MAX 256

typedef struct
{
    uint16_t Preamble;       //0xFAFB
    uint8_t FrameLengths;
    uint8_t FrameSN;
    uint8_t FrameClass;      ///< 0x00: read from leader, 0x01: trigger from child, 0x10: write from leader to child,(MSB bit(1) is ACK bit)
    uint8_t NetWorkId[8];    ///< ChildId(extaddr)
    uint8_t PID;             ///< 0: one door sensor, 1: one smart plug, 2: one sps30, 3: one ens160 ....
    uint8_t CID;             ///< Command ID(0: off, 1: on,  2:ack ota version , 3: wait ota)
    uint8_t CommandLengths;
    uint8_t CommandData[SENSOR_COMMAND_LENS_MAX];
    uint16_t FrameCRC;
} __attribute__((packed)) app_sensor_data_t;


typedef enum
{
    APP_SENSOR_CONTROL_EVENT = 0x1,
    APP_SENSOR_GET_OTA_VERSION_EVENT,
    APP_SENSOR_WAIT_OTA_EVENT,
    APP_SENSOR_DATA_RECEIVED
} app_sensor_event_t;

typedef struct
{
    uint8_t event;
    uint8_t data[300];
    uint16_t data_lens;
} __attribute__((packed)) app_sensor_event_queue_t;

/* record the interrupt event triggered in deep sleep*/
extern uint8_t g_sensor_event;
/* record the button 1 had hold event*/
extern bool g_button1_hold;

void thread_app_task_start();
void thread_app_sensor_data_generate(uint8_t event);

#ifdef __cplusplus
};
#endif
#endif
