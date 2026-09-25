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
}

Plane :: struct {
    vertices: [3][3]int,
    texturename: string,
    uv_offset: [2]int,
    rotation: int,
    uv_scale: [2]int,
}