#include "module.h"
#include "lauxlib.h"
#include "platform.h"
#include "c_stdlib.h"
#include "task/task.h"
#include "irproto.h"
#include "driver/uart.h"

// see https://www.analysir.com/blog/2017/01/29/updated-esp8266-nodemcu-backdoor-upwm-hack-for-ir-signals/
static uint8_t tx_val;
// use integer instead of real，for precision, store 100 byte instead of 1 byte
static uint32_t usec_per100bytes;

#define PIN_IRSEND 4
#define CARRIER_TX 1
#define BIT_PER_TX_BYTE 10 // 8bit + 2signal
#define USEC_PER_SECOND (1000*1000)
#define WDT_FEED_LOOP 64

#define DUTY_10_PERCENT 0xff
#define DUTY_20_PERCENT 0xfe
#define DUTY_30_PERCENT 0xfc
#define DUTY_40_PERCENT 0xf8
#define DUTY_50_PERCENT 0xf0

#define BAUDRATE_AS_CARRIER( hz ) ((hz) * BIT_PER_TX_BYTE)

#define wait_tx_empty() while((READ_PERI_REG(UART_STATUS(CARRIER_TX)) & (UART_TXFIFO_CNT<<UART_TXFIFO_CNT_S)) > 0);
#define us2ms( usec ) ((usec)/1000)

inline void delay_one_msec()
{
  os_delay_us(1000);
}

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
  int r = s(code);
  lua_pushinteger( L, r );
  
  return 1;
}

// see app/driver/uart.c:73
// If call the uart_config function directly, a pulse will be found. 
// Because the bit UART_TXD_INV is not set when write the UART_CONF0 reg.
// To avoid this, the bit should be set when call the WRITE_PERI_REG.
void set_uart( uint32 baudrate )
{
  wait_tx_empty();

  ETS_UART_INTR_DISABLE();

  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
  uart_div_modify(CARRIER_TX, UART_CLK_FREQ / (baudrate));
  WRITE_PERI_REG(
    UART_CONF0( CARRIER_TX ),
    ((STICK_PARITY_DIS & UART_PARITY_EN_M) << UART_PARITY_EN_S) //SET BIT AND PARITY MODE
    | ((NONE_BITS & UART_PARITY_M) << UART_PARITY_S )
    | ((ONE_STOP_BIT & UART_STOP_BIT_NUM) << UART_STOP_BIT_NUM_S)
    | ((EIGHT_BITS & UART_BIT_NUM) << UART_BIT_NUM_S)
    | UART_TXD_INV
  );

  //clear rx and tx fifo,not ready
  SET_PERI_REG_MASK(UART_CONF0(CARRIER_TX), UART_RXFIFO_RST | UART_TXFIFO_RST);
  CLEAR_PERI_REG_MASK(UART_CONF0(CARRIER_TX), UART_RXFIFO_RST | UART_TXFIFO_RST);

  //set rx fifo trigger( UART1 has no rx )
  // WRITE_PERI_REG(UART_CONF1(CARRIER_TX), (UartDev.rcv_buff.TrigLvl & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S);

  //clear all interrupt
  WRITE_PERI_REG(UART_INT_CLR(CARRIER_TX), 0xffff);
  //enable rx_interrupt
  SET_PERI_REG_MASK(UART_INT_ENA(CARRIER_TX), UART_RXFIFO_FULL_INT_ENA);

  ETS_UART_INTR_ENABLE();
}

void set_carrier( uint32 hz, uint32 duty )
{
  // set the pwm
  uint32 baudrate = BAUDRATE_AS_CARRIER(hz);
  usec_per100bytes = BIT_PER_TX_BYTE * USEC_PER_SECOND / (baudrate/100);
  if( duty == 1 ) tx_val = DUTY_10_PERCENT;
  else if( duty == 2 ) tx_val = DUTY_20_PERCENT;
  else if( duty == 3 ) tx_val = DUTY_30_PERCENT;
  else if( duty == 4 ) tx_val = DUTY_40_PERCENT;
  else tx_val = DUTY_50_PERCENT;

  set_uart( baudrate );
}

void send_mark( uint32 usec )
{
  int npwm = 100 * usec / usec_per100bytes;

  system_soft_wdt_feed();
  while( npwm-->0 ){
    uart_tx_one_char(CARRIER_TX, tx_val);

    if( npwm % WDT_FEED_LOOP == 0 )
      system_soft_wdt_feed();
  }
  
  wait_tx_empty();
}

void send_space( uint32 usec )
{
  if( usec <= USHRT_MAX ) os_delay_us( usec-5 );//e.g. comp, add, push, call, leave, ret.
  else {
    uint32 ms_now = system_get_time();
    uint32 ms_end = ms_now + us2ms(usec);
    uint32 us_delay = usec % 1000;
    if( ms_end < ms_now ){// overflow
      while( system_get_time() < UINT_MAX ){
	delay_one_msec();
	system_soft_wdt_feed();
      }
      delay_one_msec();
    }
    while( system_get_time() < ms_end ){
      delay_one_msec();
      system_soft_wdt_feed();
    }
    os_delay_us( us_delay - 6 );// e.g. leave, ret, compare, push, call, leave, ret
  }
}

int irsend_setup( lua_State *L )
{
  SET_PERI_REG_MASK( UART_CONF0( CARRIER_TX ), UART_TXD_INV );
  return 0;
}

int luaopen_irsend( lua_State *L )
{
  ir_set_writer( set_carrier, send_mark, send_space );
}

static const LUA_REG_TYPE irsend_map[] = {
  // for module constants
  { LSTRKEY("NEC"), LNUMVAL(IRPROTO_NEC) },
  // for module function
  { LSTRKEY("send"), LFUNCVAL(irsend_write) },
  { LSTRKEY("setup"), LFUNCVAL(irsend_setup) }, 
  // --------
  {LNILKEY, LNILVAL}
};

NODEMCU_MODULE( IRSEND, "irsend", irsend_map, luaopen_irsend );
