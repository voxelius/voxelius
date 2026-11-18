weather.add("thunderstorm", {
  realms = { weather.REALM_SURFACE },

  intensity = 1.0,
  sky_tint = { 0.14, 0.16, 0.19 },
  sky_visibility = 0.05,
  light = 0.35,
  wind = 3.0,

  -- The deck has to stay under the sky tint; at the old values it came out
  -- brighter than a storm sky and the storm lost its ceiling.
  layers = {
    {
      height = 160,
      thickness = 8,
      opacity = 0.95,
      scale = 112,
      coverage = 1.0,
      shadow = { 0.10, 0.11, 0.13 },
      lit = { 0.17, 0.18, 0.20 },
      seed = 19,
    },
    {
      height = 190,
      thickness = 12,
      opacity = 0.90,
      scale = 176,
      coverage = 1.0,
      shadow = { 0.12, 0.13, 0.16 },
      lit = { 0.20, 0.21, 0.24 },
      seed = 23,
    },
    {
      height = 220,
      thickness = 15,
      opacity = 0.85,
      scale = 224,
      coverage = 0.9,
      shadow = { 0.14, 0.15, 0.18 },
      lit = { 0.23, 0.24, 0.28 },
      seed = 29,
    },
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

  weight = 2,
})
