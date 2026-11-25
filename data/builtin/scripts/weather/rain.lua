weather.add("rain", {
  intensity = 1.0,
  sky_tint = { 0.388, 0.451, 0.541 },
  sky_visibility = 0.10,
  light = 0.50,

  cloud = {
    height = 176,
    thickness = 40,
    opacity = 0.90,
    scale = 128,
    coverage = 1.0,
    shadow = { 0.80, 0.82, 0.86 },
    lit = { 0.94, 0.95, 0.98 },
    seed = 7,
  },

  precip = {
    types = { "rain", "snow", "dry" },
    intensity = 1.0,
  },

  target = { 0.62, 0.62 },
  weight = 3.0,
})
