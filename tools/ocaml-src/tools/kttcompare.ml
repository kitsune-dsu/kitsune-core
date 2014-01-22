
(* Kttcompare.ml
 * 
 * This file implements a comparison between the set of elements (i.e.,
 * types and variables) of two versions of a program and produces a
 * summary of the differences.  It compares elements starting from
 * variables in the original program that the developer indicated that
 * they will migrate (using MIGRATE annotations).
 * 
 * If data from a xf (transformation) input file is available, then it
 * will impact which matches are checked (e.g., in the presence of
 * renaming or user-provided transformation code.
 *)


open Kttools
open Kttypes.TransformTypes
open Kttypes.Tools

module L = List
module H = Hashtbl

(* The following datatypes are used to represent the results of a
   comparison *)

type map_elem = Xftypes.xf_path

type elem_mapping =
  | MatchInit of map_elem * typ * string
  | MatchAuto of (map_elem * map_elem) * (typ * typ) * (gen_out * gen_out) * match_info
  | MatchUserCode of (map_elem * map_elem) * (typ * typ) * (gen_out * gen_out) * string
  | UnmatchedAdded of map_elem
  | UnmatchedDeleted of map_elem
  | UnmatchedType of (map_elem * map_elem) (* TODO: include type for error messages *)
  | UnmatchedError of (map_elem * map_elem) * string
      
and match_info =
  | MatchStruct of bool * elem_mapping list
  | MatchUnion of elem_mapping list
  | MatchTypedef of (typ * typ)
  | MatchVar of (gen_in * gen_in)
  | MatchEnum

let elem_to_path (_,e:string * top_level_elem) : Xftypes.xf_path_elem =
  match e with 
    | Struct sinfo -> Xftypes.PStruct sinfo.sname
    | Union sinfo -> Xftypes.PUnion sinfo.sname
    | Typedef tinfo -> Xftypes.PTypedef tinfo.tname
    | Enumeration einfo -> Xftypes.PEnum einfo.ename
    | Var vinfo -> Xftypes.PVar vinfo.vname

(* The comparison context is passed throughout the comparison code to
   represent the current state of the comparison.  It also contains
   data that is needed throughout the comparison (e.g., the set of
   rules, if any, from the xf file.  Also, it maintains dependency
   information between elements.  *)

type compcontext = {
  all_rules : Xftypes.xf_rule list;
  add_comps : bool;
  types_todo : (Xftypes.xf_path_elem list * Xftypes.xf_path_elem list) list ref;
  types_done : ((Xftypes.xf_path * Xftypes.xf_path), bool) H.t;
  requires_full_xform : ((Xftypes.xf_path * Xftypes.xf_path), bool) H.t;
  requires_shallow_xform : ((Xftypes.xf_path * Xftypes.xf_path), bool) H.t;
  matches : ((Xftypes.xf_path * Xftypes.xf_path), bool) H.t;
  match_failed : ((Xftypes.xf_path * Xftypes.xf_path), bool) H.t;
  hard_deps : ((Xftypes.xf_path * Xftypes.xf_path), (Xftypes.xf_path * Xftypes.xf_path)) H.t;
  ptr_deps : ((Xftypes.xf_path * Xftypes.xf_path), (Xftypes.xf_path * Xftypes.xf_path)) H.t;
  old_elems : (string * top_level_elem) list;
  new_elems : (string * top_level_elem) list;
  old_path : Xftypes.xf_path;
  new_path : Xftypes.xf_path;
}

(* The following functions maintain the comparison context. *)

let compcontext_create (rules:Xftypes.xf_rule list) old_elems new_elems = {
  all_rules = rules;
  add_comps = true;
  types_todo = ref [];
  types_done = H.create 17;
  requires_full_xform = H.create 17;
  requires_shallow_xform = H.create 17;
  matches = H.create 17;
  match_failed = H.create 17;
  hard_deps = H.create 17;
  ptr_deps = H.create 17;
  old_elems = old_elems;
  new_elems = new_elems;
  old_path = [];
  new_path = [];
}

let compcontext_extend_paths (ctx:compcontext) (pe0:Xftypes.xf_path_elem) (pe1:Xftypes.xf_path_elem) = 
  H.add ctx.hard_deps (pe0 :: ctx.old_path, pe1 :: ctx.new_path) (ctx.old_path, ctx.new_path);
  {
    ctx with 
      old_path = pe0 :: ctx.old_path;
      new_path = pe1 :: ctx.new_path;
  }

let compcontext_set_no_comps ctx = 
  {ctx with add_comps = false}

let compcontext_add_type_comp ctx (through_ptr:bool) (path0, path1) =
  if through_ptr then
    H.add ctx.ptr_deps (path0, path1) (ctx.old_path, ctx.new_path)
  else
    H.add ctx.hard_deps (path0, path1) (ctx.old_path, ctx.new_path);
  if ctx.add_comps && (not (H.mem ctx.types_done (path0, path1))) then
    ctx.types_todo := (path0, path1) :: !(ctx.types_todo)
      
let compcontext_set_requires_full_xform ctx = H.add ctx.requires_full_xform (ctx.old_path, ctx.new_path) true

let compcontext_set_requires_shallow_xform ctx = H.add ctx.requires_shallow_xform (ctx.old_path, ctx.new_path) true
  
let compcontext_set_match_failed ctx = H.add ctx.match_failed (ctx.old_path, ctx.new_path) true

let compcontext_record_match ctx paths = H.add ctx.matches paths true

let computeChangeFixpoint (ctx:compcontext) =
  let deep_mappings = H.fold (fun k v i -> k :: i) ctx.requires_full_xform [] in
  let shallow_mappings = H.fold (fun k v i -> k :: i) ctx.requires_shallow_xform [] in
  let rec mark_dependencies (deep:bool) mapping =
    if deep then
      (if (not (H.mem ctx.requires_full_xform mapping)) then
          begin
            H.add ctx.requires_full_xform mapping true;
            L.iter (fun d -> mark_dependencies true d) (H.find_all ctx.hard_deps mapping);
            L.iter (fun d -> mark_dependencies false d) (H.find_all ctx.ptr_deps mapping)
          end)
    else
      (if (not (H.mem ctx.requires_shallow_xform mapping)) then
          begin
            H.add ctx.requires_shallow_xform mapping true;
            L.iter (fun d -> mark_dependencies false d) (H.find_all ctx.hard_deps mapping);
            L.iter (fun d -> mark_dependencies false d) (H.find_all ctx.ptr_deps mapping)
          end)       
  in
  H.clear ctx.requires_full_xform;
  H.clear ctx.requires_shallow_xform;
  L.iter (mark_dependencies true) deep_mappings;
  L.iter (mark_dependencies false) shallow_mappings
    
(* End Comparison Compcontext Maintenance *)

let path_elem_from_type = function
  | TNamed (name, gen_in) -> Some (Xftypes.PTypedef name, gen_in)
  | TStruct (name, gen_in) -> Some (Xftypes.PStruct name, gen_in)
  | TUnion (name, gen_in) -> Some (Xftypes.PUnion name, gen_in)
  | _ -> None

(* Following are comparison functions for each of the sub-parts of the
   program elements for which we support comparison. *)

let rec compareTypeDesc (ctx:compcontext) (through_ptr:bool) (d0:generic_type_desc) (d1:generic_type_desc) : bool =
  match d0, d1 with
    | PtrTo d0', PtrTo d1' -> compareTypeDesc ctx true d0' d1' 
    | OtherGenericBinding (GenName name0), OtherGenericBinding (GenName name1) -> name0 = name1
    | ProgramType t0, ProgramType t1 ->
      compareTypes ctx t0 t1 through_ptr
    | _ -> false

and compareGenericIn (ctx:compcontext) (g0:gen_in) (g1:gen_in) (through_ptr:bool) =
  List.for_all2 (compareTypeDesc ctx through_ptr) g0 g1

and compareArgs (ctx:compcontext) args0 args1 (through_ptr:bool) =
  match args0, args1 with 
    | (n0, t0) :: rest0, (n1, t1) :: rest1 ->
      n0 = n1 && compareTypes ctx t0 t1 through_ptr && compareArgs ctx rest0 rest1 through_ptr
    | [], [] -> true
    | _ -> false

and compareTypes (ctx:compcontext) t0 t1 (through_ptr:bool) =
  match t0, t1 with 
    | TVoid gen0, TVoid gen1 -> gen0 = gen1
    | TInt ikind0, TInt ikind1 -> ikind0 = ikind1
    | TFloat fkind0, TFloat fkind1 -> fkind0 = fkind0
    | TPtr (t0', gv0, mem0), TPtr (t1', gv1, mem1) ->      
      compcontext_set_requires_shallow_xform ctx;
      compareTypes ctx t0' t1' true && mem0 = mem1
    | TPtrOpaque t0', TPtrOpaque t1' -> 
      (* FIXME: should the base types be considered in this
         comparison? don't think so *)      
      true
    | TPtrArray (t0', len0, mem0), TPtrArray (t1', len1, mem1) ->
      compcontext_set_requires_shallow_xform ctx;
      compareTypes ctx t0' t1' true && len0 = len1 && mem0 = mem1
    | TArray (t0', len0), TArray (t1', len1) ->
      len0 = len1 && compareTypes ctx t0' t1' through_ptr
    | TFun (rt0, args0, varargs0), TFun (rt1, args1, varargs1) ->
      compcontext_set_requires_shallow_xform ctx;
      (* compareTypes ctx rt0 rt1 true && varargs0 = varargs1 && compareArgs ctx args0 args1 true *)
      true
    | TBuiltin_va_list, TBuiltin_va_list -> true
    | TEnum ename0, TEnum ename1 when ename0=ename1 -> true
    | _, _ -> 
      begin
        let path_elem0 = path_elem_from_type t0 in
        let path_elem1 = path_elem_from_type t1 in
        match path_elem0, path_elem1 with
          | None, _ | _, None -> false
          | Some (from_path, gen_in0), Some (to_path, gen_in1) ->
            (match Xflang.type_renamed from_path to_path ctx.all_rules with
              | Some (Xftypes.Xform (_, _, Some code)) ->
                compcontext_set_requires_shallow_xform ctx;
                compcontext_add_type_comp ctx through_ptr ([from_path], [to_path]);
                compareGenericIn ctx gen_in0 gen_in1 through_ptr
              | Some (Xftypes.Xform (_, _, None)) ->
                compcontext_add_type_comp ctx through_ptr ([from_path], [to_path]);
                compareGenericIn ctx gen_in0 gen_in1 through_ptr                
              | Some _ -> 
                failwith "compareTypes - unexpected rule"         
              | None when from_path = to_path ->
                compcontext_add_type_comp ctx through_ptr ([from_path], [to_path]);
                compareGenericIn ctx gen_in0 gen_in1 through_ptr
              | None -> 
                compcontext_set_requires_full_xform ctx; 
                false)
      end

let compareTypesReport (ctx:compcontext) t0 t1 =
  let result = compareTypes ctx t0 t1 false in
  if not result then
    compcontext_set_match_failed ctx;
  result



let rec analyzeUsercode ctx (code:string) =
  let handle_xform_op s sargs =
    match s with
      | "$xform" -> 
        begin
          match sargs with
            | tn0 :: tn1 :: other_args -> 
              let t0 = Kttypes.Tools.parseGenericBindingType tn0 [] ignore in
              let t1 = Kttypes.Tools.parseGenericBindingType tn1 [] ignore in        
              let (e0, _), (e1, _) = 
                match (path_elem_from_type t0), (path_elem_from_type t1) with 
                  | Some e0, Some e1 -> e0, e1
                  | _ -> exitError "Error: attempt to reference transformation code involving a primitive."
              in
              (* The false here is the through_ptr argument.  I
                 believe it can be false without any problem *)
              compcontext_add_type_comp ctx false ([e0], [e1]); 
              List.iter (analyzeUsercode ctx) other_args;
              None
            | _ -> exitError ("kttcompare.ml: analyzeUsercode: $xform(t1, t2, ...) requires two type arguments, provided one: " ^ code)
        end
      | _ -> None
  in
  ignore (Usercode.handle_user_code handle_xform_op code)


let rec compareStructFields (ctx:compcontext) (fields0:field list) (fields1:field list) =
  let field_matches = ref [] in
  let reordered = ref false in
  let matched_rule = ref None in
  let key_fun f = Xftypes.PField f.fname in
  let map_key_fun f =
    let key = key_fun f in
    match Xflang.find_rule ctx.old_path (key::ctx.new_path) ctx.all_rules with
      | None -> key

      | Some (Xftypes.Xform (path_from, path_to, Some code)) ->
	analyzeUsercode ctx code;
        matched_rule := Some code;
        L.hd path_from

      | Some (Xftypes.Xform (path_from, _, None)) -> 
        L.hd path_from

      | Some (Xftypes.Init (path_to, code)) -> 
	analyzeUsercode ctx code;
        compcontext_set_requires_full_xform ctx;
        list_ref_push field_matches (MatchInit (path_to, f.ftype, code));
        raise SkipElem
  in
  let added_fun f =
    compcontext_set_requires_full_xform ctx;
    compcontext_set_match_failed ctx;
    list_ref_push field_matches (UnmatchedAdded (Xftypes.PField f.fname :: ctx.new_path))
  in
  let removed_fun f =
    compcontext_set_requires_full_xform ctx;
    list_ref_push field_matches (UnmatchedDeleted (Xftypes.PField f.fname :: ctx.new_path))
  in
  let compare_field_types f0 f1 =
    let ctx' = compcontext_extend_paths ctx (Xftypes.PField f0.fname) (Xftypes.PField f1.fname) in
    match !matched_rule with
      | None -> 
        let types_match = compareTypesReport ctx' f0.ftype f1.ftype in
        if types_match then
          let elems = (ctx'.old_path, ctx'.new_path) in
          let types = (f0.ftype, f1.ftype) in
          let generics = ([], []) in (* fields don't take generic arguments *)
          list_ref_push field_matches (MatchAuto (elems, types, generics,
                                                  MatchVar (f0.fgen_in, f1.fgen_in)))
        else
          list_ref_push field_matches (UnmatchedType (ctx'.old_path, ctx'.new_path))
      | Some code -> 
        let ctx'' = compcontext_set_no_comps ctx' in
        let types_match =  compareTypesReport ctx'' f0.ftype f1.ftype in
        matched_rule := None;
        if types_match then
          compcontext_set_requires_shallow_xform ctx''
        else
          compcontext_set_requires_full_xform ctx'';
        let elems = (ctx''.old_path, ctx''.new_path) in
        let types = (f0.ftype, f1.ftype) in
        let generics = ([], []) in (* fields don't take generics *)
        list_ref_push field_matches (MatchUserCode (elems, types, generics, code))
  in
  let reordered_fun () = compcontext_set_requires_full_xform ctx; reordered := true in
  compare_ordered_lists fields0 fields1 key_fun map_key_fun compare_field_types added_fun removed_fun reordered_fun;
  (L.rev !field_matches, !reordered)

let rec compareUnionFields (ctx:compcontext) (fields0:field list) (fields1:field list) =
  let field_matches = ref [] in
  let key_fun f = Xftypes.PField f.fname in
  let map_key_fun f = key_fun f in
  let added_fun f = list_ref_push field_matches (UnmatchedAdded (Xftypes.PField f.fname :: ctx.new_path)) in
  let removed_fun f = list_ref_push field_matches (UnmatchedDeleted (Xftypes.PField f.fname :: ctx.new_path)) in
  let compare_field_types f0 f1 =
    let ctx' = compcontext_extend_paths ctx (Xftypes.PField f0.fname) (Xftypes.PField f1.fname) in
    if compareTypesReport ctx' f0.ftype f1.ftype then
      let elems = (ctx'.old_path, ctx'.new_path) in
      let types = (f0.ftype, f1.ftype) in
      let generics = ([], []) in (* fields do not take generics *)
      list_ref_push field_matches (MatchAuto (elems, types, generics, MatchVar ([], [])))
    else
      list_ref_push field_matches (UnmatchedType (ctx'.old_path, ctx'.new_path))
  in
  compare_lists fields0 fields1 key_fun map_key_fun compare_field_types added_fun removed_fun;
  (L.rev !field_matches)

let compareGenericOut (ctx:compcontext) (go0:gen_out) (go1:gen_out) =
  let matched = ref true in
  let key_fun key = key in
  let map_key_fun = key_fun in
  let added_fun (GenName name) = matched := false in
  let removed_fun (GenName name) = matched := false in
  let compare_fun _ _ = () in
  compare_lists go0 go1 key_fun map_key_fun compare_fun added_fun removed_fun;
  !matched

let compareStructs (ctx:compcontext) (sinfo0:struct_info) (sinfo1:struct_info) =
  let (field_matches, reordered) = compareStructFields ctx sinfo0.sfields sinfo1.sfields in
  if compareGenericOut ctx sinfo0.sgen_out sinfo1.sgen_out then
    let elems = (ctx.old_path, ctx.new_path) in
    let types = (TStruct (sinfo0.sname, []), TStruct (sinfo1.sname, [])) in
    let generics = (sinfo0.sgen_out, sinfo0.sgen_out) in
    MatchAuto (elems, types, generics, MatchStruct (reordered, field_matches))
  else
    UnmatchedType (ctx.old_path, ctx.new_path)

let compareUnions (ctx:compcontext) (sinfo0:struct_info) (sinfo1:struct_info) =
  let field_matches = compareUnionFields ctx sinfo0.sfields sinfo1.sfields in
  if compareGenericOut ctx sinfo0.sgen_out sinfo1.sgen_out then
    let elems = (ctx.old_path, ctx.new_path) in 
    let types = (TUnion (sinfo0.sname, []), TUnion (sinfo1.sname, [])) in
    let generics = ([], []) in (* unions are not generic, since we can't handle them anyway *)
    MatchAuto (elems, types, generics, MatchUnion field_matches)
  else
    UnmatchedType (ctx.old_path, ctx.new_path)
    
let compareTypedefs (ctx:compcontext) (tinfo0:typedef_info) (tinfo1:typedef_info) =
  if compareTypesReport ctx tinfo0.ttype tinfo1.ttype then 
    let elems = (ctx.old_path, ctx.new_path) in
    let types = (TNamed (tinfo0.tname, []), TNamed (tinfo1.tname, [])) in 
    let generics = (tinfo0.tgen_out, tinfo1.tgen_out) in
    MatchAuto (elems, types, generics,
               MatchTypedef ((tinfo0.ttype, tinfo1.ttype)))
  else
    UnmatchedType (ctx.old_path, ctx.new_path)

let compareEnums (ctx:compcontext) (einfo0:enumeration_info) (einfo1:enumeration_info) =
  let matches = ref true in
  if einfo0.eintkind <> einfo1.eintkind then
    (matches := false; compcontext_set_match_failed ctx);
  let key_fun (n,_) = n in
  let added_fun (n,v) = matches := false in
  let removed_fun (n,v) = matches := false in
  let compare_eitems (_,v0) (_,v1) =
    if v0 <> v1 then matches := false;
  in
  if !matches then
    compare_lists einfo0.eitems einfo1.eitems key_fun key_fun compare_eitems added_fun removed_fun;
  if !matches then
    let elems = (ctx.old_path, ctx.new_path) in
    let types = (TEnum einfo0.ename, TEnum einfo1.ename) in
    let generics = ([], []) in
    MatchAuto (elems, types, generics, MatchEnum)
  else
    UnmatchedType (ctx.old_path, ctx.new_path)

let compareVars (ctx:compcontext) (vinfo0:var_info) (vinfo1:var_info) =
  if compareTypesReport ctx vinfo0.vtype vinfo1.vtype then 
    let elems = (ctx.old_path, ctx.new_path) in
    let types = (vinfo0.vtype, vinfo1.vtype) in
    let generics = ([], []) in (* global variables do not take generic arguments *)
    MatchAuto (elems, types, generics, MatchVar (vinfo0.vgen_in, vinfo1.vgen_in))
  else
    UnmatchedType (ctx.old_path, ctx.new_path)

let compareElems (ctx:compcontext) (e0_in:string * top_level_elem) (e1_in:string * top_level_elem) =
  let (_, e0) = e0_in in
  let (_, e1) = e1_in in
  let ctx' = compcontext_extend_paths ctx (elem_to_path e0_in) (elem_to_path e1_in) in
  match e0, e1 with
    | Struct sinfo0, Struct sinfo1 -> 
        compareStructs ctx' sinfo0 sinfo1;        
    | Union sinfo0, Union sinfo1 -> 
        compareUnions ctx' sinfo0 sinfo1
    | Typedef tinfo0, Typedef tinfo1 -> 
        compareTypedefs ctx' tinfo0 tinfo1
    | Enumeration einfo0, Enumeration einfo1 -> 
        compareEnums ctx' einfo0 einfo1
    | Var vinfo0, Var vinfo1 -> 
        compareVars ctx' vinfo0 vinfo1
    | _ ->
        UnmatchedType (ctx.old_path, ctx.new_path)

let getElementGenerics = function
  | Struct sinfo 
  | Union sinfo -> 
    sinfo.sgen_out
  | Typedef tinfo -> 
    tinfo.tgen_out
  | _ -> []






(* compareXformElems initiates the comparisons, based on the variables
   for which migration has been requested. *)

let compareXformElems (ctx:compcontext) xform_reqs =
  let v0_hash = H.create 17 in
  let v1_hash = H.create 17 in
  let add_hash hash elem = H.add hash ([elem_to_path elem]) elem in
  L.iter (add_hash v0_hash) ctx.old_elems;
  L.iter (add_hash v1_hash) ctx.new_elems;
  let handle_match path0 path1 : elem_mapping =
    compcontext_record_match ctx (path0, path1);
    if not (H.mem v0_hash path0) then
      UnmatchedError ((path0, path1), ("No element \"" ^ (Xflang.path_to_string path0) ^ "\" in the old version code."))
    else if not (H.mem v1_hash path1) then
      UnmatchedError ((path0, path1), ("No element \"" ^ (Xflang.path_to_string path1) ^ "\" in the new version code."))
    else
      let ((f0, e0) as felem0) = H.find v0_hash path0 in
      let ((f1, e1) as felem1) = H.find v1_hash path1 in
      match Xflang.type_renamed (L.hd path0) (L.hd path1) ctx.all_rules with
        | None 
        | Some (Xftypes.Xform (_, _, None)) ->
          compareElems ctx felem0 felem1
        | Some (Xftypes.Xform (_, _, Some code)) ->
          analyzeUsercode ctx code;
          compcontext_set_requires_full_xform (compcontext_extend_paths ctx (L.hd path0) (L.hd path1));
          let elems = (path0, path1) in
          let types = (elemToType e0, elemToType e1) in
          let generics = (getElementGenerics e0, getElementGenerics e1) in
          MatchUserCode (elems, types, generics, code)
        | Some (Xftypes.Init (_, _)) ->
          exitError "An INIT rule should not have matched when mapping two elements"           

  in
  let handle_var (name:string) =
    match Xflang.find_rule ctx.old_path (Xftypes.PVar name::ctx.new_path) ctx.all_rules with
      | None ->
        handle_match [Xftypes.PVar name] [Xftypes.PVar name]

      | Some (Xftypes.Xform (path_from, path_to, Some code)) ->
        compcontext_set_requires_full_xform (compcontext_extend_paths ctx (L.hd path_from) (L.hd path_to));
        analyzeUsercode ctx code;
        let _, e0 = H.find v0_hash path_from in
        let _, e1 = H.find v1_hash path_to in
        let elems = (path_from, path_to) in
        let types = (elemToType e0), (elemToType e1) in
        let generics = ([], []) in (* global variables don't take generics *)
        MatchUserCode (elems, types, generics, code)

      | Some (Xftypes.Xform (path_from, path_to, None)) ->
        (*TODO: remove tihs code *)
        (* if path_from <> path_to then *)
        (*   compcontext_set_requires_full_xform (compcontext_extend_paths ctx (L.hd path_from) (L.hd path_to)); *)
        handle_match path_from path_to

      | Some (Xftypes.Init (init_path, code)) ->
        analyzeUsercode ctx code;
        let _, e = H.find v1_hash init_path in
        MatchInit (init_path, (elemToType e), code)
  in
  let var_mapping_rules = L.map handle_var xform_reqs in
  let type_mapping_rules = ref [] in
  let handle_type_mapping (old_path, new_path) =
    if not (H.mem ctx.types_done (old_path, new_path)) then
      begin
        H.add ctx.types_done (old_path, new_path) false;
        list_ref_push type_mapping_rules (handle_match old_path new_path)
      end
  in
  let rec untilEmpty f lr =
    match !lr with
      | [] -> ()
      | first :: rest -> lr := rest; f first; untilEmpty f lr
  in
  untilEmpty handle_type_mapping ctx.types_todo;
  computeChangeFixpoint ctx;
  (var_mapping_rules @ !type_mapping_rules)

(* Dump a summary of the results of the comparison to stdout *)

let print_data results =
  print_endline "FULL SUMMARY";
  let print_types (t0:typ) (t1:typ) =
    let t0_str = typeToString t0 in
    let t1_str = typeToString t1 in 
    if t0_str = t1_str then
      print_endline ("[" ^ t0_str ^ "]")
    else
      print_endline ("[" ^ t0_str ^ " -> " ^ t1_str ^ "]")
  in
  let rec print_mi = function
    | MatchStruct (reordered, mappings) ->
      print_string " STRUCT";
      if reordered then print_string " REORDERED";
      print_string "\n";
      L.iter print_mapping mappings        
    | MatchUnion mappings -> print_endline " UNION"
    | MatchTypedef _ -> print_string " TYPEDEF "
    | MatchVar _ -> print_string  " VAR "
    | MatchEnum _ -> print_endline " ENUM"

  and print_mapping = function
    | MatchInit (e, _, _) ->
      print_endline ((Xflang.path_to_string e) ^ " INIT <code>")
    | MatchAuto ((e0, e1), (t0, t1), _, minfo) ->
      print_string ((Xflang.path_to_string e0) ^ " -> " ^ 
                        (Xflang.path_to_string e1) ^ " AUTO");
      print_mi minfo;
      print_types t0 t1
    | MatchUserCode ((e0, e1), _, _, _) ->
      print_endline ((Xflang.path_to_string e0) ^ " -> " ^ 
                        (Xflang.path_to_string e1) ^ " USER CODE <code>")
    | UnmatchedAdded e ->
      print_endline ((Xflang.path_to_string e) ^ " ADDED")
    | UnmatchedDeleted e ->
      print_endline ((Xflang.path_to_string e) ^ " DELETED")
    | UnmatchedType (e0, e1) ->
      print_endline ((Xflang.path_to_string e0) ^ " -> " ^ 
                        (Xflang.path_to_string e1) ^ " UNMATCHED TYPE")
    | UnmatchedError ((e0, e1), error) ->
      print_endline ((Xflang.path_to_string e0) ^ " -> " ^ 
                        (Xflang.path_to_string e1) ^ " ERROR " ^ error) 
  in
  L.iter print_mapping results


let print_results (ctx:compcontext) results =
  print_endline "MATCHES ATTEMPTED:";
  let handle_deps (old_path, new_path) _ =
    print_endline ((Xflang.path_to_string old_path) ^ " -> " ^ (Xflang.path_to_string new_path))
  in
  H.iter handle_deps ctx.matches;
  print_endline "MATCH FAILED:";
  let handle_deps (old_path, new_path) _ =
    print_endline ((Xflang.path_to_string old_path) ^ " -> " ^ (Xflang.path_to_string new_path))
  in  
  H.iter handle_deps ctx.match_failed;
  print_endline "REQUIRED TRANSFORMATIONS:";
  let handle_required (old_path, new_path) _ =
    print_endline ((Xflang.path_to_string old_path) ^ " -> " ^ (Xflang.path_to_string new_path))
  in
  H.iter handle_required ctx.requires_full_xform;
  print_endline "DEPENDENCIES:";
  let handle_deps (old_path, new_path) (dep_old_path, dep_new_path)  =
    print_endline ((Xflang.path_to_string dep_old_path) ^ " -> " ^ (Xflang.path_to_string dep_new_path) ^ " depends on " ^
                   (Xflang.path_to_string old_path)     ^ " -> " ^ (Xflang.path_to_string new_path))
  in
  H.iter handle_deps ctx.hard_deps;
  print_data results

(* Load the program element data from an .ktt file *)

let loadKttData (filename:string) =
  let kttData = loadKttData filename in
  (listifyKttElems kttData,
   listifyKttXformVars kttData)

