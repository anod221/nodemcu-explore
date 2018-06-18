-- 光耦计数器模块

local modname = "Counter"
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

pin_index = {}
one_second = 1000 * 1000

-- end static variable

-- 公开方法
local function method( static )
    local class = {}

    -- 所有method都需要有返回值
    function class:_init()                  -- 初始化
        gpio.mode( static.p, gpio.INPUT )
        gpio.trig(
            static.p, 
            "down",
            function( level, when )
                if level == 1 then
                    self:_incr( when )
                    if static.cb then
                        static.cb(when)
                    end -- end of if static.cb
                end -- end of if level
            end -- end of function
        )
        return self
    end

    function class:_incr( time )
        if static.r + one_second < time then
            static.c = static.n
            static.r = time - time % one_second
        end
        static.n = static.n + 1
        return self
    end

    function class:reset()
        static.n = 0
        static.r = 0
        static.c = 0
        return self
    end

    function class:callback( f )
        static.cb = f
    end

    function class:count()
        return static.n
    end

    function class:time()
        return tmr.now() - static.t
    end

    function class:speed()
        local now = tmr.now()
        now = now - now % one_second
        if static.r + one_second < now then
            return 0
        else
            return static.n - static.c
        end
    end

    function class:speedavg()
        return static.n / ( tmr.now() - static.t )
    end

    return class
end

-- 创建对象
local function create( pin, callback )
    if pin_index[pin] ~= nil then
        return pin_index[pin]
    end

    local static, extern = {
        -- 私有变量声明    
        p=pin,              -- gpio索引
        cb=callback,        -- 回掉函数
        t=tmr.now(),        -- 时间
        n=0,                -- 计数
        r=0,                -- 速度时间
        c=0,                -- 速度数量
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
global[modname] = Module
return Module
