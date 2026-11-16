local slabdef = require("builtin:slabdef")

local slate_prototype = {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "slate/slate_01.png",
      "slate/slate_02.png",
    }
  },

  health = 2,
  tools = { blocks.TOOL_PICK },

  emission = 0,
  dissipation = 15,

  touch = blocks.TOUCH_SOLID,
  tags = { blocks.TAG_ROCK }
}

blocks.add("slate", slate_prototype, {
  model_name = "cube",
  bcoll_name = "cube",

  drops = {
    {
      items = {
        {
          name = "slate",
          count = 1
        }
      }
    }
  }
})

slabdef.add_block("slate_slab", slate_prototype, {
  health = 1,
  fillable = { "water" },
})
