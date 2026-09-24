package main

import "core:fmt"
import "core:os"
import "core:container/xar"

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

main :: proc() {
    // test_tokenizer()
    test_parser()
}