biomes.add("vox2_marsh", {
  temperature = 22,
  humidity = 90,

  palette = {
    base = { name = "stone" },
    land_filler = { name = "dirt" },
    land_surface = { name = "grass" },
    fluid = { name = "water", states = { level = "8", source = "true" } },
    shore_surface = { name = "gravel" },
    shore_filler = { name = "gravel" },
  },

  scatter = {
    {
      feature = "fern",
      chance = 1.0 / 4.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
    },
    {
      feature = "willow_tree_01",
      chance = 1.0 / 128.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
    },
    {
      feature = "willow_tree_02",
      chance = 1.0 / 128.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
    },
    {
      feature = "willow_tree_03",
      chance = 1.0 / 128.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
    },
    {
      feature = "swamp_tree_01",
      chance = 1.0 / 512.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
    },
    {
      feature = "swamp_tree_02",
      chance = 1.0 / 512.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "tree",
      padding = 1,
    },
  },

  tints = {
    grass = { 0.30, 0.58, 0.24 },
    foliage = { 0.24, 0.50, 0.20 },
    water = { 0.22, 0.44, 0.74 },
  },
})
