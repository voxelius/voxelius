blocks.add("sandstone", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "sandstone/sandstone_01.png",
      "sandstone/sandstone_02.png",
    }
  },

  model_name = "cube",
  bcoll_name = "cube",

  health = 1,
  tools = { blocks.TOOL_PICK },

  tags = { blocks.TAG_SOIL },

  emission = 0,
  dissipation = 15,

  drops = {
    {
      items = {
        {
          name = "sandstone",
          count = 1
        }
      }
    }
  }
})
