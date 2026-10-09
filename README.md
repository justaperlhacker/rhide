# RHIDE — modern build (revival)

RHIDE is an old Turbo Vision IDE (by Robert Höhne) for DJGPP/Unix with an
integrated GDB debugger. This tree is a revival of RHIDE 1.5 that builds and
runs on a modern x86-64 Linux toolchain (tested with gcc 16.2.1 on CachyOS,
built as a native **X11** application).

See `REVIVAL-PLAN.md` for the current status, what was changed, and the
verification results. For a one-shot bootstrap of a fresh machine, use
`setup-new-host.sh.txt`.

## Source repositories

The revival spans four git repos. Clone them as siblings:

| repo | remote | branch | role |
|------|--------|--------|------|
| rhide | `justaperlhacker/rhide` | `master` | the IDE (this repo) |
| tvision | `justaperlhacker/tvision` | `modern-gcc` | Turbo Vision fork (gcc/glibc fixes) |
| gdb-5.3 | `justaperlhacker/gdb-5.3` | `modern-gcc` | embedded debugger engine fork |
| setedit-upstream | `set-soft/setedit` | `master` | SET's editor library (unmodified) |

```sh
mkdir -p ~/Projects && cd ~/Projects
git clone https://github.com/justaperlhacker/rhide.git
git clone https://github.com/justaperlhacker/tvision.git
git clone https://github.com/justaperlhacker/gdb-5.3.git
git clone https://github.com/set-soft/setedit.git setedit-upstream
git -C tvision  checkout modern-gcc
git -C gdb-5.3 checkout modern-gcc
```

The set-soft mirror is cloned as `setedit-upstream/` so it does not clash with
the revived `justaperlhacker/setedit` fork, which uses the `setedit/` name.

The build expects this exact sibling layout by default (each project also
searches `/usr/local/src` and `/usr/src`). Override with the variables below
if your paths differ.

## Dependencies

- `gcc`, `g++`, `make`, `perl`, `git`
- ncurses and gpm development files
- X11 development files, including Xmu (`libXmu`)

On Arch/CachyOS:

```sh
sudo pacman -S --needed base-devel ncurses gpm libx11 libxmu perl git
```

## Build

### 1. Turbo Vision (static)

```sh
cd ~/Projects/tvision/tvision
./configure --without-dynamic
make -j"$(nproc)"
ln -sf "$PWD/rhtv-config" ~/.local/bin/rhtv-config
```

### 2. SET's editor library

```sh
cd ~/Projects/setedit-upstream/setedit
./configure --libset --no-infview \
  --tv-include="$HOME/Projects/tvision/tvision/include" \
  --tv-lib="$HOME/Projects/tvision/tvision/makes"
make needed
make libset
```

### 3. RHIDE (this repo)

`./configure` probes the paths, so export them **before** running it:

```sh
cd ~/Projects/rhide
export TV_INC="$HOME/Projects/tvision/tvision/include"
export TVOBJ="$HOME/Projects/tvision/tvision/makes"
export SETSRC="$HOME/Projects/setedit-upstream/setedit"
export SETOBJ="$HOME/Projects/setedit-upstream/setedit/makes"
export GDB_SRC="$HOME/Projects/gdb-5.3"

./configure
make -j"$(nproc)"
make rhide
```

The gdb-5.3 subtree is built here (its configure/build are serialized, so this
step is the slow part). Outputs:

- `idegc.exe` — the real IDE binary
- `rhide` → `rhide.exe` → `idegc.exe` (symlinks)
- `rhgdb/rhgdb.exe` — standalone debugger
- `libgdb/libgdbrh.a` — the embedded GDB 5.3 engine

## Running

RHIDE links both a Turbo Vision **X11** driver and a terminal driver and picks
by priority (`X11` 100 > Linux console 90 > XTerm 60 > ncurses 10). X11 is
selected whenever `DISPLAY` connects:

```sh
# own X11 window
env DISPLAY=:0 ./rhide

# terminal driver instead
env -u DISPLAY ./rhide

# open / create a project
./rhide path/to/project.gpr
```

The X11 window uses a built-in bitmap font and sets `WM_CLASS = "tvapp",
"XTVApp"`. It has no `_NET_WM_NAME`, so find it with
`xdotool search --class XTVApp` (not by window title).

## Debug builds want DWARF 2

The embedded GDB 5.3 engine only reads DWARF **version 2**, while modern gcc
defaults to version 5. The IDE auto-appends `-gdwarf-2` to C/C++ debug builds
when `-g` is active and no explicit debug format is selected, so debugging
works out of the box. An explicit `-gdwarf*`, `-gstabs*`, `-gcoff` or
`-gxcoff` choice is always respected.

## Troubleshooting

- **`configure: Could not find Turbo Vision header files`** — export `TV_INC`
  (and the other variables in step 3) before `./configure`.
- **`make` recompiles/regenerates `*.mak`** — the build regenerates makefiles
  from the `.gpr` projects; this is normal.
- **Don't run parallel `make` inside the gdb subtree** — the 2002 gdb build
  system (libtool 1.4/automake 1.4) is not parallel-safe; the top-level
  makefile serializes it for you.
