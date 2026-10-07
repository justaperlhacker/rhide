# RHIDE Revival — Status: BUILDING + RUNNING (2026-10-07)

## Result
RHIDE 1.5 builds clean with gcc 16 on CachyOS x86_64 and runs: full IDE,
project/file dialogs, editor, clean exit. `rhgdb.exe` builds, opens sources,
quits cleanly. All work committed + pushed (see repos below).

## Repos (GitHub: justaperlhacker)
- `rhide` (master): from `danieldc/rhide` CVS history + 5 revival commits.
  `upstream` remote = danieldc (untouched).
- `tvision` (branch `modern-gcc`, fork of set-soft/tvision): termios include,
  NCURSES_INTERNALS, `<version>` vs deprecated `<ciso646>`.
- `gdb-5.3` (master=pristine tarball, branch `modern-gcc`): config.if probe,
  bfd bool-enum, obstack macros, proc_service/gregset/thread-db glibc compat,
  strsignal, gdbserver strerror() x3.
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
NOTE: unset DISPLAY (or start under X) — with DISPLAY set, TV takes the X11
driver and tmux/terminal captures stay blank. `./rhide`, `./rhgdb.exe`.

## User-facing findings (reported during testing)
1. Popup "compile with -g": the 2002 DWARF reader accepts ONLY DWARF v2
   (`dwarf2read.c`: `version != 2` → error). gcc 16 emits DWARF 5 by
   default, so even correct -g builds show "no symbols". WORKAROUND NOW:
   compile debug builds with `-g -gdwarf-2` (flag already exists in the
   IDE: Options → Compiler → Debugging → `-gdwarf-2`). Verified working
   end-to-end (rhgdb opens + follows source).
2. "Could not find hello.c": modern gcc records DW_AT_name ABSOLUTE when
   given an absolute path; rhgdb joined dirname + absolute name
   (`cwd//tmp/...`). FIXED in rhgdb/main.cc (is_absolute_path guards).

## Verified 2026-10-07: terminal debugging works with -gdwarf-2
Fresh project (default -g) builds DWARF 5 binary -> F7 gives the exact
reported popup. After ticking -gdwarf-2 (Options/Compiler/Debugging) +
Build all -> binary is DWARF 2 -> F7 starts a session with no popup;
inferior observed in ptrace_stop traced by rhide (ps: `hello ... t<
ptrace_stop`, TracerPid=rhide). Alt+X quits cleanly, no orphans.

## Known follow-ups (not done)
- Auto-append `-gdwarf-2` to IDE debug builds vs documenting it (decision).
- Thread debugging on modern kernels (engine auto-run/flow untested live).
- `make install` / packaging untested.
- gdb-5.3 `modern-gcc` branch has no upstream tracking set (`git push -u`).
- Upstream the tvision fixes to set-soft/tvision (they'd likely take them).
