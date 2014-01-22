open Kttools

let is_whitespace (c:char) : bool = (c = ' ' || c = '\t' || c = '\n')

let is_c_sep (c:char) : bool = (c = ';' || c = ',' || c = ')' || c = '.' || c = '-' || c = '}' || c = '{')

type string_ind = int * int

type code_status =
  | NormalText
  | InEscape of code_status 
  | InSingleQuotes
  | InDoubleQuotes
  | InDollarString of int
  | AfterDollarString of int * string_ind
  | InArgs of int * string_ind * string_ind list * int

let handle_user_code (sub_fun:string -> string list -> ('a * string) option) (code:string) : ('a list * string) = 
  let results = ref [] in
  let body_strings = ref [] in
  let ind_to_str si = 
    let (s, e) = si in String.sub code s (e-s)
  in
  let l = String.length code in

  let handle_match dname dargs orig_str_ind =
    match sub_fun (ind_to_str dname) (List.map ind_to_str dargs) with
      | Some (data, replacement) -> 
        results := data :: !results;
        body_strings := replacement :: !body_strings
      | None ->        
        body_strings := (ind_to_str orig_str_ind) :: !body_strings
  in

  let rec util i start status  =
    if i < l then
      begin
        let cur_char = String.get code i in
        match status with
          | NormalText ->
            if cur_char = '"' then
              util (i+1) start InDoubleQuotes
            else if cur_char = '\'' then
              util (i+1) start InSingleQuotes
            else if cur_char = '$' then
              (body_strings := ind_to_str (start, i) :: !body_strings;
               util (i+1) i (InDollarString i))
            else
              util (i+1) start NormalText
                
          | InEscape status' ->
            util (i+1) start status'
              
          | InSingleQuotes ->
            if cur_char = '\'' then
              util (i+1) start NormalText
            else if cur_char = '\\' then
              util (i+1) start (InEscape InSingleQuotes)
            else
              util (i+1) start InSingleQuotes
                
          | InDoubleQuotes ->
            if cur_char = '"' then
              util (i+1) start NormalText
            else if cur_char = '\\' then
              util (i+1) start (InEscape InDoubleQuotes)
            else
              util (i+1) start InDoubleQuotes
                
          | InDollarString dstart ->
            if (cur_char >= 'a' && cur_char <= 'z') || (cur_char >= 'A' && cur_char <= 'Z') then
              util (i+1) start (InDollarString dstart)
            else if is_whitespace cur_char then
              util (i+1) i (AfterDollarString (dstart, (start, i)))
            else if cur_char = '(' then
              util (i+1) (i+1) (InArgs (dstart, (start, i), [], 1))
            else if is_c_sep cur_char then
              let () = handle_match (start, i) [] (dstart, i) in
              util (i) i NormalText
            else
              util (i+1) dstart NormalText
                
          | AfterDollarString (dstart, s) ->
            if is_whitespace cur_char then
              util (i+1) start (AfterDollarString (dstart, s))
            else if cur_char = '(' then
              util (i+1) (i+1) (InArgs (dstart, s, [], 1))
            else 
              let () = handle_match s [] (dstart, i) in
              util (i) i NormalText
                
          | InArgs (dstart, dollar_string, args_so_far, paren_count) ->
            if cur_char = '(' then
              util (i+1) start (InArgs (dstart, dollar_string, args_so_far, paren_count+1))
            else if cur_char = ')' then
              (if paren_count = 1 then
                  let args = List.rev ((start, i) :: args_so_far) in
                  let () = handle_match dollar_string args (dstart, i+1) in
                  util (i+1) (i+1) NormalText
               else
                  util (i+1) start (InArgs (dstart, dollar_string, args_so_far, paren_count-1)))
            else if cur_char = ',' then
              (if paren_count = 1 then
                  let new_args = (start, i) :: args_so_far in
                  util (i+1) (i+1) (InArgs (dstart, dollar_string, new_args, paren_count))
               else
                  util (i+1) start status)
            else
              util (i+1) start (InArgs (dstart, dollar_string, args_so_far, paren_count))
      end
    else
      match status with
        | InDollarString dstart ->
          handle_match (start, i) [] (dstart, i)
        | AfterDollarString (dstart, s) ->
          handle_match s [] (dstart, i)
        | _ ->
          body_strings := String.sub code start (i-start) :: !body_strings;
  in
  util 0 0 NormalText;
  (!results, String.concat "" (List.rev !body_strings));;


(* 
   Below are some functions for testing the user code parser.
   TODO: turn these into self-running unit tests.
*)

let test_interpret_symb s sargs =
  match trim s with
    | "$in" -> Some (None, "var_in")
    | "$out" -> Some (None, "var_out")
    | "$base" -> Some (None, "base")
    | "$xform" -> Some (Some (List.nth sargs 0, List.nth sargs 1), "some_xform")
    | "$gen" -> Some (None, "gen_" ^ List.nth sargs 0)
    | _ -> None

let test_usercode input = 
  print_endline ("Input : " ^ input);
  let (dep_list, output) = handle_user_code test_interpret_symb input in
  List.iter (function None -> () | Some (f,t) -> print_endline (f ^ " -> " ^ t)) dep_list;
  print_endline ("Output: " ^ output);;

let unit_tests () = 
  List.iter test_usercode [
    "abcdef";
    "$test";
    "$test abc";
    "$test  abc";
    "abc $test";
    "abc  $test";
    "$in->foo";
    "$in  ->foo";
    "$gen(x)";
    "$gen  (x)";
    "$xform(struct s, typedef t)";
    " $xform(struct s, typedef t) ";
    "\"\\\"$foo\"";
  ]
