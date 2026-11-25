blocks.add("dirt", {
  render = blocks.RENDER_SOLID,
  animated = false,

  albedo = {
    default = {
      "dirt/dirt_01.png",
      "dirt/dirt_02.png",
      "dirt/dirt_03.png",
      "dirt/dirt_04.png",
    }
  },

  model_name = "cube",
  bcoll_name = "cube",

  health = 1,
  tools = { blocks.TOOL_SHOVEL },

  emission = 0,
  dissipation = 15,

  drops = {
    {
      items = {
        {
          name = "dirt",
          count = 1
        }
      }
    }
  },
})
