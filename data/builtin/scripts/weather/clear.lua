weather.add("clear", {
  realms = { weather.REALM_SURFACE, weather.REALM_SKY },

  intensity = 0.22,
  sky_tint = { 0.42, 0.60, 0.92 },
  light = 1.0,
  wind = 1.0,

  layers = {
    {
      height = 196,
      thickness = 5,
      opacity = 0.85,
      scale = 192,
      coverage = 0.6,
      shadow = { 0.82, 0.84, 0.88 },
      lit = { 0.97, 0.98, 1.00 },
      seed = 31,
    },
  },

  weight = 10,
})
