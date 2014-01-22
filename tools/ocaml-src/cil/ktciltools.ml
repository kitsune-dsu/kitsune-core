
open Cil
module E = Errormsg

(*** maps of functions, typedefs, and structs ***)

type lookup_maps_t = {
  funs : (string, varinfo) Hashtbl.t;
  typedefs : (string, typ) Hashtbl.t;
  comps : (string, compinfo) Hashtbl.t
}

(* This string is the token used to search in the code for the namespace identifier. 
   This could be moved to a different place or implemented in a different way. *)
let namespace_id = "KT_NAMESPACE"

class globalLookupVisitor (lookup_maps:lookup_maps_t) = object
  inherit nopCilVisitor
  method vglob (g:global) =
    match g with
      | GType (tinfo, _) ->
          Hashtbl.replace lookup_maps.typedefs tinfo.tname (TNamed (tinfo, []));
          SkipChildren
      | GCompTag (cinfo, _) ->
          Hashtbl.replace lookup_maps.comps cinfo.cname cinfo;
          SkipChildren
      | GFun (fdec, _) ->
          Hashtbl.replace lookup_maps.funs fdec.svar.vname fdec.svar;
          SkipChildren
      | GVarDecl (vinfo, _) ->
          (match vinfo.vtype with
             | TFun _ -> Hashtbl.replace lookup_maps.funs vinfo.vname vinfo
             | _ -> ());
          SkipChildren
      | _ -> SkipChildren
end

let buildLookupMaps (f:Cil.file) : lookup_maps_t =
  let lookup_maps = { funs = Hashtbl.create 37; typedefs = Hashtbl.create 37; comps = Hashtbl.create 37 } in
  let visitor = new globalLookupVisitor lookup_maps in
  visitCilFile visitor f;
  lookup_maps

let lookupFunction (lookup_maps:lookup_maps_t) (name:string) : varinfo =
  try
    Hashtbl.find lookup_maps.funs name
  with Not_found ->
    failwith ("Couldn't find function "^name)

let lookupTypedef (lookup_maps:lookup_maps_t) (name:string) : typ =
  Hashtbl.find lookup_maps.typedefs name

let lookupComp (lookup_maps:lookup_maps_t) (name:string) : compinfo =
  Hashtbl.find lookup_maps.comps name

(* This looks up the namespace string (if present) in the list of globals*)
let lookupNamespace (globals:global list) : Cil.exp = 
  let ns = try List.find (fun x ->
    match x with 
    | GVar (v,i,l) -> (if(v.vname = namespace_id) then true else false)
    | _ -> false) 
    globals 
    with
    | Not_found -> GText("Not_found")
    in match ns with
    | GVar (v,i,l) -> (match i.init with
      | Some e -> (match e with
        | SingleInit e -> stripCasts e (*if no stripCasts, type will be CastE not Const, 
                                        problem in matching in ktsavetypes.ml*)
        | _ -> zero)
      | _ -> zero)
    | _ -> zero

(* This turns a namespace expression into a string that can be used to insert into the code.
   Call this after using lookupNamespace if the string form is needed instead of an expression.*)
let lookupNamespaceStr (ns:Cil.exp): string option = 
	match ns with
	| Const c -> (match c with
       | CStr cs -> Some cs
       | _ ->  None )
    | _ ->  None 


let parseNumericType (s:string) =
  match s with
    | "char" -> TInt (IChar, [])
    | "signed char" -> TInt (ISChar, [])
    | "unsigned char" -> TInt (IUChar, [])
    | "_Bool" -> TInt (IBool, [])
    | "int" -> TInt (IInt, [])
    | "unsigned int" -> TInt (IUInt, [])
    | "short" -> TInt (IShort, [])
    | "unsigned short" -> TInt (IUShort, [])
    | "long" -> TInt (ILong, [])
    | "unsigned long" -> TInt (IULong, [])
    | "long long" -> TInt (ILongLong, [])
    | "unsigned long long" -> TInt (IULongLong, [])
    | "float" -> TFloat (FFloat, [])
    | "double" -> TFloat (FDouble, [])
    | "long double" -> TFloat (FLongDouble, [])
    | _ -> failwith "not a recognized numeric type"

let ptrTo (t:typ) = TPtr (t, [])

let shallowTypeAttrs = function
  | TVoid a -> a
  | TInt (_, a) -> a
  | TFloat (_, a) -> a
  | TNamed (t, a) -> a
  | TPtr (_, a) -> a
  | TArray (_, _, a) -> a
  | TComp (comp, a) -> a
  | TEnum (enum, a) -> a
  | TFun (_, _, _, a) -> a
  | TBuiltin_va_list a -> a
