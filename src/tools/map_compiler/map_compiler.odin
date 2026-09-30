package main

import "base:runtime"
import "core:fmt"
import "core:os"
import "core:container/xar"
import "core:math"
import "core:math/linalg"
import "core:slice"
import "core:math/rand"
import "core:sort"

import rl "vendor:raylib"

import "p:tokenizer"
import "p:parser"
import "p:entity"

test_tokenizer :: proc() {
    filename := "./res/trenchbroom/test_map.map"
    data, err := os.read_entire_file_from_path(filename, context.allocator)
    t: tokenizer.Tokenizer
    tokenizer.init(&t, string(data), filename)
    for {
        token := tokenizer.scan(&t)
        fmt.println(token)
        if token.kind == .EOF {
            break
        }
    }
}

test_parser :: proc() {
    filename := "./res/trenchbroom/test_map.map"
    data, err := os.read_entire_file_from_path(filename, context.allocator)
    file := new(parser.File)
    defer free(file)
    defer xar.destroy(&file.entities)
    defer xar.destroy(&file.kvps)
    defer xar.destroy(&file.brushes)
    defer xar.destroy(&file.planes)
    defer xar.destroy(&file.vertices)

    file.fullpath = filename
    file.src = string(data)
    p := parser.default_parser()
    parser.parse_file(&p, file)

    it := xar.iterator(&file.entities)
    for entity, entity_idx in xar.iterate_by_ptr(&it) {
        fmt.println("entity", entity_idx)
        for i in entity.kvps[0]..<entity.kvps[1] {
            kvp := xar.array_get_ptr_unsafe(&file.kvps, i)
            fmt.println(" ", kvp.left, " ", kvp.right, sep="")
        }
        for i in entity.brushes[0]..<entity.brushes[1] {
            fmt.println(" brush", i-entity.brushes[0])
            brush := xar.array_get_ptr_unsafe(&file.brushes, i)
            for j in brush.planes[0]..<brush.planes[1] {
                plane := xar.array_get_ptr_unsafe(&file.planes, i)
                fmt.printfln(
                    "  %v %v %v %v %v %v %v",
                    plane.vertices[0],
                    plane.vertices[1],
                    plane.vertices[2],
                    plane.texturename,
                    plane.uv_offset,
                    plane.rotation,
                    plane.uv_scale
                )
            }
        }
    }

}

calculate_brush_vertices :: proc(brush: ^entity.Brush, file: ^parser.File) {
    vertices_start := xar.array_len(file.vertices)
    vertices_count: int
    for idx0 in brush.planes[0]..<brush.planes[1] {
        plane0 := xar.array_get_ptr_unsafe(&file.planes, idx0)
        for idx1 in idx0+1..<brush.planes[1] {
            plane1 := xar.array_get_ptr_unsafe(&file.planes, idx1)
            for idx2 in idx1+1..<brush.planes[1] {
                plane2 := xar.array_get_ptr_unsafe(&file.planes, idx2)
                mat: matrix[3, 3]f32 = {
                    plane0.n.x, plane0.n.y, plane0.n.z,
                    plane1.n.x, plane1.n.y, plane1.n.z,
                    plane2.n.x, plane2.n.y, plane2.n.z,
                }
                det := linalg.matrix3x3_determinant(mat)
                if det == 0 do continue
                rhs: [3]f32 = { plane0.d, plane1.d, plane2.d }
                res := linalg.inverse(mat) * rhs
                inside := true
                for idx_dup in brush.planes[0]..<brush.planes[1] {
                    plane_dup := xar.array_get_ptr_unsafe(&file.planes, idx_dup)
                    if linalg.dot(plane_dup.n, res) > (plane_dup.d + 0.00001) {
                        inside = false
                        break
                    }
                }
                if inside {
                    // append(&vertices, res)
                    vertex := entity.Vertex{
                        position = res
                    }
                    _ = xar.array_push_back_elem(&file.vertices, vertex) or_else panic("alloc")
                    vertices_count += 1
                }
            }
        }
    }
    brush.vertices = { vertices_start, vertices_start+vertices_count }
}

test_graphical :: proc() {
    filename := "./res/trenchbroom/test_map.map"
    data, err := os.read_entire_file_from_path(filename, context.allocator)
    file := new(parser.File)
    defer free(file)
    defer xar.destroy(&file.entities)
    defer xar.destroy(&file.kvps)
    defer xar.destroy(&file.brushes)
    defer xar.destroy(&file.planes)

    file.fullpath = filename
    file.src = string(data)
    p := parser.default_parser()
    parser.parse_file(&p, file)

    brush_it := xar.iterator(&file.brushes)
    for brush in xar.iterate_by_ptr(&brush_it) {
        calculate_brush_vertices(brush, file)
    }

    rl.SetTraceLogLevel(.WARNING)
    rl.InitWindow(1200, 720, "test")

    camera: rl.Camera
    camera.position = { 10.0, 10.0, 10.0 }
    camera.target = {}
    camera.up = { 0.0, 1.0, 0.0 }
    camera.fovy = 45.0
    camera.projection = .PERSPECTIVE

    cube_position: [3]f32 = { 0.0, 0.0, 0.0 }

    rl.DisableCursor()
    rl.SetTargetFPS(144)

    for !rl.WindowShouldClose() {
        rl.UpdateCamera(&camera, .THIRD_PERSON)
        rl.BeginDrawing()
        rl.ClearBackground({20, 20, 20, 255})
        rl.BeginMode3D(camera)

        rl.DrawLine3D({}, {1, 0, 0}, rl.RED)
        rl.DrawLine3D({}, {0, 1, 0}, rl.GREEN)
        rl.DrawLine3D({}, {0, 0, 1}, rl.BLUE)

        vertex_it := xar.iterator(&file.vertices)
        for vertex in xar.iterate_by_ptr(&vertex_it) {
            rl.DrawCubeV(vertex.position, { 0.1, 0.1, 0.1 }, rl.WHITE)
        }

        // rl.DrawGrid(10, 1.0)
        rl.EndMode3D()
        rl.EndDrawing()
    }
    rl.CloseWindow()
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

canonical_edge :: proc(a, b: [2]f32) -> [2][2]f32 {
    if a.x < b.x || (a.x == b.x && a.y < b.y) {
        return {a, b}
    }
    return {b, a}
}

Bowyer_Watson :: struct {
    triangulation: [dynamic][3][2]f32,
    super_triangle: [3][2]f32
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

test_bowyer_watson :: proc() {
    // rand.reset_u64(213123)
    DESIRED_POINT_COUNT :: 1000
    points: [dynamic][2]f32
    for i in 0..<DESIRED_POINT_COUNT {
        point: [2]f32 = {
            math.round(rand.float32_range(-350, +350.0)),
            math.round(rand.float32_range(-200.0, +200.0)),
        }
        append(&points, point)
    }

    origin: [2]f32
    for p in points {
        origin += p / f32(len(points))
    }

    bw: Bowyer_Watson
    bw.super_triangle = get_super_triangle(points[:])
    append(&bw.triangulation, bw.super_triangle)

    rl.SetTraceLogLevel(.WARNING)
    rl.InitWindow(1200, 720, "test")
    rl.SetTargetFPS(144)

    camera: rl.Camera2D
    camera.target = linalg.round(origin)
    camera.offset = { 1200.0/2, 720.0/2 }
    camera.zoom = 0.9

    points_added: int

    for !rl.WindowShouldClose() {
        camera.zoom += rl.GetMouseWheelMove() * 0.25
        camera.zoom = clamp(camera.zoom, 0.1, 40.0)

        if rl.IsKeyDown(.D) {
            camera.target.x += 1.0
        }
        if rl.IsKeyDown(.A) {
            camera.target.x -= 1.0
        }
        if rl.IsKeyDown(.S) {
            camera.target.y += 1.0
        }
        if rl.IsKeyDown(.W) {
            camera.target.y -= 1.0
        }

        if rl.IsKeyDown(.SPACE) {
            switch {
                case points_added < len(points):
                    bowyer_watson_add_point(&bw, points[points_added])
                    points_added += 1
                case points_added == len(points):
                    bowyer_watson_cleanup(&bw)
                    points_added += 1
            }
        }

        rl.BeginDrawing()
        rl.ClearBackground({0, 0, 0, 255})
        rl.BeginMode2D(camera)

        color_a := rl.Color{ 34, 41, 38, 255 }
        color_b := rl.Color{ 113, 227, 176, 255 }
        for triangle, idx in bw.triangulation {
            a: f32 = f32(idx+1)/f32(len(bw.triangulation))
            color: rl.Color = {expand_values(linalg.array_cast(
                linalg.lerp(
                    linalg.array_cast(color_a, f32),
                    linalg.array_cast(color_b, f32),
                    [4]f32{ a, a, a, a }
                ),
                u8
            ))}
            rl.DrawLineV(triangle[0]*{1,-1}, triangle[1]*{1,-1}, color)
            rl.DrawLineV(triangle[1]*{1,-1}, triangle[2]*{1,-1}, color)
            rl.DrawLineV(triangle[2]*{1,-1}, triangle[0]*{1,-1}, color)
        }

        rl.EndMode2D()
        rl.EndDrawing()
    }
    rl.CloseWindow()
}

main :: proc() {
    // test_tokenizer()
    // test_parser()
    // test_graphical()
    test_bowyer_watson()
}

