 
(* xfgen.ml
 * 
 * This module implements the generation of transformation code based
 * on the results of the comparison implemented in kttcompare.ml.  
 *)

open Kttools

open Kttypes.TransformTypes
open Kttypes.Tools
open Kttcompare

open Xftypes

module H = Hashtbl
module L = List
module S = String

(* The following types and functions maintain the current contextual
   information that is passed through the code as comparisons are
   occuring. *)

type elem_key = 
  | KeyTd of string
  | KeyFn of string
  | KeyO of string
let elem_key_to_s = function
  | KeyTd s -> s
  | KeyFn s -> s
  | KeyO s -> s

type gencontext = {
  cur_elem : elem_key option;
  cur_renamer : (string -> string) option;
  print_context : string;
  soft_deps : (elem_key, elem_key) H.t;
  hard_deps : (elem_key, elem_key) H.t;
  rendered : (elem_key, string) H.t;
  prototypes : (elem_key, string) H.t;
  xform_funs : elem_key list ref;
  xform_generics : (elem_key, string list) H.t;
  td_incomplete : (elem_key, typ) H.t;
  code : string ref;
}

let gencontext_create () = {
  cur_elem = None;
  cur_renamer = None;
  print_context = "";
  soft_deps = H.create 37;
  hard_deps = H.create 37;
  rendered = H.create 37;
  prototypes = H.create 37;
  xform_funs = ref [];
  xform_generics = H.create 37;
  td_incomplete = H.create 37;
  code = ref "";
}

let gencontext_append_symbol (gen_ctx:gencontext) (s0:string) (s1:string) = 
  let append_str = if s0 = s1 then s0 else s0 ^ " -> " ^ s1 in
  { 
    gen_ctx with print_context = 
      if gen_ctx.print_context = "" then append_str
      else gen_ctx.print_context ^ "." ^ append_str
  }

let gencontext_get_symbol (gen_ctx:gencontext) = gen_ctx.print_context

let genError gen_ctx (s:string) =
  exitError (gencontext_get_symbol gen_ctx ^ ": " ^ s)

let genWarning gen_ctx (s:string) =
  print_endline (gencontext_get_symbol gen_ctx ^ ": " ^ s)

let gencontext_add_dep (gen_ctx:gencontext) (is_hard:bool) (d:elem_key) =
  match gen_ctx.cur_elem with
    | Some f -> 
      let dep_hash = if is_hard then gen_ctx.hard_deps else gen_ctx.soft_deps in
      if f <> d && not (let deps = H.find_all dep_hash f in List.mem d deps) then
	begin 
	  let s = elem_key_to_s d in
	  (* print_endline ("Adding dependency "^s^" to curelem "^(elem_key_to_s f)); *)
          H.add dep_hash f d;
	end 
    | None -> genError gen_ctx ("gencontext_add_hard_dep called with no current item " ^ elem_key_to_s d)

let gencontext_mark_incomplete (gen_ctx:gencontext) (t:typ) =
  match gen_ctx.cur_elem with
    | Some ((KeyTd td_name) as c) -> 
      if not (H.mem gen_ctx.td_incomplete c) then
        begin
	  if not ((String.sub td_name 19 2) = "__") then (* filter gcc builtins *)
            print_endline (td_name ^ " is incomplete");
          H.add gen_ctx.td_incomplete c t
        end
    | Some _ ->
      genError gen_ctx ("Did not expect to find incomplete type during processing: " ^ typeToString t)
    | None -> 
      failwith "gencontext_mark_incomplete called with no current item"

let gencontext_add_render (gen_ctx:gencontext) (k:elem_key) (rendered:string) =
  if H.mem gen_ctx.rendered k then
    (if rendered <> H.find gen_ctx.rendered k then
        genWarning gen_ctx ("Multiple conflicting definitions for :" ^ elem_key_to_s k))
  else
    H.add gen_ctx.rendered k rendered

let gencontext_add_prototype (gen_ctx:gencontext) (k:elem_key) (proto:string) =
  if H.mem gen_ctx.prototypes k then
    assert (proto = H.find gen_ctx.prototypes k)
  else
    H.add gen_ctx.prototypes k proto

let gencontext_set_current (gen_ctx:gencontext) (current:elem_key) = 
  { gen_ctx with cur_elem = Some current }

let gencontext_set_renamer (gen_ctx:gencontext) (renamer:string->string) = 
  { gen_ctx with cur_renamer = Some renamer }

let gencontext_rename (gen_ctx:gencontext) (name:string) = 
  match gen_ctx.cur_renamer with
    | Some r -> r name
    | None -> failwith "gencontext_rename called with no current renamer"

let gencontext_add_xformer (gen_ctx:gencontext) (k:elem_key) (proto:string) (rendered:string) =
  gencontext_add_prototype gen_ctx k proto;
  gencontext_add_render gen_ctx k rendered;
  gen_ctx.xform_funs := k :: !(gen_ctx.xform_funs)

let gencontext_set_xform_generics gen_ctx xform_name (gen_args0, gen_args1) =
  let generics = (L.map (fun (GenName n) -> n) gen_args0) in
  if (H.mem gen_ctx.xform_generics xform_name) then
    begin
      let prev_generics = H.find gen_ctx.xform_generics xform_name in
      assert (generics = prev_generics);
    end;
  assert (gen_args0 = gen_args1);
  H.add gen_ctx.xform_generics xform_name generics

let gencontext_get_xform_generics gen_ctx xform_name =
  assert (H.mem gen_ctx.xform_generics xform_name);
  H.find gen_ctx.xform_generics xform_name 

(* These renamers are used to ensure that the type information that we
   output to the generated file will not collide with other
   definitions from the original program that are #included in the
   global C block. *)

let old_rename (name:string) =
  "_kitsune_old_rename_" ^ name

let new_rename (name:string) =
  "_kitsune_new_rename_" ^ name

(* The following code implements rendering program elements (only the
   type definitions) into the generated file. *)

let rec render_type gen_ctx (t:typ) (under_ptr:bool) (nameopt:string option) = 
  let pname = match nameopt with
      Some v -> v | None -> ""
  in        
  match t with
    | TVoid _ -> 
      if (not under_ptr) then gencontext_mark_incomplete gen_ctx t;
      "void " ^ pname

    | TInt ik -> (ikindToString ik) ^ " " ^ pname
    | TFloat fk -> (fkindToString fk) ^ " " ^ pname
    | TPtr (pt, _, _)
    | TPtrOpaque pt
    | TPtrArray (pt, _, _) ->
      render_type gen_ctx pt true (Some ("(*" ^ pname ^ ")"))        
      
    | TArray (t', Len_Int lv) -> 
      render_type gen_ctx t' under_ptr (Some (pname ^ "[" ^ (Int64.to_string lv) ^ "]"))
    | TArray (t', _) ->
      render_type gen_ctx t' under_ptr (Some (pname ^ "[]"))
        
    | TFun (rt, args, is_varargs) ->
      if (not under_ptr) then
        gencontext_mark_incomplete gen_ctx t;
      
      let writeArg (aname, atype) =
        render_type gen_ctx atype true (Some aname)
      in
      let arg_str_list = 
        if is_varargs then
          (L.map writeArg args) @ ["..."]
        else
          (L.map writeArg args)
      in
      let arg_str = S.concat ", " arg_str_list in
      render_type gen_ctx rt true (Some (pname ^ "(" ^ arg_str ^ ")"))

    | TNamed (nm, _) ->
      let td_name = gencontext_rename gen_ctx nm in
      gencontext_add_dep gen_ctx (not under_ptr) (KeyTd td_name);
      td_name ^ " " ^ pname
    | TStruct (nm, _) ->
      let struct_name = "struct " ^ (gencontext_rename gen_ctx nm) in
      gencontext_add_dep gen_ctx (not under_ptr) (KeyO struct_name); 
      struct_name ^ " " ^ pname
    | TUnion (nm, _) ->
      let union_name = "union " ^ (gencontext_rename gen_ctx nm) in
      gencontext_add_dep gen_ctx (not under_ptr) (KeyO union_name);
      union_name ^ " " ^  pname
    | TEnum nm ->
      let enum_name = "enum " ^ (gencontext_rename gen_ctx nm) in
      gencontext_add_dep gen_ctx (not under_ptr) (KeyO enum_name);
      enum_name ^ " " ^ pname

    | TBuiltin_va_list -> 
      "__builtin_va_list " ^ pname

let render_field gen_ctx (finfo:field) =
  let bitfield_string = 
    match finfo.fbitfield with
      | None -> ""
      | Some sz -> ":" ^ (string_of_int sz)
  in
  (render_type gen_ctx finfo.ftype false (Some finfo.fname)) ^ bitfield_string ^ ";\n"

let handle_typedef gen_ctx (tinfo:typedef_info) =  
  let td_name = gencontext_rename gen_ctx tinfo.tname in
  (* let () = print_endline ("Handling typedef "^tinfo.tname^" (curelem "^td_name^")") in *)
  let gen_ctx' = gencontext_set_current gen_ctx (KeyTd td_name) in
  gencontext_add_render gen_ctx' (KeyTd td_name)
    ("typedef " ^
        (render_type gen_ctx' tinfo.ttype false (Some (gencontext_rename gen_ctx' tinfo.tname))) ^
        ";\n")

let handle_struct gen_ctx (sinfo:struct_info) =
  let struct_name = "struct " ^ (gencontext_rename gen_ctx sinfo.sname) in
  let gen_ctx' = gencontext_set_current gen_ctx (KeyO struct_name) in
  gencontext_add_prototype gen_ctx' (KeyO struct_name) (struct_name ^ ";\n");
  gencontext_add_render gen_ctx' (KeyO struct_name)
    (struct_name ^ " {\n" ^ 
       (S.concat "" (L.map (render_field gen_ctx') sinfo.sfields)) ^ 
       "};\n")

let handle_gvar gen_ctx (vinfo:var_info) =
  (* We don't render global variables in the generated transformer file *)
  ()

let render_enum_elems gen_ctx (einfo:enumeration_info) = 
  (* This is broken, if we start supporting enums, we need to render
     renamed versions of the enum values *)
  " { PLACEHOLDER_" ^ (gencontext_rename gen_ctx einfo.ename) ^ " };\n"

let handle_enum gen_ctx (einfo:enumeration_info) =
  let enum_name = "enum " ^ gencontext_rename gen_ctx einfo.ename in
  gencontext_add_prototype gen_ctx (KeyO enum_name) (enum_name ^ ";\n");
  gencontext_add_render gen_ctx (KeyO enum_name) (enum_name ^ render_enum_elems gen_ctx einfo)

let handle_union gen_ctx (sinfo:struct_info) =
  let union_name = "union " ^ (gencontext_rename gen_ctx sinfo.sname) in
  let gen_ctx' = gencontext_set_current gen_ctx (KeyO union_name) in
  gencontext_add_prototype gen_ctx (KeyO union_name) (union_name ^ ";\n");
  gencontext_add_render gen_ctx (KeyO union_name)
    (union_name ^ " {\n" ^ 
       (S.concat "" (L.map (render_field gen_ctx') sinfo.sfields)) ^
       "};\n")
  
let handle_elem gen_ctx (_, e) =
  match e with
    | Struct sinfo -> handle_struct gen_ctx sinfo
    | Union sinfo -> handle_union gen_ctx sinfo
    | Typedef tinfo -> handle_typedef gen_ctx tinfo
    | Enumeration einfo -> handle_enum gen_ctx einfo
    | Var vinfo -> handle_gvar gen_ctx vinfo      
 
(* The next section implements transformation function body generation. *)

let render_func_proto (name:string) (args : string list) renamer =
  "void " ^ name ^ "(" ^ (S.concat "," args) ^ ");\n"

let render_func (name:string) (args : string list) (body:string) renamer =
  "void " ^ name ^ "(" ^ (S.concat "," args) ^ ") {\n" ^ body ^"\n}\n"

let render_path_elem = function
  | PStruct s -> "struct_" ^ s
  | PUnion s -> "union_" ^ s
  | PTypedef s -> "typedef_" ^ s
  | PEnum s -> "enum_" ^ s
  | PField s -> s
  | PVar s -> 
    if S.contains s '#' then
      let idx = S.index s '#' in
      "local_" ^ (S.sub s (idx+1) ((S.length s) - (idx+1))) ^
        "_fun_" ^ (S.sub s 0 idx)
    else
      s

let deref n = "(*" ^ n ^ ")"
let addrof n = "&(" ^ n ^ ")"
let sizeof n = "sizeof(" ^ n ^ ")"
let compute_gen_arg_var_name gen_arg_name = "gen_" ^ gen_arg_name;;

let render_xform_func_name e_old e_new =
  match L.hd e_old, L.hd e_new with
    | fe, te ->
      "_kitsune_transform_" ^ (render_path_elem fe) ^ "_to_" ^ (render_path_elem te)

let render_init_func_name e_new = 
  match L.hd e_new with
    | PVar k -> Ktdecl.xform_from_key k
    | _ -> failwith "Unexpected non-var argument to render_init_func_name"

let rec render_usercode gen_ctx compare_ctx (repl_in:string option) (repl_out:string option) (repl_base:string option) (code:string) =
  let handle_match_fun s sargs =
    match s with
      | "$out" -> 
        (match repl_out with 
          | None -> genError gen_ctx "$out not allowed in this context"
          | Some out_expr -> Some ((), out_expr))

      | "$in" -> 
        (match repl_in with
          | None -> genError gen_ctx "$in not allowed in this context"
          | Some in_expr -> Some ((), in_expr))

      | "$oldtype" ->
        begin
          match sargs with
            | [type_name] -> 
              let t = Kttypes.Tools.parseGenericBindingType type_name [] ignore in
              let r = render_type (gencontext_set_renamer gen_ctx old_rename) t false None in
              (* gencontext_add_dep gen_ctx r; *)
              Some ((), r)
            | _ ->
              genError gen_ctx "$old_t requires a single type argument"
        end

      | "$newtype" ->
        begin
          match sargs with
            | [type_name] -> 
              let t = Kttypes.Tools.parseGenericBindingType type_name [] ignore in
              let r = render_type (gencontext_set_renamer gen_ctx new_rename) t false None in
              (* gencontext_add_dep gen_ctx r; *)
              Some ((), r)
            | _ ->
              genError gen_ctx "$newtype requires a single type argument"
        end

      | "$base" -> 
        (match repl_base with
          | None -> genError gen_ctx "$base not allowed in this context"
          | Some base_expr -> Some ((), base_expr))

      | "$gen" -> 
        (* TODO: add checking that this generic variable exists in this context *)
        assert (List.length sargs = 1);         
        Some ((), (compute_gen_arg_var_name (List.nth sargs 0)))

      | "$xform" -> 
        begin
          match sargs with
            | tn0 :: tn1 :: other_args -> 
              let t0 = Kttypes.Tools.parseGenericBindingType tn0 [] ignore in
              let t1 = Kttypes.Tools.parseGenericBindingType tn1 [] ignore in        
              let (e0, _), (e1, _) = 
                match (path_elem_from_type t0), (path_elem_from_type t1) with 
                | Some e0, Some e1 -> e0, e1
                | _ -> genError gen_ctx "Error: attempt to reference transformation code involving a primitive."
              in
              let xform_name = render_xform_func_name [e0] [e1] in
              gencontext_add_dep gen_ctx false (KeyFn xform_name);
              let deep_xform = Hashtbl.mem compare_ctx.requires_full_xform ([e0], [e1]) in
	      (* should the below be deleted? *)
              (* let shallow_xform = Hashtbl.mem compare_ctx.requires_shallow_xform ([e0], [e1]) in *)
              let copy_opt = if deep_xform then "XF_DEEP" else "XF_SHALLOW" in
              let result =
                if (List.length other_args > 0) then
                  "XF_CLOSURE(" ^ xform_name ^ ", " ^ copy_opt ^ ", " ^
                    sizeof (render_type (gencontext_set_renamer gen_ctx old_rename) t0 false None) ^ ", " ^
                    sizeof (render_type (gencontext_set_renamer gen_ctx new_rename) t1 false None) ^ ", " ^
                    (S.concat ", " (List.map (render_usercode gen_ctx compare_ctx repl_in repl_out repl_base) other_args)) ^
                    ")"
                else
                  "XF_LIFT(" ^ xform_name ^ ", " ^ copy_opt ^ ", " ^
                    sizeof (render_type (gencontext_set_renamer gen_ctx old_rename) t0 false None) ^ ", " ^
                    sizeof (render_type (gencontext_set_renamer gen_ctx new_rename) t1 false None) ^ 
                    ")"
              in
              Some ((), result)
            | _ -> genError gen_ctx ("xfgen.ml: render_usercode ($xform(t1, t2, ...) requires two type arguments:" ^code)
        end
      | _ -> None
  in
  snd (Usercode.handle_user_code handle_match_fun code)
;;

let rec check_toplevel_type pred (t0:typ) (t1:typ) =
  let p0 = Kttcompare.path_elem_from_type t0 in
  let p1 = Kttcompare.path_elem_from_type t1 in
  match p0, p1 with
    | Some (p0, _), Some (p1, _) -> pred ([p0], [p1])      
    | _, _ -> 
      match t0, t1 with
        | TArray(t0', l0), TArray(t1', l1) ->
          l0 = l1 && check_toplevel_type pred t0' t1'
        | _, _ -> false
;;
      
let generate_trans_func calling_gen_ctx compare_ctx e =
  let rec generate_genin_xform gen_ctx gd0 gd1 len_base =
    match gd0, gd1 with
      | OtherGenericBinding (GenName n0), OtherGenericBinding (GenName n1) ->
        assert (n0 = n1);
        (compute_gen_arg_var_name n0)
      | ProgramType t0, ProgramType t1 ->
        generate_xform gen_ctx t0 t1 len_base
      | PtrTo gd0', PtrTo gd1' ->
        "XF_PTR(" ^ (generate_genin_xform gen_ctx gd0' gd1' len_base) ^ ")"
      | _ -> genError gen_ctx "Unexpected: unmatched generic descriptors"
  and generate_xform gen_ctx t0 t1 len_base : string =
    let len_to_string = function
      | Len_Unknown -> 
        genError gen_ctx "Cannot generate serialization when the field length is unknown"
      | Len_Int l -> Int64.to_string l 
      | Len_Field l -> 
        (match len_base with
          | None -> l
          | Some s -> s ^ "." ^ l)
      | Len_Nullterm ->
        failwith "Not yet handled"
    in
    match t0, t1 with
      | TPtr (_, Some (GenName gv0), _), TPtr (_, Some (GenName gv1), _) ->
        assert (gv0 = gv1);
        (compute_gen_arg_var_name gv0)

      (* Ultimately, the following two cases will produce errors.
         Need to decide whether to issue the errors here or wait
         until they occur in the recursive call... for now, the
         choice is to wait. *)
      | TNamed (nm, _), _ when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->
          generate_xform gen_ctx (H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))) t1 len_base
      | _, TNamed (nm, _) when H.mem gen_ctx.td_incomplete (KeyTd (new_rename nm)) ->
          generate_xform gen_ctx t0 (H.find gen_ctx.td_incomplete (KeyTd (new_rename nm))) len_base

      | TPtr (TNamed (nm, _), gen_name, mem_fns), _ when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->
          generate_xform gen_ctx (TPtr ((H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))), gen_name, mem_fns)) t1 len_base
      | _, TPtr (TNamed (nm, _), gen_name, mem_fns) when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->
          generate_xform gen_ctx t0 (TPtr ((H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))), gen_name, mem_fns)) len_base

      | TFun _, TFun _ ->
          (* make this an assertion as it is unexpected behavior *)
          (* genError gen_ctx "Cannot generate code for a fun -> fun transformation." *)
          "(assert(0),NULL)"

      | TPtr (TFun _, _, _), TPtr (TFun _, _, _) ->
        "XF_FPTR()"

      | TPtr (t0', _, _), TPtr (t1', _, _) ->
        "XF_PTR(" ^ (generate_xform gen_ctx t0' t1' len_base) ^ ")"

      | TPtrArray (t0', len0, _), TPtrArray (t1', len1, _) ->
        assert(len0 = len1); (* this is probably not quite right *)
        begin
          match len0 with
            | Len_Nullterm ->
              "XF_NTARRAY(" ^
                sizeof (render_type (gencontext_set_renamer gen_ctx old_rename) t0' false None) ^ ", " ^
                sizeof (render_type (gencontext_set_renamer gen_ctx new_rename) t1' false None) ^ ", " ^
                (generate_xform gen_ctx t0' t1' len_base) ^ ")"
            | _ ->
              "XF_PTRARRAY(" ^
                (len_to_string len0) ^ ", " ^
                "sizeof(" ^ render_type (gencontext_set_renamer gen_ctx old_rename) t0' false None ^ "), " ^
                "sizeof(" ^ render_type (gencontext_set_renamer gen_ctx new_rename) t1' false None ^ "), " ^
                (generate_xform gen_ctx t0' t1' len_base) ^ ")"
        end

      | TArray (t0', len0), TArray(t1', len1) ->
        assert(len0 = len1); (* this is probably not quite right *)
        "XF_ARRAY(" ^
          (len_to_string len0) ^ ", " ^
          sizeof (render_type (gencontext_set_renamer gen_ctx old_rename) t0' false None) ^ ", " ^
          sizeof (render_type (gencontext_set_renamer gen_ctx new_rename) t1' false None) ^ ", " ^
          (generate_xform gen_ctx t0' t1' len_base) ^ ")"
                              
      | TInt _, TInt _
      | TFloat _, TFloat _
      | TEnum _, TEnum _
      | TBuiltin_va_list, TBuiltin_va_list
      | TUnion _, TUnion _
      | TPtrOpaque _, TPtrOpaque _ ->
        "XF_RAW(" ^ sizeof (render_type (gencontext_set_renamer gen_ctx new_rename) t1 false None) ^ ")"

      | TVoid None, TVoid None ->
        genError gen_ctx "Cannot generate code for a void -> void transformation without generic annotations."

      | TVoid (Some (GenName gv0)), TVoid (Some (GenName gv1)) ->
        assert (gv0 = gv1);
        (compute_gen_arg_var_name gv0)
          
      | _ ->
        let gen_in0 = getTypeGenIns t0 in
        let gen_in1 = getTypeGenIns t1 in
        let render_gen_arg gd0 gd1 : string =
          generate_genin_xform gen_ctx gd0 gd1 len_base
        in
        let path_elem0, _ = option_get_unsafe (path_elem_from_type t0) in
        let path_elem1, _ = option_get_unsafe (path_elem_from_type t1) in
        let deep_xform = Hashtbl.mem compare_ctx.requires_full_xform ([path_elem0], [path_elem1]) in
        let shallow_xform = Hashtbl.mem compare_ctx.requires_shallow_xform ([path_elem0], [path_elem1]) in
        if deep_xform || shallow_xform then
          begin
            let target_fun = render_xform_func_name [path_elem0] [path_elem1] in
            gencontext_add_dep gen_ctx false (KeyFn target_fun);
            let generic_args = gencontext_get_xform_generics calling_gen_ctx (KeyFn target_fun) in
            if not ((L.length gen_in0) = (L.length gen_in1) && (L.length gen_in1) = (L.length generic_args)) then
              genError gen_ctx ("Generic arguments for element do match the expected arguments for its type (" ^
                                   (Xflang.path_to_string [path_elem0] ^ " -> " ^ Xflang.path_to_string [path_elem1]) ^ ")");

            let copy_opt = if deep_xform then "XF_DEEP" else "XF_SHALLOW" in                        
            if (L.length generic_args) = 0 then
              "XF_LIFT(" ^ target_fun ^ ", " ^ copy_opt ^ ", " ^
                sizeof (render_type (gencontext_set_renamer gen_ctx old_rename) t0 false None) ^ ", " ^
                sizeof (render_type (gencontext_set_renamer gen_ctx new_rename) t1 false None) ^ ")"
            else
              "XF_CLOSURE(" ^ target_fun ^ ", " ^ copy_opt ^ ", " ^
                sizeof (render_type (gencontext_set_renamer gen_ctx old_rename) t0 false None) ^ ", " ^
                sizeof (render_type (gencontext_set_renamer gen_ctx new_rename) t1 false None) ^ ", " ^
                (S.concat ", " (L.map2 render_gen_arg gen_in0 gen_in1)) ^ ")"
          end
        else
          "XF_RAW(" ^ sizeof (render_type (gencontext_set_renamer gen_ctx new_rename) t1 false None) ^ ")"
  in
  let generate_field_xform gen_ctx full_xform old_name new_name m =
    match m with
      | MatchInit (me1, _, code) ->
        begin
          assert full_xform;
          match me1 with
            | PField fname1 :: _ ->
              let gen_ctx = gencontext_append_symbol gen_ctx "" fname1 in
              "{" ^ (render_usercode gen_ctx compare_ctx None (Some (new_name ^ "->" ^ fname1)) 
                       (Some (deref old_name)) code) ^ "}\n"
            | _ -> genError gen_ctx "Unexpected: paths for fields should be PFields"
        end

      | MatchAuto ((me0, me1), (t0, t1), _, MatchVar (_, _)) ->
        begin
          let deref_old, deref_new = deref old_name, deref new_name in
          match me0, me1 with
            | PField fname0 :: _, PField fname1 :: _ ->
              let field_full_xform = Hashtbl.mem compare_ctx.requires_full_xform (me0, me1) in
              let field_ptr_xform = Hashtbl.mem compare_ctx.requires_shallow_xform (me0, me1) in
              let field_is_shallow = 
                (check_toplevel_type (Hashtbl.mem compare_ctx.requires_shallow_xform) t0 t1) &&
                  not (check_toplevel_type (Hashtbl.mem compare_ctx.requires_full_xform) t0 t1)
              in 
              let gen_ctx = gencontext_append_symbol gen_ctx fname0 fname1 in
              let copy_op = 
                if full_xform && field_is_shallow then
                  "XF_ASSIGN(" ^ 
                    (deref_new ^ "." ^ fname1) ^ ", " ^
                    (deref_old ^ "." ^ fname0) ^ ");\n"
                else ""
              in
              let transform_op = 
                if full_xform || field_full_xform || field_ptr_xform then
                  "XF_INVOKE(" ^ (generate_xform gen_ctx t0 t1 (Some deref_old)) ^ ", " ^
                    (addrof (deref_old ^ "." ^ fname0)) ^ ", " ^
                    (addrof (deref_new ^ "." ^ fname1)) ^ ");\n"
                else ""
              in
              copy_op ^ transform_op
            | _ -> genError gen_ctx "Unexpected: paths for fields should be PFields"
        end

      | MatchAuto ((me0, me1), (t0, t1), _, _) ->
        failwith "generate_field_xform: unepected elem type"
          
      | MatchUserCode ((me0, me1), (t0, t1), _, code) -> 
        begin
          match me0, me1 with
            | PField fname0 :: _, PField fname1 :: _ ->
              let (src_var, src_var_init) =
                if full_xform then
                  (old_name ^ "->" ^ fname0, "") 
                else
                  let tmp_name = "fld_tmp" in
                  (tmp_name, (render_type (gencontext_set_renamer gen_ctx old_rename) t0 false (Some tmp_name)) ^ " = " ^ old_name ^ "->" ^ fname0 ^ ";\n")                  
              in
              let gen_ctx = gencontext_append_symbol gen_ctx fname0 fname1 in
              "{" ^ src_var_init ^ 
                (render_usercode gen_ctx compare_ctx (Some (src_var)) (Some (new_name ^ "->" ^ fname1)) (Some (deref old_name)) code) ^ "}\n"
            | _ -> failwith "Unexpected: paths for fields should be PFields"
        end
      | UnmatchedDeleted _ -> ""
      | UnmatchedAdded _ | UnmatchedType _ | UnmatchedError _ ->
        failwith "Unexpected unmatched element found in generate_trans_fun"
  in
  let generate_auto_body gen_ctx full_xform ptr_xform in_var out_var t0 t1 (g0_out, g1_out) minfo =
    match minfo with
      | MatchStruct (order_changed, field_matches) ->
        S.concat "" (L.map (generate_field_xform gen_ctx full_xform in_var out_var) field_matches)
      | MatchUnion (field_matches) -> 
        (* FIXME: this is broken - we shouldn't do transformation for all elements of a union *)
        S.concat "\n" (L.map (generate_field_xform gen_ctx full_xform in_var out_var) field_matches)
      | MatchTypedef (t0', t1') ->
        "XF_INVOKE(" ^ (generate_xform gen_ctx t0' t1' None) ^ ", " ^ in_var ^ ", " ^ out_var ^ ");"
      | MatchVar (g0_in, g1_in) ->
        "XF_INVOKE(" ^ (generate_xform gen_ctx t0 t1 None) ^ ", " ^ in_var ^ ", " ^ out_var ^ ");"
      | MatchEnum ->
        "XF_INVOKE(" ^ (generate_xform gen_ctx t0 t1 None) ^ ", " ^ in_var ^ ", " ^ out_var ^ ");"
  in
  let generics_decls arg_var num_args_var generics =
    let create_generic_local idx gen_name =
      "closure *" ^ (compute_gen_arg_var_name gen_name) ^ " = " ^ 
        arg_var ^ "[" ^ (string_of_int idx) ^ "];\n"
    in
    "assert(num_args == " ^ (string_of_int (L.length generics)) ^ ");\n" ^
      (S.concat "" (map_index create_generic_local generics))
  in
  match e with
    | MatchInit (([PVar key_new] as me1), t1, code) ->
      begin
        let var_out = "local_out" in
        let fname = render_init_func_name me1 in
        let gen_ctx = gencontext_set_current calling_gen_ctx (KeyFn fname) in
        let gen_ctx = gencontext_append_symbol gen_ctx "" key_new in
        let lookup_new =
          (render_type (gencontext_set_renamer gen_ctx new_rename) (ptrTo t1) false (Some var_out)) ^ ";\n" ^
            var_out ^ " = " ^ Ktdecl.render_get_val_new_call key_new ^ ";\n" ^
            "assert(" ^ var_out ^ ");\n"
        in
        let body = lookup_new ^ 
          "{" ^ (render_usercode gen_ctx compare_ctx None (Some (deref var_out)) None code) ^ "}"
        in
        let proto = render_func_proto fname [] id in
        let impl = render_func fname [] body id in
        gencontext_add_xformer gen_ctx (KeyFn fname) proto impl
      end

    | MatchUserCode (([PVar key_old], ([PVar key_new] as me1)), (t0, t1), generics, code) -> 
        begin
          let (var_in, var_out) = ("local_in", "local_out") in
          let fname = render_init_func_name me1 in
          let gen_ctx = gencontext_set_current calling_gen_ctx (KeyFn fname) in
          let gen_ctx = gencontext_append_symbol gen_ctx key_old key_new in
          let lookup_old =
            (render_type (gencontext_set_renamer gen_ctx old_rename) (ptrTo t0) false (Some var_in)) ^ ";\n" ^
              var_in ^ " = " ^ Ktdecl.render_get_val_old_call key_old ^ ";\n" ^
              "assert(" ^ var_in ^ ");\n"
          in
          let lookup_new =
            (render_type (gencontext_set_renamer gen_ctx new_rename) (ptrTo t1) false (Some var_out)) ^ ";\n" ^
              var_out ^ " = " ^ Ktdecl.render_get_val_new_call key_new ^ ";\n" ^
              "assert(" ^ var_out ^ ");\n"
          in
          let body = lookup_old ^ lookup_new ^ 
            (render_usercode gen_ctx compare_ctx (Some (deref var_in)) (Some (deref var_out)) None code)
          in
          let proto = render_func_proto fname [] id in
          let impl = render_func fname [] body id in
          gencontext_add_xformer gen_ctx (KeyFn fname) proto impl
        end

    | MatchAuto ((([PVar key_old] as me0), ([PVar key_new] as me1)), (t0, t1), generics, MatchVar (g0_in, g1_in)) ->
      let full_xform = Hashtbl.mem compare_ctx.requires_full_xform (me0, me1) in
      let ptr_xform = Hashtbl.mem compare_ctx.requires_shallow_xform (me0, me1) in
      let var_is_shallow = 
        (check_toplevel_type (Hashtbl.mem compare_ctx.requires_shallow_xform) t0 t1) &&
          not (check_toplevel_type (Hashtbl.mem compare_ctx.requires_full_xform) t0 t1)
      in
      let (var_in, var_out) = ("local_in", "local_out") in
      let fname = render_init_func_name me1 in
      let gen_ctx = gencontext_set_current calling_gen_ctx (KeyFn fname) in
      let gen_ctx = gencontext_append_symbol gen_ctx key_old key_new in
      let lookup_old =
        (render_type (gencontext_set_renamer gen_ctx old_rename) (ptrTo t0) false (Some var_in)) ^ ";\n" ^
          var_in ^ " = " ^ Ktdecl.render_get_val_old_call key_old ^ ";\n" ^
          "assert(" ^ var_in ^ ");\n"
      in
      let lookup_new =
        (render_type (gencontext_set_renamer gen_ctx new_rename) (ptrTo t1) false (Some var_out)) ^ ";\n" ^
          var_out ^ " = " ^ Ktdecl.render_get_val_new_call key_new ^ ";\n" ^
          "assert(" ^ var_out ^ ");\n"
      in
      let copy_op = 
        if var_is_shallow || not (full_xform || ptr_xform) then 
          "XF_ASSIGN(" ^ (deref var_out) ^ ", " ^ (deref var_in) ^ ");\n" 
        else ""
      in
      let xform_call = 
        if full_xform || ptr_xform then
          generate_auto_body gen_ctx full_xform ptr_xform var_in var_out t0 t1 generics (MatchVar (g0_in, g1_in))
        else ""
      in
      let body = lookup_old ^ lookup_new ^ copy_op ^ xform_call in
      let proto = render_func_proto fname [] id in
      let impl = render_func fname [] body id in
      gencontext_add_xformer gen_ctx (KeyFn fname) proto impl
        
    | MatchAuto ((me0, me1), (t0, t1), generics, minfo) ->
      let full_xform = Hashtbl.mem compare_ctx.requires_full_xform (me0, me1) in
      let ptr_xform = Hashtbl.mem compare_ctx.requires_shallow_xform (me0, me1) in      
      if full_xform || ptr_xform then
        begin
          let (var_in, var_out, num_args_var, args_var) = ("arg_in", "arg_out", "num_args", "args") in
          let local_in, local_out = ("local_in", "local_out") in
          let fname = render_xform_func_name me0 me1 in
          let gen_ctx = gencontext_set_current calling_gen_ctx (KeyFn fname) in      
          let gen_ctx = gencontext_append_symbol gen_ctx (Xflang.path_to_string me0) (Xflang.path_to_string me1) in
          let args = [ "void *" ^ var_in; "void *" ^ var_out; "int " ^ num_args_var; "void **" ^ args_var] in
          let local_init = 
            render_type (gencontext_set_renamer gen_ctx old_rename) (ptrTo t0) false (Some local_in) ^ " = " ^ var_in ^ ";\n" ^
            render_type (gencontext_set_renamer gen_ctx new_rename) (ptrTo t1) false (Some local_out) ^ " = " ^ var_out ^ ";\n"
          in
          let body =
            local_init ^
              (generics_decls args_var num_args_var (gencontext_get_xform_generics gen_ctx (KeyFn fname))) ^
              (generate_auto_body gen_ctx full_xform ptr_xform local_in local_out t0 t1 generics minfo)
          in
          let proto = render_func_proto fname args id in
          let impl = render_func fname args body id in
          gencontext_add_xformer gen_ctx (KeyFn fname) proto impl
        end
            
    | MatchUserCode ((me0, me1), (t0, t1), generics, code) -> 
      let full_xform = Hashtbl.mem compare_ctx.requires_full_xform (me0, me1) in
      let ptr_xform = Hashtbl.mem compare_ctx.requires_shallow_xform (me0, me1) in
      if full_xform || ptr_xform then
        begin
          let (var_in, var_out, num_args_var, args_var) = ("arg_in", "arg_out", "num_args", "args") in
          let local_in, local_out = ("local_in", "local_out") in
          let fname = render_xform_func_name me0 me1 in
          let gen_ctx = gencontext_set_current calling_gen_ctx (KeyFn fname) in
          let gen_ctx = gencontext_append_symbol gen_ctx (Xflang.path_to_string me0) (Xflang.path_to_string me1) in
          let args = [ "void *" ^ var_in; "void *" ^ var_out; "int " ^ num_args_var; "void **" ^ args_var] in
          let local_init = 
            render_type (gencontext_set_renamer gen_ctx old_rename) (ptrTo t0) false (Some local_in) ^ " = " ^ var_in ^ ";\n" ^
            render_type (gencontext_set_renamer gen_ctx new_rename) (ptrTo t1) false (Some local_out) ^ " = " ^ var_out ^ ";\n"
          in
          let body =
            local_init ^              
            (generics_decls args_var num_args_var (gencontext_get_xform_generics gen_ctx (KeyFn fname))) ^
              "\n{" ^ (render_usercode gen_ctx compare_ctx (Some (deref local_in)) (Some (deref local_out)) None code) ^ "}"
          in
          let proto = render_func_proto fname args id in
          let impl = render_func fname args body id in
          gencontext_add_xformer gen_ctx (KeyFn fname) proto impl
        end

    | UnmatchedDeleted me -> ()
    | UnmatchedAdded _ | UnmatchedType _ | UnmatchedError _ | MatchInit _ -> 
      failwith "Unexpected unmatched element found in generate_trans_fun"


let grab_generic_sigs gen_ctx = function
  | MatchInit _ | UnmatchedDeleted _ | UnmatchedAdded _ | UnmatchedType _
  | UnmatchedError _ | MatchAuto (_, _, _, MatchVar (_, _)) ->
    ()
  | MatchUserCode ((me0, me1), _, generics, _)
  | MatchAuto ((me0, me1), _, generics, _) ->
    (*print_endline (Xflang.path_to_string me0 ^ " -> " ^ Xflang.path_to_string me1);*)
    gencontext_set_xform_generics gen_ctx (KeyFn (render_xform_func_name me0 me1)) generics


let handle_undefined_types gen_ctx =
  let iter_fun _ dep_key =
    if not (H.mem gen_ctx.rendered dep_key) then
      match dep_key with
        | KeyTd name -> 
          print_endline ("ERROR: cannot generate prototype for typedef: " ^ name)
        | _ -> 
          H.add gen_ctx.prototypes dep_key (elem_key_to_s dep_key ^ ";")            
  in
  H.iter iter_fun gen_ctx.soft_deps;
  H.iter iter_fun gen_ctx.hard_deps

let register_renames rules =
  let map_fun = function 
    | Xform ([PVar v0], [PVar v1], _) ->
      "transform_register_renaming(\"" ^ v0 ^ "\", \"" ^ v1 ^ "\");\n"
    | _ -> ""
  in
  "__attribute__((constructor)) void _kitsune_register_renames() {\n" ^ 
    (S.concat "" (L.map map_fun rules)) ^
    "}\n"



let write_ordered_symbols (chan:out_channel) gen_ctx =
  let written = H.create 37 in
  let started = H.create 37 in
  let prototype_written = H.create 37 in
  let prototype_started = H.create 37 in
  let funs = ref [] in
  let rec write_rendered (hard:bool) (key:elem_key) =
    let rec handle_deps hard key =
      let handle_dep h k =
        match k with
          | KeyTd n when h ->
            write_rendered false k;
            List.iter (write_rendered true) (H.find_all gen_ctx.hard_deps k)
          | _ -> 
            write_rendered h k
      in
      L.iter (handle_dep false) (H.find_all gen_ctx.soft_deps key);
      L.iter (handle_dep hard) (H.find_all gen_ctx.hard_deps key);
    in
    if not (H.mem written key) then
      begin
        if H.mem started key || not hard then 
          begin
            match key with
              | KeyTd td_name ->
                if (not (H.mem written key)) && H.mem started key then 
                  exitError ("ERROR circular dependency for " ^ td_name)
                      
                H.add started key ();
                handle_deps hard key;
                (* Prevent Not_found excption in hash table *)
		let outp =
      		    try H.find gen_ctx.rendered key
      		    with Not_found -> exitError( "ERROR No entry found for key" ^ elem_key_to_s key)
		in output_string chan outp;
                H.add written key ();
              | KeyO n | KeyFn n ->
                if (not (H.mem prototype_written key)) && (H.mem prototype_started key)  then
                  exitError ("ERROR: circular dependencies in prototypes involving " ^ elem_key_to_s key)
                else if not (H.mem prototype_written key) then
                  begin
                    H.add prototype_started key ();
                    H.add prototype_written key ();
                    if H.mem gen_ctx.prototypes key then
                      match key with
                        | KeyFn _ -> funs := (H.find gen_ctx.prototypes key) :: !funs
                        | _ -> output_string chan (H.find gen_ctx.prototypes key)
                    else
                      exitError ("ERROR: could not render prototype for: " ^ elem_key_to_s key);
                  end
          end
        else 
          begin
            H.add started key ();
            handle_deps true key;
            H.add written key ();
            if H.mem gen_ctx.rendered key then
              match key with
                | KeyFn _ -> funs := (H.find gen_ctx.rendered key) :: !funs
                | _ -> output_string chan (H.find gen_ctx.rendered key)
            else
              print_endline ("ERROR: could not render: " ^ elem_key_to_s key);
          end
      end
  in
  L.iter (write_rendered true) !(gen_ctx.xform_funs);
  output_string chan !(gen_ctx.code);
  let check_prototypes key _ =
    if not (H.mem written key) then
      write_rendered true key
  in
  H.iter check_prototypes prototype_written;
  L.iter (output_string chan) (List.rev !funs)




let generate_file (chan:out_channel) compare_ctx preamble rename_fun v0_elems v1_elems results =
  let gen_ctx = gencontext_create () in
  output_string chan "#define E_NOANNOT\n";
  output_string chan "#include <kitsune.h>\n";
  output_string chan "#include <transform.h>\n";
  output_string chan "#include <assert.h>\n";
  output_string chan "#include <stdlib.h>\n";
  output_string chan rename_fun;
  (match preamble with
    | None -> ();
    | Some code -> gen_ctx.code := code);
  L.iter (handle_elem (gencontext_set_renamer gen_ctx old_rename)) v0_elems;
  L.iter (handle_elem (gencontext_set_renamer gen_ctx new_rename)) v1_elems;
  handle_undefined_types gen_ctx;
  L.iter (grab_generic_sigs gen_ctx) results;
  L.iter (generate_trans_func gen_ctx compare_ctx) results;
  write_ordered_symbols chan gen_ctx

    
    
let rec check_trans mappings =
  let check_mapping = function
    | UnmatchedAdded me ->
      ["Unable to match added symbol " ^ (Xflang.path_to_string me)]
    | UnmatchedType (me0, me1) ->
      ["Unable to automatically support type change between:\n  " ^ 
          (Xflang.path_to_string me0) ^ "\nand\n  " ^ (Xflang.path_to_string me1)]
    | UnmatchedError ((me0, me1), error) -> [error]
    | MatchAuto (_, _, _, MatchStruct (_, smappings))
    | MatchAuto (_, _, _, MatchUnion smappings) ->
      check_trans smappings
    | MatchInit _ | MatchAuto _ | MatchUserCode _ | UnmatchedDeleted _ -> []     
  in
  L.flatten (L.map check_mapping mappings)

(*
  Code to parse the command-line options and initiate the comparison.
*)
let parseArgs () =
  match Array.to_list Sys.argv with
    | [_; out_file; v0_file; v1_file; xf_file] ->
      (out_file, v0_file, v1_file, Some xf_file)
    | [_; out_file; v0_file; v1_file] ->
      (out_file, v0_file, v1_file, None)
    | _ ->
      exitError "Please provide ktt file for each version (v0 and v1)."

let xfgen_main () =
  let (out_file, v0_file, v1_file, xf_file) = parseArgs () in
  let (preamble, rules) = Xflang.parseXFRules xf_file in
  let rename_fun = register_renames rules in
  let v0_elems, _ = loadKttData v0_file in
  let v1_elems, v1_xform_reqs = loadKttData v1_file in
  let compare_ctx = compcontext_create rules v0_elems v1_elems in
  let results = compareXformElems compare_ctx (L.map snd v1_xform_reqs) in
  match check_trans results with
    | [] -> 
      let out_chan = open_out out_file in
      generate_file out_chan compare_ctx preamble rename_fun v0_elems v1_elems results
    | errors ->
      prerr_endline ("ERROR: Could not generate transformation code:");
      L.iter prerr_endline errors;
      exit 1;;

xfgen_main ()
