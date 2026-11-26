local slabdef = require("builtin:slabdef")
local logdef = require("builtin:logdef")

logdef.add_block("spruce_log", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "spruce/spruce_wood_01.png",
      "spruce/spruce_wood_02.png",
    },
    north = {
      "spruce/spruce_log.png"
    },
    south = {
      "spruce/spruce_log.png"
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
          name = "spruce_log",
          count = 1
        }
      }
    }
  }
})

blocks.add("spruce_wood", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "spruce/spruce_wood_01.png",
      "spruce/spruce_wood_02.png",
    }
  },

  model_name = "leaves_spruce",
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
          name = "spruce_wood",
          count = 1
        }
      }
    }
  }
})

local spruce_planks_prototype = {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "spruce/spruce_planks_01.png",
      "spruce/spruce_planks_02.png",
    }
  },

  health = 2,
  tools = { blocks.TOOL_AXE },

  emission = 0,
  dissipation = 15,

  tags = { blocks.TAG_WOOD }
}

blocks.add("spruce_planks", spruce_planks_prototype, {
  model_name = "leaves_spruce",
  bcoll_name = "cube",

  drops = {
    {
      items = {
        {
          name = "spruce_planks",
          count = 1
        }
      }
    }
  }
})

slabdef.add_block("spruce_planks_slab", spruce_planks_prototype, {
  health = 1,
  fillable = { "water" },
})

blocks.add("spruce_leaves", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "spruce/spruce_leaves.png"
    }
  },

  model_name = "leaves_spruce",
  bcoll_name = "cube",

  health = 1,
  tools = { blocks.TOOL_XBLADE },

  tags = { blocks.TAG_SFOIL },

  emission = 0,
  dissipation = 2,
})
