weather.add("overcast", {
  realms = { weather.REALM_SURFACE },

  intensity = 1.0,
  sky_tint = { 0.388, 0.451, 0.541 },
  sky_visibility = 0.15,
  light = 0.60,
  wind = 1.5,

  layers = {
    {
      height = 188,
      thickness = 6,
      opacity = 0.95,
      scale = 192,
      coverage = 1.0,
      shadow = { 0.34, 0.36, 0.40 },
      lit = { 0.48, 0.50, 0.55 },
      seed = 3,
    },
  },

  weight = 3,
})
