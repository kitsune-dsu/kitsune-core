
open Cil
open Ktciltools
open Kttools

module KtT = Kttypes.TransformTypes
module KtT_Tools = Kttypes.Tools
module E = Errormsg

module S = String
module L = List

let trans_ikind = function 
  | IChar -> KtT.IChar
  | ISChar -> KtT.ISChar
  | IUChar -> KtT.IUChar
  | IBool -> KtT.IBool
  | IInt -> KtT.IInt
  | IUInt -> KtT.IUInt
  | IShort -> KtT.IShort
  | IUShort -> KtT.IUShort
  | ILong -> KtT.ILong
  | IULong -> KtT.IULong
  | ILongLong -> KtT.ILongLong
  | IULongLong -> KtT.IULongLong
    
let trans_fkind = function
  | FFloat -> KtT.FFloat
  | FDouble -> KtT.FDouble
  | FLongDouble -> KtT.FLongDouble
    
let parseArrayLength (len_attr:attrparam) : KtT.array_len = 
  match len_attr with 
    | AInt i -> KtT.Len_Int (Int64.of_int i)
    | ADot(ACons ("self", []), field_name) -> KtT.Len_Field field_name
    | _ -> E.s (E.error "unsupported array size specifier")
    

let is_c_identifier (s:string) = (* TODO: implement me *) true

(* TODO: this parsing is somewhat ad-hoc and is probably pretty
   finicky - should find a way to avoid it or clean it up *)
let rec parseMappings (s:string) : KtT.gen_in =
  let commaSplit s = 
    let s_length = S.length s in
    let rec split_at_commas start_idx current_idx brace_depth =
      if current_idx = s_length then
        [S.sub s start_idx (current_idx - start_idx)]
      else
        match s.[current_idx] with
          | '<' -> split_at_commas start_idx (current_idx+1) (brace_depth+1)
          | '>' when brace_depth=0 ->
            E.s (E.error "unable to parse generic type annotation... unexpected right angle brace (>)")
          | '>' -> split_at_commas start_idx (current_idx+1) (brace_depth-1)
          | ',' when brace_depth=0 ->
            (S.sub s start_idx (current_idx - start_idx)) :: split_at_commas (current_idx+1) (current_idx+1) 0
          | _ -> split_at_commas start_idx (current_idx+1) brace_depth                          
    in
    split_at_commas 0 0 0
  in
  let rec parse_type_desc (desc:string) =
    let desc = trim desc in
    let dlen = S.length desc in
    match desc.[dlen-1] with 
      | '*' -> 
        KtT.PtrTo (parse_type_desc (Str.first_chars desc (dlen-1)))
      | '>' ->
        let langle_index = 
          try 
            S.index desc '<'          
          with
            | Not_found ->
              E.s (E.error "unable to parse generic type annotation... missing left angle brace (<)")
        in
        let main_t = trim (Str.first_chars desc langle_index) in
        if not (is_c_identifier main_t) then
          E.s (E.error "unable to parse generic type annotation... expected type name (%s)" main_t)
        else
          let gen_args_str = S.sub desc (langle_index+1) (dlen-langle_index-2) in
          KtT.ProgramType (Kttypes.Tools.parseGenericBindingType main_t 
                             (parseMappings gen_args_str) (fun s -> E.s (E.error "Error: %s" s)))
      | ']' ->
        if desc.[0] = '[' then
          begin
            let annot_str = String.sub desc 1 (dlen - 2) in
            if annot_str = "opaque" then
              KtT.ProgramType (KtT.TPtrOpaque (KtT.TVoid None))
            else
              E.s (E.error "unrecognized annotation: %s" annot_str)
          end
        else
          E.s (E.error "unable to parse generic type mapping... missing left angle brace ([)")
            
      | _ ->
        if desc.[0] = '@' then
          KtT.OtherGenericBinding (KtT.GenName (S.sub desc 1 (dlen-1)))
        else
          KtT.ProgramType (Kttypes.Tools.parseGenericBindingType desc [] (fun s -> E.s (E.error "Error: %s" s)))
  in
  List.map parse_type_desc (commaSplit s)

let ptr_attr_str = "e_ptr"
let ptr_opaque_attr_str = "e_ptropaque"
let ptrarray_attr_str = "e_ptrarray"
let array_attr_str = "e_array"
let custom_malloc_attr_str = "e_custom_malloc"
let str_zero_attr_str = "e_sz"
let generic_attr_str = "e_generic"
let genuse_attr_str = "e_genuse"
let type_attr_str = "e_type"

let xform_attrs = [ ptr_attr_str; ptrarray_attr_str; array_attr_str; custom_malloc_attr_str;
                    str_zero_attr_str; generic_attr_str; type_attr_str; genuse_attr_str ]

let specialTypeAttrs = function
  | TNamed (ti, a) ->
      addAttributes  (filterAttributes generic_attr_str (shallowTypeAttrs ti.ttype)) (dropAttribute generic_attr_str a)
  | TComp (comp, a) ->
      addAttributes (filterAttributes generic_attr_str comp.cattr) (dropAttribute generic_attr_str a)
  | TEnum (enum, a) ->
      (* don't support enums, so this wasn't given much thought *)
      addAttributes (filterAttributes generic_attr_str enum.eattr) (dropAttribute generic_attr_str a)
  | other ->
      shallowTypeAttrs other
        
type type_annot_t =
  | NoAnnot
  | AnnotPtr 
  | AnnotPtrOpaque
  | AnnotPtrArray of KtT.array_len
  | AnnotArray of KtT.array_len
  | AnnotGen of KtT.generic_name

let parseTypeAttr attrs inner_gen_opt : (type_annot_t * KtT.gen_out * KtT.gen_in * KtT.mem_mgmt) =
  let out_gens = ref None in
  let in_gens = ref None in
  let annot = ref None in
  let mem_mgmt = ref None in

  let inner_genout = ref inner_gen_opt in
  let rec parseTypeAttr' (outer:bool) (attr:string * attrparam list) : unit =
    match attr with
      | (aname, aparams) when aname = ptr_attr_str -> keep_one annot AnnotPtr

      | (aname, aparams) when aname = ptr_opaque_attr_str -> keep_one annot AnnotPtrOpaque 

      | (aname, aparams) when aname = ptrarray_attr_str ->
          begin
            match aparams with
              | [] -> E.s (E.error "%s expects an array length argument" ptrarray_attr_str)
              | [length] -> keep_one annot (AnnotPtrArray (parseArrayLength length))
              | _ -> E.s (E.error "unexpected arguments to %s" ptrarray_attr_str)
          end

      | (aname, aparams) when aname = array_attr_str ->
          begin
            match aparams with
              | [] -> E.s (E.error "%s expects an array length argument" array_attr_str)
              | [length] -> keep_one annot (AnnotArray (parseArrayLength length))
              | _ -> E.s (E.error "unexpected arguments to %s" array_attr_str)
          end

      | (aname, aparams) when aname = custom_malloc_attr_str ->
          begin
            match aparams with
              | [] | [_] -> E.s (E.error "%s expects serializer and deserializer function names as arguments" custom_malloc_attr_str)
              | [AStr malloc_name; AStr free_name] -> keep_one mem_mgmt (KtT.CustomMalloc (malloc_name, free_name))
              | _ -> E.s (E.error "unexpected arguments to %s" custom_malloc_attr_str)
          end

      | (aname, []) when aname = str_zero_attr_str -> keep_one annot (AnnotPtrArray KtT.Len_Nullterm)

      | (aname, [AStr genstr]) when aname = generic_attr_str ->
          (*print_endline ("E_GENERIC(" ^ genstr ^ ")");*)
          let make_gen_name (s:string) = 
            let s = trim s in
            if not (s.[0] = '@') then
              E.s (E.error "Expected generic variable to begin with @, but found: %s" s)
            else
              KtT.GenName (S.sub s 1 (S.length s - 1)) 
          in
          let (typelist : KtT.gen_out) = List.map make_gen_name (Str.split (Str.regexp ",") genstr) in
          (match !inner_genout with
            | Some gen_out when gen_out = typelist -> inner_genout := None
            | _ -> keep_one out_gens typelist)

      | (aname, [AStr typestr]) when aname = type_attr_str ->
        (*print_endline ("E_T(" ^ typestr ^ ")");*)
        if not (typestr.[0] = '@') then
          E.s (E.error "Expected generic variable to begin with @, but found: %s" typestr)
        else          
          keep_one annot (AnnotGen (KtT.GenName (S.sub typestr 1 (S.length typestr - 1))))
          
      | (aname, [AStr typestr]) when aname = genuse_attr_str ->
        (*print_endline ("E_G(" ^ typestr ^ ")");*)
        let mappings = parseMappings typestr in
        keep_one in_gens mappings
        
      | (aname, _) when aname = str_zero_attr_str ->
          E.s (E.error "%s annotation does not take arguments" aname)

      | (_aname, _) when outer ->
          (* Only report an 'unexpected' error for annotations inside
             other annotations.  Unknown top-level annotations are not
             erroneous since they may be targetted to other tools. *)
          ()

      | (aname, _) -> E.s (E.error "unexpected annotation: %s" aname)
  in
  let parseAttr (attr:attribute) =
    let Attr (aname, aparams) = attr in parseTypeAttr' true (aname, aparams)
  in
  List.iter parseAttr attrs;
  match !annot with
    | None -> (NoAnnot, option_get_safe !out_gens [], option_get_safe !in_gens [], option_get_safe !mem_mgmt KtT.DefaultMalloc)
    | Some a -> (a, option_get_safe !out_gens [], option_get_safe !in_gens [], option_get_safe !mem_mgmt KtT.DefaultMalloc);;

let rec cilToKitsuneType (t:typ) : KtT.typ =
  let annot, out_gens, in_gens, mem_mgmt = parseTypeAttr (specialTypeAttrs t) None in
  match t, annot with

    | TVoid _, AnnotGen g  ->
      KtT.TVoid (Some g)
    | TPtr (t', _), AnnotGen g ->
      KtT.TPtr (cilToKitsuneType t', Some g, mem_mgmt)

    | TVoid _, _ -> KtT.TVoid None

    | TInt (ik, _), _ -> KtT.TInt (trans_ikind ik)
    | TFloat (fk, _), _ -> KtT.TFloat (trans_fkind fk)

    | TPtr (t', _), AnnotPtrArray len ->
      KtT.TPtrArray (cilToKitsuneType t', len, mem_mgmt)

    | TPtr (t', _), AnnotPtrOpaque ->
      KtT.TPtrOpaque (cilToKitsuneType t')

    | TPtr (t', _), AnnotPtr
    | TPtr (t', _), NoAnnot ->
      KtT.TPtr (cilToKitsuneType t', None, mem_mgmt)

    | TArray (t', Some (Const (CInt64 (size, _, _))), _), AnnotArray _
    | TArray (t', Some (Const (CInt64 (size, _, _))), _), NoAnnot ->
        (* TODO - assert that code and annotation match! *)
        KtT.TArray (cilToKitsuneType t', KtT.Len_Int size)
    | TArray (t', None, _), AnnotArray len ->
        KtT.TArray (cilToKitsuneType t', len)
    | TArray (t', None, _), NoAnnot ->
        KtT.TArray (cilToKitsuneType t', KtT.Len_Unknown)
    | TArray (t', len, _), NoAnnot ->
        KtT.TArray (cilToKitsuneType t', (KtT.Len_Int (Int64.of_int (lenOfArray len))))

    | TFun (return_t, x, is_varargs, _), _ ->
      let trans_arg (arg_name, arg_typ, arg_attrs) =
              (* TODO: what to do with attributes on the argument types? (i.e., arg_attrs *)
        (arg_name, cilToKitsuneType arg_typ) 
      in
      (match x with Some args -> 
        KtT.TFun (cilToKitsuneType return_t, List.map trans_arg args, is_varargs)
	| None -> 
	  KtT.TFun (cilToKitsuneType return_t, [], is_varargs))
    | TFun (return_t, None, is_varargs, _), _ ->             
        E.s (E.error "UNEXPECTED ERROR! Arguments types for function pointer types should be known")
        
    | TNamed (tinfo, tattr), _ ->
        KtT.TNamed (tinfo.tname, in_gens)

    | TComp (cinfo, _), _ when cinfo.cstruct ->
        KtT.TStruct (cinfo.cname, in_gens)
    | TComp (cinfo, _), _ when not cinfo.cstruct ->
        KtT.TUnion (cinfo.cname, in_gens)

    | TEnum (einfo, _), _ ->
        KtT.TEnum einfo.ename

    | TBuiltin_va_list _, _ ->
        KtT.TBuiltin_va_list

    | _ ->
      E.s (E.error "UNEXPECTED ERROR! Bad type / annotation combination.")
;;

let handleVar (vinfo:varinfo) func_opt (ns_opt:string option) = 
  let (rep, out_gens, in_gens, mem_mgmt) = parseTypeAttr vinfo.vattr None in
  let ktt_type = cilToKitsuneType vinfo.vtype in
  assert (out_gens == []);
  let filename_opt = if vinfo.vstorage = Static then Some vinfo.vdecl.file else None in
  let funcname_opt, var_name = 
    match filterAttributes "hoisted_static" vinfo.vattr with
      | [Attr (_, [AStr f; AStr v])] -> (Some f, v)
      | [] -> 
        begin
          match func_opt with 
            | None -> 
              (None, vinfo.vname)
            | Some fdec -> 
              (Some fdec.svar.vname, vinfo.vname)
        end
      | _ -> E.s (E.error "Unexpected: multiple hoisted_static attributes!")    
  in
  let key_name = Ktdecl.render_var_key (ns_opt, filename_opt, funcname_opt, var_name) in  
  if mem_mgmt <> KtT.DefaultMalloc then
    (match ktt_type with
      | KtT.TPtr _ | KtT.TPtrArray _ -> ()
      | _ -> E.s (E.error "Custom allocators can only be provided for pointer-types!"));
  KtT.Var {
    KtT.vname = key_name;
    KtT.vtype = ktt_type;
    KtT.vgen_in = in_gens;
    KtT.vmem_mgmt = mem_mgmt;
  };;


let prev_globals = Hashtbl.create 37

let handleGlobal (g:Cil.global) (ns_opt:string option) : KtT.top_level_elem list =
  match g with
    | GType (tinfo, _) ->
      let type_attrs =
        match tinfo.ttype with
          | TNamed (tinfo, _) ->
            hashtbl_find_safe prev_globals ("typedef " ^ tinfo.tname)
          | TComp (cinfo, _) ->
            hashtbl_find_safe prev_globals ("struct " ^ cinfo.cname)
          | _ -> None
      in
      let (rep, out_gens, in_gens, mem_mgmt) = parseTypeAttr (typeAttrs tinfo.ttype) type_attrs in
      Hashtbl.add prev_globals ("typedef " ^ tinfo.tname) out_gens;
      [ KtT.Typedef {
        KtT.tname = tinfo.tname;
        KtT.ttype = cilToKitsuneType tinfo.ttype;
        KtT.tgen_out = out_gens;
      }]

    | GCompTag (cinfo, _) ->
      let (rep, out_gens, in_gens, mem_mgmt) = parseTypeAttr cinfo.cattr None in
      Hashtbl.add prev_globals ("struct " ^ cinfo.cname) out_gens;
      assert (rep == NoAnnot);
      assert (in_gens == []);
      assert (mem_mgmt == KtT.DefaultMalloc);
      let make_field finfo : KtT.field = 
        let (frep, fout_gens, fin_gens, fmem_mgmt) = parseTypeAttr cinfo.cattr None in
        { 
          KtT.fname = finfo.fname;
          KtT.ftype = cilToKitsuneType finfo.ftype;
          KtT.fbitfield = finfo.fbitfield;
          KtT.fgen_out = fout_gens;
          KtT.fgen_in = fin_gens;
        }
      in
      let sinfo = {
        KtT.sname = cinfo.cname;        
        KtT.sfields = List.map make_field cinfo.cfields;
        KtT.sgen_out = out_gens;
      } in
      if cinfo.cstruct then
          [KtT.Struct sinfo]
        else
          [KtT.Union sinfo]

    | GEnumTag (einfo, _) ->
      let make_eitem (n, v, _) =
        match v with 
          | Const (CInt64 (int_val, _, _)) -> (n, int_val)
          | _ -> E.s (E.error "Failed to interpret enum (%s) value (%s)." einfo.ename n)
      in
      [ KtT.Enumeration {
        KtT.ename = einfo.ename;
        KtT.eitems = List.map make_eitem einfo.eitems;
        KtT.eintkind = trans_ikind einfo.ekind;
      }]

    | GVar (vinfo, _, _) -> [handleVar vinfo None ns_opt]
    | _ -> []
