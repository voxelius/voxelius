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

  biomes = {
    "surface_beach",
    "surface_desert",
    "surface_forest_oak",
    "surface_forest_pine",
    "surface_hills_grassy",
    "surface_hills_rocky",
    "surface_plains",
  },
})
