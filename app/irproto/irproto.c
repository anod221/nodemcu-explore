#include "irproto.h"
#include "c_stdio.h"
#include "c_stdlib.h"
#include "platform.h"

typedef struct{
  setup_carrier setup;
  irmark mark;
  irspace space;
} ir_writer, *ir_writer_pointer;
ir_writer_pointer engine = NULL;

#define dummy() system_soft_wdt_feed()

int ir_set_writer( setup_carrier setup, irmark mark, irspace space )
{
  if( engine == NULL ){
    engine = c_malloc( sizeof(ir_writer) );
  }
  if( engine == NULL ) return -1;
  engine->setup = setup;
  engine->mark = mark;
  engine->space = space;
  return 0;
}

irsender irproto_encode_map[] = {
  NULL,
  ir_send_nec
};
irproto irproto_decode_map[] = {
  NULL,
  ir_recv_nec
};

//**************************************
// see https://www.sbprojects.net/knowledge/ir/nec.php
// TIME CONSTRANT FOR NEC IR PROTOCOL
#define ACCEPTABLE_BIAS 150	// 协议接收j时间的可接受误差范围

#define AGC_MARK 9000
#define AGC_SPACE 4500
#define AGC_REPEAT_SPACE 2250
#define LOGICAL_MARK 560
#define LOGIC_B0_SPACE (1120 - LOGICAL_MARK)
#define LOGIC_B1_SPACE (2250 - LOGICAL_MARK)

#define UNCERTAIN_PULSE_TIME (ACCEPTABLE_BIAS+AGC_MARK+AGC_SPACE)

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

static uint32 reverse_per_byte( uint32 c )
{
  uint32 retval = 0;
  for( uint32 mask = 0xff, nshift = 0; mask > 0; mask <<= 8, nshift += 8 )
    {
      uint8_t byte = (c & mask) >> nshift, rev = 0;
      for( uint8_t cur = 1; cur > 0; cur <<= 1 ){
	rev <<= 1;
	rev |= (byte & cur) ? 1 : 0;
      }
      retval |= rev << nshift;
    }
  return retval;
}

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
    return match( usec, AGC_MARK, AGC_GOES );
  else if( state == AGC_GOES ){
    if( match( usec, AGC_SPACE, LOGICAL_EXPECT ) == LOGICAL_EXPECT ){
      *data = 0;
      *len = 0;
    }
    return LOGICAL_EXPECT;
  }
  else if( state == LOGICAL_EXPECT )
    return match( usec, LOGICAL_MARK, LOGICAL_GOES );
  else if( state == LOGICAL_GOES ){
    if( match(usec, LOGIC_B0_SPACE, 1) == 1 )
      *data &= ~(((uint32_t)1)<<(*len));
    else if( match(usec, LOGIC_B1_SPACE, 1) == 1 )
      *data |= (1<<(*len));
    else return IRPROTO_STATE_ERROR;
    
    (*len)++;
    return ( *len == 32 ) ? EOP_EXPECT : LOGICAL_EXPECT;
  }
  else if( state == EOP_EXPECT ){
    *ready = 1;
    return match( usec, LOGICAL_MARK, EOP_GOES );
  }
  else return IRPROTO_STATE_ERROR;
}

int ir_send_nec( uint32_t code )
{
  if( engine == NULL ) return -1;

  code = reverse_per_byte(code);// notice that the nec is LSB ahead
    
  // nec载波
  engine->setup( 37900, 4 );
  
  const irmark MARK = engine->mark;
  const irspace SPACE = engine->space;
  uint32_t mask = ~( (uint32_t)(~0)>>1 );
  
  // AGC
  dummy();
  MARK( AGC_MARK );
  SPACE( AGC_SPACE );
  for( ; mask > 0; mask >>= 1 ){
    MARK( LOGICAL_MARK );
    SPACE( (code&mask) ? LOGIC_B1_SPACE : LOGIC_B0_SPACE );
  }
  MARK( LOGICAL_MARK );
  dummy();
  return 0;
}
