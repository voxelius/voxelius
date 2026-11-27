blocks.add("snow", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "snow/snow_01.png",
    },
  },

  model_name = "cube",
  bcoll_name = "cube",

  health = 1,
  tools = { blocks.TOOL_SHOVEL },

  tags = { blocks.TAG_SOIL },

  emission = 0,
  dissipation = 15,

  drops = {},
})
