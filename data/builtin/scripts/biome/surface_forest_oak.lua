biomes.add("surface_forest_oak", {
  temperature = 8,
  humidity = 55,

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
      feature = "oak_tree_01",
      chance = 1.0 / 32.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "oak_tree",
      padding = 1,
    },
    {
      feature = "oak_tree_02",
      chance = 1.0 / 32.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "oak_tree",
      padding = 1,
    },
    {
      feature = "oak_tree_03",
      chance = 1.0 / 32.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "oak_tree",
      padding = 1,
    },
  },

  tints = {
    grass = { 0.35, 0.68, 0.22 },
    foliage = { 0.28, 0.58, 0.18 },
    water = { 0.20, 0.42, 0.78 },
  },
})
