#ifndef __RFID_H__
#define __RFID_H__

#include "c_stdint.h"

#ifdef __cplusplus
//extern "C"{
#endif

/* 
   basic outter function
   see: https://github.com/ondryaso/pi-rc522
 */
typedef enum {
  REQ_CARD_IDLE = 0x26,
  REQ_CARD_ALL = 0x52
} RfidReqMode;

typedef int rfid;
typedef int res_t;

rfid rfid_init( int spi_id, int pin_ss, int pin_rst );
res_t rfid_request( rfid, RfidReqMode );
res_t rfid_anticoll( rfid );
res_t rfid_select( rfid );
res_t rfid_auth( rfid );

res_t rfid_halt( rfid );
res_t rfid_read( rfid );
res_t rfid_write( rfid );
res_t rfid_increment( rfid );
res_t rfid_decrement( rfid );
res_t rfid_restore( rfid );
res_t rfid_transfer( rfid );

#define RFID_DEV_MAX 4
#define RFID_INIT_NOMOREDEV -1
#define RFID_INIT_NOMOREMEM -2

#define RFID_INVALID -1

#define RES_SUCCESS 0
#define RES_FAILED -1

#ifdef __cplusplus
//}
#endif

#endif//__RFID_H__
