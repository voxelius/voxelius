biomes.add("vox2_tundra", {
  temperature = 6,
  humidity = 50,

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
      chance = 1.0 / 32.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
    },
    {
      feature = "pine_tree_01",
      chance = 1.0 / 1024.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "pine_tree",
      padding = 2,
    },
    {
      feature = "pine_tree_02",
      chance = 1.0 / 1024.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
      group = "pine_tree",
      padding = 2,
    },
  },

  tints = {
    grass = { 0.48, 0.60, 0.44 },
    foliage = { 0.36, 0.50, 0.40 },
    water = { 0.22, 0.40, 0.66 },
  },
})
