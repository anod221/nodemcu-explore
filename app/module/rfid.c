#include "module.h"
#include "c_stdlib.h"
#include "platform.h"
#include "lauxlib.h"
#include "rfid.h"

static const LUA_REG_TYPE rfid_map[] = {
  // for module function
  //  { LSTRKEY("setup"),  },
  //  { LSTRKEY("request"),  },
  //  { LSTRKEY("anticoll"), },
  //  { LSTRKEY("select"), },
  //  { LSTRKEY("auth"), },
  //  { LSTRKEY("read"), },
  // for module constant
  { LSTRKEY("test"), LNUMVAL(20) },
  { LNILKEY, LNILVAL }
};

NODEMCU_MODULE( RFID, "rfid", rfid_map, NULL );
