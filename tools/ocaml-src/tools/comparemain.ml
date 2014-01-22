
open Kttcompare
open Kttools

(*
  Code to parse the command-line options and initiate the comparison.
*)
let parseArgs () =
  match Array.to_list Sys.argv with
    | _ :: [v0_file; v1_file; xf_file] ->
        (v0_file, v1_file, Some xf_file)
    | _ :: [v0_file; v1_file] ->
        (v0_file, v1_file, None)
    | _ -> 
        exitError "Please provide ktt file for each version (v0 and v1)."

let xfcomp_main () =
  let (v0_file, v1_file, xf_file) = parseArgs () in
  let (preamble, rules) = Xflang.parseXFRules xf_file in
  let v0_elems, _ = loadKttData v0_file in
  let v1_elems, v1_xform_reqs = loadKttData v1_file in
  let ctx = compcontext_create rules v0_elems v1_elems in
  let results = compareXformElems ctx (List.map snd v1_xform_reqs) in
  print_results ctx results;;

xfcomp_main () 
