#include "module.h"
#include "lauxlib.h"
#include "platform.h"

static uint16_t dup_mask = 0;	// set bit to 1 when pin is bind
static int pin_callback[GPIO_PIN_NUM];

#define pin_bit(pin) ( 1<<(pin) )
#define pin_has_set(val, pin) ( ((val) & pin_bit(pin)) != 0 )
#define pin_set( val, pin ) ( (val) | pin_bit(pin) )

static uint32_t ICACHE_FLASH_ATTR gpio_intr_handler( uint32_t mask ){
  if( mask & dup_mask ){
    GPIO_REG_WRITE( GPIO_STATUS_W1TC_ADDRESS, dup_mask );
    
  }
  return mask & (~dup_mask);
}

// irrecv.bind( pin, callback_fun( n ) end )
static int ICACHE_FLASH_ATTR rcrecv_bind( lua_State *L ){
  const uint8_t pin = luaL_checkinteger(L, 1);
  luaL_argcheck(L, 0 <= pin && pin < GPIO_PIN_NUM, 1, "invalid pin index");
    
  const int callback = LUA_NOREF;
  if( lua_type(L, 2) == LUA_TFUNCTION
      || lua_type(L, 2) == LUA_TLIGHTFUNCTION ){
    lua_pushvalue(L, 2);
    callback = luaL_ref(L, LUA_REGISTRYINDEX);
  }
  else luaL_argcheck(L, 0, 2, "invalid callback type");
  
  if( pin_has_set( dup_mask, pin ) ){
    // replace the callback
    if( pin_callback[pin] != LUA_NOREF ){
      luaL_unref(L, LUA_REGISTRYINDEX, pin_callback[pin]);
    }
    pin_callback[pin] = callback;
  }
  else{
    // set pin function
    platform_gpio_mode(pin, PLATFORM_GPIO_OUTPUT, PLATFORM_GPIO_FLOAT );
    platform_gpio_intr_init( pin, GPIO_PIN_INTR_ANYEDGE );

    // update hook mask
    dup_mask = pin_set(dup_mask, pin);
    platform_gpio_register_intr_hook(dup_mask, gpio_intr_handler);    
  }
}

// Module function map
static const LUA_REG_TYPE irrecv_map[] = {
  { LSTRKEY( "bind" ), LFUNCVAL( rcrecv_bind ) },
  { LNILKEY, LNILVAL }
};

int luaopen_irrecv( lua_State *L ){
  // TODO: Make sure that the GPIO system is initialized
  for( int i=0; i < GPIO_PIN_NUM; ++i )
    pin_callback = LUA_NOREF;
  return 0;
}

NODEMCU_MODULE( IRRECV, "irrecv", irrecv_map, luaopen_irrecv );
