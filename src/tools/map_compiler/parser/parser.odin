package parser

import "core:fmt"
import "core:container/xar"
import "core:strconv"

import "../tokenizer"

Warning_Handler :: #type proc(pos: tokenizer.Pos, fmt: string, args: ..any)
Error_Handler   :: #type proc(pos: tokenizer.Pos, fmt: string, args: ..any)

File :: struct {
    fullpath: string,
    src:  string,

    entities: xar.Array(Entity, 4),
    kvps: xar.Array(KeyValuePair, 4),
    brushes: xar.Array(Brush, 4),
    planes: xar.Array(Plane, 4),

    syntax_warning_count: int,
    syntax_error_count: int,
}

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

Parser :: struct {
    file: ^File,
    tok: tokenizer.Tokenizer,

    warn: Warning_Handler,
    err: Error_Handler,

    prev_tok: tokenizer.Token,
    curr_tok: tokenizer.Token,

    expr_level: int,

    error_count: int,

    fix_count: int,
    fix_prev_pos: tokenizer.Pos,

    peeking: bool,
}

MAX_FIX_COUNT :: 10

default_warning_handler :: proc(pos: tokenizer.Pos, msg: string, args: ..any) {
    fmt.eprintf("%s(%d:%d): Warning: ", pos.file, pos.line, pos.column)
    fmt.eprintf(msg, ..args)
    fmt.eprintf("\n")
}
default_error_handler :: proc(pos: tokenizer.Pos, msg: string, args: ..any) {
    fmt.eprintf("%s(%d:%d): ", pos.file, pos.line, pos.column)
    fmt.eprintf(msg, ..args)
    fmt.eprintf("\n")
}

warn :: proc(p: ^Parser, pos: tokenizer.Pos, msg: string, args: ..any) {
    if p.warn != nil {
        p.warn(pos, msg, ..args)
    }
    p.file.syntax_warning_count += 1
}

error :: proc(p: ^Parser, pos: tokenizer.Pos, msg: string, args: ..any) {
    if p.err != nil {
        p.err(pos, msg, ..args)
    }
    p.file.syntax_error_count += 1
    p.error_count += 1
}

end_pos :: proc(tok: tokenizer.Token) -> tokenizer.Pos {
    pos := tok.pos
    pos.offset += len(tok.text)
    pos.column += len(tok.text)
    return pos
}

default_parser :: proc() -> Parser {
    return Parser{
        warn = default_warning_handler,
        err = default_error_handler,
    }
}

next_token0 :: proc(p: ^Parser) -> bool {
    p.curr_tok = tokenizer.scan(&p.tok)
    if p.curr_tok.kind == .EOF {
        return false
    }
    return true
}

consume_comment :: proc(p: ^Parser) {
    if p.curr_tok.kind == .Comment do next_token0(p)
    return
}

advance_token :: proc(p: ^Parser) -> tokenizer.Token {
    p.prev_tok = p.curr_tok
    prev := p.prev_tok
    if next_token0(p) {
        consume_comment(p)
    }
    return prev;
}

peek_token_kind :: proc(p: ^Parser, kind: tokenizer.Token_Kind, lookahead := 0) -> (ok: bool) {
    prev_parser := p^
    p.peeking = true

    defer {
        p^ = prev_parser
        p.peeking = false
    }

    p.tok.err = nil
    for i := 0 ; i <= lookahead; i += 1 {
        advance_token(p)
    }
    ok = p.curr_tok.kind == kind
    return
}

peek_token :: proc(p: ^Parser, lookahead := 0) -> (tok: tokenizer.Token) {
    prev_parser := p^
    p.peeking = true

    defer {
        p^ = prev_parser
        p.peeking = false
    }

    p.tok.err = nil
    for i := 0; i <= lookahead; i += 1 {
        advance_token(p)
    }
    tok = p.curr_tok
    return
}

expect_token :: proc(p: ^Parser, kind: tokenizer.Token_Kind) -> tokenizer.Token {
    prev := p.curr_tok
    if prev.kind != kind {
        e := tokenizer.token_kind_to_string(kind)
        g := tokenizer.token_kind_to_string(prev.kind)
        error(p, prev.pos, "expected '%s', got '%s'", e, g)
    }
    advance_token(p)
    return prev
}

expect_operator :: proc(p: ^Parser) -> tokenizer.Token {
    prev := p.curr_tok
    if !tokenizer.is_operator(prev.kind) {
        g := tokenizer.token_kind_to_string(prev.kind)
        error(p, prev.pos, "expected an operator, got '%s'", g)
    }
    advance_token(p)
    return prev
}

allow_token :: proc(p: ^Parser, kind: tokenizer.Token_Kind) -> bool {
    if p.curr_tok.kind == kind {
        advance_token(p)
        return true
    }
    return false
}

parse_file :: proc(p: ^Parser, file: ^File) -> bool {
    zero_parser: {
        p.prev_tok = {}
        p.curr_tok = {}
        p.expr_level = 0
    }

    p.file = file
    tokenizer.init(&p.tok, file.src, file.fullpath, p.err)
    if p.tok.ch <= 0 {
        return true
    }

    advance_token(p)

    for p.curr_tok.kind != .EOF {
        skip_comments(p)
        entity := parse_entity(p)
        if entity != nil {
            fmt.print(entity)
        }
    }

    return true
}

skip_comments :: proc(p: ^Parser) {
    for p.curr_tok.kind == .Comment {
        advance_token(p)
    }
}

parse_entity :: proc(p: ^Parser) -> ^Entity {
    expect_token(p, .Open_Brace)
    entity := xar.push_back_elem_and_get_ptr(&p.file.entities, Entity{}) or_else panic("alloc")
    kvps_start := xar.array_len(p.file.kvps)
    brushes_start := xar.array_len(p.file.brushes)
    kvps_count, brushes_count: int
    for p.curr_tok.kind != .Close_Brace && p.curr_tok.kind != .EOF {
        #partial switch p.curr_tok.kind {
            case .String:
                kvp := parse_keyvaluepair(p)
                if kvp != nil {
                    kvps_count += 1
                }
            case .Open_Brace:
                brush := parse_brush(p)
                if brush != nil {
                    brushes_count += 1
                }
            case:
                fix_advance_to_next_entity(p)
        }
    }
    entity.kvps = { kvps_start, kvps_start+kvps_count }
    entity.brushes = { brushes_start , brushes_start+brushes_count }
    expect_token(p, .Close_Brace)
    return nil
}
fix_advance_to_next_entity :: proc(p: ^Parser) {
    for {
        tok := p.curr_tok
        #partial switch t := p.curr_tok; t.kind {
        case .EOF, .Newline:
            return
        }
        advance_token(p)
    }
}

parse_keyvaluepair :: proc(p: ^Parser) -> ^KeyValuePair {
    left := expect_token(p, .String)
    right := expect_token(p, .String)
    if left.kind == .String && right.kind == .String {
        kvp := xar.push_back_elem_and_get_ptr(&p.file.kvps, KeyValuePair{
            left = left.text,
            right = right.text
        }) or_else panic("alloc")
        return kvp
    } else {
        return nil
    }
}

parse_brush :: proc(p: ^Parser) -> ^Brush {
    expect_token(p, .Open_Brace)
    brush := xar.push_back_elem_and_get_ptr(&p.file.brushes, Brush{}) or_else panic("alloc")
    planes_start := xar.array_len(p.file.planes)
    planes_count: int
    for p.curr_tok.kind != .Close_Brace && p.curr_tok.kind != .EOF {
        if p.curr_tok.kind == .Open_Paren {
            plane := parse_plane(p)
            if plane != nil {
                planes_count += 1
            }
        } else {
            tok := advance_token(p)
            error(p, tok.pos, "expected a plane, got %s", tokenizer.token_kind_to_string(tok.kind))
            fix_advance_to_next_entity(p)
        }
    }
    expect_token(p, .Close_Brace)
    brush.planes = { planes_start, planes_start+planes_count }
    return brush
}

parse_plane :: proc(p: ^Parser) -> ^Plane {
    plane := xar.push_back_elem_and_get_ptr(&p.file.planes, Plane{}) or_else panic("alloc")
    for v in 0..<3 {
        expect_token(p, .Open_Paren)
        for e in 0..<3 {
            tok_int := expect_token(p, .Integer)
            tok_v := strconv.parse_int(tok_int.text) or_continue
            plane.vertices[v][e] = tok_v
        }
        expect_token(p, .Close_Paren)
    }

    tok_id := expect_token(p, .Identifier)
    plane.texturename = tok_id.text

    for i in 0..<2 {
        tok_int := expect_token(p, .Integer)
        v := strconv.parse_int(tok_int.text) or_continue
        plane.uv_offset[i] = v
    }

    {
        tok_int := expect_token(p, .Integer)
        v, ok := strconv.parse_int(tok_int.text)
        if ok do plane.rotation = v
    }

    for i in 0..<2 {
        tok_int := expect_token(p, .Integer)
        v := strconv.parse_int(tok_int.text) or_continue
        plane.uv_scale[i] = v
    }

    return plane
}