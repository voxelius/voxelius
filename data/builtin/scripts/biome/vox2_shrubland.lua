biomes.add("vox2_shrubland", {
  temperature = 22,
  humidity = 30,

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
  },

  tints = {
    grass = { 0.62, 0.70, 0.34 },
    foliage = { 0.50, 0.62, 0.28 },
    water = { 0.26, 0.50, 0.82 },
  },
})
