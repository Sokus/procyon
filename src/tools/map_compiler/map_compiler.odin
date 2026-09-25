package main

import "core:fmt"
import "core:os"
import "core:container/xar"
import "core:math"
import "core:math/linalg"

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

main :: proc() {
    // test_tokenizer()
    // test_parser()
    test_graphical()
}

