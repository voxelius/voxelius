worlds.add("superflat", {
  directory = "superflat",
  generator = "superflat",
  spawn = { 0, 1, 0 },

  biome_selector = {
    strategy = worlds.SELECTION_CONSTANT,
    biome = "surface_plains",
  },

  weather = {
    "clear",
    "overcast",
    "rain",
    "thunderstorm",
  },
})
