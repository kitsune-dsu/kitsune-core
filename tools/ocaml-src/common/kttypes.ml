open Kttools

module S = String
module L = List

module TransformTypes =
struct 
  type ikind =
    | IChar
    | ISChar
    | IUChar
    | IBool
    | IInt
    | IUInt
    | IShort
    | IUShort
    | ILong
    | IULong
    | ILongLong
    | IULongLong

  type fkind = 
    | FFloat 
    | FDouble 
    | FLongDouble

  type mem_mgmt =
    | DefaultMalloc
    | CustomMalloc of string * string

  type array_len =
    | Len_Unknown
    | Len_Nullterm
    | Len_Int of int64
    | Len_Field of string

  type generic_name =
    | GenName of string

  and generic_type_desc = 
    | OtherGenericBinding of generic_name
    | ProgramType of typ
    | PtrTo of generic_type_desc

  and gen_out = generic_name list
  and gen_in = generic_type_desc list

  and typ = 
    | TVoid of generic_name option
    | TInt of ikind
    | TFloat of fkind
    | TPtr of typ * generic_name option * mem_mgmt
    | TPtrOpaque of typ
    | TPtrArray of typ * array_len * mem_mgmt
    | TArray of typ * array_len
    | TFun of typ * (string * typ) list * bool (* bool is varargs *)
    | TNamed of string * gen_in
    | TStruct of string * gen_in
    | TUnion of string * gen_in
    | TEnum of string
    | TBuiltin_va_list
        
  type field = {
    fname : string;
    ftype : typ;
    fbitfield : int option;
    fgen_out : gen_out;
    fgen_in : gen_in;
  }

  type struct_info = {
    sname : string;
    sfields : field list;
    sgen_out : gen_out;
  }

  type typedef_info = {
    tname : string;
    ttype : typ;
    tgen_out : gen_out;
  }

  type enumeration_info = {
    ename : string;
    eitems : (string * int64) list;
    eintkind : ikind;
  }

  type var_info = {
    vname : string;
    vtype : typ;
    vgen_in : gen_in;
    vmem_mgmt : mem_mgmt;
  }

  type top_level_elem =
    | Struct of struct_info
    | Union of struct_info
    | Typedef of typedef_info
    | Enumeration of enumeration_info
    | Var of var_info
end;;

module KtT = TransformTypes

module Tools =
struct 
  open KtT
  type marshal_type = 
      ((string * top_level_elem list * string list) option *          
         ((string * top_level_elem list * string list) list) option)
      
  let saveKttData (filename:string) (data:marshal_type) : unit =
    let out_channel = open_out filename in
    let () = Marshal.to_channel out_channel data [] in
    let () = close_out out_channel in
    ()
      
  let loadKttData (filename:string) : marshal_type =
    let in_chan = open_in filename in
    let result = Marshal.from_channel in_chan in
    let () = close_in in_chan in
    result

  let elemToUniqueString = function
    | Struct sinfo -> "struct " ^ sinfo.sname
    | Union sinfo -> "union " ^ sinfo.sname
    | Typedef tinfo -> "typedef " ^ tinfo.tname
    | Enumeration einfo -> "enum " ^ einfo.ename
    | Var vinfo -> "var " ^ vinfo.vname

  let elemToType = function
    | Struct sinfo -> TStruct (sinfo.sname, [])
    | Union sinfo -> TUnion (sinfo.sname, [])
    | Typedef tinfo -> TNamed (tinfo.tname, [])
    | Enumeration einfo -> TEnum (einfo.ename)
    | Var vinfo -> vinfo.vtype

  let ikindToString = function
    | IChar -> "char"
    | ISChar -> "signed char"
    | IUChar -> "unsigned char"
    | IBool -> "_Bool"
    | IInt -> "int"
    | IUInt -> "unsigned int"
    | IShort -> "short"
    | IUShort -> "unsigned short"
    | ILong -> "long"
    | IULong -> "unsigned long"
    | ILongLong -> "long long"
    | IULongLong -> "unsigned long long"

  let fkindToString = function
    | FFloat -> "float"
    | FDouble -> "double"
    | FLongDouble -> "long double"

  let mallocToString = function
    | DefaultMalloc -> ""
    | CustomMalloc (alloc, free) -> "[" ^ alloc ^ ", " ^ free ^ "]"

  let lenToString = function
    | Len_Nullterm -> "<NT>"
    | Len_Int i -> "<" ^ (Int64.to_string i) ^ ">"
    | Len_Field s -> "<" ^ s ^ ">"

  let rec descToString = function
    | OtherGenericBinding (GenName bname) -> bname
    | ProgramType t' -> typeToString t'
    | PtrTo desc -> descToString desc ^ "*"

  and genInToString (gi:gen_in) =
    match List.map descToString gi with
      | [] -> ""
      | _ as mappings -> "<" ^ (String.concat "," mappings) ^ ">"

  and typeToString = function
    | TVoid (Some (GenName name)) -> "void<" ^ name ^ ">"
    | TVoid None -> "void"
    | TInt ik -> ikindToString ik
    | TFloat fk -> fkindToString fk
    | TPtr (t', None, mem) -> "ptr_single" ^ (mallocToString mem)  ^ "(" ^ (typeToString t') ^ ")"
    | TPtr (t', Some (GenName name), mem) -> typeToString t' ^ "<@" ^ name ^ ">" ^ (mallocToString mem)
    | TPtrOpaque t' -> "ptr_opaque" ^ "(" ^ (typeToString t') ^ ")"
    | TPtrArray (t', len, mem) -> "ptr_array" ^ (lenToString len) ^ (mallocToString mem) ^ "(" ^ (typeToString t') ^ ")"
    | TArray (t', len) -> "array" ^ (lenToString len) ^ "(" ^ (typeToString t') ^ ")"
    | TFun (rt, args, varargs) -> "funptr"
    | TNamed (tdname, gen_in) -> tdname ^ (genInToString gen_in)
    | TStruct (sname, gen_in) -> "struct " ^ sname ^ (genInToString gen_in)
    | TUnion (uname, gen_in) -> "union " ^ uname ^ (genInToString gen_in)
    | TEnum ename -> "enum " ^ ename
    | TBuiltin_va_list -> "var_args"

  let getTypeGenIns = function
    | TVoid _ | TInt _ | TFloat _ | TPtr _ | TPtrOpaque _ | TPtrArray _ | TArray _ | TFun _ | TEnum _ | TBuiltin_va_list -> 
      []
    | TNamed (_, gen_in)
    | TStruct (_, gen_in)
    | TUnion (_, gen_in) ->
      gen_in

  let listifyKttElems (data:marshal_type) : (string * top_level_elem) list =
    let map_fun (src_file, elems, _) = List.map (fun e -> (src_file, e)) elems in
    match data with
      | (None, Some decls) -> List.flatten (List.map map_fun decls)
      | (Some decl, None) -> map_fun decl
      | _ -> Kttools.exitError "Invalid Ktt file"

  let listifyKttXformVars (data:marshal_type) : (string * string) list =
    let map_fun (src_file, _, vars) = List.map (fun g -> (src_file, g)) vars in
    match data with
      | (None, Some decls) -> List.flatten (List.map map_fun decls)
      | (Some decl, None) -> map_fun decl
      | _ -> Kttools.exitError "Invalid Ktt file"

  let ptrTo (t:typ) = TPtr (t, None, DefaultMalloc)

let parseGenericBindingType (s:string) (genargs:gen_in) (error:string->unit) =
  let s = trim s in
  let result =
    match s with
      | "char" -> KtT.TInt KtT.IChar
      | "signed char" -> KtT.TInt KtT.ISChar
      | "unsigned char" -> KtT.TInt KtT.IUChar
      | "_Bool" -> KtT.TInt KtT.IBool
      | "int" -> KtT.TInt KtT.IInt
      | "unsigned int" -> KtT.TInt KtT.IUInt
      | "short" -> KtT.TInt KtT.IShort
      | "unsigned short" -> KtT.TInt KtT.IUShort
      | "long" -> KtT.TInt KtT.ILong
      | "unsigned long" -> KtT.TInt KtT.IULong
      | "long long" -> KtT.TInt KtT.ILongLong
      | "unsigned long long" -> KtT.TInt KtT.IULongLong
      | "float" -> KtT.TFloat KtT.FFloat
      | "double" -> KtT.TFloat KtT.FDouble
      | "long double" -> KtT.TFloat KtT.FLongDouble
      | _ ->
        if string_starts_with s "struct " then
          KtT.TStruct (trim (string_remove_prefix s "struct "), genargs)
        else if string_starts_with s "union " then
          KtT.TUnion (trim (string_remove_prefix s "union "), genargs)
        else if string_starts_with s "enum " then
          KtT.TEnum (trim (string_remove_prefix s "enum "))
        else if string_starts_with s "typedef " then
          KtT.TNamed (trim (string_remove_prefix s "typedef "), genargs)
        else
          KtT.TNamed (s, genargs)
  in
  ignore (match result with
    | KtT.TFloat _ | KtT.TInt _ ->
      if genargs <> [] then error "Primitive types cannot accept generic arguments."
    | _ -> ());
  result

end
