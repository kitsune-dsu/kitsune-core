
open Kttools
module KtT = Kttypes.TransformTypes
module KtT_Tools = Kttypes.Tools

let parseArgs () =
  match Array.to_list Sys.argv with
    | [] | [_] -> 
      exitError "Please provide list of ktt files to be combined."
    | _ :: output_file :: input_files ->
      (output_file, input_files)
        
let handleFile (f:string) =
  let in_data = KtT_Tools.loadKttData f in
  match in_data with
    | (Some file_ktt_data, None) ->
        [file_ktt_data]
    | (None, Some ktt_list) ->
        ktt_list
    | _ -> 
        exitError "Invalid Ktt file";;
            
let main () =
  let output_file, input_files = parseArgs () in
  let comb_ktt_data = List.flatten (List.map handleFile input_files) in
  let (out_data:KtT_Tools.marshal_type) = (None, Some comb_ktt_data) in 
  KtT_Tools.saveKttData output_file out_data;;

main () 
