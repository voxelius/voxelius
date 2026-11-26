biomes.add("vox2_savanna", {
  temperature = 26,
  humidity = 25,

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
      chance = 1.0 / 8.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
    },
    {
      feature = "oak_tree_01",
      chance = 1.0 / 4096.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
    },
    {
      feature = "oak_tree_02",
      chance = 1.0 / 4096.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
    },
    {
      feature = "oak_tree_03",
      chance = 1.0 / 4096.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
    },
  },

  tints = {
    grass = { 0.72, 0.72, 0.32 },
    foliage = { 0.62, 0.62, 0.26 },
    water = { 0.28, 0.52, 0.80 },
  },
})
