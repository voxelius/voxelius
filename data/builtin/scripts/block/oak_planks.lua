local slabdef = require("builtin:slabdef")

local oak_planks_prototype = {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "oak/oak_planks_01.png",
      "oak/oak_planks_02.png",
    }
  },

  health = 2,
  tools = { blocks.TOOL_AXE },

  emission = 0,
  dissipation = 15,

  tags = { blocks.TAG_WOOD }
}

blocks.add("oak_planks", oak_planks_prototype, {
  model_name = "cube",
  bcoll_name = "cube",

  drops = {
    {
      items = {
        {
          name = "oak_planks",
          count = 1
        }
      }
    }
  }
})

slabdef.add_block("oak_planks_slab", oak_planks_prototype, {
  health = 1,
  fillable = { "water" },
})
