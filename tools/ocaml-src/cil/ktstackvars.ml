
open Cil
open Ktciltools
module E = Errormsg

type stackvars_library_defs_t =
    {
      note_entry : varinfo;
      note_exit : varinfo;
      note_local : varinfo;
      note_formal : varinfo;
    }

let getStackvarsDefs (f:file) : stackvars_library_defs_t =
  let lookup_maps = buildLookupMaps f in
  {
    note_entry = lookupFunction lookup_maps "stackvars_note_entry";
    note_exit = lookupFunction lookup_maps "stackvars_note_exit";
    note_local = lookupFunction lookup_maps "stackvars_note_local";
    note_formal = lookupFunction lookup_maps "stackvars_note_formal";
  }

(** Note: need to issue a warning if file contains a longjmp (or
    setjmp) since both could may interact poorly with our stack
    tracking code! *)

(** Note: need to ensure that we, at update time, replace var and function
    names with strdup'ed versions!  Or maybe not?  Will the stack from the
    old version persist beyond when we hit our update point? *)

let makeFunEntryStmt (defs:stackvars_library_defs_t) (f:fundec) : stmt =
  mkStmtOneInstr (Call (None, Lval (var defs.note_entry), [mkString f.svar.vname], locUnknown))

let makeFunExitStmt (defs:stackvars_library_defs_t) (f:fundec) : stmt =
  mkStmtOneInstr (Call (None, Lval (var defs.note_exit), [mkString f.svar.vname], locUnknown))

let makeLocalInitStmt (defs:stackvars_library_defs_t) (v:varinfo) : stmt =
  mkStmtOneInstr (Call (None, Lval (var defs.note_local),
                        [mkString v.vname; (AddrOf (var v)); (SizeOf v.vtype)], locUnknown))

let makeFormalInitStmt (defs:stackvars_library_defs_t) (v:varinfo) : stmt =
  mkStmtOneInstr (Call (None, Lval (var defs.note_formal),
                        [mkString v.vname; (AddrOf (var v)); (SizeOf v.vtype)], locUnknown))

let notelocals_attr_str = "e_notelocals"

let rec shouldNoteLocals (attrs:attribute list) : bool =
  match attrs with
    | Attr (name, _) :: _ when name = notelocals_attr_str -> true
    | [] -> false
    | _ :: rest -> shouldNoteLocals rest    

class globalLookupVisitor (defs:stackvars_library_defs_t) = object
  inherit nopCilVisitor
  val curr_func = ref None

  method vfunc (f:fundec) : fundec visitAction =
    if shouldNoteLocals f.svar.vattr then
      begin
        curr_func := Some f;
        let new_stmts =
          (makeFunEntryStmt defs f) ::
            ((List.map (makeFormalInitStmt defs) f.sformals) @
                (List.map (makeLocalInitStmt defs) f.slocals))
        in
        f.sbody.bstmts <- (new_stmts @ f.sbody.bstmts);
        DoChildren
      end
    else
      SkipChildren

  method vstmt (s:stmt) : stmt visitAction =
    match s.skind, !curr_func with
      | Return _, Some f ->
          let exit_stmt = makeFunExitStmt defs f in
          ChangeTo (mkStmt (Block (mkBlock [exit_stmt; s])))
      | _, None ->
          E.s (E.error "Unexpected stmt without outer function.");
      | _ ->
          DoChildren
end

let doStackVarHandler (f:file) : unit =
  let visitor = new globalLookupVisitor (getStackvarsDefs f) in
  visitCilFileSameGlobals visitor f

let run = ref false

let feature : featureDescr =
  {
    fd_name = "stackvars";
    fd_enabled = ref true;
    fd_description = "note stack variables for transfer";
    fd_extraopt = [];
    fd_doit = (fun (f:file) -> if not !run then doStackVarHandler f; run := true);
    fd_post_check = false;
  }

let feature_compat : featureDescr =
  {
    fd_name = "stackvars";
    fd_enabled = ref false;
    fd_description = "note stack variables for transfer";
    fd_extraopt = [];
    fd_doit = (fun (f:file) -> if not !run then doStackVarHandler f; run := true);
    fd_post_check = false;
  }
