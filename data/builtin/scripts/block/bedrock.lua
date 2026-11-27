blocks.add("bedrock", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "bedrock/bedrock_01.png",
      "bedrock/bedrock_02.png",
      "bedrock/bedrock_03.png",
      "bedrock/bedrock_04.png",
    },
  },

  model_name = "cube",
  bcoll_name = "cube",

  tools = { blocks.TOOL_PICK },

  tags = { blocks.TAG_ROCK },

  emission = 0,
  dissipation = 15,

  drops = {},
})
