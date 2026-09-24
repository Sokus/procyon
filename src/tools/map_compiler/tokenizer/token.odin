package tokenizer

Token :: struct {
    kind: Token_Kind,
    text: string,
    pos: Pos,
}

Pos :: struct {
    file: string,
    offset: int, // starting at 0
    line:   int, // starting at 1
    column: int, // starting at 1
}

Token_Kind :: enum {
    Invalid,
    EOF,
    Newline,
    Comment,

    B_Literal_Begin,
        Identifier, // __TB_empty
        Integer,
        Float,
        String,     // "classname", "worldspawn"
    B_Literal_End,

    B_Operator_Begin,
        Add,           // +
        Sub,           // -
        Mul,           // *
        Quo,           // /
        Open_Paren,    // (
        Close_Paren,   // )
        Open_Bracket,  // [
        Close_Bracket, // ]
        Open_Brace,    // {
        Close_Brace,   // }
        Colon,         // :
        Comma,         // ,
    B_Operator_End,

    Count,
}

tokens := [Token_Kind.Count]string{
    "invalid",
    "EOF",
    "newline",
    "comment",

    "",
    "identifier",
    "integer",
    "float",
    "string",
    "",

    "",
    "+",
    "-",
    "*",
    "/",
    "(",
    ")",
    "[",
    "]",
    "{",
    "}",
    ":",
    ",",
    ""
}

token_kind_to_string :: proc(kind: Token_Kind) -> string {
    if .Invalid <= kind && kind < .Count {
        return tokens[kind];
    }
    return "Invalid"
}

is_operator :: proc(kind: Token_Kind) -> bool {
    #partial switch kind {
    case .B_Operator_Begin ..= .B_Operator_End:
        return true
    }
    return false
}

token_precedence :: proc(kind: Token_Kind) -> int {
    #partial switch kind {
    case .Add, .Sub:
        return 1
    case .Mul, .Quo:
        return 2
    }
    return 0
}
