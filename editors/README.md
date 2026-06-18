# Editor support for cboomer config files

Syntax highlighting for `$HOME/.config/cboomer/config`.

## Vim / Neovim

Both editors use the same syntax file. Neovim reads Vimscript natively.

Files:

`vim/cboomer.vim` -- Syntax highlighting (keys, strings, numbers, booleans, comments)
`vim/ftdetect.vim` -- Auto-detect `*/cboomer/config` as the `cboomer` filetype

### Install (Vim)

```console
cp vim/cboomer.vim ~/.vim/syntax/
mkdir -p ~/.vim/ftdetect
cp vim/ftdetect.vim ~/.vim/ftdetect/
```

Or add to `~/.vimrc`:

```vim
au BufRead,BufNewFile */cboomer/config setfiletype cboomer
```

### Install (Neovim)

```console
cp vim/cboomer.vim ~/.config/nvim/syntax/
mkdir -p ~/.config/nvim/ftdetect
cp vim/ftdetect.vim ~/.config/nvim/ftdetect/
```

Or add to `~/.config/nvim/init.lua`:

```lua
vim.api.nvim_create_autocmd({"BufRead", "BufNewFile"}, {
  pattern = "*/cboomer/config",
  command = "setfiletype cboomer",
})
```

## Emacs

One file:

`emacs/cboomer-mode.el` - Major mode with font-lock highlighting for keys, strings, numbers, booleans, comments

### Install

Add to your init file (`~/.emacs` or `~/.emacs.d/init.el`):

```elisp
(load "path/to/cboomer-mode.el")
```

Or place `cboomer-mode.el` in `~/.emacs.d/` and add:

```elisp
(load "~/.emacs.d/cboomer-mode.el")
```

The mode auto-activates for any file named `cboomer/config`.
