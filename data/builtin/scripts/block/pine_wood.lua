local logdef = require("builtin:logdef")

logdef.add_block("pine_log", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "pine/pine_wood_01.png",
      "pine/pine_wood_02.png",
      "pine/pine_wood_03.png",
    },
    north = {
      "pine/pine_log.png"
    },
    south = {
      "pine/pine_log.png"
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
          name = "pine_log",
          count = 1
        }
      }
    }
  }
})

blocks.add("pine_wood", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "pine/pine_wood_01.png",
      "pine/pine_wood_02.png",
      "pine/pine_wood_03.png",
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
          name = "pine_wood",
          count = 1
        }
      }
    }
  }
})
