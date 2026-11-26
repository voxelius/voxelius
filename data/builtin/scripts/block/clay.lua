blocks.add("clay", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "clay/clay_01.png",
      "clay/clay_02.png",
      "clay/clay_03.png",
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
          name = "clay",
          count = 1
        }
      }
    }
  }
})
