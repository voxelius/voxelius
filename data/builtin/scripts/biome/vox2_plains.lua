biomes.add("vox2_plains", {
  temperature = 15,
  humidity = 45,

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
      feature = "bush",
      chance = 1.0 / 6.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
    },
    {
      feature = "oak_tree_01",
      chance = 1.0 / 1400.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
      slope = 2,
    },
    {
      feature = "oak_tree_02",
      chance = 1.0 / 1400.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
      slope = 2,
    },
    {
      feature = "oak_tree_03",
      chance = 1.0 / 1400.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
      slope = 2,
    },
    {
      feature = "birch_tree_01",
      chance = 1.0 / 2048.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
      slope = 2,
    },
    {
      feature = "birch_tree_02",
      chance = 1.0 / 2048.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
      slope = 2,
    },
  },

  tints = {
    grass = { 0.57, 0.74, 0.30 },
    foliage = { 0.42, 0.66, 0.28 },
    water = { 0.25, 0.48, 0.85 },
  },
})
