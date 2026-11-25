blocks.add("glass", {
  render = blocks.RENDER_ALPHA,
  animated = false,

  albedo = {
    default = {
      "glass/glass.png",
    }
  },

  model_name = "cube",
  bcoll_name = "cube",

  health = 1,

  emission = 0,
  dissipation = 0,

  drops = {
    {
      items = {
        {
          name = "glass",
          count = 1
        }
      }
    }
  }
})
