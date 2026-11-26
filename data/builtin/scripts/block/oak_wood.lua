local logdef = require("builtin:logdef")

logdef.add_block("oak_log", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "oak/oak_wood_01.png",
      "oak/oak_wood_02.png",
    },
    north = {
      "oak/oak_log.png"
    },
    south = {
      "oak/oak_log.png"
    },
  },

  health = 4,
  tools = { blocks.TOOL_AXE },

  emission = 0,
  dissipation = 15,

  tags = { blocks.TAG_WOOD },

  drops = {
    {
      items = {
        {
          name = "oak_log",
          count = 1
        }
      }
    }
  }
})

blocks.add("oak_wood", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "oak/oak_wood_01.png",
      "oak/oak_wood_02.png",
    }
  },

  model_name = "cube",
  bcoll_name = "cube",

  health = 4,
  tools = { blocks.TOOL_AXE },

  emission = 0,
  dissipation = 15,

  tags = { blocks.TAG_WOOD },

  drops = {
    {
      items = {
        {
          name = "oak_wood",
          count = 1
        }
      }
    }
  }
})
