package main

import "base:runtime"
import "core:math"
import "core:math/linalg"
import "core:slice"

Bowyer_Watson :: struct {
    triangulation: [dynamic][3][2]f32,
    super_triangle: [3][2]f32
}

get_super_triangle :: proc(in_points: [][2]f32) -> (sp_points: [3][2]f32) {
    pos_min: [2]f32 = { math.INF_F32, math.INF_F32 }
    pos_max: [2]f32 = { -math.INF_F32, -math.INF_F32 }
    for p in in_points {
        pos_min = linalg.min(pos_min, p)
        pos_max = linalg.max(pos_max, p)
    }
    center := (pos_min + pos_max) * 0.5
    delta := max(pos_max.x - pos_min.x, pos_max.y - pos_min.y)

    sp_points = {
        { center.x - 2.0 * delta, center.y - delta },
        { center.x,               center.y + 2.0 * delta },
        { center.x + 2.0 * delta, center.y - delta },
    }
    return sp_points
}

triangle_orientation :: proc(triangle: [3][2]f32) -> f32 {
    cross := (
        (triangle[1].x - triangle[0].x) * (triangle[2].y - triangle[0].y) -
        (triangle[1].y - triangle[0].y) * (triangle[2].x - triangle[0].x)
    )
    return math.sign(cross)
}

canonical_edge :: proc(a, b: [2]f32) -> [2][2]f32 {
    if a.x < b.x || (a.x == b.x && a.y < b.y) {
        return {a, b}
    }
    return {b, a}
}

in_circumcircle :: proc(triangle: [3][2]f32, point: [2]f32) -> bool {
    a := triangle[0] - point
    b := triangle[1] - point
    c := triangle[2] - point
    a_sq := a*a
    b_sq := b*b
    c_sq := c*c
    mat: matrix[3, 3]f32 = {
        a.x, a.y, a_sq.x+a_sq.y,
        b.x, b.y, b_sq.x+b_sq.y,
        c.x, c.y, c_sq.x+c_sq.y,
    }
    det := linalg.determinant(mat)
    ori := triangle_orientation(triangle)
    if ori > 0 {
        return det > 0
    } else if ori < 0 {
        return det < 0
    }
    return false
}

bowyer_watson_add_point :: proc(bw: ^Bowyer_Watson, point: [2]f32) {
    bad_triangles: [dynamic][3][2]f32
    polygon:       [dynamic][2][2]f32
    defer delete(bad_triangles)
    defer delete(polygon)
    for triangle in bw.triangulation {
        if in_circumcircle(triangle, point) {
            append(&bad_triangles, triangle)
        }
    }
    for bad_triangle0, idx0 in bad_triangles {
        edges0: [][2][2]f32 = {
            canonical_edge(bad_triangle0[0], bad_triangle0[1]),
            canonical_edge(bad_triangle0[1], bad_triangle0[2]),
            canonical_edge(bad_triangle0[2], bad_triangle0[0]),
        }
        for edge0 in edges0 {
            edge_used: bool
            bad_triangle1_loop: for bad_triangle1, idx1 in bad_triangles {
                if idx0 == idx1 do continue
                edges1: [][2][2]f32 = {
                    canonical_edge(bad_triangle1[0], bad_triangle1[1]),
                    canonical_edge(bad_triangle1[1], bad_triangle1[2]),
                    canonical_edge(bad_triangle1[2], bad_triangle1[0]),
                }
                for edge1 in edges1 {
                    if edge0 == edge1 {
                        edge_used = true
                        break bad_triangle1_loop
                    }
                }
            }
            if !edge_used {
                append(&polygon, edge0)
            }
        }
    }
    for bad_triangle in bad_triangles {
        for idx := len(bw.triangulation)-1; idx >= 0; idx -= 1 {
            if bw.triangulation[idx] == bad_triangle {
                runtime.ordered_remove(&bw.triangulation, idx)
            }
        }
    }
    for edge in polygon {
        triangle: [3][2]f32 = {
            edge[0],
            edge[1],
            point,
        }
        if triangle_orientation(triangle) < 0 {
            triangle[1], triangle[2] = triangle[2], triangle[1]
        }
        append(&bw.triangulation, triangle)
    }
}

bowyer_watson_cleanup :: proc(bw: ^Bowyer_Watson) {
    for idx := len(bw.triangulation)-1; idx >= 0; idx -= 1 {
        for super_triangle_vertex in bw.super_triangle {
            if slice.contains(bw.triangulation[idx][:], super_triangle_vertex) {
                runtime.ordered_remove(&bw.triangulation, idx)
                break
            }
        }
    }
}
