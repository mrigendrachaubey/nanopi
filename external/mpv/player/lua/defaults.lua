-- Compatibility shim for lua 5.2/5.3
unpack = unpack or table.unpack

-- these are used internally by lua.c
mp.UNKNOWN_TYPE.info = "this value is inserted if the C type is not supported"
mp.UNKNOWN_TYPE.type = "UNKNOWN_TYPE"

mp.ARRAY.info = "native array"
mp.ARRAY.type = "ARRAY"

mp.MAP.info = "native map"
mp.MAP.type = "MAP"

function mp.get_script_name()
    return mp.script_name
end

function mp.get_opt(key, def)
    local opts = mp.get_property_native("options/script-opts")
    local val = opts[key]
    if val == nil then
        val = def
    end
    return val
end

-- For dispatching script_binding. This is sent as:
--      script_message_to $script_name $binding_name $keystate
-- The array is indexed by $binding_name, and has functions like this as value:
--      fn($binding_name, $keystate)
local dispatch_key_bindings = {}

local message_id = 0
local function reserve_binding()
    message_id = message_id + 1
    return "__keybinding" .. tostring(message_id)
end

local function dispatch_key_binding(name, state)
    local fn = dispatch_key_bindings[name]
    if fn then
        fn(name, state)
    end
end

-- "Old", deprecated API

-- each script has its own section, so that they don't conflict
local default_section = "input_dispatch_" .. mp.script_name

-- Set the list of key bindings. These will override the user's bindings, so
-- you should use this sparingly.
-- A call to this function will remove all bindings previously set with this
-- function. For example, set_key_bindings({}) would remove all script defined
-- key bindings.
-- Note: the bindings are not active by default. Use enable_key_bindings().
--
-- list is an array of key bindings, where each entry is an array as follow:
--      {key, callback_press, callback_down, callback_up}
-- key is the key string as used in input.conf, like "ctrl+a"
--
-- callback can be a string too, in which case the following will be added like
-- an input.conf line: key .. " " .. callback
-- (And callback_down is ignored.)
function mp.set_key_bindings(list, section, flags)
    local cfg = ""
    for i = 1, #list do
        local entry = list[i]
        local key = entry[1]
        local cb = entry[2]
        local cb_down = entry[3]
        local cb_up = entry[4]
        if type(cb) ~= "string" then
            local mangle = reserve_binding()
            dispatch_key_bindings[mangle] = function(name, state)
                local event = state:sub(1, 1)
                local is_mouse = state:sub(2, 2) == "m"
                local def = (is_mouse and "u") or "d"
                if event == "r" then
                    return
                end
                if event == "p" and cb then
                    cb()
                elseif event == "d" and cb_down then
                    cb_down()
                elseif event == "u" and cb_up then
                    cb_up()
                elseif event == def and cb then
                    cb()
                end
            end
            cfg = cfg .. key .. " script_binding " ..
                  mp.script_name .. "/" .. mangle .. "\n"
        else
            cfg = cfg .. key .. " " .. cb .. "\n"
        end
    end
    mp.input_define_section(section or default_section, cfg, flags)
end

function mp.enable_key_bindings(section, flags)
    mp.input_enable_section(section or default_section, flags)
end

function mp.disable_key_bindings(section)
    mp.input_disable_section(section or default_section)
end

function mp.set_mouse_area(x0, y0, x1, y1, section)
    mp.input_set_section_mouse_area(section or default_section, x0, y0, x1, y1)
end

-- "Newer" and more convenient API

local key_bindings = {}

local function update_key_bindings()
    for i = 1, 2 do
        local section, flags
        local def = i == 1
        if def then
            section = "input_" .. mp.script_name
            flags = "default"
        else
            section = "input_forced_" .. mp.script_name
            flags = "force"
        end
        local cfg = ""
        for k, v in pairs(key_bindings) do
            if v.forced ~= def then
                cfg = cfg .. v.bind .. "\n"
            end
        end
        mp.input_define_section(section, cfg, flags)
        -- TODO: remove the section if the script is stopped
        mp.input_enable_section(section, "allow-hide-cursor|allow-vo-dragging")
    end
end

local function add_binding(attrs, key, name, fn, rp)
    rp = rp or ""
    if (type(name) ~= "string") and (not fn) then
        fn = name
        name = reserve_binding()
    end
    local bind = key
    local repeatable = rp == "repeatable" or rp["repeatable"]
    if rp["forced"] then
        attrs.forced = true
    end
    local key_cb, msg_cb
    if not fn then
        fn = function() end
    end
    if rp["complex"] then
        local key_states = {
            ["u"] = "up",
            ["d"] = "down",
            ["r"] = "repeat",
            ["p"] = "press",
        }
        key_cb = function(name, state)
            fn({
                event = key_states[state:sub(1, 1)] or "unknown",
                is_mouse = state:sub(2, 2) == "m"
            })
        end
        msg_cb = function()
            fn({event = "press", is_mouse = false})
        end
    else
        key_cb = function(name, state)
            -- Emulate the same semantics as input.c uses for most bindings:
            -- For keyboard, "down" runs the command, "up" does nothing;
            -- for mouse, "down" does nothing, "up" runs the command.
            -- Also, key repeat triggers the binding again.
            local event = state:sub(1, 1)
            local is_mouse = state:sub(2, 2) == "m"
            if event == "r" and not repeatable then
                return
            end
            if is_mouse and (event == "u" or event == "p") then
                fn()
            elseif (not is_mouse) and (event == "d" or event == "r" or event == "p") then
                fn()
            end
        end
        msg_cb = fn
    end
    attrs.bind = bind .. " script_binding " .. mp.script_name .. "/" .. name
    attrs.name = name
    key_bindings[name] = attrs
    update_key_bindings()
    dispatch_key_bindings[name] = key_cb
    mp.register_script_message(name, msg_cb)
end

function mp.add_key_binding(...)
    add_binding({forced=false}, ...)
end

function mp.add_forced_key_binding(...)
    add_binding({forced=true}, ...)
end

function mp.remove_key_binding(name)
    key_bindings[name] = nil
    dispatch_key_bindings[name] = nil
    update_key_bindings()
    mp.unregister_script_message(name)
end

local timers = {}

local timer_mt = {}
timer_mt.__index = timer_mt

function mp.add_timeout(seconds, cb)
    local t = mp.add_periodic_timer(seconds, cb)
    t.oneshot = true
    return t
end

function mp.add_periodic_timer(seconds, cb)
    local t = {
        timeout = seconds,
        cb = cb,
        oneshot = false,
    }
    setmetatable(t, timer_mt)
    t:resume()
    return t
end

function timer_mt.stop(t)
    if timers[t] then
        timers[t] = nil
        t.next_deadline = t.next_deadline - mp.get_time()
    end
end

function timer_mt.kill(t)
    timers[t] = nil
    t.next_deadline = nil
end
mp.cancel_timer = timer_mt.kill

function timer_mt.resume(t)
    if not timers[t] then
        local timeout = t.next_deadline
        if timeout == nil then
            timeout = t.timeout
        end
        t.next_deadline = mp.get_time() + timeout
        timers[t] = t
    end
end

-- Return the timer that expires next.
local function get_next_timer()
    local best = nil
    for t, _ in pairs(timers) do
        if (best == nil) or (t.next_deadline < best.next_deadline) then
            best = t
        end
    end
    return best
end

function mp.get_next_timeout()
    local timer = get_next_timer()
    if not timer then
        return
    end
    local now = mp.get_time()
    return timer.next_deadline - now
end

-- Run timers that have met their deadline.
-- Return: next absolute time a timer expires as number, or nil if no timers
local function process_timers()
    while true do
        local timer = get_next_timer()
        if not timer then
            return
        end
        local now = mp.get_time()
        local wait = timer.next_deadline - now
        if wait > 0 then
            return wait
        else
            if timer.oneshot then
                timer:kill()
            else
                timer.next_deadline = now + timer.timeout
            end
            timer.cb()
        end
    end
end

local messages = {}

function mp.register_script_message(name, fn)
    messages[name] = fn
end

function mp.unregister_script_message(name)
    messages[name] = nil
end

local function message_dispatch(ev)
    if #ev.args > 0 then
        local handler = messages[ev.args[1]]
        if handler then
            handler(unpack(ev.args, 2))
        end
    end
end

local property_id = 0
local properties = {}

function mp.observe_property(name, t, cb)
    local id = property_id + 1
    property_id = id
    properties[id] = cb
    mp.raw_observe_property(id, name, t)
end

function mp.unobserve_property(cb)
    for prop_id, prop_cb in pairs(properties) do
        if cb == prop_cb then
            properties[prop_id] = nil
        end
    end
end

local function property_change(ev)
    local prop = properties[ev.id]
    if prop then
        prop(ev.name, ev.data)
    end
end

-- used by default event loop (mp_event_loop()) to decide when to quit
mp.keep_running = true

local event_handlers = {}

function mp.register_event(name, cb)
    local list = event_handlers[name]
    if not list then
        list = {}
        event_handlers[name] = list
    end
    list[#list + 1] = cb
    return mp.request_event(name, true)
end

function mp.unregister_event(cb)
    for name, sub in pairs(event_handlers) do
        local found = false
        for i, e in ipairs(sub) do
            if e == cb then
                found = true
                break
            end
        end
        if found then
            -- create a new array, just in case this function was called
            -- from an event handler
            local new = {}
            for i = 1, #sub do
                if sub[i] ~= cb then
                    new[#new + 1] = sub[i]
                end
            end
            event_handlers[name] = new
            if #new == 0 then
                mp.request_event(name, false)
            end
        end
    end
end

-- default handlers
mp.register_event("shutdown", function() mp.keep_running = false end)
mp.register_event("client-message", message_dispatch)
mp.register_event("property-change", property_change)

-- sent by "script_binding"
mp.register_script_message("key-binding", dispatch_key_binding)

mp.msg = {
    log = mp.log,
    fatal = function(...) return mp.log("fatal", ...) end,
    error = function(...) return mp.log("error", ...) end,
    warn = function(...) return mp.log("warn", ...) end,
    info = function(...) return mp.log("info", ...) end,
    verbose = function(...) return mp.log("v", ...) end,
    debug = function(...) return mp.log("debug", ...) end,
}

_G.print = mp.msg.info

package.loaded["mp"] = mp
package.loaded["mp.msg"] = mp.msg

_G.mp_event_loop = function()
    mp.dispatch_events(true)
end

local function call_event_handlers(e)
    local handlers = event_handlers[e.event]
    if handlers then
        for _, handler in ipairs(handlers) do
            handler(e)
        end
    end
end

mp.use_suspend = true

function mp.dispatch_events(allow_wait)
    local more_events = true
    if mp.use_suspend then
        mp.suspend()
    end
    while mp.keep_running do
        local wait = process_timers()
        if wait == nil then
            wait = 1e20 -- infinity for all practical purposes
        end
        if more_events or wait < 0 then
            wait = 0
        end
        -- Resume playloop - important especially if an error happened while
        -- suspended, and the error was handled, but no resume was done.
        if wait > 0 then
            mp.resume_all()
            if allow_wait ~= true then
                return
            end
        end
        local e = mp.wait_event(wait)
        -- Empty the event queue while suspended; otherwise, each
        -- event will keep us waiting until the core suspends again.
        if mp.use_suspend then
            mp.suspend()
        end
        more_events = (e.event ~= "none")
        if more_events then
            call_event_handlers(e)
        end
    end
end

-- additional helpers

function mp.osd_message(text, duration)
    if not duration then
        duration = "-1"
    else
        duration = tostring(math.floor(duration * 1000))
    end
    mp.commandv("show_text", text, duration)
end

local hook_table = {}
local hook_registered = false

local function hook_run(id, cont)
    local fn = hook_table[tonumber(id)]
    if fn then
        fn()
    end
    mp.commandv("hook_ack", cont)
end

function mp.add_hook(name, pri, cb)
    if not hook_registered then
        mp.register_script_message("hook_run", hook_run)
        hook_registered = true
    end
    local id = #hook_table + 1
    hook_table[id] = cb
    mp.commandv("hook_add", name, id, pri)
end

local mp_utils = package.loaded["mp.utils"]

function mp_utils.format_table(t, set)
    if not set then
        set = { [t] = true }
    end
    local res = "{"
    -- pretty expensive but simple way to distinguish array and map parts of t
    local keys = {}
    local vals = {}
    local arr = 0
    for i = 1, #t do
        if t[i] == nil then
            break
        end
        keys[i] = i
        vals[i] = t[i]
        arr = i
    end
    for k, v in pairs(t) do
        if not (type(k) == "number" and k >= 1 and k <= arr and keys[k]) then
            keys[#keys + 1] = k
            vals[#keys] = v
        end
    end
    for i = 1, #keys do
        if #res > 1 then
            res = res .. ", "
        end
        if i > arr then
            res = res .. mp_utils.to_string(keys[i], set) .. " = "
        end
        res = res .. mp_utils.to_string(vals[i], set)
    end
    res = res .. "}"
    return res
end

function mp_utils.to_string(v, set)
    if type(v) == "string" then
        return "\"" .. v .. "\""
    elseif type(v) == "table" then
        if set then
            if set[v] then
                return "[cycle]"
            end
            set[v] = true
        end
        return mp_utils.format_table(v, set)
    else
        return tostring(v)
    end
end

function mp_utils.getcwd()
    return mp.get_property("working-directory")
end

return {}
