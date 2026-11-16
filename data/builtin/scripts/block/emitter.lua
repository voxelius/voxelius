blocks.add("emitter", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "emitter/emitter.png",
    }
  },

  model_name = "cube",
  bcoll_name = "cube",

  health = 1,
  tools = { blocks.TOOL_SHOVEL },

  touch = blocks.TOUCH_SOLID,

  emission = 15,
  dissipation = 0,
})
