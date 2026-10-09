# RHIDE Revival — Status: BUILT + RUNNING under X11 (2026-10-07)

## Result
RHIDE 1.5 builds clean with gcc 16.2.1 on CachyOS x86_64 and runs as a
native X11 window (Turbo Vision X11 driver). Full IDE: project/file
dialogs, editor, build, integrated debugger, clean exit. `rhgdb.exe`
builds too. The debugger is a fork of GDB 5.3 embedded as a library
(see "Why GDB 5.3" below) — not optional under this architecture.

## Repos (GitHub: justaperlhacker)
- `rhide` (master): from `danieldc/rhide` CVS history + revival commits.
  `upstream` remote = danieldc (untouched).
- `tvision` (branch `modern-gcc`, fork of set-soft/tvision): termios include,
  NCURSES_INTERNALS, `<version>` vs deprecated `<ciso646>`.
- `gdb-5.3` (master=pristine tarball, branch `modern-gcc`): config.if probe,
  bfd bool-enum, obstack macros, proc_service/gregset/thread-db glibc compat,
  strsignal, gdbserver strerror() x3 (all details below).
- `setedit`, upstream clone, UNMODIFIED (no fork needed).

## How to build (from scratch)
```
# deps (order matters)
cd ~/Projects/tvision/tvision && ./configure --without-dynamic && make -j$(nproc)
ln -sf $PWD/rhtv-config ~/.local/bin/rhtv-config
cd ~/Projects/setedit/setedit && ./configure --libset --no-infview \
  --tv-include=$HOME/Projects/tvision/tvision/include \
  --tv-lib=$HOME/Projects/tvision/tvision/makes \
  && make needed && make libset
# rhide
cd ~/Projects/rhide
export TV_INC=$HOME/Projects/tvision/tvision/include \
 TVOBJ=$HOME/Projects/tvision/tvision/makes \
 SETSRC=$HOME/Projects/setedit/setedit SETOBJ=$HOME/Projects/setedit/setedit/makes \
 GDB_SRC=$HOME/Projects/gdb-5.3
./configure && make -j$(nproc) && make rhide
```
NOTE: the exports must be set *before* `./configure` (it probes them).
`setup-new-host.sh.txt` clones the forks and does the whole thing.

## Running (X11)
tvision is built `HAVE_X11=yes`; the driver list is priority-ordered
(`classes/tscreen.cc`): X11 100 > Linux console 90 > XTerm 60 > ncurses 10,
and X11 is chosen only if `DISPLAY` connects. So:
```
env DISPLAY=:0 ./rhide [project.gpr]
```
opens RHIDE's own X11 window (`WM_CLASS = "tvapp","XTVApp"`, built-in 8x16
bitmap font, no X core-font dependency). Unset `DISPLAY` to use the terminal
driver instead. The X11 window has no `_NET_WM_NAME`, so `wmctrl`/`xdotool`
by-name lookups won't find it; use `xdotool search --class XTVApp`.

## Verified 2026-10-07 (this host, gcc 16.2.1, XWayland)
- Build: `idegc.exe` (8.9M, links X11/Xmu/ncurses/gpm, embedded gdb symbols),
  `rhide` -> `rhide.exe` -> `idegc.exe`, `rhgdb/rhgdb.exe` (6.5M),
  `libgdb/libgdbrh.a` (5.9M).
- X11: window renders menu bar / Project Window / status bar; interactively
  added a source item, edited, built and quit with `xdotool` + `import`.
- Auto `-gdwarf-2` (previously "not runtime-verified"): a fresh project with
  default options built `hello`/`hello.o` as **DWARF Version 2** (readelf),
  while plain `gcc -g` gives Version 5. `F7` (Trace into) starts a session
  (inferior in state `t<` under rhide, ppid = rhide) with **no** "compile
  with -g" popup. `Alt+X` quits cleanly, no orphaned inferior.
- Deterministic flag check: `./gpr2mak.exe -o /tmp/x.mak rhide.gpr`
  -> `C_DEBUG_FLAGS=-g -gdwarf-2`.

## Auto -gdwarf-2 (implemented + runtime-verified)
GDB 5.3's DWARF reader only accepts DWARF v2 (`dwarf2read.c`: `version != 2`),
while gcc >= 12 defaults to DWARF 5. `TF(C_DEBUG_FLAGS)` in `idespec.cc`
appends `-gdwarf-2` when `-g*` is active without an explicit debug format
(`-gdwarf*`, `-gstabs*`, `-gcoff`, `-gxcoff` always respected).

## Why GDB 5.3 (not optional)
`librhgdb/*.c` call GDB internals directly (`init_gdb`,
`handle_gdb_command`, `lookup_symbol`, `find_pc_line`, ...); `libgdb`
preprocesses GDB's private headers into `libgdbrh.h` and archives GDB's own
`.o` files into `libgdbrh.a`, linked into `rhide`/`rhgdb`. Modern GDB (6+)
is C++ with no embeddable C API, so it cannot be a drop-in; the alternatives
are a large rewrite (drive system gdb via MI) or teaching gdb-5.3 to read
DWARF 5. `configure.in` hardcodes `gdb-5.3` (fallback `gdb-5.0`).

## gdb-5.3 `modern-gcc` fixes required
Earlier: `config.if` glibc probe, `bfd/bfd-in.h` bool-as-keyword, `obstack.h`
cast-as-lvalue. Added 2026-10-07 (branch tip):
1. `include/obstack.h`: `obstack_int_grow` lost its line-continuation
   backslash (broke `libiberty/obstack.o`).
2. `gdb/gdb_proc_service.h`: modern glibc `<proc_service.h>` defines
   `psaddr_t` but no `paddr_t`; include `gregset.h` and `typedef psaddr_t
   paddr_t`.
3. `gdb/lin-lwp.c`: `extern const char *strsignal(int)` conflicts with
   glibc's `char *strsignal(int)`.
4. `gdb/gdbserver/{utils.c,gdbreplay.c,linux-low.c}`: `sys_nerr`/
   `sys_errlist` were removed from glibc; use `strerror(errno)`.

## rhide build fix
`TF(C_WARN_FLAGS)` in `idespec.cc` appends `-Wno-error=overloaded-virtual`
when `-Werror` is active (GCC >= 11 enables `-Woverloaded-virtual` via
`-Wall`; old TV/setedit deliberately hides virtuals). Previously this was
hand-patched into generated `.mak` files, which was lost on regeneration;
it now comes from the spec so `rhgdb` builds with `-Wall -Werror`.

## Known follow-ups (not done)
- `make install` / packaging untested.
- Thread debugging on modern kernels (libthread_db path) untested live.
- Upstream the `-Wno-error=overloaded-virtual` spec shim / tvision fixes
  (set-soft/tvision would likely take the tvision ones).
- Commit + push the gdb-5.3 and rhide fixes above to the forks so
  `setup-new-host.sh.txt` reproduces on a clean machine.
