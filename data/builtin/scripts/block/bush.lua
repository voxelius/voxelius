blocks.add("bush", {
  render = blocks.RENDER_ALPHA,

  albedo = {
    default = {
      "bush/bush_01.png"
    }
  },

  model_name = "bush",
  bcoll_name = "bush",

  collidable = false,
  selectable = true,

  replaceable = true,

  tags = { blocks.TAG_XFOIL }
})
