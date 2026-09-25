package main

import "core:fmt"
import "core:os"
import "core:container/xar"
import "core:math"
import "core:math/linalg"

import rl "vendor:raylib"

import "p:tokenizer"
import "p:parser"

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

        it := xar.iterator(&file.planes)
        for plane in xar.iterate_by_ptr(&it) {
            origin: [3]f32
            vertices: [3][3]f32
            for v_int, v_idx in plane.vertices {
                vertices[v_idx] = {
                    f32(v_int.x) / 32.0,
                    f32(v_int.z) / 32.0,
                    f32(v_int.y) / 32.0
                }
                origin += vertices[v_idx]/3.0
            }
            normal := linalg.normalize(linalg.cross(vertices[1] - vertices[0], vertices[2] - vertices[0]))
            fmt.println(origin, normal)
            // rl.DrawTriangleStrip3D(raw_data(&vertices), 3, rl.RED)
            rl.DrawCubeV(origin, { 0.1, 0.1, 0.1 }, rl.RED)
            rl.DrawLine3D(origin, origin+normal, rl.RED)
        }

        rl.DrawGrid(10, 1.0)
        rl.EndMode3D()
        rl.EndDrawing()
        break
    }
    rl.CloseWindow()

}

test_math :: proc() {
    Plane :: struct {
        p: [3]f32,
        n: [3]f32,
        d: f32,
    }
    planes := []Plane{
        { p = { 1, 0, 0 }, n = { 1, 0, 0 } },
        { p = { 0, 1, 0 }, n = { 0, 1, 0 } },
        { p = { 0, 0, 1 }, n = { 0, 0, 1 } },
        { p = { -1, 0, 0 }, n = { -1, 0, 0 } },
        { p = { 0, -1, 0 }, n = { 0, -1, 0 } },
        { p = { 0, 0, -1 }, n = { 0, 0, -1 } },
        { p = { 0.75, 0.75, 0.75 }, n = linalg.normalize([3]f32{ 0.75, 0.75, 0.75 }) }
    }
    for &plane in planes {
        plane.d = plane.p.x * plane.n.x + plane.p.y * plane.n.y + plane.p.z * plane.n.z
    }

    vertices: [dynamic][3]f32
    for idx0 in 0..<len(planes) {
        plane0 := &planes[idx0]
        for idx1 in idx0+1..<len(planes) {
            plane1 := &planes[idx1]
            for idx2 in idx1+1..<len(planes) {
                plane2 := &planes[idx2]
                mat: matrix[3, 3]f32 = {
                    plane0.n.x, plane0.n.y, plane0.n.z,
                    plane1.n.x, plane1.n.y, plane1.n.z,
                    plane2.n.x, plane2.n.y, plane2.n.z,
                }
                det := linalg.matrix3x3_determinant(mat)
                if det == 0 do continue
                rhs: [3]f32 = { plane0.d, plane1.d, plane2.d }
                res := linalg.inverse(mat) * rhs
                // def inside(x, planes, eps=1e-8):
                //     return all(np.dot(n, x) <= d + eps for n, d in planes)
                inside := true
                for &plane in planes {
                    if linalg.dot(plane.n, res) > plane.d + math.F32_EPSILON {
                        inside = false
                        break
                    }
                }
                if inside do append(&vertices, res)
            }
        }
    }


    rl.SetTraceLogLevel(.WARNING)
    rl.InitWindow(1200, 720, "test")

    camera: rl.Camera
    camera.position = { 10.0, 10.0, 10.0 }
    camera.target = {}
    camera.up = { 0.0, 1.0, 0.0 }
    camera.fovy = 45.0
    camera.projection = .PERSPECTIVE

    rl.DisableCursor()
    rl.SetTargetFPS(144)

    for !rl.WindowShouldClose() {
        rl.UpdateCamera(&camera, .THIRD_PERSON)
        rl.BeginDrawing()
        rl.ClearBackground({20, 20, 20, 255})
        rl.BeginMode3D(camera)
        for v in vertices {
            rl.DrawCubeV(v, {0.1, 0.1, 0.1}, rl.RED)
        }

        rl.DrawGrid(10, 1.0)
        rl.EndMode3D()
        rl.EndDrawing()
    }
    rl.CloseWindow()
}

main :: proc() {
    // test_tokenizer()
    // test_parser()
    // test_graphical()
    test_math()
}

