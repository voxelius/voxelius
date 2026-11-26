local logdef = require("builtin:logdef")

-- The stem is a log-family block: it has the orientation state and the log
-- model turns its side texture by 90 degrees, exactly like tree bark.
logdef.add_block("mushroom_stem", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "mushroom/mushroom_stem.png"
    },
    north = {
      "mushroom/mushroom_stem.png"
    },
    south = {
      "mushroom/mushroom_stem.png"
    },
  },

  health = 1,
  tools = { blocks.TOOL_AXE },

  emission = 0,
  dissipation = 15,

  tags = { blocks.TAG_WOOD },

  drops = {
    {
      items = {
        {
          name = "mushroom_stem",
          count = 1
        }
      }
    }
  }
})

blocks.add("red_mushroom_cap", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    top = {
      "mushroom/red_mushroom_cap_01.png",
      "mushroom/red_mushroom_cap_02.png",
      "mushroom/red_mushroom_cap_03.png",
      "mushroom/red_mushroom_cap_04.png",
    },
    bottom = {
      "mushroom/mushroom_trama_01.png",
      "mushroom/mushroom_trama_02.png",
      "mushroom/mushroom_trama_03.png",
    },
    default = {
      "mushroom/red_mushroom_cap_01.png",
      "mushroom/red_mushroom_cap_02.png",
      "mushroom/red_mushroom_cap_03.png",
      "mushroom/red_mushroom_cap_04.png",
    },
  },

  model_name = "cube",
  bcoll_name = "cube",

  health = 1,

  emission = 0,
  dissipation = 15,

  drops = {
    {
      items = {
        {
          name = "red_mushroom_cap",
          count = 1
        }
      }
    }
  }
})

blocks.add("brown_mushroom_cap", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    top = {
      "mushroom/brown_mushroom_cap_01.png",
      "mushroom/brown_mushroom_cap_02.png",
    },
    bottom = {
      "mushroom/mushroom_trama_01.png",
      "mushroom/mushroom_trama_02.png",
      "mushroom/mushroom_trama_03.png",
    },
    default = {
      "mushroom/brown_mushroom_cap_01.png",
      "mushroom/brown_mushroom_cap_02.png",
    },
  },

  model_name = "cube",
  bcoll_name = "cube",

  health = 1,

  emission = 0,
  dissipation = 15,

  drops = {
    {
      items = {
        {
          name = "brown_mushroom_cap",
          count = 1
        }
      }
    }
  }
})

blocks.add("red_mushroom", {
  render = blocks.RENDER_ALPHA,

  albedo = {
    default = {
      "mushroom/red_mushroom_01.png",
      "mushroom/red_mushroom_02.png",
    }
  },

  model_name = "plant",
  bcoll_name = "bush",

  collidable = false,
  selectable = true,

  replaceable = true,

  tags = { blocks.TAG_XFOIL }
})

blocks.add("brown_mushroom", {
  render = blocks.RENDER_ALPHA,

  albedo = {
    default = {
      "mushroom/brown_mushroom_01.png",
    }
  },

  model_name = "plant",
  bcoll_name = "bush",

  collidable = false,
  selectable = true,

  replaceable = true,

  tags = { blocks.TAG_XFOIL }
})
