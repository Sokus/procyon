package tokenizer

import "core:fmt"
import "core:unicode"
import "core:unicode/utf8"

Error_Handler :: #type proc(pos: Pos, fmt: string, args: ..any)

Tokenizer :: struct {
    path: string,
    src: string,
    err: Error_Handler,

    // tokenizing state
    ch: rune,
    offset: int,
    read_offset: int,
    line_offset: int,
    line_count: int,

    error_count: int
}

init :: proc(t: ^Tokenizer, src: string, path: string, err: Error_Handler = default_error_handler) {
    t.path = path
    t.src = src
    t.err = err
    t.ch = ' '
    t.offset = 0
    t.read_offset = 0
    t.line_count = 0

    advance_rune(t)
    if t.ch == utf8.RUNE_BOM {
        advance_rune(t)
    }
}

@(private)
offset_to_pos :: proc(t: ^Tokenizer, offset: int) -> Pos {
    line := t.line_count
    column := offset - t.line_offset + 1

    return Pos {
        file = t.path,
        offset = offset,
        line = line,
        column = column,
    }
}

default_error_handler :: proc(pos: Pos, msg: string, args: ..any) {
    fmt.eprintf("%s(%d:%d) ", pos.file, pos.line, pos.column);
    fmt.eprintf(msg, ..args)
    fmt.eprintf("\n")
}


error :: proc(t: ^Tokenizer, offset: int, msg: string, args: ..any) {
    pos := offset_to_pos(t, offset)
    if t.err != nil {
        t.err(pos, msg, ..args)
    }
    t.error_count += 1
}

advance_rune :: proc(t: ^Tokenizer) {
	if t.read_offset < len(t.src) {
		t.offset = t.read_offset
		if t.ch == '\n' {
			t.line_offset = t.offset
			t.line_count += 1
		}
		r, w := rune(t.src[t.read_offset]), 1
		switch {
		case r == 0:
			error(t, t.offset, "illegal character NUL")
		case r >= utf8.RUNE_SELF:
			r, w = utf8.decode_rune_in_string(t.src[t.read_offset:])
			if r == utf8.RUNE_ERROR && w == 1 {
				error(t, t.offset, "illegal UTF-8 encoding")
			} else if r == utf8.RUNE_BOM && t.offset > 0 {
				error(t, t.offset, "illegal byte order mark")
			}
		}
		t.read_offset += w
		t.ch = r
	} else {
		t.offset = len(t.src)
		if t.ch == '\n' {
			t.line_offset = t.offset
			t.line_count += 1
		}
		t.ch = -1
	}
}

peek_byte :: proc(t: ^Tokenizer, offset := 0) -> byte {
    if t.read_offset+offset < len(t.src) {
        return t.src[t.read_offset+offset]
    }
    return 0
}

skip_whitespace :: proc(t: ^Tokenizer) {
    for {
        switch t.ch {
        case ' ', '\t', '\r', '\n':
            advance_rune(t)
        case:
            return
        }
    }
}

is_letter :: proc(r: rune) -> bool {
    if r < utf8.RUNE_SELF {
        switch r {
        case '_':
            return true
        case 'A'..='Z', 'a'..='z':
            return true
        }
    }
    return unicode.is_letter(r)
}

is_digit :: proc(r: rune) -> bool {
    if '0' <= r && r <= '9' {
        return true
    }
    return unicode.is_digit(r)
}

digit_value :: proc(r: rune) -> int {
    switch r {
    case '0'..='9':
        return int(r-'0')
    case 'A'..='F':
        return int(r-'A' + 10)
    case 'a'..='f':
        return int(r-'a' + 10)
    }
    return 16
}

scan_comment :: proc(t: ^Tokenizer) -> string {
    if t.ch == '/' {
        advance_rune(t)
    } else {
        error(t, t.offset, "missing '/' to be a comment")
    }
    skip_whitespace(t)
    offset := t.offset
    for t.ch != '\n' && t.ch != '\r' && t.ch >= 0 {
        advance_rune(t)
    }
    return string(t.src[offset : t.offset])
}

scan_number :: proc(t: ^Tokenizer) -> (Token_Kind, string) {
    scan_mantissa :: proc(t: ^Tokenizer, base: int) {
        for digit_value(t.ch) < base {
            advance_rune(t)
        }
    }
    scan_exponent :: proc(t: ^Tokenizer, kind: ^Token_Kind) {
        if t.ch == 'e' || t.ch == 'E' {
            kind^ = .Float
            advance_rune(t)
            if t.ch == '-' || t.ch == '+' {
                advance_rune(t)
            }
            if digit_value(t.ch) < 10 {
                scan_mantissa(t, 10)
            } else {
                error(t, t.offset, "illegal floating-point exponent")
            }
        }
    }
    scan_fraction :: proc(t: ^Tokenizer, kind: ^Token_Kind) -> (early_exit: bool) {
        if t.ch == '.' && peek_byte(t) == '.' {
            return true
        }
        if t.ch == '.' {
            kind^ = .Float
            advance_rune(t)
            scan_mantissa(t, 10)
        }
        return false
    }

    offset := t.offset
    kind := Token_Kind.Integer
    seen_point: bool

    if t.ch == '0' {
        int_base :: proc(t: ^Tokenizer, kind: ^Token_Kind, base: int, msg: string) {
            prev := t.offset
            advance_rune(t)
            scan_mantissa(t, base)
            if t.offset - prev <= 1 {
                kind^ = .Invalid
                error(t, t.offset, msg)
            }
        }

        advance_rune(t)
        switch t.ch {
        case 'x':
            int_base(t, &kind, 16, "illegal hexadecimal integer")
        case:
            seen_point = false
            scan_mantissa(t, 10)
            if t.ch == '.' {
                seen_point = true
                if scan_fraction(t, &kind) {
                    return kind, string(t.src[offset : t.offset])
                }
            }
            scan_exponent(t, &kind)
            return kind, string(t.src[offset : t.offset])
        }
    }

    scan_mantissa(t, 10)
    if scan_fraction(t, &kind) {
        return kind, string(t.src[offset : t.offset])
    }
    scan_exponent(t, &kind)

    return kind, string(t.src[offset : t.offset])
}

scan_identifier :: proc(t: ^Tokenizer) -> string {
    offset := t.offset
    for is_letter(t.ch) || is_digit(t.ch) {
        advance_rune(t)
    }
    return string(t.src[offset : t.offset])
}

scan_string :: proc(t:^Tokenizer, quote: rune) -> string {
    offset := t.offset-1
    for {
        r: rune = t.ch
		if (r == '\n' || r < 0) {
            error(t, offset, "String literal not terminated")
            break
		}
		advance_rune(t)
		if (r == quote) {
            break
		}
    }
    return string(t.src[offset : t.offset])
}

scan :: proc(t: ^Tokenizer) -> Token {
    skip_whitespace(t)

    offset := t.offset

    kind: Token_Kind
    lit: string
    pos := offset_to_pos(t, offset)

    switch ch := t.ch; true {
    case is_letter(ch):
        lit = scan_identifier(t)
        kind = .Identifier
    case '0' <= ch && ch <= '9':
        kind, lit = scan_number(t)
    case ch == '\n':
        advance_rune(t)
        kind = .Newline
    case:
        advance_rune(t)
        switch ch {
        case -1: kind = .EOF
        case '"':
            kind = .String
            lit = scan_string(t, ch)
        case '+': kind = .Add
        case '-':
            kind = .Sub
            if '0' <= t.ch && t.ch <= '9' {
                kind, _ = scan_number(t)
                lit = string(t.src[offset:t.offset])
            }
        case '*': kind = .Mul
        case '/':
            kind = .Quo
            if t.ch == '/' {
                kind = .Comment
                lit = scan_comment(t)
            }
        case '(': kind = .Open_Paren
        case ')': kind = .Close_Paren
        case '[': kind = .Open_Bracket
        case ']': kind = .Close_Bracket
        case '{': kind = .Open_Brace
        case '}': kind = .Close_Brace
        case ':': kind = .Colon
        case ',': kind = .Comma
        case:
            if ch != utf8.RUNE_BOM {
                error(t, t.offset, "illegal character '%r': %d", ch, ch)
            }
            kind = .Invalid
        }
    }

    if lit == "" {
        lit = string(t.src[offset : t.offset])
    }

    return Token{kind, lit, pos}
}