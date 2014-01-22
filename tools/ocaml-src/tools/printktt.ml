
module KtT = Kttypes.TransformTypes
module KtT_Tools = Kttypes.Tools

let rec typeToStr (t:KtT.typ) =
  match t with 
    | KtT.Void _ -> "void"
    | KtT.Int _ -> "integral"
    | KtT.Float _ -> "floatish"
    | KtT.Ptr (t', _) -> "ptr_to " ^ (typeToStr t')
    | KtT.PtrArray (t', _, _) -> "ptr_to_array_of " ^ (typeToStr t')
    | KtT.Array (t', _) -> "array_of " ^ (typeToStr t')
    | KtT.Fun _ -> "function pointer"
    | KtT.Named (tname, _) -> "typedef " ^ tname
    | KtT.Comp (cname, _) -> "struct " ^ cname
    | KtT.Enum (ename) -> "enum " ^ ename
    | KtT.Builtin_va_list -> "va_list" 

let printField (f:KtT.field) =
  print_endline (f.KtT.fname ^ ": " ^ (typeToStr f.KtT.ftype))

let printKtTElem (e:KtT.top_level_elem) =
  match e with
    | KtT.Struct sinfo -> 
        print_endline ("struct " ^ sinfo.KtT.sname);
        List.iter printField sinfo.KtT.sfields
    | KtT.Typedef tinfo -> 
        print_endline ("typedef " ^ tinfo.KtT.tname ^ " : " ^ (typeToStr tinfo.KtT.ttype))
    | KtT.Enumeration einfo -> 
        print_endline ("enum " ^ einfo.KtT.ename)
    | KtT.GlobalVar vinfo -> 
        print_endline ("gvar " ^ vinfo.KtT.vname ^ " : " ^ (typeToStr vinfo.KtT.vtype))

let exitError (s:string) = 
  prerr_endline s;
  exit 1  

let parseArgs () =
  match Array.to_list Sys.argv with
    | [] | [_] -> 
      exitError "Please provide list of ktt files to be combined."
    | _ :: output_file :: input_files ->
      (output_file, input_files)

let printFileData (filename, decls) = 
  print_endline filename;
  List.iter printKtTElem decls
        
let handleFile (f:string) =
  let in_data = KtT_Tools.loadKttData f in
  match in_data with
    | (Some file_ktt_data, None) ->
        printFileData file_ktt_data 
    | (None, Some ktt_list) ->
        List.iter printFileData ktt_list
    | _ -> 
        exitError "Invalid Ktt file"

let main () =
  let output_file, input_files = parseArgs () in
  List.iter handleFile input_files;;

main () 
