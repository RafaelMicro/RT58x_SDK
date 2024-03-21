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

void _app_init(void);
void _app_process_action();
void _app_exit(void);

void _Udp_Data_Send(uint16_t PeerPort, otIp6Address PeerAddr, uint8_t *data, uint16_t buffer_lens);

#endif
