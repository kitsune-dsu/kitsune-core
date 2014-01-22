

%{
  open Xftypes
  open Kttools

  let make_rule code paths =
    match paths, code with
      | [f; t], _ -> Xform (f, t, code)
      | [s], Some c -> Init (s, c)
      | [s], None -> failwith "INIT blocks must include initialization code"
      | _ -> failwith "Xfparser.make_rule: Unexpected set of paths"

  let fix_path (p:xf_path_elem list) =
    let rev_path = List.rev p in
    match rev_path with
      | PField name :: rest -> List.rev (PVar name :: rest)
      | _ -> p
%}

%token STARTC ENDC
%token XFORM INIT DOT COMMA COLON
%token <string> STRUCT 
%token <string> UNION 
%token <string> TYPEDEF 
%token <string> ENUM
%token <string> NAME
%token <char> CCHAR
%token EOF
%start main             /* the entry point */
%type <string option * Xftypes.xf_rule list> main
%%

main:
| c_block rules { (Some $1, $2) }
| rules { (None, $1) }

rules:
| xform_decl rules { $1 @ $2  }
| EOF { [] }

xform_decl:
| xform_label_multiple c_block { List.map (make_rule (Some $2)) $1 } 
| xform_label_single { [make_rule None $1] }

xform_label_multiple:
| xform_label_single COMMA xform_label_multiple { $1 :: $3  }
| xform_label_single COLON { [$1] }

xform_label_single:
| path XFORM path { [fix_path $1; fix_path $3] }
| INIT path { [fix_path $2] }

path:
| path DOT path_elem { $3 :: $1 }
| path_elem { [$1] }

path_elem:
| STRUCT { PStruct $1 }
| UNION { PUnion $1 }
| TYPEDEF { PTypedef $1 }
| ENUM { PEnum $1 }
| NAME { PField $1 }

c_block:
| STARTC c_body { implode $2 }

c_body:
| CCHAR c_body { $1 :: $2 }
| ENDC { [] }


