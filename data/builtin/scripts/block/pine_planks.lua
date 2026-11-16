local slabdef = require("builtin:slabdef")

local pine_planks_prototype = {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "pine/pine_planks_01.png",
      "pine/pine_planks_02.png",
    }
  },

  health = 2,
  tools = { blocks.TOOL_AXE },

  emission = 0,
  dissipation = 15,

  touch = blocks.TOUCH_SOLID,
  tags = { blocks.TAG_WOOD }
}

blocks.add("pine_planks", pine_planks_prototype, {
  model_name = "cube",
  bcoll_name = "cube",

  drops = {
    {
      items = {
        {
          name = "pine_planks",
          count = 1
        }
      }
    }
  }
})

slabdef.add_block("pine_planks_slab", pine_planks_prototype, {
  health = 1,
  fillable = { "water" },
})
