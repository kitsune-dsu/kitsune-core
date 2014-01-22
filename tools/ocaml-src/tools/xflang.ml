open Kttools
open Xftypes

let is_var_path_elem = function
  | PVar _ -> true
  | _ -> false

let print_path_elem = function
  | PStruct s -> "struct " ^ s
  | PUnion s -> "union " ^ s
  | PTypedef s -> "typedef " ^ s
  | PEnum s -> "enum " ^ s
  | PField s | PVar s -> s

let path_to_string p =
  String.concat "." (List.rev_map print_path_elem p)

let rule_to_string = function
  | Xform (from_path, to_path, _) -> (path_to_string from_path) ^ " -> " ^ (path_to_string to_path)
  | Init (to_path, _) -> "INIT " ^ (path_to_string to_path)

let determine_match (needed_path:xf_path_elem list) (check_path:xf_path_elem list) =
  let rec check_after_match np cp =
    match np, cp with
      | n1 :: r1, n2 :: r2 when n1 = n2 -> check_after_match r1 r2
      | [], [] -> true
      | _ -> false
  in
  let rec check np cp =
    match np, cp with
      | n1 :: r1, n2 :: r2 when is_var_path_elem n2 ->
          (* if the check_path doesn't start with a type, require a complete match *)
          n1 = n2 && (check_after_match r1 r2)
      | n1 :: r1, n2 :: r2 when n1 = n2 ->
          (check_after_match r1 r2) || (check r1 cp)
      | n1 :: r1, n2 :: r2 ->
          (check r1 cp)
      | _, [] -> failwith "Did not expect an attempt to match with an empty list!"
      | [], _ -> false
  in
  check needed_path check_path

let reportAmbiguity new_path matches =
  prerr_endline ("Ambiguous transformation rules for \"" ^ (path_to_string new_path) ^ "\":");
  List.iter prerr_endline (List.map rule_to_string matches);
  exitError "Comparison terminated"

let type_renamed (from_t:xf_path_elem) (to_t:xf_path_elem) (all_rules:xf_rule list) : xf_rule option =
  let filter_fun = function
    | Xform (from_path, to_path, _) -> from_path = [from_t] && to_path = [to_t]
    | Init (to_path, _) -> false
  in
  match List.filter filter_fun all_rules with
    | [] -> None
    | [r] -> Some r
    | _ as matches -> reportAmbiguity [to_t] matches
  
let find_rule (from_path_req:xf_path) (to_path_req:xf_path) (all_rules:xf_rule list) : xf_rule option =
  let filter_fun = function
    | Xform (from_path, to_path, _) ->
      from_path_req = (List.tl from_path) && to_path = to_path_req
    | Init (to_path, _) ->
      to_path_req = to_path
  in
  match List.filter filter_fun all_rules with
    | [] -> None
    | [r] -> Some r
    | _ as matches -> reportAmbiguity to_path_req matches

let lex_func =
  let cur_tokenizer = ref Xflexer.xftoken in
  fun (lexbuf) ->
    let (token, code_state) = (!cur_tokenizer) lexbuf in
    cur_tokenizer := (match code_state with 
                        | Xflexer.XFCode -> Xflexer.xftoken
                        | Xflexer.CCode nd -> Xflexer.ccode nd);
    token

let parseXFRules (xf_file_opt:string option) : (string option * xf_rule list) =
  match xf_file_opt with
    | None -> (None, [])
    | Some filename -> 
      let file_in = open_in filename in
      let lexbuf = Lexing.from_channel file_in in
      Xfparser.main lex_func lexbuf  
