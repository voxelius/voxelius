# Block Collisions

Block collisions define the physical shape for touch and movement response. These models are separate from render geometry.

Lighting reads the same shape: a block only absorbs light across the cell faces the collision covers. A shape that fills half a cell, such as a slab, leaves the other half open for light.

Block collisions live under: `namespace:collisions/block`

## Top-level fields

|Field|Type|Required|Default|Description|
|----|----|----|----|----|
|`elements`|`object[]`|yes|N/D|List of axis-aligned boxes that form the model|

## Element object

|Field|Type|Required|Default|Description|
|----|----|----|----|----|
|`min`|`number[3]`|yes|N/D|AABB start in 1/16ths of a block|
|`max`|`number[3]`|yes|N/D|AABB end in 1/16ths of a block|

## Examples

### Example: cube collision

```json
{
  "elements": [
    {
      "min": [0, 0, 0],
      "max": [16, 16, 16]
    }
  ]
}
```

### Example: slab collision

```json
{
  "elements": [
    {
      "min": [0, 0, 0],
      "max": [16, 8, 16]
    }
  ]
}
```

### Example: stairs collision

```json
{
  "elements": [
    {
      "min": [0, 0, 0],
      "max": [16, 8, 16]
    },
    {
      "min": [0, 8, 0],
      "max": [8, 16, 16],
    }
  ]
}
```
