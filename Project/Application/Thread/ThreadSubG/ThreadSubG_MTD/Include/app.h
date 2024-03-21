#ifndef APP_H
#define APP_H

#if PLAFFORM_CONFIG_ENABLE_SUBG == TRUE
#define DEF_CHANNEL 1
#else
#define DEF_CHANNEL 11
#endif

#define UDP_PORT 5678

#define SENSOR_COMMAND_LENS_MAX 256

typedef struct
{
    uint8_t CommandLengths;
    uint8_t CommandClass;
    uint8_t Command;
    uint8_t CommandData[SENSOR_COMMAND_LENS_MAX];
    uint8_t CommandCRC;
} __attribute__((packed)) app_sensor_playload_t;

typedef struct
{
    uint32_t Preamble;
    uint8_t SyncWord;
    uint8_t NetWorkId[6];
    uint8_t NodeId[6];
    uint8_t FrameSN;
    uint8_t FrameControl;
    uint8_t FrameLengths;
    app_sensor_playload_t FramePayLoad;
    uint16_t FrameCRC;
} __attribute__((packed)) app_sensor_data_t;

void _app_init(void);
void _app_process_action();
void _app_exit(void);

void _Udp_Data_Send(uint16_t PeerPort, otIp6Address PeerAddr, uint8_t *data, uint16_t buffer_lens);

#endif
