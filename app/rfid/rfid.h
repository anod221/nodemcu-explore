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
int rfid_init();
int rfid_request();
int rfid_anticoll();
int rfid_getcrc();
int rfid_select();
int rfid_auth();

int rfid_halt();
int rfid_read();
int rfid_write();
int rfid_restart();

  
#ifdef __cplusplus
//}
#endif

#endif//__RFID_H__
