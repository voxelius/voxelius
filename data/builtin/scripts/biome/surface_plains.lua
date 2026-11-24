biomes.add("surface_plains", {
  temperature = 12,
  humidity = 50,

  palette = {
    basic = { name = "stone" },
    filler = { name = "dirt" },
    surface = { name = "grass" },
    fluid = { name = "water", states = { level = "8", source = "true" } },
  },

  scatter = {
    {
      feature = "bush",
      chance = 1.0 / 4.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
    },
    {
      feature = "oak_tree_01",
      chance = 1.0 / 4096.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "oak_tree",
      padding = 2,
    },
    {
      feature = "oak_tree_02",
      chance = 1.0 / 4096.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "oak_tree",
      padding = 2,
    },
    {
      feature = "oak_tree_03",
      chance = 1.0 / 4096.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "oak_tree",
      padding = 2,
    },
  },

  tints = {
    grass = { 0.57, 0.74, 0.30 },
    foliage = { 0.42, 0.66, 0.28 },
    water = { 0.25, 0.48, 0.85 },
  },
})
