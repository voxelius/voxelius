biomes.add("surface_desert", {
  temperature = 80,
  humidity = 20,
  continentalness = 60,
  erosion = 65,
  weirdness = 50,

  palette = {
    basic = { name = "stone" },
    filler = { name = "sandstone" },
    surface = { name = "sand" },
    fluid = { name = "water", states = { level = "8", source = "true" } },
  },

  tints = {
    grass = { 0.75, 0.72, 0.32 },
    foliage = { 0.65, 0.60, 0.25 },
    water = { 0.30, 0.55, 0.80 },
  },
})
