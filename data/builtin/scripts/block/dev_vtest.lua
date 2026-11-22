blocks.add("dev_vtest", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "dev/vtest_f1.png",
      "dev/vtest_f2.png",
      "dev/vtest_f3.png",
      "dev/vtest_f4.png",
    }
  },

  model_name = "cube",
  bcoll_name = "cube",

  health = 1,

  touch = blocks.TOUCH_SOLID,

  emission = 0,
  dissipation = 15,
})
