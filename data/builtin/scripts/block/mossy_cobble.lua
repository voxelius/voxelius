local slabdef = require("builtin:slabdef")

-- The moss is the tint the mossy_cube model applies through this mask; the
-- albedo is the plain cobble, so both blocks share one stone pattern.
local mossy_cobble_prototype = {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "cobble/cobble_01.png",
      "cobble/cobble_02.png",
    }
  },

  masks = {
    top = "cobble/cobblemoss_mask.png",
    bottom = "cobble/cobblemoss_mask.png",
    north = "cobble/cobblemoss_mask.png",
    south = "cobble/cobblemoss_mask.png",
    east = "cobble/cobblemoss_mask.png",
    west = "cobble/cobblemoss_mask.png",
  },

  health = 2,
  tools = { blocks.TOOL_PICK },

  emission = 0,
  dissipation = 15,

  tags = { blocks.TAG_ROCK }
}

blocks.add("mossy_cobble", mossy_cobble_prototype, {
  model_name = "mossy_cube",
  bcoll_name = "cube",

  drops = {
    {
      items = {
        {
          name = "mossy_cobble",
          count = 1
        }
      }
    }
  }
})

slabdef.add_block("mossy_cobble_slab", mossy_cobble_prototype, {
  health = 1,
  fillable = { "water" },
})
