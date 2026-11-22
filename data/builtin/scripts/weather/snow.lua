weather.add("snow", {
  intensity = 0.7,
  sky_tint = { 0.60, 0.65, 0.75 },
  sky_visibility = 0.55,
  light = 0.80,

  cloud = {
    height = 430,
    thickness = 24,
    opacity = 0.85,
    scale = 96,
    coverage = 0.8,
    shadow = { 0.72, 0.76, 0.84 },
    lit = { 0.90, 0.94, 1.00 },
    seed = 11,
  },

  precipitation = {
    kind = "snow",
    intensity = 0.8,
    speed = 2.2,
    drift = 0.4,
    size = 0.09,
    color = { 0.92, 0.95, 1.00 },
  },

  target = { 0.42, 0.72 },
  weight = 2.0,
})
