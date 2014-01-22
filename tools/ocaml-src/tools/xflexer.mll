
(* xflang.mll *)

(* perhaps make the name rule here more permissive since the C
   compiler will complain later anyway *)

{
  open Xfparser

  let print_tokens (tl : token list) : unit = 
    let str_of_token = function
      | STARTC -> "STARTC\n"
      | ENDC -> "ENDC\n"
      | XFORM -> "XFORM\n"
      | INIT -> "INIT\n"
      | DOT -> "DOT\n"
      | STRUCT n -> "STRUCT\n"
      | UNION n -> "UNION\n"
      | TYPEDEF n -> "TYPEDEF\n"
      | ENUM n -> "ENUM\n"
      | COMMA -> "COMMA\n"
      | COLON -> "COLON\n"
      | NAME n -> ("NAME " ^ n ^ "\n")
      | CCHAR c -> (String.make 1 c)
      | EOF -> "EOF\n"
    in
    List.iter print_string (List.map str_of_token tl)

  type code_state =
    | XFCode 
    | CCode of int
}

rule 
  xftoken = parse
    | "{" 
        { (STARTC, (CCode 1)) }
    | "}" 
        { failwith "unexpected" }
    | "->" 
        { (XFORM, XFCode) }
    | "INIT"
        { (INIT, XFCode) }
    | "."
        { (DOT, XFCode) }
    | "struct " (['$' '_' 'A'-'Z' 'a'-'z'] ['$' '_' 'A'-'Z' 'a'-'z' '0'-'9'] * as name)
        { (STRUCT name, XFCode) }
    | "union " (['$' '_' 'A'-'Z' 'a'-'z'] ['$' '_' 'A'-'Z' 'a'-'z' '0'-'9'] * as name)
        { (UNION name, XFCode) }
    | "typedef " (['$' '_' 'A'-'Z' 'a'-'z'] ['$' '_' 'A'-'Z' 'a'-'z' '0'-'9'] * as name)
        { (TYPEDEF name, XFCode) }
    | "enum " (['$' '_' 'A'-'Z' 'a'-'z'] ['$' '_' 'A'-'Z' 'a'-'z' '0'-'9'] * as name)
        { (ENUM name, XFCode) }
    | ","
        { (COMMA, XFCode) }
    | ":"
        { (COLON, XFCode) }
    | ['$' '_' 'A'-'Z' 'a'-'z'] ['$' '_' 'A'-'Z' 'a'-'z' '0'-'9' '#' '/' '@' '.'] * as id
        { (NAME id, XFCode) }
    | eof { (EOF, XFCode) }
    | _ { xftoken lexbuf }
and
  ccode pcount = parse
    | "{"  as c
        { (CCHAR c, CCode (pcount+1)) }
    | "}" as c
        { if pcount = 1 then (ENDC, XFCode) else (CCHAR c, CCode (pcount-1)) }
    | _ as c 
        { (CCHAR c, CCode pcount) }
