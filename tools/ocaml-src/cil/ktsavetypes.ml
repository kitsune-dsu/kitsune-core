

open Cil
open Ktciltools
open Kttools

open Ciltoktt
open Pretty

module KtT_Tools = Kttypes.Tools

let getLocal (name:string) (f:fundec) =
  let matches = List.filter (fun v -> v.vname = name) (f.sformals @ f.slocals) in
  match matches with
    | [v] -> Some v
    | [] -> None
    | _ -> E.s (E.error "Error - getLocal did not expect to find multiple locals named: %s" name)

class detectXformVisitor noted_locals xform_vars = object
  inherit nopCilVisitor

  val mutable current_fundec_opt = None
    
  method vfunc (f:fundec) : fundec visitAction =
    current_fundec_opt <- Some f;
    DoChildren

  method vinst (i:instr) : instr list visitAction =
    match current_fundec_opt with
      | None -> E.s (E.error "Unexpected Error! Instruction encountered before reaching a function.")
      | Some f ->
          begin
            match i with
              | Call (_, Lval (Var v, NoOffset), arg_list, _) when v.vname = "kitsune_register_var" ->
                  begin
                    match arg_list with 
                      | [varname_arg; funcname_arg; filename_arg; ns_arg; _; _; Const (CInt64 (automigrate, _, _))] ->
                          let parse_string_arg = function
                            | Const (CStr arg_string) -> Some arg_string
                            | Const (CInt64 (i64, _, _)) when i64 = Int64.of_int 0 -> None
                          in
                          let varname = match parse_string_arg varname_arg with
                            | Some s -> s
                            | None -> E.s (E.error "Error - bad use of kitsune_register_var")
                          in
                          let name_key = Ktdecl.render_var_key (parse_string_arg ns_arg, parse_string_arg filename_arg, 
                                                                parse_string_arg funcname_arg, varname) in
                          if automigrate = Int64.of_int 1 then
                            xform_vars := name_key :: !xform_vars                              
                      | _ -> E.s (E.error "Error - bad use of kitsune_register_var")
                  end;
                  SkipChildren
                    
              | Call (_, Lval (Var v, NoOffset), arg_list, _) when v.vname = "stackvars_note_local" ->
                  begin
                    match arg_list with 
                      | [Const (CStr var_name); _; _] -> 
                          begin
                            match getLocal var_name f with
                              | None -> E.s (E.error "Attempted to note local \"%s\" that is not present in function \"%s\"" 
                                               var_name f.svar.vname)
                              | Some v -> noted_locals := handleVar v (Some f) None :: !noted_locals
                          end
                      | _ -> E.s (E.error "Error - bad use of stackvars_note_local")
                  end;
                  SkipChildren

              | Call (_, Lval (Var v, NoOffset), arg_list, _) when v.vname = "kitsune_get_xform" ->
                  begin
                    match arg_list with 
                      | [varname_arg; funcname_arg; filename_arg; namespace_arg] ->
                        let parse_arg = function
                          | Const (CStr name) -> Some name
                          | _ -> None
                        in
                        let namespace = parse_arg namespace_arg in
                        let filename = parse_arg filename_arg in
                        let funcname = parse_arg funcname_arg in
                        let var_name = match parse_arg varname_arg with
                          | Some n -> n
                          | None -> E.s (E.error "Error - bad use of kitsune_get_xform")
                        in
                        let name_key = Ktdecl.render_var_key (namespace, filename, funcname, var_name) in
                        xform_vars := name_key :: !xform_vars                                                   
                      | _ -> E.s (E.error "Error - bad use of kitsune_get_xform")
                  end;
                        SkipChildren
              | _ ->
                  SkipChildren
          end
end

let detectXformCalls (f:file) =
  let noted_locals = ref [] in
  let xform_vars = ref [] in
  visitCilFile (new detectXformVisitor noted_locals xform_vars) f;
  (!noted_locals, !xform_vars)
    
class removeAnnotationsVisitor = object
  inherit nopCilVisitor
  method vattr (a:attribute) : attribute list visitAction =
    match a with
      | Attr (aname, _) when List.mem aname xform_attrs ->
          ChangeTo []
      | _ ->
          SkipChildren
end

(* strip out any serialization annotations after the serializers have
   been generated to avoid compiler warnings *)
let removeSerializerAnnotations (f:file) : unit =
  visitCilFile (new removeAnnotationsVisitor) f

(* lookup the namespace from the list of globals and pass it to the global vars if necessary*)
let gatherTypes (f:file) : KtT.top_level_elem list =
  let ns_str = lookupNamespaceStr (lookupNamespace f.globals) in
  List.flatten (List.map (fun glob -> handleGlobal glob ns_str ) f.globals )


let genDefsOutputFile = ref None

let doSaveTypes (f:file) : unit =
  (* loop through globals *)
  let ktt_data = gatherTypes f in
  let noted_locals, xform_vars = detectXformCalls f in
  (match !genDefsOutputFile with
     | Some filename ->
       let out_data = (Some (f.fileName, ktt_data @ noted_locals , xform_vars), None) in
       KtT_Tools.saveKttData filename out_data           
     | None ->
         (* TODO: warning that the function information is not being extracted. *)
         ());  
  removeSerializerAnnotations f;
  ()

let run = ref false

let feature : featureDescr =
  {
    fd_name = "ktsavetypes";
    fd_enabled = ref true;
    fd_description = "Extract types and symbols from source file";
    fd_extraopt = [
      ("--typesfile-out", Arg.String (fun s -> genDefsOutputFile := Some s), "name of file to which generated code is written")
    ];
    fd_doit = (fun (f:file) -> if not !run then doSaveTypes f; run := true);
    fd_post_check = false;
  }


let feature_compat : featureDescr =
  {
    fd_name = "ktsavetypes";
    fd_enabled = ref false;
    fd_description = "Extract types and symbols from source file";
    fd_extraopt = [
      ("--typesfile-out", Arg.String (fun s -> genDefsOutputFile := Some s), "name of file to which generated code is written")
    ];
    fd_doit = (fun (f:file) -> if not !run then doSaveTypes f; run := true);
    fd_post_check = false;
  }
