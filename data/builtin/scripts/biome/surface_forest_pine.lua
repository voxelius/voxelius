biomes.add("surface_forest_pine", {
  temperature = 25,
  humidity = 55,
  continentalness = 60,
  erosion = 50,
  weirdness = 55,

  palette = {
    basic = { name = "stone" },
    filler = { name = "dirt" },
    surface = { name = "grass" },
    fluid = { name = "water", states = { level = "8", source = "true" } },
  },

  scatter = {
    {
      feature = "bush",
      chance = 1.0 / 3.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
    },
    {
      feature = "pine_tree_01",
      chance = 1.0 / 32.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "pine_tree",
      padding = 1,
    },
    {
      feature = "pine_tree_02",
      chance = 1.0 / 32.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "pine_tree",
      padding = 1,
    },
    {
      feature = "pine_tree_03",
      chance = 1.0 / 32.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "pine_tree",
      padding = 2,
    },
    {
      feature = "pine_tree_04",
      chance = 1.0 / 32.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "pine_tree",
      padding = 2,
    },
  },

  tints = {
    grass = { 0.38, 0.62, 0.30 },
    foliage = { 0.22, 0.48, 0.28 },
    water = { 0.18, 0.40, 0.72 },
  },
})
