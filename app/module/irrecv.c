#include "module.h"
#include "lauxlib.h"
#include "platform.h"
#include "irproto.h"
#include "c_stdlib.h"
#include "task/task.h"

static task_handle_t callback_stub;
static uint32_t dup_mask = 0;	// set bit to 1 when pin is bind

#define pin_bit(pin) ( 1<<GPIO_ID_PIN(pin_num[pin]) )
#define pin_has_set(val, pin) ( ((val) & pin_bit(pin)) != 0 )
#define pin_set( val, pin ) ( (val) | pin_bit(pin) )

typedef struct {
  int                 state;		// 当前状态
  uint32              prev_pulse_time;  // 上一个edge到达时间
  int	              callback;		// 回调函数
  irproto             proto;		// ir协议
  uint32_t            protodata;	// 读取到的数据，最多32b
  // flag: bitmap for multi usage
  // bit[5..0] = valid data size for protodata
  // bit[6] = level for prevent pulse
  // bit[7] = level for initial pulse
  // bit[8] = 1 for protodata is ready to callback
  uint32_t            flag; 
} ir_pin_reader, *ir_pin_reader_pointer;

static ir_pin_reader_pointer pin_reader[GPIO_PIN_NUM];

#define LEVEL_INIT_BIT 8
#define LEVEL_PREV_BIT 8

#define FLAG_VALID_BIT 5
#define FLAG_PREVLV_BIT 6
#define FLAG_INITLV_BIT 7
#define FLAG_READY_BIT 8

#define get_prev_level( reader ) (((reader)->flag & (1<<FLAG_PREVLV_BIT))>>FLAG_PREVLV_BIT)
#define set_prev_level( reader, level ) (reader)->flag = (level) ? ((reader)->flag | (1<<FLAG_PREVLV_BIT)) : ((reader)->flag & ~(1<<FLAG_PREVLV_BIT))

#define set_init_level( reader, level ) (reader)->flag = (level) ? ((reader)->flag | (1<<FLAG_INITLV_BIT)) : ((reader)->flag & ~(1<<FLAG_INITLV_BIT))
#define reset_init_level( reader ) set_prev_level( reader, (reader)->flag & (1<<FLAG_INITLV_BIT) )

#define get_proto_validbit( reader ) ( (reader)->flag & ((1<<(FLAG_VALID_BIT+1))-1) )
#define set_proto_validbit( reader, bitlen ) (reader)->flag = ((reader)->flag & ~((1<<(FLAG_VALID_BIT+1))-1)) | ((1<<(FLAG_VALID_BIT+1))-1) & (bitlen)
#define get_proto_dataready( reader ) (((reader)->flag & (1<<FLAG_READY_BIT))>>FLAG_READY_BIT)
#define set_proto_dataready( reader, ready ) (reader)->flag = (ready) ? ((reader)->flag | (1<<FLAG_READY_BIT)) : ((reader)->flag & ~(1<<FLAG_READY_BIT))

static uint32 ICACHE_FLASH_ATTR gpio_intr_handler( uint32 mask ){
  if( mask & dup_mask ){
    GPIO_REG_WRITE( GPIO_STATUS_W1TC_ADDRESS, dup_mask );
    uint32 now = system_get_time();
    uint32_t protodata;
    int8_t writebits;
    int8_t protoready;
    int do_callback = 0;
 
    for( int pin=0; mask > 0; mask >>= 1, ++pin ) {
      if( mask & 1 ){
	int io = pin_num_inv[pin];
	uint32 level = platform_gpio_read( io );
	ir_pin_reader_pointer reader = pin_reader[io];
	protodata = reader->protodata;
	writebits = get_proto_validbit(reader);
	protoready = 0;

	// 开始读取协议
	reader->state = reader->proto(
				      reader->state,
				      IRPROTO_LEVEL_PARAM( level, get_prev_level(reader) ),
				      now-reader->prev_pulse_time,
				      &protodata,
				      &writebits,
				      &protoready);
	// assert( writebits <= 8 )
	reader->prev_pulse_time = now;
	reader->protodata = protodata;
	set_proto_validbit(reader, writebits);
	set_proto_dataready( reader, protoready );
	do_callback = do_callback || protoready;
      }
    }

    if( do_callback ){
      task_post_medium( callback_stub, (task_param_t)0 );
    }
  }
  return mask & (~dup_mask);
}

static void perform_ondata_callback( lua_State *L, ir_pin_reader_pointer reader ){
  uint32_t v = reader->protodata;
  uint32_t len = get_proto_validbit(reader);
  int narg = 0;
  if( len > 0 ){
    lua_rawgeti(L, LUA_REGISTRYINDEX, reader->callback);
    for( narg = 0 ; len > 0; len -= 8, v >>= 8, ++narg ){
      lua_pushinteger(L, v & 0xff);
    }
    lua_call(L, narg, 0);
  }
}

// callback the lua function for the pin which protodata is ready 
static void irrecv_ondata(task_param_t param, uint8 priority)
{
  uint32_t n;
  int pin;
  lua_State *L = lua_getstate();
  for( n = dup_mask, pin = 0; n > 0; ++pin, n >>= 1 ){
    if( n & 1 ){
      int io = pin_num_inv[pin];
      ir_pin_reader_pointer reader = pin_reader[io];
      if( get_proto_dataready(reader) ){
	perform_ondata_callback(L, reader);
      }
    }
  }
}

// irrecv.bind( pin, protocol, callback_fun( n ) end )
static int ICACHE_FLASH_ATTR irrecv_bind( lua_State *L ){
  const uint8_t pin = luaL_checkinteger(L, 1);
  luaL_argcheck(L, 0 <= pin && pin < GPIO_PIN_NUM, 1, "invalid pin index");

  const int type = luaL_checkinteger( L, 2 );
  luaL_argcheck(L, 0 < type && type < IRPROTO_MAX, 2, "invalid protocol");
    
  int callback = LUA_NOREF;
  if( lua_type(L, 3) == LUA_TFUNCTION
      || lua_type(L, 3) == LUA_TLIGHTFUNCTION ){
    lua_pushvalue(L, 3);
    callback = luaL_ref(L, LUA_REGISTRYINDEX);
  }
  else luaL_argcheck(L, 0, 3, "invalid callback type");
  
  ir_pin_reader_pointer reader;
  if( pin_has_set( dup_mask, pin ) ){
    reader = pin_reader[pin];

    // replace the callback
    if( reader->callback != LUA_NOREF ){
      luaL_unref(L, LUA_REGISTRYINDEX, reader->callback);
    }

    reader->state = IRPROTO_STATE_ERROR;
    reader->prev_pulse_time = 0;
    reader->callback = callback;
    reset_init_level( reader );
    set_proto_dataready(reader, 0);
    set_proto_validbit(reader, 0);
    reader->protodata = 0;
    reader->proto = ir_recv_nec; /* TODO: 按type修改 */
  }
  else{
    // set pin function
    platform_gpio_mode(pin, PLATFORM_GPIO_INPUT, PLATFORM_GPIO_FLOAT );
    platform_gpio_intr_init( pin, GPIO_PIN_INTR_ANYEDGE );

    reader = c_malloc( sizeof( ir_pin_reader ) );
    reader->state = IRPROTO_STATE_ERROR;
    reader->prev_pulse_time = 0;
    reader->callback = callback;
    uint32 level = platform_gpio_read( pin );
    set_init_level( reader, level );
    reset_init_level( reader );
    set_proto_dataready( reader, 0 );
    set_proto_validbit( reader, 0 );
    reader->protodata = 0;
    reader->proto = ir_recv_nec;
    pin_reader[pin] = reader;

    // update hook mask
    dup_mask = pin_set(dup_mask, pin);
    platform_gpio_register_intr_hook(dup_mask, gpio_intr_handler);
  }
  // TODO: DELETE BEGIN
  platform_gpio_mode(4, PLATFORM_GPIO_OUTPUT, PLATFORM_GPIO_FLOAT);
  platform_gpio_write(4, PLATFORM_GPIO_HIGH);
  // TODO: DELETE END
}

// Module function map
static const LUA_REG_TYPE irrecv_map[] = {
  // for module function
  { LSTRKEY( "bind" ), LFUNCVAL( irrecv_bind ) },
  // for module constant
  { LSTRKEY( "NEC" ), LNUMVAL( IRPROTO_NEC ) },
  // ---------
  { LNILKEY, LNILVAL }
};

int luaopen_irrecv( lua_State *L ){
  // TODO: Make sure that the GPIO system is initialized
  os_bzero( pin_reader, sizeof(ir_pin_reader_pointer) * GPIO_PIN_NUM );
  callback_stub = task_get_id( irrecv_ondata );
  return 0;
}

NODEMCU_MODULE( IRRECV, "irrecv", irrecv_map, luaopen_irrecv );
