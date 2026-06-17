;;; cboomer-mode.el --- major mode for cboomer config files

(defvar cboomer-keywords
  '(("min_scale" . font-lock-keyword-face)
    ("scroll_speed" . font-lock-keyword-face)
    ("drag_friction" . font-lock-keyword-face)
    ("scale_friction" . font-lock-keyword-face)
    ("ppm_save" . font-lock-keyword-face)
    ("ppm_save_path" . font-lock-keyword-face)
    ("default_shader" . font-lock-keyword-face)
    ("mirror" . font-lock-keyword-face)
    ("flashlight_radius" . font-lock-keyword-face)
    ("scroll_invert" . font-lock-keyword-face)))

(defvar cboomer-font-lock-keywords
  `(("#.*$" . font-lock-comment-face)
    ("\"[^\"]*\"" . font-lock-string-face)
    ("'[^']*'" . font-lock-string-face)
    ("\\<\\(true\\|false\\|yes\\|no\\|on\\|off\\)\\>" . font-lock-builtin-face)
    ("\\<[0-9]+\\(\\.[0-9]+\\)?\\>" . font-lock-constant-face)
    ,@(mapcar (lambda (kw)
                `(,(concat "\\<" (car kw) "\\>") . ,(cdr kw)))
              cboomer-keywords)))

(define-derived-mode cboomer-mode fundamental-mode "cboomer"
  "Major mode for editing cboomer config files."
  (setq font-lock-defaults '(cboomer-font-lock-keywords))
  (setq comment-start "#"))

;;;###autoload
(add-to-list 'auto-mode-alist '("/cboomer/config\\'" . cboomer-mode))

(provide 'cboomer-mode)
;;; cboomer-mode.el ends here
