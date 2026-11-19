weather.add("snow", {
  realms = { weather.REALM_SKY },

  intensity = 0.7,
  sky_tint = { 0.60, 0.65, 0.75 },
  sky_visibility = 0.55,
  light = 0.80,
  wind = 0.8,

  cloud = {
    -- realm_sky.cc keeps sky islands inside y = 426: 320 base + 96 bias + 10 density;
    -- the deck sits just above that envelope
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

  weight = 2,
})
