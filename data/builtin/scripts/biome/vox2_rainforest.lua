biomes.add("vox2_rainforest", {
  temperature = 24,
  humidity = 90,

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
      chance = 1.0 / 3.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
    },
    {
      feature = "oak_tree_01",
      chance = 1.0 / 64.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
      slope = 2,
    },
    {
      feature = "oak_tree_02",
      chance = 1.0 / 64.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
      slope = 2,
    },
    {
      feature = "oak_tree_03",
      chance = 1.0 / 64.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
      slope = 2,
    },
  },

  tints = {
    grass = { 0.22, 0.66, 0.18 },
    foliage = { 0.16, 0.54, 0.14 },
    water = { 0.18, 0.42, 0.74 },
  },
})
