#include "irproto.h"
#include "c_stdio.h"
#include "platform.h"

uint32 ir_recv_nec(uint32 state, uint32 level, uint32 usec, int32_t *data, int8_t *len)
{
  c_printf("time:%d", usec);
  platform_gpio_write(4, (level&1) ? PLATFORM_GPIO_HIGH : PLATFORM_GPIO_LOW );
  return state;
}
