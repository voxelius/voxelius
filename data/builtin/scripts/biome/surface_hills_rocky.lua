biomes.add("surface_hills_rocky", {
  realm = biomes.REALM_SURFACE,

  temperature = 30,
  humidity = 40,
  continentalness = 75,
  erosion = 15,
  weirdness = 50,

  palette = {
    basic = { name = "stone" },
    filler = { name = "stone" },
    surface = { name = "stone" },
    fluid = { name = "water", states = { level = "8", source = "true" } },
  },

  tints = {
    grass = { 0.55, 0.68, 0.40 },
    foliage = { 0.40, 0.55, 0.32 },
    water = { 0.22, 0.42, 0.78 },
  },
})
