-- ssd1306

local Module = {}
local global = global or _G

-- 保护包代码上下文
local locale = {}
setfenv( 1, setmetatable({}, {
    __index = function( t, k )
        if global[k] then 
            return global[k] 
        elseif locale[k] then
            return locale[k]
        end
    end, 
    __newindex = function( t, k, v )
        locale[k] = v
    end
}) )

-- begin static variable
local oled_type = "ssd1306_128x64_i2c"
local oled_sla = 0x3c
-- end static variable

-- 公开方法
local function method( static )
    local class = {}

    -- 所有method都需要有返回值
    function class:_init()                  -- 初始化
        i2c.setup( 0, static.a, static.l, i2c.SLOW )
        static.g = u8g[oled_type](oled_sla)
        static.g:setFont( u8g.font_6x10 )
        return self
    end

    function class:disp( render )
    	local g = static.g
    	g:firstPage()
    	repeat
    		render( g )
    	until g:nextPage() == false
    	return self
    end

    function class:show( serv_uri, onfail, render )
    	if http == nil then
    		error( "no http module" )
    	end
    	http.get( serv_uri, nil, function( code, data, header )
    		if code ~= 200 then
    			if type(onfail) == "function" then
                    onfail(code)
                end
    		else
    			local function r( g )
    				g:drawBitmap(0, 0, 16, 64, data)
    				if render ~= nil then render(g) end
    			end
    			self:disp( r )
    		end
    	end )
    	return self
    end

    return class
end

-- 创建对象
local function create( pin_sda, pin_scl )
    local static, extern = {
        -- 私有变量声明
        a=pin_sda,
        l=pin_scl,
        g=nil
    }

    local o = setmetatable( -- 构造对象
        method(static), 
        {__index = extern, __newindex = extern} 
    )

    return o:_init()        -- RAII
end

-- mount the module
Module.create = create
return Module
