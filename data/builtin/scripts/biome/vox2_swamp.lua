biomes.add("vox2_swamp", {
  temperature = 22,
  humidity = 90,

  palette = {
    base = { name = "stone" },
    land_filler = { name = "dirt" },
    land_surface = { name = "grass" },
    fluid = { name = "water", states = { level = "8", source = "true" } },
    shore_surface = { name = "mud" },
    shore_filler = { name = "mud" },
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
      chance = 1.0 / 64.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "oak_tree",
      padding = 1,
    },
    {
      feature = "oak_tree_02",
      chance = 1.0 / 64.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "oak_tree",
      padding = 1,
    },
    {
      feature = "oak_tree_03",
      chance = 1.0 / 64.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "oak_tree",
      padding = 1,
    },
  },

  tints = {
    grass = { 0.30, 0.58, 0.24 },
    foliage = { 0.24, 0.50, 0.20 },
    water = { 0.18, 0.36, 0.34 },
  },
})
