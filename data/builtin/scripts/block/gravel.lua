blocks.add("gravel", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "gravel/gravel_01.png",
      "gravel/gravel_02.png",
      "gravel/gravel_03.png",
    }
  },

  model_name = "cube",
  bcoll_name = "cube",

  health = 1,
  tools = { blocks.TOOL_SHOVEL },

  tags = { blocks.TAG_ROCK },

  emission = 0,
  dissipation = 15,

  drops = {
    {
      items = {
        {
          name = "gravel",
          count = 1
        }
      }
    }
  }
})
