local flowdef = require("builtin:flowdef")

local slabdef = {}

local ORIENTATIONS <const> = { "top", "bottom", "double" }
local EMPTY <const> = "none"

-- A bottom slab fills the lower half of its cell, so the cell boundary above it
-- is open; a top slab is the other way around. Both expose their side faces
local FLUID_SIDES <const> = (1 << blocks.FACE_NORTH) | (1 << blocks.FACE_SOUTH) | (1 << blocks.FACE_EAST) | (1 << blocks.FACE_WEST)
local FLUID_BOTTOM <const> = FLUID_SIDES | (1 << blocks.FACE_TOP)
local FLUID_TOP <const> = FLUID_SIDES | (1 << blocks.FACE_BOTTOM)

local function orientation_overrides(orientation, health, drop_name)
  local overrides = {
    health = health,
    drops = {
      {
        items = {
          {
            name = drop_name,
            count = orientation == "double" and 2 or 1,
          }
        }
      }
    },
  }

  if orientation == "top" then
    overrides.model_facing = blocks.FACE_TOP
    overrides.bcoll_offset = { 0, 8, 0 }
    overrides.fluid_faces = FLUID_TOP
  elseif orientation == "bottom" then
    overrides.model_facing = blocks.FACE_BOTTOM
    overrides.fluid_faces = FLUID_BOTTOM
  elseif orientation == "double" then
    overrides.model_name = "cube"
    overrides.bcoll_name = "cube"
  end

  return overrides
end

-- A filled cell holds a water source, never a flowing block, so a slab placed
-- into flowing water stays dry and the flow simply ends there
local function holds_source(bx, by, bz)
  if world.get_fluid(bx, by, bz) == 0 then
    return false
  end

  if not world.get_state(bx, by, bz, "level") then
    return true
  end

  return world.get_state(bx, by, bz, "source") == "true"
end

local function fill_name(bx, by, bz, fillable)
  if not holds_source(bx, by, bz) then
    return EMPTY
  end

  local fluid = world.get_fluid(bx, by, bz)

  for _, name in ipairs(fillable) do
    if fluids.get(name) == fluid then
      return name
    end
  end

  return EMPTY
end

local function slab_state(bx, by, bz)
  local state = world.get_state(bx, by, bz, "orientation")

  if state == "top" or state == "bottom" or state == "double" then
    return state
  end

  return nil
end

function slabdef.on_place(identifier, fillable)
  return function(bx, by, bz, target, occupant, actor)
    local slab_id = blocks.get(identifier)

    if target.stem == slab_id then
      local state = slab_state(target.bx, target.by, target.bz)

      if state == "bottom" and target.face == blocks.FACE_TOP then
        world.set_state(target.bx, target.by, target.bz, "orientation", "double")
        if fillable then
          world.set_state(target.bx, target.by, target.bz, flowdef.filled_state, EMPTY)
        end
        return nil
      elseif state == "top" and target.face == blocks.FACE_BOTTOM then
        world.set_state(target.bx, target.by, target.bz, "orientation", "double")
        if fillable then
          world.set_state(target.bx, target.by, target.bz, flowdef.filled_state, EMPTY)
        end
        return nil
      end
    end

    local occupant_state = slab_state(bx, by, bz)

    if occupant_state then
      if occupant_state ~= "double" then
        world.set_state(bx, by, bz, "orientation", "double")
        if fillable then
          world.set_state(bx, by, bz, flowdef.filled_state, EMPTY)
        end
      end

      return nil
    end

    if occupant.id ~= blocks.NULL_BLOCK and not occupant.replaceable then
      return nil
    end

    local state = "bottom"

    if target.face == blocks.FACE_BOTTOM or target.ry > 0.5 then
      state = "top"
    end

    local result = { orientation = state }

    if fillable then
      result[flowdef.filled_state] = fill_name(bx, by, bz, fillable)
    end

    return result
  end
end

function slabdef.on_break()
  return function(bx, by, bz, actor)
    local fluid = world.get_fluid(bx, by, bz)

    if fluid ~= 0 and flowdef.place_source(fluid, bx, by, bz) then
      return true
    end

    return nil
  end
end

function slabdef.add_block(identifier, prototype, options)
  options = options or {}

  local health = options.health or prototype.health or 2
  local drop_name = options.drop_name or identifier
  local fillable = options.fillable
  local state_name = flowdef.filled_state

  if fillable and #fillable == 0 then
    fillable = nil
  end

  local states = {
    orientation = {
      hint = { "top", "bottom", "double" },
      default = "bottom",
    }
  }

  if fillable and #fillable > 0 then
    local hint = { EMPTY }

    for _, name in ipairs(fillable) do
      table.insert(hint, name)
    end

    states[state_name] = {
      hint = hint,
      default = EMPTY,
    }
  end

  local variants = {}

  for _, orientation in ipairs(ORIENTATIONS) do
    if fillable and #fillable > 0 then
      table.insert(variants, {
        when = { orientation = orientation, [state_name] = EMPTY },
        overrides = orientation_overrides(orientation, health, drop_name),
      })

      for _, name in ipairs(fillable) do
        local overrides = orientation_overrides(orientation, health, drop_name)

        -- A double slab is a full cube and never holds a fluid
        if orientation ~= "double" then
          overrides.fluid_name = name
        end

        table.insert(variants, {
          when = { orientation = orientation, [state_name] = name },
          overrides = overrides,
        })
      end
    else
      table.insert(variants, {
        when = { orientation = orientation },
        overrides = orientation_overrides(orientation, health, drop_name),
      })
    end
  end

  local def = {
    model_name = "slab",
    bcoll_name = "slab",
    states = states,
    variants = variants,
    on_place = options.on_place or slabdef.on_place(identifier, fillable),
  }

  if fillable and #fillable > 0 then
    -- A filled slab keeps a source that still flows out of its open faces, so
    -- it ticks like a waterlogged block does in vanilla
    def.on_tick = options.on_tick or flowdef.tick
    def.on_break = options.on_break or slabdef.on_break()
  end

  for k, v in pairs(options) do
    if k ~= "fillable" and def[k] == nil then
      def[k] = v
    end
  end

  return blocks.add(identifier, prototype, def)
end

return slabdef
