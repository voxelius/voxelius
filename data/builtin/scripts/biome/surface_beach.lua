biomes.add("surface_beach", {
  temperature = 60,
  humidity = 60,
  continentalness = 35,
  erosion = 80,
  weirdness = 50,

  palette = {
    basic = { name = "stone" },
    filler = { name = "sand" },
    surface = { name = "sand" },
    fluid = { name = "water", states = { level = "8", source = "true" } },
  },

  tints = {
    grass = { 0.70, 0.72, 0.40 },
    foliage = { 0.55, 0.62, 0.30 },
    water = { 0.28, 0.55, 0.88 },
  },
})
