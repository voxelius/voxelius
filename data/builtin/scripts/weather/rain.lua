weather.add("rain", {
  realms = { weather.REALM_SURFACE },

  intensity = 1.0,
  sky_tint = { 0.388, 0.451, 0.541 },
  light = 0.50,
  wind = 1.2,

  layers = {
    {
      height = 176,
      thickness = 8,
      opacity = 0.90,
      scale = 128,
      coverage = 1.0,
      shadow = { 0.28, 0.30, 0.35 },
      lit = { 0.40, 0.43, 0.49 },
      seed = 7,
    },
    {
      height = 200,
      thickness = 10,
      opacity = 0.85,
      scale = 176,
      coverage = 0.9,
      shadow = { 0.32, 0.34, 0.39 },
      lit = { 0.46, 0.49, 0.55 },
      seed = 13,
    },
  },

  precipitation = {
    kind = "rain",
    intensity = 1.0,
    speed = 28.0,
    drift = 0.6,
    size = 0.06,
    color = { 0.55, 0.66, 0.80 },
  },

  weight = 3,
})
