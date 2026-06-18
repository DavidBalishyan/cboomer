if exists("b:current_syntax")
    finish
endif

syntax match cboomerComment "#.*$" contains=@Spell

syntax keyword cboomerKey
    \ min_scale
    \ scroll_speed
    \ drag_friction
    \ scale_friction
    \ ppm_save
    \ ppm_save_path
    \ default_shader
    \ mirror
    \ flashlight_radius
    \ scroll_invert
    \ osd
    \ smooth_reset
    \ font
    \ screenshot_dir
    \ screenshot_format

syntax match cboomerOperator "="

syntax region cboomerString start='"' end='"'
syntax region cboomerChar start="'" end="'"

syntax match cboomerNumber "\<[0-9]\+\(\.[0-9]\+\)\?"

syntax keyword cboomerBool true false yes no on off

highlight default link cboomerComment Comment
highlight default link cboomerKey Identifier
highlight default link cboomerOperator Operator
highlight default link cboomerString String
highlight default link cboomerChar Character
highlight default link cboomerNumber Number
highlight default link cboomerBool Boolean

let b:current_syntax = "cboomer"

" vim: ts=8 sts=2 sw=2 et
