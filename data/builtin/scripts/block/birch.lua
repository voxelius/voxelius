local slabdef = require("builtin:slabdef")
local logdef = require("builtin:logdef")

logdef.add_block("birch_log", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "birch/birch_wood_01.png",
      "birch/birch_wood_02.png",
    },
    north = {
      "birch/birch_log.png"
    },
    south = {
      "birch/birch_log.png"
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
          name = "birch_log",
          count = 1
        }
      }
    }
  }
})

blocks.add("birch_wood", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "birch/birch_wood_01.png",
      "birch/birch_wood_02.png",
    }
  },

  model_name = "leaves_birch",
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
          name = "birch_wood",
          count = 1
        }
      }
    }
  }
})

local birch_planks_prototype = {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "birch/birch_planks_01.png",
      "birch/birch_planks_02.png",
    }
  },

  health = 2,
  tools = { blocks.TOOL_AXE },

  emission = 0,
  dissipation = 15,

  tags = { blocks.TAG_WOOD }
}

blocks.add("birch_planks", birch_planks_prototype, {
  model_name = "leaves_birch",
  bcoll_name = "cube",

  drops = {
    {
      items = {
        {
          name = "birch_planks",
          count = 1
        }
      }
    }
  }
})

slabdef.add_block("birch_planks_slab", birch_planks_prototype, {
  health = 1,
  fillable = { "water" },
})

blocks.add("birch_leaves", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "birch/birch_leaves.png"
    }
  },

  model_name = "leaves_birch",
  bcoll_name = "cube",

  health = 1,
  tools = { blocks.TOOL_XBLADE },

  tags = { blocks.TAG_SFOIL },

  emission = 0,
  dissipation = 2,
})
