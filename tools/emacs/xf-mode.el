;; (defvar xf-mode-hook nil)

;; (defvar xf-mode-map
;;   (let ((map (make-sparse-keymap)))
;;     map)
;;   "Keymap for Kitsune XFGen major mode")

(require 'cc-mode)
(add-to-list 'auto-mode-alist '("\\.xf\\'" . xf-mode))

(defvar xf-keywords
'("$in" "$out" "$xform" "XF_ASSIGN" "XF_FPTR" 
  "XF_PTR" "XF_ASSIGN" "XF_INVOKE"))

(defvar xf-events
  '("INIT")
  )

(defvar xf-font-lock-defaults
  `((
     ( ,(regexp-opt xf-keywords 'words) . font-lock-builtin-face)
     ( ,(regexp-opt xf-events 'words) . font-lock-function-name-face)
     )))


(define-derived-mode xf-mode c-mode "xfgen"
  "Major mode for editing Kitsune xfgen files"
  (c-initialize-cc-mode t)
  (c-init-language-vars c-mode)
  (c-common-init 'c-mode) ; Or perhaps (c-basic-common-init 'some-mode)
  (font-lock-add-keywords nil '("$in" "$out" "$xform" "XF_ASSIGN" "XF_FPTR" 
				"XF_PTR" "XF_ASSIGN" "XF_INVOKE"))
  
  )
(provide 'xf-mode)

