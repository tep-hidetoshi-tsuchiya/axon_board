#ifndef __T2A_PACKET_H__
#define __T2A_PACKET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

#define PACKET_DATA_LEN     0x20
#define PACKET_DIC_LEN      0x02
#define PACKET_AUTHCODE_LEN 0x02
#define PACKET_RND_LEN      0x02
#define PACKET_CRC16_LEN    0x02
#define PACKET_SIZE         (1 + 1 + PACKET_DATA_LEN + PACKET_CRC16_LEN)

typedef struct __ACK_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  rfu[25];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED ACK_PACKET;

typedef struct __NACK_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  err_code;
    uint16_t crc16;
} __PACKED NACK_PACKET;

typedef struct __PORT_STATUS {
    uint8_t count;
    struct {
        uint8_t valid : 1;         // 0: not valid, 1: valid
        uint8_t soldout : 1;       // 0: available, 1: sold out
        uint8_t solenoid : 1;      // 0: off, 1: on
        uint8_t optic_sensor : 1;  // 0: not detected, 1: detected
        uint8_t cash_return : 1;   // 0: not returned, 1: returned
        uint8_t cash_block : 1;    // 0: not blocked, 1: blocked
        uint8_t reset_flag : 1;    // 0: no reset, 1: reset
        uint8_t reserved : 1;
    } status;
} __PACKED PORT_STATUS;

typedef struct __CASH_SETTING {
    uint8_t port_1 : 4;
    uint8_t port_2 : 4;
    uint8_t port_3 : 4;
    uint8_t port_4 : 4;
    uint8_t port_5 : 4;
    uint8_t port_6 : 4;
    uint8_t port_7 : 4;
    uint8_t port_8 : 4;
    uint8_t port_9 : 4;
    uint8_t rfu : 4;
} __PACKED CASH_SETTING;

typedef struct __STATUS_PACKET {
    uint8_t      header;
    uint8_t      len;
    uint8_t      id;
    uint8_t      mode;
    uint8_t      fw_ver;
    PORT_STATUS  port_status[9];
    CASH_SETTING cash_setting;
    uint16_t     dic;
    uint16_t     auth_code;
    uint16_t     rnd;
    uint16_t     crc16;
} __PACKED STATUS_PACKET;

typedef struct __NOP_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  rfu[25];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED NOP_PACKET;

typedef struct __STOP_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  rfu[25];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED STOP_PACKET;

typedef struct __SOMA_SERIAL_REQ_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  rfu[25];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED SOMA_SERIAL_REQ_PACKET;

typedef struct __SOMA_SERIAL_RESP_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  serial_number[6];
    uint8_t  rfu[19];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED SOMA_SERIAL_RESP_PACKET;

typedef struct __AXON_SERIAL_REQ_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  port_num;
    uint8_t  rfu[24];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED AXON_SERIAL_REQ_PACKET;

typedef struct __AXON_SERIAL_RESP_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  error_flag;
    uint8_t  serial_number[6];
    uint8_t  fw_ver;
    uint8_t  rfu[17];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED AXON_SERIAL_RESP_PACKET;

typedef struct __SET_AXON_REQ_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  port_num;
    uint8_t  set_port;
    uint8_t  set_led;
    uint8_t  rfu[22];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED SET_AXON_REQ_PACKET;

typedef struct __AXON_STATUS_REQ_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  port_num;
    uint8_t  rfu[24];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED AXON_STATUS_REQ_PACKET;

typedef struct __AXON_STATUS_RESP_PACKET {
    uint8_t header;
    uint8_t len;
    uint8_t id;
    uint8_t port_num;
    struct {
        uint8_t solenoid : 1;  // 0: off, 1: on
        uint8_t led_status : 3;
        uint8_t cash_block : 1;
        uint8_t rfu : 3;
    } __PACKED status;
    uint8_t    rfu[23];
    uint16_t   dic;
    uint16_t   auth_code;
    uint16_t   rnd;
    uint16_t   crc16;
} __PACKED AXON_STATUS_RESP_PACKET;

typedef struct __USER_MEM_WRITE_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  data[24];
    uint8_t  rfu[1];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED USER_MEM_WRITE_PACKET;

typedef struct __USER_MEM_READ_REQ_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  rfu[25];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED USER_MEM_READ_REQ_PACKET;

typedef struct __USER_MEM_READ_RESP_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  data[24];
    uint8_t  rfu[1];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED USER_MEM_READ_RESP_PACKET;

typedef struct __SOMA_FIRM_UPDATE_REQ_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  rfu[25];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED SOMA_FIRM_UPDATE_REQ_PACKET;

typedef struct __SET_OP_KEY_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  key[32];
    uint16_t crc;
} __PACKED SET_OP_KEY_PACKET;

typedef struct __SOMA_REBOOT_REQ_PACKET {
    uint8_t  header;
    uint8_t  len;
    uint8_t  id;
    uint8_t  rfu[25];
    uint16_t dic;
    uint16_t auth_code;
    uint16_t rnd;
    uint16_t crc16;
} __PACKED SOMA_REBOOT_REQ_PACKET;

typedef union __T2S_PACKET {
    ACK_PACKET                  ack;
    NACK_PACKET                 nack;
    STATUS_PACKET               status;
    NOP_PACKET                  nop;
    STOP_PACKET                 stop;
    SOMA_SERIAL_REQ_PACKET      soma_serial_req;
    SOMA_SERIAL_RESP_PACKET     soma_serial_resp;
    AXON_SERIAL_REQ_PACKET      axon_serial_req;
    AXON_SERIAL_RESP_PACKET     axon_serial_resp;
    SET_AXON_REQ_PACKET         set_axon_req;
    AXON_STATUS_REQ_PACKET      axon_status_req;
    AXON_STATUS_RESP_PACKET     axon_status_resp;
    USER_MEM_WRITE_PACKET       user_mem_write;
    USER_MEM_READ_REQ_PACKET    user_mem_read_req;
    USER_MEM_READ_RESP_PACKET   user_mem_read_resp;
    SOMA_FIRM_UPDATE_REQ_PACKET soma_firm_update_req;
    SET_OP_KEY_PACKET           set_op_key;
    SOMA_REBOOT_REQ_PACKET      soma_reboot_req;
} __PACKED T2A_PACKET;

#ifdef __cplusplus
}
#endif
#endif  // __T2A_PACKET_H__