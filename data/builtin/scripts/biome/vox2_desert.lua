biomes.add("vox2_desert", {
  temperature = 30,
  humidity = 10,

  palette = {
    base = { name = "stone" },
    land_filler = { name = "sandstone" },
    land_surface = { name = "sand" },
    fluid = { name = "water", states = { level = "8", source = "true" } },
    shore_surface = { name = "sand" },
    shore_filler = { name = "sand" },
  },

  tints = {
    grass = { 0.75, 0.72, 0.32 },
    foliage = { 0.65, 0.60, 0.25 },
    water = { 0.30, 0.55, 0.80 },
  },
})
