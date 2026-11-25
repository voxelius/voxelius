blocks.add("dev_test", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    top = { "dev/test_top.png" },
    bottom = { "dev/test_bottom.png" },
    north = { "dev/test_north.png" },
    south = { "dev/test_south.png" },
    east = { "dev/test_east.png" },
    west = { "dev/test_west.png" },
  },

  model_name = "cube",
  bcoll_name = "cube",

  health = 1,

  emission = 0,
  dissipation = 15,
})
