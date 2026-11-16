local flowdef = {}

-- This library follows net.minecraft.world.level.material.FlowingFluid.
-- A cell stores the vanilla amount in "level" and keeps "source" apart, so a
-- non-source cell with level == max_level is the falling state. A block that
-- can hold a fluid declares a string state (flowdef.filled_state) whose value
-- is the fluid name, and a variant that binds the fluid through fluid_name.

-- A sentinel value used to indicate if
-- a slope search has failed to find a drop
local SLOPE_INF <const> = 1000

-- Horizontal offsets for the four cardinal directions. The last field is the
-- face of the neighbour that a fluid would enter through.
local HORIZONTAL <const> = {
  { 1, 0, 0, blocks.FACE_WEST },
  { -1, 0, 0, blocks.FACE_EAST },
  { 0, 0, 1, blocks.FACE_NORTH },
  { 0, 0, -1, blocks.FACE_SOUTH },
}

-- The blockstate that marks a cell as filled with a fluid. Its values are
-- fluid names, eg "water"; "none" means the cell is empty. Change this before
-- any fillable block is registered.
flowdef.filled_state = "fluid"

-- Flow configs registered during mod load, keyed by fluid id after commit.
local registered = {}
local flows = {}
local resolved = false

local function default_config(config_src)
  config_src = config_src or {}

  local result = {}
  result.fluid = config_src.fluid
  result.full_height = config_src.full_height or 14
  result.max_level = config_src.max_level or 8
  result.level_decrease = config_src.level_decrease or 1
  result.delay = config_src.delay or 5
  result.infinite = config_src.infinite ~= false
  result.slope_range = config_src.slope_range or 4

  return result
end

local function resolve_flows()
  if resolved then
    return
  end

  resolved = true

  for _, config in ipairs(registered) do
    local fluid_id = fluids.get(config.fluid)
    local block_id = blocks.get(config.block_name)

    if fluid_id ~= 0 and block_id ~= 0 then
      config.fluid_id = fluid_id
      config.block_id = block_id
      flows[fluid_id] = config
    end
  end
end

function flowdef.register(block_name, config_src)
  local config = default_config(config_src)
  config.block_name = block_name
  config.fluid = config_src.fluid or block_name

  table.insert(registered, config)
  resolved = false
end

local function read_cell(config, bx, by, bz)
  if world.get_fluid(bx, by, bz) ~= config.fluid_id then
    return nil
  end

  local level_state = world.get_state(bx, by, bz, "level")

  if not level_state then
    -- A filled block always holds a full source.
    return {
      level = config.max_level,
      source = true,
    }
  end

  return {
    level = tonumber(level_state) or 0,
    source = world.get_state(bx, by, bz, "source") == "true",
  }
end

local function amount_of(cell, config)
  if not cell then
    return 0
  end

  if cell.source then
    return config.max_level
  end

  return cell.level
end

local function is_falling(cell, config)
  return cell and not cell.source and cell.level >= config.max_level
end

-- True when the cell can take this fluid through the given face right now.
local function can_fill_cell(config, bx, by, bz, face)
  if world.get_fluid(bx, by, bz) ~= 0 then
    return false
  end

  if world.get_state_fluid(bx, by, bz, flowdef.filled_state, config.fluid) ~= config.fluid_id then
    return false
  end

  return world.get_fluid_faces(bx, by, bz) & (1 << face) ~= 0
end

-- Vanilla's canMaybePassThrough: the fluid may enter a cell that can hold it,
-- but never a cell that already holds a source of the same fluid.
local function can_reach_cell(config, bx, by, bz, face)
  local cell = read_cell(config, bx, by, bz)

  if cell then
    return not cell.source
  end

  return blocks.is_replaceable(world.get_block(bx, by, bz)) or can_fill_cell(config, bx, by, bz, face)
end

-- Vanilla's canBeReplacedWith: a cell that already holds fluid keeps its own
-- level and is never overwritten sideways or from above.
local function can_replace_cell(config, bx, by, bz, face)
  if world.get_fluid(bx, by, bz) ~= 0 then
    return false
  end

  return blocks.is_replaceable(world.get_block(bx, by, bz)) or can_fill_cell(config, bx, by, bz, face)
end

-- Vanilla's isWaterHole: the cell can drop into the one below.
local function is_fluid_hole(config, bx, by, bz)
  if not blocks.is_replaceable(world.get_block(bx, by, bz)) then
    -- A solid cell can leave only through its bottom face.
    if not can_fill_cell(config, bx, by, bz, blocks.FACE_BOTTOM) then
      return false
    end
  end

  if read_cell(config, bx, by - 1, bz) then
    return true
  end

  return blocks.is_replaceable(world.get_block(bx, by - 1, bz)) or can_fill_cell(config, bx, by - 1, bz, blocks.FACE_TOP)
end

local function source_neighbour_count(config, bx, by, bz)
  local count = 0

  for _, dir in ipairs(HORIZONTAL) do
    local neighbour = read_cell(config, bx + dir[1], by, bz + dir[3])

    if neighbour and neighbour.source then
      count = count + 1
    end
  end

  return count
end

local function is_solid(bx, by, bz)
  local id = world.get_block(bx, by, bz)

  if id == blocks.NULL_BLOCK or world.get_fluid(bx, by, bz) ~= 0 then
    return false
  end

  return not blocks.is_replaceable(id)
end

-- Vanilla's getNewLiquid: the state a cell settles into on its own.
local function new_cell(config, bx, by, bz)
  local highest = 0
  local sources = 0

  for _, dir in ipairs(HORIZONTAL) do
    local neighbour = read_cell(config, bx + dir[1], by, bz + dir[3])

    if neighbour then
      if neighbour.source then
        sources = sources + 1
      end

      if neighbour.level > highest then
        highest = neighbour.level
      end
    end
  end

  if config.infinite and sources >= 2 then
    local below = read_cell(config, bx, by - 1, bz)

    if (below and below.source) or is_solid(bx, by - 1, bz) then
      return {
        level = config.max_level,
        source = true,
      }
    end
  end

  if read_cell(config, bx, by + 1, bz) then
    return {
      level = config.max_level,
      source = false,
    }
  end

  local level = highest - config.level_decrease

  if level < 1 then
    return nil
  end

  return {
    level = level,
    source = false,
  }
end

local function write_cell(config, bx, by, bz, cell, face)
  local existing = read_cell(config, bx, by, bz)

  if existing then
    local matched = existing.level == cell.level and existing.source == cell.source

    world.set_state(bx, by, bz, "level", tostring(cell.level))
    world.set_state(bx, by, bz, "source", cell.source and "true" or "false")

    return not matched
  end

  if can_fill_cell(config, bx, by, bz, face) then
    world.set_state(bx, by, bz, flowdef.filled_state, config.fluid)
    return true
  end

  if not blocks.is_replaceable(world.get_block(bx, by, bz)) then
    return false
  end

  world.set_block(bx, by, bz, config.block_id)
  world.set_state(bx, by, bz, "level", tostring(cell.level))
  world.set_state(bx, by, bz, "source", cell.source and "true" or "false")

  return true
end

-- Vanilla's getSlopeDistance: how far a corridor runs before it finds a drop.
local function slope_distance(config, bx, by, bz, pass, from_dx, from_dz)
  local lowest = SLOPE_INF

  for _, dir in ipairs(HORIZONTAL) do
    if dir[1] ~= from_dx or dir[3] ~= from_dz then
      local nx = bx + dir[1]
      local nz = bz + dir[3]

      if can_reach_cell(config, nx, by, nz, dir[4]) then
        if is_fluid_hole(config, nx, by, nz) then
          return pass
        end

        if pass < config.slope_range then
          local distance = slope_distance(config, nx, by, nz, pass + 1, -dir[1], -dir[3])

          if distance < lowest then
            lowest = distance
          end
        end
      end
    end
  end

  return lowest
end

-- Vanilla's getSpread: the directions that lead to the nearest drop, each with
-- the state the target cell computes for itself.
local function spread_directions(config, bx, by, bz)
  local lowest = SLOPE_INF
  local spreads = {}

  for _, dir in ipairs(HORIZONTAL) do
    local nx = bx + dir[1]
    local nz = bz + dir[3]

    if can_reach_cell(config, nx, by, nz, dir[4]) then
      local cell = new_cell(config, nx, by, nz)

      if cell then
        local distance

        if is_fluid_hole(config, nx, by, nz) then
          distance = 0
        else
          distance = slope_distance(config, nx, by, nz, 1, -dir[1], -dir[3])
        end

        if distance < lowest then
          spreads = {}
          lowest = distance
        end

        if distance <= lowest and can_replace_cell(config, nx, by, nz, dir[4]) then
          table.insert(spreads, {
            dx = dir[1],
            dz = dir[3],
            face = dir[4],
            cell = cell,
          })
        end
      end
    end
  end

  return spreads
end

local function spread_sides(config, bx, by, bz, cell)
  local spread = amount_of(cell, config) - config.level_decrease

  if is_falling(cell, config) then
    spread = config.max_level - config.level_decrease
  end

  if spread < 1 then
    return
  end

  for _, entry in ipairs(spread_directions(config, bx, by, bz)) do
    local nx = bx + entry.dx
    local nz = bz + entry.dz

    if write_cell(config, nx, by, nz, entry.cell, entry.face) and world.get_state(nx, by, nz, "level") then
      world.schedule(nx, by, nz, config.delay)
    end
  end
end

-- Vanilla's spread: always fall first, and only fan out at the bottom or at
-- the top of a waterfall fed by three or more sources.
local function spread(config, bx, by, bz, cell)
  if can_reach_cell(config, bx, by - 1, bz, blocks.FACE_TOP) then
    local below = new_cell(config, bx, by - 1, bz)

    if below and can_replace_cell(config, bx, by - 1, bz, blocks.FACE_TOP) then
      if write_cell(config, bx, by - 1, bz, below, blocks.FACE_TOP) and world.get_state(bx, by - 1, bz, "level") then
        world.schedule(bx, by - 1, bz, config.delay)
      end

      if source_neighbour_count(config, bx, by, bz) >= 3 then
        spread_sides(config, bx, by, bz, cell)
      end

      return
    end
  end

  if cell.source or not is_fluid_hole(config, bx, by, bz) then
    spread_sides(config, bx, by, bz, cell)
  end
end

local function update_fluid(config, bx, by, bz)
  local cell = read_cell(config, bx, by, bz)

  if not cell then
    return
  end

  if not cell.source then
    local new = new_cell(config, bx, by, bz)

    if not new then
      world.set_block(bx, by, bz, blocks.NULL_BLOCK)
      return
    end

    if new.level ~= cell.level or new.source ~= cell.source then
      if write_cell(config, bx, by, bz, new, blocks.FACE_TOP) then
        world.schedule(bx, by, bz, config.delay)
      end

      cell = new
    end
  end

  spread(config, bx, by, bz, cell)
end

-- The scheduled tick shared by fluid blocks and fillable blocks.
function flowdef.tick(bx, by, bz, source)
  -- Only a placed fluid block ticks. A filled block is a static source, like
  -- a waterlogged block in vanilla.
  if not world.get_state(bx, by, bz, "level") then
    return
  end

  resolve_flows()

  local config = flows[world.get_fluid(bx, by, bz)]

  if not config then
    return
  end

  if source == blocks.TICK_NEIGHBOUR then
    world.schedule(bx, by, bz, config.delay)
    return
  end

  if source ~= blocks.TICK_SCRIPTED and source ~= blocks.TICK_RANDOM then
    return
  end

  update_fluid(config, bx, by, bz)
end

-- Places a source of the given fluid, eg when a filled block is broken.
function flowdef.place_source(fluid_id, bx, by, bz)
  resolve_flows()

  local config = flows[fluid_id]

  if not config then
    return false
  end

  world.set_block(bx, by, bz, config.block_id)
  world.set_state(bx, by, bz, "level", tostring(config.max_level))
  world.set_state(bx, by, bz, "source", "true")
  world.schedule(bx, by, bz, config.delay)

  return true
end

function flowdef.states(config_src)
  local config = default_config(config_src)
  local level_hints = {}

  for level = 1, config.max_level do
    table.insert(level_hints, tostring(level))
  end

  return {
    level = {
      default = tostring(config.max_level),
      hint = level_hints,
    },
    source = {
      default = "false",
      hint = { "true", "false" },
    },
  }
end

function flowdef.variants(config_src)
  local config = default_config(config_src)

  local variants = {
    {
      when = { source = "true" },
      overrides = {
        fluid_level = config.full_height,
      },
    },
    {
      when = { source = "false", level = tostring(config.max_level) },
      overrides = {
        fluid_level = config.full_height,
      },
    }
  }

  for level = 1, config.max_level - 1 do
    local height = math.max(1, math.floor(level * (config.full_height - 1) / (config.max_level - 1)))

    table.insert(variants, {
      when = { source = "false", level = tostring(level) },
      overrides = {
        fluid_level = height,
      },
    })
  end

  return variants
end

function flowdef.on_place(config_src)
  local config = default_config(config_src)

  return function(bx, by, bz, target, occupant, actor)
    if not config.fluid_id then
      config.fluid_id = 0
      if config.fluid then
        config.fluid_id = fluids.get(config.fluid)
      end
    end

    -- Using a fluid on a fillable block fills that block instead of placing a
    -- fluid block next to it, like a bucket does.
    local can_fill = config.fluid_id ~= 0
    can_fill = can_fill and world.get_fluid(target.bx, target.by, target.bz) == 0
    can_fill = can_fill and world.get_state_fluid(target.bx, target.by, target.bz, flowdef.filled_state, config.fluid) == config.fluid_id

    if can_fill then
      world.set_state(target.bx, target.by, target.bz, flowdef.filled_state, config.fluid)
      return nil
    end

    if not occupant.replaceable then
      return nil
    end

    world.schedule(bx, by, bz, config.delay)

    return {
      level = tostring(config.max_level),
      source = "true",
    }
  end
end

return flowdef
