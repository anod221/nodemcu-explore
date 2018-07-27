#include "c_stdlib.h"
#include "platform.h"
#include "rfid.h"
#include "rc522.h"

static rfid_dev rfid_devices[RFID_DEV_MAX];

rfid rfid_init( int spi_id, int pin_ss, int pin_rst )
{
  int slot, i;
  for( i=0; i < RFID_DEV_MAX; ++i ){
    if( rfid_devices[i] == NULL ){
      slot = i;
      break;
    }
  }
  if( i == RFID_DEV_MAX ) return RFID_INIT_NOMOREDEV;

  rfid_dev dev;
  dev = c_malloc( sizeof(*dev) );
  if( dev == NULL ) return RFID_INIT_NOMOREMEM;

  dev->spi_id = spi_id;
  dev->pin_ss = pin_ss;
  dev->pin_rst = pin_rst;
  rfid_devices[slot] = dev;

  rc522_init( dev );
  return slot;
}

#define CHECK_RFID_VALID(id) (0 <= (id) && (id) < RFID_DEV_MAX)
#define check_rfid(id, dev)   rfid_dev dev;	    \
  {						    \
    if( !CHECK_RFID_VALID(id) ) {		    \
      return RFID_INVALID;			    \
    } else {					    \
      dev = rfid_devices[id];			    \
    }						    \
  }

// mcu -> m1
// cmd: 0x26/0x52
// data: <none>
// m1 => mcu
// data: 0 = 0x04, 1 = 0x00
res_t rfid_request( rfid id, RfidReqMode mode )
{
  check_rfid(id, dev);

  u8 packet[1] = {mode};
  u8 response[4];
  int resp_blen = rc522_sendcmd( dev, CMD_TRANSCEIVE, packet, 1, response);
  if( resp_blen < 0 ) return RES_FAILED;
  os_printf("resp for req<%xh>: %xh %xh|%d", mode, response[0], response[1], resp_blen);
  return RES_SUCCESS;
}

res_t rfid_anticoll( rfid id )
{
  check_rfid(id, dev);

  
  return RES_SUCCESS;
}

res_t rfid_select( rfid id )
{
  check_rfid(id, dev);

  
  return RES_SUCCESS;
}

res_t rfid_auth( rfid id )
{
  check_rfid(id, dev);

  
  return RES_SUCCESS;
}

res_t rfid_halt( rfid id )
{
  check_rfid(id, dev);

  
  return RES_SUCCESS;
}

res_t rfid_read( rfid id )
{
  check_rfid(id, dev);

  
  return RES_SUCCESS;
}

res_t rfid_write( rfid id )
{
  check_rfid(id, dev);

  
  return RES_SUCCESS;
}

res_t rfid_increment( rfid id )
{
  check_rfid(id, dev);

  
  return RES_SUCCESS;
}

res_t rfid_decrement( rfid id )
{
  check_rfid(id, dev);

  
  return RES_SUCCESS;
}

res_t rfid_restore( rfid id )
{
  check_rfid(id, dev);

  
  return RES_SUCCESS;
}

res_t rfid_transfer( rfid id )
{
  check_rfid(id, dev);

  
  return RES_SUCCESS;
}
