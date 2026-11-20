biomes.add("surface_hills_grassy", {
  realm = biomes.REALM_SURFACE,

  temperature = 40,
  humidity = 50,
  continentalness = 70,
  erosion = 25,
  weirdness = 50,

  palette = {
    basic = { name = "stone" },
    filler = { name = "dirt" },
    surface = { name = "grass" },
    fluid = { name = "water", states = { level = "8", source = "true" } },
  },

  scatter = {
    {
      feature = "bush",
      chance = 1.0 / 8.0,
      need_above = { blocks.TAG_GAS },
      need_below = { blocks.TAG_TURF },
    },
  },

  tints = {
    grass = { 0.50, 0.70, 0.35 },
    foliage = { 0.36, 0.58, 0.28 },
    water = { 0.22, 0.45, 0.80 },
  },
})
