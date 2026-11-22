worlds.add("overworld", {
  directory = "overworld",
  generator = "vox17",

  min_y = -64,
  max_y = 255,
  spawn = { 0, 32, 0 },

  visuals = {
    sky = {
      day = {
        horizon = { 0.64, 0.76, 0.94 },
        zenith = { 0.26, 0.46, 0.85 },
      },

      night = {
        horizon = { 0.020, 0.030, 0.060 },
        zenith = { 0.004, 0.007, 0.018 },
      },

      twilight_height = 0.35,
    },

    sun = {
      tilt = 0.0,
    },

    moon = {
      color = { 0.86, 0.90, 1.0 },
    },

    stars = {
      count = 5120,
    },

    clouds = {},
  },

  weather = {
    "clear",
    "overcast",
    "rain",
    "thunderstorm",
  },

  biome_selector = {
    strategy = worlds.SELECTION_NEAREST,
    axes = {
      "temperature",
      "humidity",
      "continentalness",
      "erosion",
      "weirdness",
    },
    weights = { 1.0, 1.0, 0.75, 1.0, 1.5 },
    entries = {
      { biome = "surface_beach",        target = { 60, 60, 35, 80, 50 } },
      { biome = "surface_desert",       target = { 80, 20, 60, 65, 50 } },
      { biome = "surface_forest_oak",   target = { 50, 70, 55, 60, 45 } },
      { biome = "surface_forest_pine",  target = { 25, 55, 60, 50, 55 } },
      { biome = "surface_hills_grassy", target = { 40, 50, 70, 25, 50 } },
      { biome = "surface_hills_rocky",  target = { 30, 40, 75, 15, 50 } },
      { biome = "surface_plains",       target = { 50, 55, 55, 70, 50 } },
    },
  },
})
