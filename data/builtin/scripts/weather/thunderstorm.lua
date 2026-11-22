weather.add("thunderstorm", {
  intensity = 1.0,
  sky_tint = { 0.14, 0.16, 0.19 },
  sky_visibility = 0.05,
  light = 0.35,

  -- The storm gets its darkness from the sky tint and the hidden sun,
  -- not from the albedo: the cloud stays white and the light goes away
  cloud = {
    height = 160,
    thickness = 64,
    opacity = 0.95,
    scale = 112,
    coverage = 1.0,
    shadow = { 0.84, 0.85, 0.88 },
    lit = { 0.97, 0.98, 1.00 },
    seed = 19,
  },

  precipitation = {
    kind = "rain",
    intensity = 1.0,
    speed = 34.0,
    drift = 1.2,
    size = 0.07,
    color = { 0.32, 0.38, 0.46 },
  },

  strikes_per_minute = 6.0,
  strike_radius = 96.0,

  target = { 0.82, 0.55 },
  weight = 2.0,
})
