blocks.add("fern", {
  render = blocks.RENDER_ALPHA,

  albedo = {
    default = {
      "fern/fern_01.png"
    }
  },

  model_name = "bush",
  bcoll_name = "bush",

  collidable = false,
  selectable = true,

  replaceable = true,

  tags = { blocks.TAG_XFOIL }
})
