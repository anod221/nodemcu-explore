#include "module.h"
#include "lauxlib.h"
#include "platform.h"
#include "c_stdlib.h"
#include "task/task.h"
#include "irproto.h"

// see https://www.analysir.com/blog/2017/01/29/updated-esp8266-nodemcu-backdoor-upwm-hack-for-ir-signals/
static uint8_t tx_val;
static uint32_t usec_perbyte;

// irsend.send( code, mode=nil )
static int ICACHE_FLASH_ATTR irsend_write( lua_State *L )
{
  uint32_t code = luaL_checkinteger(L, 1);
  int mode = IRPROTO_NEC;
  if( lua_type(L, 2) == LUA_TNUMBER ){
    mode = luaL_checkinteger(L, 2);
  }
  luaL_argcheck(L, 0 < mode && mode < IRPROTO_MAX, 2, "invalid mode");

  irsender s = irproto_encode_map[mode];
  UartConfig curcfg = uart_get_config(0);
  ETS_UART_INTR_DISABLE();
  int r = s(code);
  ETS_UART_INTR_ENABLE();
  lua_pushinteger( L, r );
  
  platform_uart_setup( 0, curcfg.baut_rate, curcfg.data_bits, curcfg.parity, curcfg.stop_bits );

  return 1;
}

extern UartDevice UartDev;
void set_uart( uint32 baudrate )
{
  UartDev.baut_rate = baudrate;
  UartDev.data_bits = EIGHT_BITS;
  UartDev.stop_bits = ONE_STOP_BIT;
  UartDev.parity = NONE_BITS;
  uart_setup( 1 );
  SET_PERI_REG_MASK( UART_CONF0(1), BIT22 );
}

#define SET_PERI_REG_MASK(reg, mask)   WRITE_PERI_REG((reg), (READ_PERI_REG(reg)|(mask)))
void set_carrier( uint32 khz, uint32 duty )
{
  // set the pwm
  uint32 baudrate = khz * 1000 * 10;
  usec_perbyte = 10 * 1000000 / baudrate;// 1s = 1000 * 1000 us
  set_uart( baudrate );
  if( duty == 1 ) tx_val = 0xff;
  else if( duty == 2 ) tx_val = 0xfe;
  else if( duty == 3 ) tx_val = 0xfc;
  else if( duty == 4 ) tx_val = 0xf8;
  else tx_val = 0xf0;
}

void send_mark( uint32 usec )
{
  int npwm = usec / usec_perbyte;

  system_soft_wdt_feed();
  while( npwm-->0 ){
    uart_tx_one_char(1, tx_val);
    if( npwm % 64 == 0 ) system_soft_wdt_feed();
  }
  
  while ((READ_PERI_REG(UART_STATUS(1)) & (UART_TXFIFO_CNT<<UART_TXFIFO_CNT_S)) > 0);
}

void send_space( uint32 usec )
{
  if( usec <= 65535 ) os_delay_us( usec-5 );//e.g. comp, add, push, call, leave, ret.
  else {
    uint32 now = system_get_time();
    uint32 end = now + usec/1000;
    uint32 delay = usec % 1000;
    if( end < now ){// overflow
      while( system_get_time() < (uint32)0xffffffff ){
	os_delay_us(1000);
	system_soft_wdt_feed();
      }
      os_delay_us(1000);
    }
    while( system_get_time() < end ){
      os_delay_us(1000);
      system_soft_wdt_feed();
    }
    os_delay_us( delay - 6 );// e.g. leave, ret, compare, push, call, leave, ret
  }
}

int irsend_setup( lua_State *L ){
  platform_gpio_mode( 4, PLATFORM_GPIO_OUTPUT, PLATFORM_GPIO_FLOAT );
  platform_gpio_write( 4, PLATFORM_GPIO_LOW );
}

int luaopen_irsend( lua_State *L )
{
  ir_set_writer( set_carrier, send_mark, send_space );
}

static const LUA_REG_TYPE irsend_map[] = {
  // for module constants
  { LSTRKEY("NEC"), LNUMVAL(IRPROTO_NEC) },
  // for module function
  { LSTRKEY("setup"), LFUNCVAL(irsend_setup) },
  { LSTRKEY("send"), LFUNCVAL(irsend_write) },
  // --------
  {LNILKEY, LNILVAL}
};

NODEMCU_MODULE( IRSEND, "irsend", irsend_map, luaopen_irsend );
