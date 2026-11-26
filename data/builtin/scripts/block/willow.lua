local slabdef = require("builtin:slabdef")
local logdef = require("builtin:logdef")

logdef.add_block("willow_log", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "willow/willow_wood_01.png",
      "willow/willow_wood_02.png",
    },
    north = {
      "willow/willow_log.png"
    },
    south = {
      "willow/willow_log.png"
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
          name = "willow_log",
          count = 1
        }
      }
    }
  }
})

blocks.add("willow_wood", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "willow/willow_wood_01.png",
      "willow/willow_wood_02.png",
    }
  },

  model_name = "leaves_willow",
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
          name = "willow_wood",
          count = 1
        }
      }
    }
  }
})

local willow_planks_prototype = {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "willow/willow_planks_01.png",
      "willow/willow_planks_02.png",
    }
  },

  health = 2,
  tools = { blocks.TOOL_AXE },

  emission = 0,
  dissipation = 15,

  tags = { blocks.TAG_WOOD }
}

blocks.add("willow_planks", willow_planks_prototype, {
  model_name = "leaves_willow",
  bcoll_name = "cube",

  drops = {
    {
      items = {
        {
          name = "willow_planks",
          count = 1
        }
      }
    }
  }
})

slabdef.add_block("willow_planks_slab", willow_planks_prototype, {
  health = 1,
  fillable = { "water" },
})

blocks.add("willow_leaves", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "willow/willow_leaves.png"
    }
  },

  model_name = "leaves_willow",
  bcoll_name = "cube",

  health = 1,
  tools = { blocks.TOOL_XBLADE },

  tags = { blocks.TAG_SFOIL },

  emission = 0,
  dissipation = 2,
})
