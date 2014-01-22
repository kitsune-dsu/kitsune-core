
(*** Utility functions - need to move to their own module ***)
let id x = x

let rev_implode (l:char list) : string =
  let len = (List.length l) in
  let res = String.create len in
  let rec imp i = function
    | [] -> res
    | c :: l -> res.[i] <- c; imp (i - 1) l in
  imp (len-1) l

let implode (l:char list) : string =
  let len = (List.length l) in
  let res = String.create len in
  let rec imp i = function
    | [] -> res
    | c :: l -> res.[i] <- c; imp (i + 1) l in
  imp 0 l


let map_index (f:(int -> 'a -> 'b)) (l:'a list) : 'b list =
  let rec map_util i l = 
    match l with
      | h :: t -> f i h :: map_util (i+1) t
      | [] -> []
  in
  map_util 0 l
            
let rec catOptionList (l:'a option list) : 'a list =
  match l with
    | Some x :: rest -> x :: catOptionList rest
    | None :: rest -> catOptionList rest
    | [] -> []

let option_get_unsafe (e:'a option) : 'a =
  match e with
    | None -> failwith "Option_get - did not expect None argument"
    | Some x -> x

let option_get_safe (e:'a option) (o:'a) : 'a =
  match e with
    | None -> o
    | Some x -> x

let hashtbl_find_safe (h : ('a, 'b) Hashtbl.t) (key:'a) : 'b option =
  try
    Some (Hashtbl.find h key)
  with
    | Not_found -> None

let keep_first (current : 'a option ref) (upd : 'a) : unit =
  if !current = None then
    current := Some upd

let keep_last (current : 'a option ref) (upd : 'a) : unit =
  current := Some upd

let keep_one (current : 'a option ref) (upd : 'a) : unit =
  if !current <> None then
    failwith "Found conflicting annotations";
  current := Some upd

let list_ref_push (lr : 'a list ref) (e : 'a) = lr := e :: !lr

let index_of (e:'a) (l:'a list) = 
  let rec find (i:int) = function
    | [] -> raise Not_found
    | e' :: _ when e' = e -> i
    | _ :: l' -> find (i+1) l'
  in
  find 0 l
    
let trim s =
  let is_space c = (c = ' ' || c = '\t' || c = '\n') in
  let rec skip_space dir i =
    if is_space s.[i] (* throws Invalid_argument *)
    then skip_space dir (i+dir)
    else i
  in
  try
    let start = skip_space 1 0
    and stop = skip_space (-1) ((String.length s) - 1) in
    String.sub s start (stop-start+1)
  with Invalid_argument _ -> ""

let rec map_find (mapping:('a * 'b) list) (missing:'a -> unit) (e:'a) =
  match mapping with
    | (e', desc) :: _ when e' = e -> (e', desc)
    | _ :: rest -> map_find rest missing e
    | [] -> (missing e); raise Not_found

let rec fix_order (mapping:('a * 'b) list) (order:'a list) (missing:'a -> unit) : ('a * 'b) list =
  List.map (map_find mapping missing) order

let iter_count (f:'a -> int -> unit) (l:'a list) : unit =
  let rec iter_count_rec l i =
    match l with
      | e :: rest -> (f e i); iter_count_rec rest (i+1)
      | [] -> ()
  in
  iter_count_rec l 0

let exitError (s:string) = 
  prerr_endline s;
  exit 1  

(* REFACTOR: refactor the compare_lists functions to use a sum type to
   indicate whether to skip, etc. rather than using the SkipElem
   exception *)

exception SkipElem

let util_compare_lists old_l new_l key_fun map_key_fun compare_fun added_fun removed_fun check_order reordered_fun =
  let create_hash l =
    let hash = Hashtbl.create 17 in
    let iter_fun e =
      Hashtbl.add hash (key_fun e) e
    in
    List.iter iter_fun l;
    hash
  in
  let old_hash = create_hash old_l in
  let match_found = Hashtbl.create 17 in
  let added_or_removed = ref false in
  let iter_new_elem e =
    try 
      let map_key = map_key_fun e in
      if Hashtbl.mem old_hash map_key then
        begin
          Hashtbl.add match_found map_key true;
          compare_fun (Hashtbl.find old_hash map_key) e
        end
      else
        (added_or_removed := true; added_fun e)
    with
      | SkipElem -> added_or_removed := true
  in
  let iter_old_elem e =
    if not (Hashtbl.mem match_found (key_fun e)) then
      (added_or_removed := true; removed_fun e)
  in
  List.iter iter_new_elem new_l;
  List.iter iter_old_elem old_l;
  if check_order && not !added_or_removed then
    begin
      let ordered = ref true in
      let check_order old_e new_e =
        if (key_fun old_e) <> (map_key_fun new_e) then
          ordered := false
      in
      (try List.iter2 check_order old_l new_l
       with Invalid_argument _ -> ordered := false);
      if not !ordered then
        reordered_fun ()        
    end

let compare_lists old_l new_l key_fun map_key_fun compare_fun added_fun removed_fun =
  util_compare_lists old_l new_l key_fun map_key_fun compare_fun added_fun removed_fun false (fun () -> failwith "Unexpected")

let compare_ordered_lists old_l new_l key_fun map_key_fun compare_fun added_fun removed_fun reordered_fun =
  util_compare_lists old_l new_l key_fun map_key_fun compare_fun added_fun removed_fun true reordered_fun

let rec list_last (l:'a list) : 'a =
  match l with
    | e :: [] -> e
    | e :: rest -> list_last rest
    | [] -> failwith "Kttools.list_last: list should not be empty"

let string_starts_with s pattern = 
  String.length pattern < String.length s && Str.first_chars s (String.length pattern) = pattern;;

let string_remove_prefix s prefix =
  assert (string_starts_with s prefix);
  let prefix_len = String.length prefix in
  String.sub s prefix_len ((String.length s) - prefix_len);;

let filter_map (f:'a -> 'b option) (l:'a list) : ('b list) =
  let filter_fun = function
    | None -> false
    | Some _ -> true
  in
  let remove_option = function
    | None -> failwith "filter_map: unexpected"
    | Some e -> e
  in
  List.map remove_option (List.filter filter_fun (List.map f l))
