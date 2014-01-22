

let parse_var_key (k:string) : (string option * string option * string option * string) =
  let get_prefix s d =
    match Str.split (Str.regexp_string d) s with
      | [prefix; s'] -> (Some prefix, s')
      | _ -> (None, s)
  in
  let ns, n' = get_prefix k "@" in
  let filename, n'' = get_prefix n' "/" in
  let funcname, n''' = get_prefix n'' "#" in
  (ns, filename, funcname, n''')

let add_suffix s suffix separator = 
  match suffix with
    | Some p -> s ^ p ^ separator
    | None -> s

let render_var_key (namespace, filename, funcname, varname) =
  let k = add_suffix "" namespace "@" in
  let k' = add_suffix k filename "/" in
  let k'' = add_suffix k' funcname "#" in
  k'' ^ varname



let render_xform_name (namespace, filename, funcname, varname) =  
  let filename' = 
    match filename with
      | None -> None
      | Some fn -> Some (Str.global_replace (Str.regexp_string ".") "_" fn)
  in
  let filename'' = 
    match filename' with
      | None -> None
      | Some fn -> Some (Str.global_replace (Str.regexp_string "-") "_" fn)
  in
  let namespace' = 
    match namespace with
      | None -> None
      | Some fn -> Some (Str.global_replace (Str.regexp_string ".") "_" fn)
  in
  let n = add_suffix "_kitsune_transform_" namespace' "___ns___" in
  let n' = add_suffix n filename'' "___file___" in
  let n'' = add_suffix n' funcname "___func___" in
  n'' ^ varname


let rec rmsubdir (k:string) = 
  if (String.length k < 3) then k 
  else if ((String.sub k 0 3) = "../") then (rmsubdir (String.sub k 3 ((String.length k)-3 )))
  else  k

let xform_from_key (k:string) =
  render_xform_name (parse_var_key (rmsubdir k))

let opt_to_arg = function
  | None -> "0"
  | Some s -> "\"" ^ s ^ "\""

let render_get_val_old_call (key:string) = 
  let (namespace_opt, filename_opt, funcname_opt, var_name) = (parse_var_key (rmsubdir key))  in
  "kitsune_get_symbol_addr_old(\"" ^ 
    var_name ^ "\", " ^ 
    opt_to_arg funcname_opt ^ ", " ^
    opt_to_arg filename_opt ^ ", " ^
    opt_to_arg namespace_opt ^ ")"


let render_get_val_new_call (key:string) = 
  let (namespace_opt, filename_opt, funcname_opt, var_name) = (parse_var_key (rmsubdir key))  in
  "kitsune_get_symbol_addr_new(\"" ^ 
    var_name ^ "\", " ^ 
    opt_to_arg funcname_opt ^ ", " ^
    opt_to_arg filename_opt ^ ", " ^
    opt_to_arg namespace_opt ^ ")"
