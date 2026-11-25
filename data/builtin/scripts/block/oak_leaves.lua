blocks.add("oak_leaves", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "oak/oak_leaves.png",
    }
  },

  model_name = "leaves",
  bcoll_name = "cube",

  health = 1,
  tools = { blocks.TOOL_XBLADE },

  tags = { blocks.TAG_SFOIL },

  emission = 0,
  dissipation = 2,
})
