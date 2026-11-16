local flowdef = require("builtin:flowdef")

local slabdef = {}

local ORIENTATIONS <const> = { "top", "bottom", "double" }
local EMPTY <const> = "none"

-- A bottom slab fills the lower half of its cell, so the cell boundary above it
-- is open; a top slab is the other way around. Both expose their side faces.
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

local function fill_name(bx, by, bz, fillable)
  local fluid = world.get_fluid(bx, by, bz)

  if fluid == 0 then
    return EMPTY
  end

  for _, name in ipairs(fillable) do
    if fluids.get(name) == fluid then
      return name
    end
  end

  return EMPTY
end

function slabdef.on_place(identifier, fillable)
  return function(bx, by, bz, target, occupant, actor)
    local slab_id = blocks.get(identifier)

    if target.stem == slab_id then
      local state = world.get_state(target.bx, target.by, target.bz, "orientation")

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

    if occupant.id == slab_id then
      local state = world.get_state(bx, by, bz, "orientation")
      if state ~= "double" then
        world.set_state(bx, by, bz, "orientation", "double")
        if fillable then
          world.set_state(bx, by, bz, flowdef.filled_state, EMPTY)
        end
        return nil
      end
    end

    if occupant.id ~= blocks.NULL_BLOCK and occupant.id ~= slab_id and not occupant.replaceable then
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

        -- A double slab is a full cube and never holds a fluid.
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
