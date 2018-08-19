-- 光耦计数器模块

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

local pin_index = {}
Module.ACTIVE_ON_LOW = gpio.LOW
Module.ACTIVE_ON_HIGH = gpio.HIGH
Module.CIRCUIT_NORMAL_OPEN = 0
Module.CIRCUIT_NORMAL_CLOSED = 1

-- end static variable

-- 公开方法
local function method( static )
    local class = {}

    -- 所有method都需要有返回值
    function class:_init()                  -- 初始化
        gpio.mode( static.p, gpio.OUTPUT )
        return self
    end

    -- 电路闭合
    function class:turn_on()
        local l = static.a
        if static.c == Module.CIRCUIT_NORMAL_CLOSED then
            if static.a == gpio.HIGH then 
                l = gpio.LOW
            else l = gpio.HIGH end
        end
        gpio.write( static.p, l )
        
        return self
    end

    -- 电路断开
    function class:turn_off()
        local l = static.a
        if static.c == Module.CIRCUIT_NORMAL_OPEN then
            if static.a == gpio.HIGH then 
                l = gpio.LOW
            else l = gpio.HIGH end
        end
        gpio.write( static.p, l )
        
        return self
    end

    -- 查看电路是否闭合
    function class:state()
        local l, L = static.a, gpio.read(static.p)
        if static.c == Module.CIRCUIT_NORMAL_CLOSED then
            if static.a == gpio.HIGH then 
                l = gpio.LOW
            else l = gpio.HIGH end
        end
        
        if l == L then 
            return "on"
        else 
            return "off" 
        end
    end

    return class
end

-- 创建对象
local function create( pin, active_mode, circuit_type )
    if pin_index[pin] ~= nil then
        return pin_index[pin]
    end

    local static, extern = {
        -- 私有变量声明    
        p=pin,              -- gpio索引
        a=active_mode,      -- 继电器触发类型
        c=circuit_type,     -- 继电器外接电路类型
    }, {}                   -- 公开变量容器

    local o = setmetatable( -- 构造对象
        method(static), 
        {__index = extern, __newindex = extern} 
    )

    pin_index[pin] = o
    return o:_init()        -- RAII
end

-- mount the module
Module.create = create
return Module
