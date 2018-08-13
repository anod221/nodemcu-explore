#include "module.h"
#include "lauxlib.h"
#include "platform.h"
#include "c_stdlib.h"
#include "task/task.h"

#include <stdio.h>
#include <ctype.h>
#include <stdint.h>

typedef struct {
  char date[7];			/* ddmmyy */
  char time[11];		/* hhmmss.xxx */
  char lon[13];			/* dddmm.mmmmmN */
  char lat[13];			/* dddmm.mmmmmE */
  char spd[10];			/* xxxx.xxxx */
  char cmg[10];			/* xxxx.xxxx */
  char magn[11];		/* xxxx.xxxxE */
  uint8_t gps;			/* boolean for gps data */
} gprmc_data;

#define has_capacity( p, l, s ) ( p+s <= l+sizeof(l) )
#define has_gps( d ) ((d)->gps != 0)

// 下面的变量本来放在parse里面的，但是放进去heap马上小了1000
// 只好放外面来了，放在外面不会影响heap的大小。。。
static gprmc_data result;
static uint8_t state = 0;
static char *pbuff;
static uint8_t checksum;
static uint8_t msgsum;

static gprmc_data* parse( char c ){
  gprmc_data* retval = NULL;
  switch( state ){
  case 0:
    if( c == '$' ) {
      checksum = 0;
      msgsum = 0;
      state = 1;
    }
    break;
  case 1:
    if( c == 'G' ) state = 2;
    else state = 0;
    break;
  case 2:
    if( c == 'P' ) state = 3;
    else state = 0;
    break;
  case 3:
    if( c == 'R' ) state = 4;
    else state = 0;
    break;
  case 4:
    if( c == 'M' ) state = 5;
    else state = 0;
    break;
  case 5:
    if( c == 'C' ) state = 6;
    else state = 0;
    break;
  case 6:
    if( c == ','){
      pbuff = result.time;
      state = 7;
    }
    else state = 0;
    break;
  case 7:
    if( c == '.' || isdigit(c) ){
      if( has_capacity(pbuff, result.time, 2) ) *pbuff++ = c; /* 留出\0和当前字符两个位置 */
    }
    else if( c == ',' ){
      if( has_capacity(pbuff, result.time, 1) ) *pbuff = '\0';
      state = 8;
    }
    else state = 0;
    break;
  case 8:
    if( c == 'A' ){
      result.gps = 1;
    }
    else if( c == 'V' ){
      result.gps = 0;
    }
    else if( c == ',' ){
      pbuff = result.lat;
      state = 9;
    }
    else state = 0;
    break;
  case 9:
    if( c == '.' || isdigit(c) ){
      if( has_capacity(pbuff, result.lat, 3) ) *pbuff++ = c; /* 留出当前字符，N/S和终结符 */
    }
    else if( c == ',' ){
      state = 10;
    }
    else state = 0;
    break;
  case 10:
    if( c == 'N' || c == 'S' ){
      if( has_capacity(pbuff, result.lat, 2) ) *pbuff++ = c;
    }
    else if( c == ',' ){
      if( has_capacity(pbuff, result.lat, 1) ) *pbuff = '\0';
      pbuff = result.lon;
      state = 11;
    }
    else state = 0;
    break;
  case 11:
    if( c == '.' || isdigit(c) ){
      if( has_capacity(pbuff, result.lon, 3) ) *pbuff++ = c;
    }
    else if( c == ',' ){
      state = 12;
    }
    else state = 0;
    break;
  case 12:
    if( c == 'W' || c == 'E' ){
      if( has_capacity(pbuff, result.lon, 2) ) *pbuff++ = c;
    }
    else if( c == ',' ){
      if( has_capacity(pbuff, result.lon, 1) ) *pbuff = '\0';
      pbuff = result.spd;
      state = 13;
    }
    else state = 0;
    break;
  case 13:
    if( c == '.' || isdigit(c) ){
      if( has_capacity(pbuff, result.spd, 2) ) *pbuff++ = c;
    }
    else if( c == ',' ){
      if( has_capacity(pbuff, result.spd, 1) ) *pbuff = '\0';
      pbuff = result.cmg;
      state = 14;
    }
    else state = 0;
    break;
  case 14:
    if( c == '.' || isdigit(c) ){
      if( has_capacity(pbuff, result.cmg, 2) ) *pbuff++ = c;
    }
    else if( c == ',' ){
      if( has_capacity(pbuff, result.cmg, 1) ) *pbuff = '\0';
      pbuff = result.date;
      state = 15;
    }
    else state = 0;
    break;
  case 15:
    if( isdigit(c) ){
      if( has_capacity(pbuff, result.date, 2) ) *pbuff++ = c;
    }
    else if( c == ',' ){
      if( has_capacity(pbuff, result.date, 1) ) *pbuff = '\0';
      pbuff = result.magn;
      state = 16;
    }
    else state = 0;
    break;
  case 16:
    if( c == '.' || isdigit(c) ){
      if( has_capacity(pbuff, result.magn, 3) ) *pbuff++ = c;
    }
    else if( c == ',' ){
      state = 17;
    }
    else state = 0;
    break;
  case 17:
    if( c == 'W' || c == 'E' ){
      if( has_capacity(pbuff, result.magn, 2) ) *pbuff++ = c;
    }
    else if( c == ',' ){
      if( has_capacity(pbuff, result.magn, 1) ) *pbuff = '\0';
      state = 18;
    }
    else if( c == '*' ){
      if( has_capacity(pbuff, result.magn, 1) ) *pbuff = '\0';
      state = 20;
    }
    else state = 0;
    break;
  case 18:
    if( c == 'A' || c == 'D' || c == 'E' || c == 'N' ){
      // DO SOMETHING
      state = 19;
    }
    else state = 0;
    break;
  case 19:
    if( c == '*' ){
      state = 20;
    }
    else state = 0;
    break;
  case 20:
    if( isdigit(c) ){
      msgsum = (msgsum<<4) | (c - '0');
    }
    else if( 'a' <= c && c <= 'f' ){
      msgsum = (msgsum<<4) | (c - 'a' + 0xa);
    }
    else if( 'A' <= c && c <= 'F' ){
      msgsum = (msgsum<<4) | (c - 'A' + 0xa);
    }
    else if( c == '\r' ){
      state = (msgsum == checksum) ? 21 : 0;
    }
    else state = 0;
    break;
  case 21:
    retval = &result;
  default:
    state = 0;
  }

  if( 1 < state && state < 20 )
    checksum ^= c;

  return retval;
}

static int ICACHE_FLASH_ATTR nmea_parse( lua_State *L )
{
  int found = 0;
  gprmc_data tmp;
  size_t len = 0;
  const char *s = luaL_optlstring(L, 1, "", &len);
  if( len != 0 ){
    for( const char *p = s; p != s+len; ++p ){
      gprmc_data *res = parse( *p );
      if( res && has_gps(res) ){
	memcpy( &tmp, res, sizeof(tmp) );
	found = 1;
      }
    }

    if( found ){// set return value
      lua_pushstring( L, tmp.date );
      lua_pushstring( L, tmp.time );
      lua_pushstring( L, tmp.lon );
      lua_pushstring( L, tmp.lat );
      lua_pushstring( L, tmp.spd );
      lua_pushstring( L, tmp.cmg );
      lua_pushstring( L, tmp.magn );
      return 7;
    }
    else return 0;
  }
  return 0;
}

static const LUA_REG_TYPE tinynmea_map[] = {
  { LSTRKEY("parse"), LFUNCVAL(nmea_parse) },
  { LNILKEY, LNILVAL }
};

NODEMCU_MODULE( TINYNMEA, "tinynmea", tinynmea_map, NULL );

