package entity

Entity :: struct {
    kvps: [2]int,
    brushes: [2]int,
}

KeyValuePair :: struct {
    left: string,
    right: string,
}

Brush :: struct {
    planes: [2]int,
    vertices: [2]int,
}

Plane :: struct {
    vertices: [3][3]f32,
    texturename: string,
    uv_offset: [2]f32,
    rotation: int,
    uv_scale: [2]int,

    // calculated:
    n: [3]f32,
    d: f32
}

Vertex :: struct {
    plane: int,
    position: [3]f32,
}