blocks.add("sand", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "sand/sand_01.png",
      "sand/sand_02.png",
      "sand/sand_03.png",
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
          name = "sand",
          count = 1
        }
      }
    }
  }
})
