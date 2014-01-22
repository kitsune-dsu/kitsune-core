open Kttools

type xf_path_elem =
  | PStruct of string
  | PUnion of string
  | PTypedef of string
  | PEnum of string
  | PField of string
  | PVar of string

type xf_path = xf_path_elem list

type xf_rule = 
  | Init of (xf_path * string)
  | Xform of (xf_path * xf_path * string option)
