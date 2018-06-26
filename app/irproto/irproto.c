#include "irproto.h"
#include "c_stdio.h"
#include "platform.h"

// see https://www.sbprojects.net/knowledge/ir/nec.php

// TIME CONSTRANT FOR NEC IR PROTOCOL
#define ACCEPTABLE_BIAS 150	// 协议接收j时间的可接受误差范围

#define AGC_BURST 9000
#define AGC_BURST_SPACE 4500
#define AGC_REPEAT_SPACE 2250
#define LOGICAL_BURST 560
#define LOGIC_B0_SPACE (1120 - LOGICAL_BURST)
#define LOGIC_B1_SPACE (2250 - LOGICAL_BURST)

#define UNCERTAIN_PULSE_TIME (ACCEPTABLE_BIAS+AGC_BURST+AGC_BURST_SPACE)

#define u32_byte_reverse(c) ( ((c) & 0x000000FFU) << 24 |	\
			      ((c) & 0x0000FF00U) << 8 |	\
			      ((c) & 0x00FF0000U) >> 8 |	\
			      ((c) & 0xFF000000U) >> 24 ) 

// state
enum{
  AGC_EXPECT,
  AGC_GOES,
  LOGICAL_EXPECT,
  LOGICAL_GOES,
  EOP_EXPECT,
  EOP_GOES
};

#define match( t, expect, next ) ( (abs((t)-(expect)) < ACCEPTABLE_BIAS) ? (next) : IRPROTO_STATE_ERROR )

irproto irproto_decode_map[] = {
  NULL,
  ir_recv_nec
};

// usec:
// example 48989587,9107,4495,614,553,613,549,616,551,613,552,613,552,612,554,612,553,612,552,613,1665,585,1665,586,1664,587,1638,611,1640,611,1664,585,1639,611,1665,586,1664,586,554,611,553,613,1664,586,1664,587,552,613,551,614,553,612,552,614,1664,586,1664,587,552,613,553,612,1640,611,1640,611,1640,609,39669,9121,2221,611
// example 12608055,9122,4496,613,552,615,551,614,553,612,551,615,552,614,552,614,552,614,550,616,1664,586,1664,588,1640,610,1664,587,1664,587,1640,611,1638,613,1640,611,1664,587,1639,612,1664,587,552,613,552,614,552,614,1664,587,552,613,553,613,553,613,552,613,1665,587,1664,586,1639,613,551,613,1643,610
// example 57419939,9117,4498,612,553,613,551,614,555,611,552,613,552,614,553,612,553,613,551,613,1642,610,1640,611,1641,609,1641,610,1638,613,1665,586,1664,587,1640,610,1665,586,552,614,1663,587,552,613,553,613,554,611,1664,587,551,613,554,612,1665,586,553,613,1664,587,1664,586,1642,609,553,612,1664,586
uint32 ir_recv_nec(uint32 state, uint32 level, uint32 usec, uint32_t *data, int8_t *len, int8_t *ready)
{
  *ready = 0;
  if( usec > UNCERTAIN_PULSE_TIME ) {
    return AGC_EXPECT;
  }

  if( state == AGC_EXPECT )
    return match( usec, AGC_BURST, AGC_GOES );
  else if( state == AGC_GOES ){
    if( match( usec, AGC_BURST_SPACE, LOGICAL_EXPECT ) == LOGICAL_EXPECT ){
      *data = 0;
      *len = 0;
    }
    return LOGICAL_EXPECT;
  }
  else if( state == LOGICAL_EXPECT )
    return match( usec, LOGICAL_BURST, LOGICAL_GOES );
  else if( state == LOGICAL_GOES ){
    if( match(usec, LOGIC_B0_SPACE, 1) == 1 )
      *data &= ~(((uint32_t)1)<<(*len));
    else if( match(usec, LOGIC_B1_SPACE, 1) == 1 )
      *data |= (1<<(*len));
    else return IRPROTO_STATE_ERROR;
    
    (*len)++;
    if( *len == 32 ){
      *ready = 1;
      return EOP_EXPECT;
    }
    else return LOGICAL_EXPECT;
  }
  else if( state == EOP_EXPECT ){
    *ready = (*len == 32) ? 1 : 0;
    return match( usec, LOGICAL_BURST, EOP_GOES );
  }
  else return IRPROTO_STATE_ERROR;
}
