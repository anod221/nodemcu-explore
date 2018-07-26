#include "platform.h"
#include "rfid.h"
#include "rc522.h"

// ======== for initialize ========
#define rc522_pin_init( v ) {\
    platform_gpio_mode( (v) PLATFORM_GPIO_OUTPUT, PLATFORM_GPIO_FLOAT ); \
    platform_gpio_write( (v), PLATFORM_GPIO_HIGH ); \
  }
#define rc522_reg_init( d, r, v ) rc522_writereg( (d), (r), CAST(u8, v) )  

void rc522_init( rfid_dev dev )
{
  // set the gpio pin
  rc522_pin_init( dev->pin_ss );
  rc522_pin_init( dev->pin_rst );

  // reset all register
  rc522_reset( dev );

  // initial register
  bTModeReg tmode = {  };

  // turn on the antenna
  rc522_antenna( dev, RC522_ANTENNA_ON );
}

// ======== for register r/w ========
typedef struct {
u8 :1, address:6, flag:1;
} rc522_spiaddr;

#define SPI_READ  1
#define SPI_WRITE 0

#define RC522_SPI_OPEN(d) platform_gpio_write((d)->pin_ss, PLATFORM_GPIO_LOW)
#define RC522_SPI_CLOSE(d) platform_gpio_write((d)->pin_ss, PLATFORM_GPIO_HIGH)
#define SPI_BITSIZE sizeof(u8)

void rc522_writereg( rfid_dev dev, u8 addr, u8 val )
{
  RC522_SPI_OPEN(dev);

  rc522_spiaddr regaddr = { addr, SPI_WRITE };
  spi_data_type data = (CAST(u8, regaddr) << 8) | val;
  platform_spi_send( dev->spi_id, SPI_BITSIZE*2, data );
  
  RC522_SPI_CLOSE(dev);
}

u8 rc522_readreg( rfid_dev dev, u8 addr )
{
  RC522_SPI_OPEN(dev);

  rc522_spiaddr regaddr = { addr, SPI_READ };
  spi_data_type data = CAST(u8, regaddr) << 8;
  spi_data_type res = platform_spi_send_recv( dev->spi_id, SPI_BITSIZE*2, data );
  
  RC522_SPI_CLOSE(dev);
  
  return (u8) res;
}

#define readreg( d, r ) CAST(b##r, rc522_readreg((d), (r)))
#define writereg(d, r, b) rc522_writereg((d), (r), CAST(u8, b))

void rc522_reset( rfid_dev dev )
{
  rc522_sendcmd( dev, CMD_SOFT_RESET );
}

void rc522_antenna( rfid_dev dev, AntennaStatus st )
{
  bTxControlReg v = readreg( dev, TxControlReg );
  v.Tx1RFEn = st;
  v.Tx2RFEn = st;
  writereg( dev, TxControlReg, v );
}

// ======== for send command ========
#define clear_fifo(dev) { \
  bFIFOLevelReg flag = {0, 1};			\
  writereg(dev, FIFOLevelReg, flag);		\
  }

static int to_fifo(rfid_dev dev, u8 *buff, size_t sz)
{
  if( sz > FIFO_SIZE ) return -1;

  // clear FIFO
  clear_fifo( dev );

  // send multi bytes at one time
  RC522_SPI_OPEN( dev );
  
  // send addr
  rc522_spiaddr addr = { FIFODataReg, SPI_WRITE };
  spi_data_type t = (spi_data_type)CAST( u8, addr );
  platform_spi_send( dev->spi_id, SPI_BITSIZE, t );

  // send data
  platform_spi_blkwrite( dev->spi_id, sz, buff );

  RC522_SPI_CLOSE( dev );
  return 0;
}

static size_t from_fifo(rfid_dev dev, u8 *buff)
{
  bFIFOLevelReg r = readreg(dev, FIFOLevelReg);
  if( r.FIFOLevel == 0 ) return 0;

  // platform spi 
  size_t sz = r.FIFOLevel;
  if( sz == 0 ) return 0;

  // init array
  int id = dev->spi_id;
  rc522_spiaddr d = { FIFODataReg, SPI_READ };
  os_memset( buff, (int)CAST(u8, d), sz );
  
  RC522_SPI_OPEN(dev);

  platform_spi_blkwrite( id, sz, buff );
  platform_spi_send( id, SPI_BITSIZE, 0 );
  platform_spi_blkread( id, sz, buff );

  RC522_SPI_CLOSE(dev);
  return sz;
}

static void wait_cmd( dev )
{
  bCommandReg r = readreg(dev, CommandReg);
  
  switch( r.Command ){
  case CMD_IDLE: return 0;
  case CMD_MEM:
  case CMD_GENERATE_RANDOM_ID:
  case CMD_TRANSMIT:
  case CMD_NO_CMD_CHANGE:
  case CMD_RECEIVE:
  case CMD_MFAUTHENT:
  case CMD_SOFT_RESET:
    break;
  default:
    // won't auto finish 
    r = { CMD_IDLE, 0, 0 };
    writereg( dev, CommandReg, r );
    return 1;
  }

  // will auto finish 
  system_soft_wdt_feed();
  while( r.Command != CMD_IDLE ){
    os_delay_us( 100 );
    r = readreg( dev, CommandReg );
  }

  return 0;
}

#define exec_cmd( dev, cmd ) {			\
  bCommandReg bcmd = {cmd, 0, 0};		\
  writereg( dev, CommandReg, bcmd );		\
  }

#define rc522_cmd_nofifo exec_cmd

#define wait_crc_done(dev) { \
  bStatus1Reg bst = readreg(dev, Status1Reg); \  
while( bst.CRCReady == 0 ){			\
  system_soft_wdt_feed();			\
  os_delay_us( 100 );				\
  bst = readreg(dev, Status1Reg);		\
 }						\
}

static int rc522_cmd_crc( rfid_dev dev, u8* buff, size_t sz )
{
  // if current command is crc then fill fifo only
  bCommandReg bcmd = readreg(dev, CommandReg);
  if( r.Command == CMD_CALC_CRC ){
    wait_crc_done( dev );
    to_fifo(dev, buff, sz);
  }
  else {
    wait_cmd( dev );
    to_fifo(dev, buff, sz);

    bcmd = {CMD_CALC_CRC, 0, 0};
    writereg(dev, CommandReg, bcmd);
  }

  wait_crc_done(dev);

  // read the result
  bCRCResultRegL brl = readreg( dev, CRCResultRegL );
  bCRCResultRegH brh = readreg( dev, CRCResultRegH );
  return (brh.CRCResultMSB << 8) | brl.CRCResultLSB;
}

static int rc522_cmd_readfifo( rfid_dev dev, u8 *buff )
{
  wait_cmd( dev );

  // flush the fifo
  clear_fifo( dev );
  writereg( dev, FIFOLevelReg, flag );

  // exec the cmd
  exec_cmd( dev, CMD_TRANSMIT );
  wait_cmd( dev );
  
  // read
  size_t retval = from_fifo( dev, buff );
  return (int) retval;
}

#define MEM_PROTOCOL_SIZE 25
static int rc522_cmd_mem(rfid_dev dev, u8 *buff, u8 isWrite)
{
  wait_cmd( dev );

  bCommandReg bcmd = {CMD_MEM, 0, 0};
  if( isWrite ){
    to_fifo(dev, buff, MEM_PROTOCOL_SIZE);
    writereg( dev, CommandReg, bcmd );
    wait_cmd( dev );
  }
  else {
    clear_fifo(dev);
    writereg( dev, CommandReg, bcmd );
    wait_cmd( dev );
    from_fifo(dev, buff);
  }
  
  return MEM_PROTOCOL_SIZE;
}

#define AUTH_PROTOCOL_SIZE 12

static int rc522_cmd_auth( rfid_dev dev, u8 *data )
{
  wait_cmd(dev);
  to_fifo(dev, data, AUTH_PROTOCOL_SIZE);
  exec_cmd(dev, CMD_MFAUTHENT);
  wait_cmd(dev);

  bErrorReg err = readreg(dev, ErrorReg);
  if( err.ProtocolErr ) return -1;
  
  bStatus2Reg res = readreg(dev, Status2Reg);
  if( res.MFCrypto1On )
    return 1;
  else return 0;
}

static int rc522_cmd_transceive(rfid_dev dev, u8 *arg, size_t szarg, u8 *res)
{
  wait_cmd( dev );
  
  
}

// name        num FIFO :len:   :flush:   :fin:
// Idle         0    n    -        -        Y
// Mem          1   Y     25    Y@r,n@w     Y
// GenRandID    2    n    -        -        Y
// CalcCRC      3   Y     ∞        n        n
// Transmit     4   Y    <64       n        Y
// NoCmdChange  7    n    -        -        Y
// Receive      8    n    -        -        Y
// Transceive   12  Y     ∞        Y        n
// MFAuthent    14  Y     12       n        Y
// SoftReset    15   n    -        -        Y

// retval: <0 == error  >=0 == result
int rc522_sendcmd(
   rfid_dev dev, u8 cmd,
   u8 *arg, size_t szarg,
   u8 *res )
{
  // without FIFO
  switch( cmd ){
  case CMD_IDLE:
    rc522_cmd_nofifo(dev, cmd);
    return 1;
  case CMD_GENERATE_RANDOM_ID:
  case CMD_NO_CMD_CHANGE:
  case CMD_RECEIVE:
  case CMD_SOFT_RESET:
    rc522_cmd_nofifo(dev, cmd);
    wait_cmd(dev);
    return 1;
  case CMD_CALC_CRC:
    return rc522_cmd_crc(dev, arg, szarg);
  case CMD_TRANSMIT:
    return rc522_cmd_readfifo(dev, res);
  case CMD_MEM:
    if( arg == NULL )
      return rc522_cmd_mem(dev, res, 0);
    else
      return rc522_cmd_mem(dev, arg, 1);
  case CMD_MFAUTHENT:
    return rc522_cmd_auth(dev, arg);
  case CMD_TRANSCEIVE:
  default:
    return rc522_cmd_transceive(dev, arg, szarg, res);
  }
}
