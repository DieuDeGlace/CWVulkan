-- Example CubeVulkan Lua mod (mod API version 2): the aquatic encounters add-on as a script.
-- Copy this folder into the engine's mods/ directory to run it. It follows the same rules as the
-- built-in add-on (src/AquaticPlan.cpp), so use it only with the built-in one switched off
-- ([Creatures] AquaticEnemies=0), or both will make schools.

mod = { name = "AquaticLua", version = "1", changes_world = true }

local M32 = 0xffffffff
local FISH_FLAG, BOSS_FLAG = 0x100, 0x40000
local NEUTRAL, HOSTILE = 4, 1
local OCEAN, WATER = 1, 2
local SEA_CRAB, CRAB, SAPPHIRE_FISH, LEMON_FISH, SEAHORSE, MERMAN, SHARK = 107, 106, 145, 146, 147, 149, 150
local LANTERN_FISH, MAW_FISH, PIRANHA, BLOWFISH, SNAIL, CROCODILE, SPITTER = 152, 153, 154, 155, 295, 75, 86
local GREEN_SLIME, BLUE_SLIME = 37, 40
local SEA_RACES = { SEA_CRAB, SAPPHIRE_FISH, LEMON_FISH, SEAHORSE, MERMAN, SHARK, LANTERN_FISH, MAW_FISH, PIRANHA, BLOWFISH, SNAIL }
local INLAND_RACES = { CRAB, SAPPHIRE_FISH, LEMON_FISH, SEAHORSE, SHARK, LANTERN_FISH, MAW_FISH, PIRANHA, BLOWFISH, SNAIL }
local BOSS_RACES = { CROCODILE, SHARK, MAW_FISH, PIRANHA, SEA_CRAB }
local OFFSETS = { { 0, 0 }, { 3, 1 }, { -3, -1 }, { 1, -3 }, { -1, 3 }, { 4, -3 }, { -4, 3 } }

-- 32-bit unsigned arithmetic on Lua's 64-bit integers.
local function water_hash(x, y, seed)
    local h = (((x & M32) * 0x9e3779b9) & M32) ~ (((y & M32) * 0x85ebca6b) & M32) ~ (seed & M32)
    h = ((h ~ (h >> 16)) * 0x7feb352d) & M32
    return h ~ (h >> 15)
end
local function modulo(a, n) return ((a % n) + n) % n end
local function passive(race)
    return race == SAPPHIRE_FISH or race == LEMON_FISH or race == SEAHORSE or race == LANTERN_FISH or race == BLOWFISH or race == SNAIL
end
local function bottom_dweller(race)
    return race == CRAB or race == SEA_CRAB or race == SPITTER or (race >= GREEN_SLIME and race <= BLUE_SLIME)
end

-- The water at a column: ok, floor (first water block), surface (first block above the water).
local function column(world, x, y)
    local ground = math.floor(world.ground(x, y))
    local top = math.min(ground + 40, 400)
    local lowest = math.max(ground - 60, -200)
    while top > lowest do
        local kind, loaded = world.block_type(x, y, top)
        if not loaded then return false end
        if kind == WATER then break end
        if kind ~= 0 then return false end
        top = top - 1
    end
    local bottom = top
    while bottom > lowest and world.block_type(x, y, bottom) == WATER do bottom = bottom - 1 end
    local floor, surface = bottom + 1, top + 1
    return surface - floor >= 2, floor, surface
end

local function plan(world, zx, zy, seed)
    local roll = water_hash(zx, zy, seed)
    local start_x, start_y = zx * 64, zy * 64
    for probe = 0, 15 do
        local slot = (probe + roll % 16) % 16
        local x, y = start_x + 8 + (slot % 4) * 16, start_y + 8 + (slot // 4) * 16
        local ok, floor, surface = column(world, x, y)
        if ok then
            local depth = surface - floor
            local sea = depth >= 6 and world.biome(x, y) == OCEAN
            local skip
            if sea then skip = modulo(zx + zy, 2) ~= 0 else skip = modulo(zx + zy + math.fmod(seed, 3), 3) ~= 0 end
            if not skip then
                local school = { boss = (roll >> 16) % 4 == 0 and depth >= 6, members = {} }
                school.race = sea and SEA_RACES[(roll >> 8) % #SEA_RACES + 1] or INLAND_RACES[(roll >> 8) % #INLAND_RACES + 1]
                if school.boss then
                    school.race = BOSS_RACES[(roll >> 4) % (sea and 5 or 4) + 1]
                elseif not sea and (roll >> 20) % 3 == 0 then -- a lake's own kind
                    local lake = water_hash(x // 128, y // 128, seed)
                    school.race = lake % 3 == 0 and CRAB or lake % 3 == 1 and SPITTER or GREEN_SLIME + (lake // 3) % 4
                end
                school.hostility = passive(school.race) and NEUTRAL or HOSTILE
                school.stars = school.boss and 4 or (roll >> 11) % 3
                local wanted = school.boss and 1 or 2 + (roll >> 5) % 2
                for _, offset in ipairs(OFFSETS) do
                    local px, py = x + offset[1], y + offset[2]
                    local fine, low, high = column(world, px, py)
                    if fine and high - low >= 2 then
                        local column_depth = high - low
                        local below = 2 + roll % 3
                        if sea and column_depth >= 8 and (roll >> 4) % 2 ~= 0 then
                            below = column_depth // 4 + (roll >> 12) % (column_depth - column_depth // 4 - 1)
                        end
                        local z
                        if bottom_dweller(school.race) then
                            z = low + 0.6
                        else
                            z = high - below
                            if z < low + 0.6 then z = low + 0.6 elseif high - 0.6 < z then z = high - 0.6 end
                        end
                        school.members[#school.members + 1] = { px + 0.5, py + 0.5, z + 0.0 }
                        if #school.members >= wanted then break end
                    end
                end
                if #school.members >= (school.boss and 1 or 2) then return school end
            end
        end
    end
    return nil
end

local done, since_update = {}, 0

function on_world_step(world, milliseconds)
    since_update = since_update + milliseconds
    if since_update < 1000 then return end
    since_update = 0
    local made_total = 0
    for _, player in ipairs(world.players()) do
        local pzx, pzy = math.floor(player[1]) // 64, math.floor(player[2]) // 64
        for zx = pzx - 2, pzx + 2 do
            for zy = pzy - 2, pzy + 2 do
                local key = zx .. "," .. zy
                if not done[key] and world.zone_loaded(zx, zy) then
                    local school = plan(world, zx, zy, world.seed)
                    if not school then
                        done[key] = true
                    else
                        local made = false
                        for _, at in ipairs(school.members) do
                            local id = world.spawn_creature(school.race, school.hostility, school.stars, at[1], at[2], at[3])
                            if id ~= 0 then
                                made = true
                                made_total = made_total + 1
                                world.add_flags(id, FISH_FLAG | (school.boss and BOSS_FLAG or 0))
                            end
                        end
                        if made then done[key] = true end
                    end
                end
            end
        end
    end
    if made_total > 0 then cwv.log("made " .. made_total .. " sea creatures") end
end

cwv.log("Aquatic encounters running as a Lua mod")
