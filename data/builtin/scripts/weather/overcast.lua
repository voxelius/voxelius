weather.add("overcast", {
  realms = { weather.REALM_SURFACE },

  intensity = 1.0,
  sky_tint = { 0.388, 0.451, 0.541 },
  sky_visibility = 0.15,
  light = 0.60,
  wind = 1.5,

  cloud = {
    height = 188,
    thickness = 16,
    opacity = 0.95,
    scale = 192,
    coverage = 1.0,
    shadow = { 0.78, 0.80, 0.84 },
    lit = { 0.95, 0.96, 0.99 },
    seed = 3,
  },

  weight = 3,
})
