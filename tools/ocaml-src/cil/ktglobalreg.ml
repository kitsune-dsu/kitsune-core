open Cil
open Ktciltools
open Kttools
module E = Errormsg
module L = List

type migrate_policy =
  | AutoMigrate
  | NoMigrate

(* Command-line options *)
let defaultMigratePolicy = ref AutoMigrate

let get_hoist_info (var:varinfo) =
  List.fold_left
    (fun prev attr -> 
       match attr with
         | Attr ("hoisted_static", [AStr f; AStr v]) -> Some (f, v)
         | _ -> prev)
    None var.vattr

let is_writable (var:varinfo) =
  (not (hasAttribute "const" (typeAttrs var.vtype))) &&
    match var.vtype with
      | TFun _ -> false
      | TArray (t', _, _) -> 
        (not (hasAttribute "const" (typeAttrs t')))         
      | _ -> true

let get_migrate_policy (var:varinfo) =
  if not (is_writable var) then
    NoMigrate
  else
    let policy_opt = List.fold_left
      (fun prev attr -> 
        (* TODO: issue errors in the case of contradictory annotations *)
        match attr with
          | Attr ("e_auto_migrate", []) -> Some AutoMigrate
          | Attr ("e_manual_migrate", []) -> Some NoMigrate
          | _ -> prev)
      None
      var.vattr
    in
    option_get_safe policy_opt !defaultMigratePolicy

let do_register_globals (f:file) : unit =
  let process_global ((statics, nonstatics) as prev) g =
    (* Add if not extern or inline *)
    let add_conditionally v =
      if v.vinline then
        prev
      else
        match v.vstorage with
          | NoStorage when (not (L.mem v nonstatics)) -> (statics, v :: nonstatics)
          | Static when (not (L.mem v statics)) -> (v::statics, nonstatics)
          | Extern | _ -> prev
    in
    match g with
      | GVar (v, _, _) -> add_conditionally v
      | GFun (f, _) -> add_conditionally f.svar
      | _ -> prev
  in
  let namespace = lookupNamespace f.globals in
  let (statics, nonstatics) = foldGlobals f process_global ([], []) in
  let file_func_name = "__kitsune_register_symbols" in
  (* Format.print_string ("Generating static extraction function: " ^ file_func_name ^ "\n"); *)
  let extract_func = emptyFunction file_func_name in
  let lookup_maps = buildLookupMaps f in
  let addr_put = lookupFunction lookup_maps "kitsune_register_var" in
  let migrate_policy_to_carg = function NoMigrate -> zero | AutoMigrate -> one in    
  let register_static v =
    let migrate_arg = migrate_policy_to_carg (get_migrate_policy v) in
    (* Format.print_string (" - Static Identifier: " ^ v.vname ^ "\n"); *)
    let name_parts =
      match get_hoist_info v with
        | None -> [mkString v.vname; zero; mkString v.vdecl.file; namespace]
        | Some (func_name, var_name) -> [mkString var_name; mkString func_name; mkString v.vdecl.file; namespace]
    in
    mkStmtOneInstr (Call (None, Lval (var addr_put), name_parts @ [mkCast ((AddrOf (var v))) voidPtrType; SizeOfE (Lval (var v)); migrate_arg], locUnknown))
  in
  let register_nonstatic v =
    let migrate_arg = migrate_policy_to_carg (get_migrate_policy v) in
    (* Format.print_string (" - Nonstatic Identifier: " ^ v.vname ^ "\n"); *)
    let name_parts = [mkString v.vname; zero; zero; zero] in
    mkStmtOneInstr (Call (None, Lval (var addr_put), 
			  name_parts @ [mkCast ((AddrOf (var v))) voidPtrType;
					SizeOfE (Lval (var v)); migrate_arg], locUnknown))
  in
  extract_func.sbody.bstmts <- 
    (L.map register_static statics) @ 
    (L.map register_nonstatic nonstatics) @ 
    extract_func.sbody.bstmts;
  extract_func.svar.vstorage <- Static;
  extract_func.svar.vattr <- addAttribute (Attr ("constructor", [])) extract_func.svar.vattr;
  f.globals <- f.globals @ [GFun (extract_func, locUnknown)];;

let run = ref false

let feature : featureDescr = {
  fd_name = "globalreg";
  fd_enabled = ref true;
  fd_description = "instruments program to register the addresses of globals with the kitsune runtime";
  fd_extraopt = ["--noautomigrate", 
		 Arg.Unit (fun () -> defaultMigratePolicy := NoMigrate), 
		 "Disable auto-migrating global and local-static variables";
		 "--automigrate", 
		 Arg.Unit (fun () -> defaultMigratePolicy := AutoMigrate), 
		 "Enable auto-migrating global and local-static variables";
		];
  fd_doit = (fun (f:file) -> if not !run then do_register_globals f; run := true);
  fd_post_check = false;
}

let feature_compat : featureDescr = {
  fd_name = "globalreg";
  fd_enabled = ref false;
  fd_description = "instruments program to register the addresses of globals with the kitsune runtime";
  fd_extraopt = ["--automigrate", 
		 Arg.Unit (fun () -> defaultMigratePolicy := AutoMigrate), 
		 "Enable auto-migrating global and local-static variables";
		 "--noautomigrate", 
		 Arg.Unit (fun () -> defaultMigratePolicy := NoMigrate), 
		 "Disable auto-migrating global and local-static variables"
		];
  fd_doit = (fun (f:file) -> if not !run then do_register_globals f; run := true);
  fd_post_check = false;
}

