biomes.add("vox2_seasonal_forest", {
  temperature = 17,
  humidity = 70,

  palette = {
    base = { name = "stone" },
    land_filler = { name = "dirt" },
    land_surface = { name = "grass" },
    fluid = { name = "water", states = { level = "8", source = "true" } },
    shore_surface = { name = "sand" },
    shore_filler = { name = "sand" },
  },

  scatter = {
    {
      feature = "fern",
      chance = 1.0 / 4.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
    },
    {
      feature = "birch_tree_01",
      chance = 1.0 / 128.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
    },
    {
      feature = "birch_tree_02",
      chance = 1.0 / 128.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
    },
    {
      feature = "birch_tree_03",
      chance = 1.0 / 128.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
    },
    {
      feature = "birch_tree_04",
      chance = 1.0 / 128.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
    },
    {
      feature = "oak_tree_01",
      chance = 1.0 / 512.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
    },
    {
      feature = "oak_tree_02",
      chance = 1.0 / 512.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
    },
  },

  tints = {
    grass = { 0.36, 0.68, 0.22 },
    foliage = { 0.30, 0.58, 0.18 },
    water = { 0.20, 0.42, 0.78 },
  },
})
