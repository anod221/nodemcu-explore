#ifndef _IRPROTO_H
#define _IRPROTO_H

#include "c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IRPROTO_LEVEL_CURRENT(level) ((level) & 0xff)
#define IRPROTO_LEVEL_PREVENT(level) (((level) & 0xff00)>>8)
#define IRPROTO_LEVEL_PARAM(current, prevent) ((((current) & 0xff)) | (((prevent)<<8) & 0xff00))
#define IRPROTO_STATE_ERROR -1

  enum {
    IRPROTO_NEC = 1,
    IRPROTO_MAX
  };

  // ================
  // 发送相关
  // ================
  typedef void (*setup_carrier)( uint32 hz, uint32 duty_cicle );
  typedef void (*irmark)( uint32 usec );
  typedef void (*irspace)( uint32 desc );
  typedef int (*irsender)( uint32_t code );

  irsender irproto_encode_map[IRPROTO_MAX];
  
  int ir_set_writer( setup_carrier, irmark, irspace );
  int ir_send_nec( uint32_t code );

  // ================
  // 接收相关
  // ================
  // 就是一个状态机。为了能够兼容多数的红外协议，所以
  // 需要替换proto函数
  typedef uint32 (*irproto)( 	// 返回：下一状态
    uint32 state, 		// 当前状态
    uint32 level, 		// b[8]上一个中断点位, b[0]当前中断电位
    uint32 pulse_edge_usec,	// 距离上一个脉冲边缘的时间，单位为毫秒
    uint32_t *protodata,	// 此次处理得到的数据写入到protodata
    int8_t *write_bits,		// 此次处理总共写入了多少个位到protodata
    int8_t *data_ready		// 是否应该开始回调luac脚本
  );
  
  irproto irproto_decode_map[IRPROTO_MAX];
  
  uint32 ir_recv_nec( uint32, uint32, uint32, uint32_t*, int8_t*, int8_t* );
  
#ifdef __cplusplus
}
#endif

#endif//_IRPROTO_H
