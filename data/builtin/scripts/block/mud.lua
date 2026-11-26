blocks.add("mud", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "mud/mud_01.png",
      "mud/mud_02.png",
    }
  },

  model_name = "cube",
  bcoll_name = "cube",

  health = 1,
  tools = { blocks.TOOL_SHOVEL },

  tags = { blocks.TAG_SOIL },

  emission = 0,
  dissipation = 15,

  drops = {
    {
      items = {
        {
          name = "mud",
          count = 1
        }
      }
    }
  },
})
