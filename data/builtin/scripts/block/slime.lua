blocks.add("slime", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "slime/slime_01.png",
    }
  },

  model_name = "cube",
  bcoll_name = "cube",

  health = 1,

  bounce = { 0, 1, 0 },
  friction = 0.8,

  emission = 0,
  dissipation = 15,

  drops = {
    {
      items = {
        {
          name = "slime",
          count = 1
        }
      }
    }
  }
})
