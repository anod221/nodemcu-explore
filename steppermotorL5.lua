-- 5线4相步进电机模块

local modname = "StepperMotorL5"
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

Module.S4 = 1           -- 单4拍
Module.D4 = 2           -- 双4拍
Module.H8 = 3           -- 8拍，h指的是hybrid

Module.CCW = 1          -- 方向，逆时针
Module.CW = 2           -- 方向，顺时针

StepPerDegree = 2048/360

DefSpeed = 10

L, H = gpio.LOW, gpio.HIGH

PotentialS4 = {
    {L,H,H,H},
    {H,L,H,H},
    {H,H,L,H},
    {H,H,H,L},
}

PotentialD4 = {
    {L,L,H,H},
    {H,L,L,H},
    {H,H,L,L},
    {L,H,H,L},
}

PotentialH8 = {
    {L,H,H,H},
    {L,L,H,H},
    {H,L,H,H},
    {H,L,L,H},
    {H,H,L,H},
    {H,H,L,L},
    {H,H,H,L},
    {L,H,H,L},
};

Potential = { PotentialS4, PotentialD4, PotentialH8 }

-- end static variable

-- 公开方法
local function method( static )
    local class = {}

    local function _do_sig( potentialA, potentialB, potentialC, potentialD )
        gpio.write( static.A, potentialA )
        gpio.write( static.B, potentialB )
        gpio.write( static.C, potentialC )
        gpio.write( static.D, potentialD )
    end
    
    local function _wait()
        _do_sig( H,H,H,H )
    end

    local function _do_step()
        local param = static.r
        local t = static.t
        if param == nil then
            t:stop()
            _wait()
            return
        end

        local potential_table = Potential[ static.m ]
        local s = param.step
        if param.dir == Module.CW then
            s = s * -1
        end
        s = s % #potential_table
        local sig = potential_table[s]
        if sig == nil then 
            sig = potential_table[ #potential_table ]
        end
        _do_sig( unpack(sig) )

        if param.step <= param.loop then
            param.step = param.step + 1
        else
            if static.cb then
                pcall( static.cb )
            end
            static.r = nil
        end
    end

    -- 所有method都需要有返回值
    function class:_init()                  -- 初始化
        gpio.mode( static.A, gpio.OUTPUT, gpio.PULLUP )
        gpio.mode( static.B, gpio.OUTPUT, gpio.PULLUP )
        gpio.mode( static.C, gpio.OUTPUT, gpio.PULLUP )
        gpio.mode( static.D, gpio.OUTPUT, gpio.PULLUP )
        _wait()

        static.t = tmr.create()
        static.t:register( static.s, tmr.ALARM_AUTO, _do_step )
        return self
    end

    function class:step( direct, count, cb ) -- 步进
        if direct ~= Module.CW and direct ~= Module.CCW then
            error( "invalid parameter direct" )
        end
        
        if static.r then return false end
        static.r = { dir=direct, loop=count, cb=cb, step=1 }
        static.t:start()
        
        _do_step()
        return true
    end

    function class:degrotate( direct, degree, cb )
        local count = math.floor( degree * StepPerDegree )
        return self:step( direct, count, cb )
    end

    function class:destroy()
        local t = static.t
        if static.r then
            t:stop()
            _wait()
            static.r = nil
        end

        t:unregister()
        static.t = nil
        static.m = nil
    end

    return class
end

-- 创建对象
local function create( coilA, coilB, coilC, coilD, mode, delay )
    mode = mode or Module.H8
    delay = delay or DefSpeed

    local static, extern = {
        -- 私有变量声明
        A=coilA,            -- 线圈A
        B=coilB,            -- 线圈B
        C=coilC,            -- 线圈C
        D=coilD,            -- 线圈D
        m=mode,             -- 模式
        s=delay,            -- 速度
    }, {}

    local o = setmetatable( -- 构造对象
        method(static), 
        {__index = extern, __newindex = extern} 
    )

    return o:_init()        -- RAII
end

-- mount the module
Module.create = create
global[modname] = Module
return Module
