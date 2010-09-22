
;; This file contains lisp definitions for klatexformula integration into (X)Emacs.
;; Typing Ctrl-C K will launch klatexformula with the current selected region.

;; INSTALLATION INSTRUCTIONS
;; Install this file:
;;  - either into your ~/.xemacs/ or to ~/.klfemacs.el or ~/emacs-definitions/klfemacs.el or whatever name you want
;;  - or into the global /usr/share/emacs/site-lisp or any other entry in your EMACSLOADPATH or load-path
;; Then include this file into your ~/.xemacs/custom.el or ~/.emacs:
;;  - either directly with absolute location:
;;      (load "~/.xemacs/klfemacs.el" t t)
;;  - or referencing it using load-path search:
;;      (load "klfemacs.el" t t)
;;
;; Alternatively, you may just copy and paste the code below directly into your ~/.xemacs/custom.el or ~/.emacs
;; or ~/.gnu-emacs-custom
;;


(defun klf-region-run ()
  "Run KLatexFormula"
  (interactive)
  (start-process "klatexformula" "*KLatexFormula*" "klatexformula" "--daemonize" "-I" "--latexinput" (buffer-substring (region-beginning) (region-end)))
)
(global-set-key [(control c) (k)] 'klf-region-run)
