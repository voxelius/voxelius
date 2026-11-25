local slabdef = require("builtin:slabdef")

local cobble_prototype = {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "cobble/cobble_01.png",
      "cobble/cobble_02.png",
    }
  },

  health = 2,
  tools = { blocks.TOOL_PICK },

  emission = 0,
  dissipation = 15,

  tags = { blocks.TAG_ROCK }
}

blocks.add("cobble", cobble_prototype, {
  model_name = "cube",
  bcoll_name = "cube",

  drops = {
    {
      items = {
        {
          name = "cobble",
          count = 1
        }
      }
    }
  }
})

slabdef.add_block("cobble_slab", cobble_prototype, {
  health = 1,
  fillable = { "water" },
})
