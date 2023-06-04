
## P3D
```c
float scale;
uint32_t num_vertex;
uint16_t position[3*num_vertex];
uint16_t normal[3*num_vertex];
uint16_t texcoord[2*num_vertex];
uint32_t color[num_vertex];

uint32_t num_index;
uint32_t index[num_index];

uint_16_t num_meshes;
struct p3d_mesh {
  uint32_t num_index;
  uint32_t index_offset;

  uint8_t has_diffuse_color;
  uint32_t diffuse_color;

  uint8_t has_diffuse_texture;
  uint8_t diffuse_texture_name_length;
  char *diffuse_texture_name;
}
struct p3d_mesh meshes[num_meshes];
```

## PP3D
```
[scale]
[# of meshes]
  [vertex type]
  [vertices] (size dependent on vertex type)
  [indices] (size dependent on vertex type)
  [has diffuse color?]
    [diffuse color]
  TODO: diffuse color map
```