-- init.lua

local mqtt_cfg = {
   ip = "172.20.10.11",
   port = 61613,
   user = "nodemcu",
   pwd = "esp8266",
   retry = 3
}
local wifi_cfg = {
   ssid = "anophone",
   pwd = "11111111"
}
local oled_cfg = {
   i2c = {
      sda = 5,
      scl = 6,
      sla = 0x3c
   },
   chip = "ssd1306_128x64",
   line = 4
}

local oled_dev

local function setup_oled()
	local u8g_driver
	if oled_cfg.i2c then
		i2c.setup( 0, oled_cfg.i2c.sda, oled_cfg.i2c.scl, i2c.SLOW )
		u8g_driver = string.format("%s_i2c", oled_cfg.chip)
		oled_dev = u8g[u8g_driver]( oled_cfg.i2c.sla )
	else
		-- TODO
		_G = _G
	end

	oled_dev:setFont( u8g.font_6x10 )
end

local function oled( line, str, ... )
	if oled_dev == nil then return end

	oled_dev:firstPage()
	repeat
		if type(line) == "table" then
			for l, s in pairs( line ) do
				str = string.format( unpack(s) )
				oled_dev:drawStr( 0, l*16-3, str )
			end
		else
			str = string.format( str, ... )
			oled_dev:drawStr( 0, line*16 - 3, str )
		end
	until oled_dev:nextPage() == false
end

local function debug( str, ... )
	return oled(1, str, ...)
end

local function setup_uart( handler )
	uart.setup( 0, 9600, 8, 0, 1, 0 )
	uart.alt(1)  -- use d7 to receive data
	uart.on( "data", 128, handler, 0 )
end

local function setup_wifi( next )
	local nconn = 1
	local function conn()
		local ip = wifi.sta.getip()
		if ip == nil then
			debug("wifi config<%d>", nconn)

			ip = wifi.sta.getip()
			if ip == nil then
				nconn = nconn+1
				tmr.alarm( 0, 7500, tmr.ALARM_SINGLE, conn )
			else
				debug("ip %s", ip)
				next()
			end
		else
			debug("ip %s", ip)
			next()
		end
	end

	wifi.setmode( wifi.STATION );
	wifi.sta.config( wifi_cfg )
	conn()
end

function main()
	local cli = mqtt.Client( "gps-receiver", 300, mqtt_cfg.user, mqtt_cfg.pwd )

	local function on_gps( data )
		cli:publish( "gps", data, 0, 0 )
	end

	local function on_nmea( c, t, data )
		local nmea = sjson.decode( data )
		oled( {[2]={"LA:%s", nmea.loc.dmm.latitude}, [3]={"LO:%s", nmea.loc.dmm.longitude}} );
	end

	local on_init, on_error
	local nretry = 1
	local function setup_mqtt()
		debug("connecting<%d>", nretry )
		cli:connect( mqtt_cfg.ip, mqtt_cfg.port, 0, on_init, on_error )
	end

	on_init = function()
		debug( "mqtt connected" )
		cli:publish( "gps", "mqtt connect", 0, 0 )

		cli:subscribe( "nmea", 0 );
		cli:on( "message", on_nmea )

		setup_uart( on_gps )
	end

	on_error = function( c, err )
		debug("mqtt failed:%d", err )
		if nretry <= mqtt_cfg.retry then
		 nretry = nretry + 1
		 tmr.alarm( 0, 5000, tmr.ALARM_SINGLE, setup_mqtt )
		end
	end

	setup_mqtt()
end

function start()
   setup_oled()
   setup_wifi( main )
end

start()