#ifndef _IRPROTO_H
#define _IRPROTO_H

#include "c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IRPROTO_LEVEL_CURRENT(level) ((level) & 0xff)
#define IRPROTO_LEVEL_PREVENT(level) (((level) & 0xff00)>>8)
#define IRPROTO_LEVEL_PARAM(current, prevent) ((((current) & 0xff)) | (((prevent)<<8) & 0xff00))
  
  uint32 ir_recv_nec( uint32, uint32, uint32, uint32_t*, int8_t*, int8_t* );
  
#ifdef __cplusplus
}
#endif

#endif//_IRPROTO_H
