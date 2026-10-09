/* Copyright (C) 1996-2000 Robert H�hne, see COPYING.RH for details */
/* This file is part of RHIDE. */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define Uses_TRect
#define Uses_TColorDialog
#define Uses_TColorGroup
#define Uses_TColorItem
#define Uses_TPalette
#define Uses_TProgram
#define Uses_TDeskTop
#define Uses_TIDEFileEditor

#define Uses_TDialog
#define Uses_TListBox
#define Uses_TScrollBar
#define Uses_TLButton
#define Uses_TStringCollection
#define Uses_TFileDialog
#define Uses_TScreen
#define Uses_MsgBox

#define Uses_tvutilFunctions
#include <libide.h>

#include <rhutils.h>
#include <libtvuti.h>

#include "rhide.h"
#include "rhidehis.h"

static TPalette *temp_pal = NULL;

#if 0
class _TColorItem:public TColorItem
{
public:
  _TColorItem(const char *nm, uchar idx);
};

_TColorItem::_TColorItem(const char *nm, uchar idx):
TColorItem(_(nm), idx)
{
}

static TColorGroup &
windows_colors(char *name, int base)
{
  return
    *new TColorGroup(name)
    + *new _TColorItem(__("Frame disabled"), base)
    + *new _TColorItem(__("Frame"), base + 0x01)
    + *new _TColorItem(__("Frame icons"), base + 0x02)
    + *new _TColorItem(__("Scroll bar page"), base + 0x03)
    + *new _TColorItem(__("Scroll bar icons"), base + 0x04)
    + *new _TColorItem(__("Static text"), base + 0x05)
    + *new _TColorItem(__("Selected text"), base + 0x06)
#if 0
    + *new _TColorItem(__("reserved"), base + 0x07)
#endif
    ;
}

static TColorGroup &
editor_colors(char *name, int base)
{
  return
    *new TColorGroup(name)
    + *new _TColorItem(__("Frame disabled"), base)
    + *new _TColorItem(__("Frame/background"), base + 0x01)
    + *new _TColorItem(__("Frame icons"), base + 0x02)
    + *new _TColorItem(__("Scroll bar page"), base + 0x03)
    + *new _TColorItem(__("Scroll bar icons"), base + 0x04)
    + *new _TColorItem(__("normal text"), base + 0x05)
    + *new _TColorItem(__("marked text"), base + 0x06)
    + *new _TColorItem(__("comment"), base + 0x07)
    + *new _TColorItem(__("reserved word"), base + 0x08)
    + *new _TColorItem(__("identifier"), base + 0x09)
    + *new _TColorItem(__("symbol"), base + 0x0A)
    + *new _TColorItem(__("string"), base + 0x0B)
    + *new _TColorItem(__("integer"), base + 0x0C)
    + *new _TColorItem(__("float"), base + 0x0D)
    + *new _TColorItem(__("octal"), base + 0x0E)
    + *new _TColorItem(__("hex"), base + 0x0F)
    + *new _TColorItem(__("character"), base + 0x10)
    + *new _TColorItem(__("preprocessor"), base + 0x11)
    + *new _TColorItem(__("illegal char"), base + 0x12)
    + *new _TColorItem(__("user defined words"), base + 0x13)
    + *new _TColorItem(__("CPU line"), base + 0x14)
    + *new _TColorItem(__("Breakpoint"), base + 0x15)
    + *new _TColorItem(__("symbol2"), base + 0x16)
    + *new _TColorItem(__("Cross cursor"), base + 0x17)
    + *new _TColorItem(__("editor statusline"), base + 0x18)
    + *new _TColorItem(__("parens matching"), base + 0x19)
    + *new _TColorItem(__("rectangle block"), base + 0x1A);
}
#else

static void
addItem(TColorGroup * &group, int index, const char *name,
        const char *group_name)
{
  if (strcmp(name, "reserved") == 0)
    return;
  if (*group_name)
  {
    if (!group)
      group = new TColorGroup(_(group_name));
    else
      *group = *group + *new TColorGroup(_(group_name));
  }
  *group = *group + *new TColorItem(_(name), index);
}

#include <pal.h>
#undef S
#undef S_
#define S(index,foreground,background,name,comment...) \
  addItem(group,0x##index,#name,"");
#define S_(index,foreground,background,name,_group,comment...) \
  addItem(group,0x##index,#name,#_group);
#endif

static TColorDialog *
GetColorDialog()
{
  TColorDialog *c;

  TColorGroup *group = NULL;

  cpIDEColor
    c = new TColorDialog(&TProgram::application->getPalette(), group);
  if (temp_pal)
    delete temp_pal;

  temp_pal = new TPalette(TProgram::application->getPalette());
  c->setData(&TProgram::application->getPalette());
  return c;
}

void
Colors()
{
  TColorDialog *c = GetColorDialog();

  if (TProgram::application->validView(c) != 0)
  {
    if (TProgram::deskTop->execView(c) == cmCancel)
    {
      // restore the old palette
      TProgram::application->getPalette() = *temp_pal;
    }
    // force to reread the chached colors for the editor
    TIDEFileEditor::colorsCached = 0;

    Repaint();
    destroy(c);
  }
}

/*----------------------------------------------------------------------*/
/*  Color themes                                                        */
/*                                                                      */
/*  A theme is the set of IDE color-palette entries (index 1..N, each    */
/*  byte (background << 4) | foreground).  Themes live as readable       */
/*  INDEX=FG,BG text files; the active one is remembered in             */
/*  $(GET_HOME)/.rhide/theme and restored on startup.                    */
/*----------------------------------------------------------------------*/

static const char *theme_header =
  "# RHIDE color theme\n"
  "# <palette-index>=<foreground>,<background>\n"
  "# colors: 0 Black 1 Blue 2 Green 3 Cyan 4 Red 5 Magenta 6 Brown\n"
  "#         7 Lightgray 8 Darkgray 9 Lightblue 10 Lightgreen\n"
  "#         11 Lightcyan 12 Lightred 13 Lightmagenta 14 Yellow 15 White\n";

static const char *theme_dir_name = "/.rhide/themes";

static char *
user_theme_dir(void)
{
  char *home, *dir = NULL;

  home = expand_rhide_spec("$(GET_HOME)");
  string_dup(dir, home);
  if (*dir)
  {
    char *rh = NULL;

    string_dup(rh, home);
    string_cat(rh, "/.rhide");
    mkdir(rh, 0700);
    string_cat(rh, "/themes");
    mkdir(rh, 0700);
    string_free(rh);
    string_cat(dir, theme_dir_name);
  }
  string_free(home);
  return dir;
}

static char *
active_theme_file(void)
{
  char *home, *file = NULL, *rh = NULL;

  home = expand_rhide_spec("$(GET_HOME)");
  string_dup(file, home);
  if (*file)
  {
    string_dup(rh, home);
    string_cat(rh, "/.rhide");
    mkdir(rh, 0700);
    string_free(rh);
    string_cat(file, "/.rhide/theme");
  }
  string_free(home);
  return file;
}

static int
apply_theme_file(const char *path)
{
  FILE *f;
  unsigned char *base = NULL, *buf;
  int len = 0, applied = 0;
  char line[512];

  f = fopen(path, "r");
  if (!f)
    return 0;
  GetThemePalette(&base, &len);
  if (len <= 0)
  {
    fclose(f);
    return 0;
  }
  buf = new unsigned char[len];
  memcpy(buf, base, len);
  while (fgets(line, sizeof(line), f))
  {
    char *p = line, *eq;
    int idx, fg, bg;

    while (*p == ' ' || *p == '\t')
      p++;
    if (*p == '#' || *p == '\n' || *p == '\0')
      continue;
    eq = strchr(p, '=');
    if (!eq)
      continue;
    if (sscanf(p, "%d", &idx) != 1)
      continue;
    if (sscanf(eq + 1, "%d,%d", &fg, &bg) != 2)
      continue;
    if (idx >= 1 && idx <= len && fg >= 0 && fg <= 15 && bg >= 0 && bg <= 15)
    {
      buf[idx - 1] = (unsigned char) ((bg << 4) | fg);
      applied++;
    }
  }
  fclose(f);
  if (applied)
    ApplyThemePalette(buf, len);
  delete[] buf;
  return applied;
}

static int
save_theme_file(const char *path)
{
  FILE *f;
  unsigned char *buf = NULL;
  int len = 0, i;

  GetThemePalette(&buf, &len);
  f = fopen(path, "w");
  if (!f)
    return 0;
  fputs(theme_header, f);
  for (i = 1; i <= len; i++)
  {
    int c = buf[i - 1];

    fprintf(f, "%d=%d,%d\n", i, c & 0x0f, (c >> 4) & 0x0f);
  }
  fclose(f);
  return 1;
}

static void
save_active_theme(void)
{
  char *f = active_theme_file();

  if (f && *f)
    save_theme_file(f);
  string_free(f);
}

/* Built-in "Dark": brighten foregrounds, darken the light backgrounds.
   Deliberately simple; save it and tweak for a custom theme. */
static unsigned char
dark_bg(unsigned char c)
{
  switch (c)
  {
    case 7:  return 0;    /* Lightgray -> Black    */
    case 15: return 8;    /* White     -> Darkgray */
    case 8:  return 15;
    case 1:  return 0;    /* Blue      -> Black    */
    case 2:  return 1;    /* Green     -> Blue (selection) */
    case 3:  return 8;    /* Cyan      -> Darkgray (lists, scroll bars) */
    case 4:  return 0;    /* Red       -> Black    */
    case 6:  return 4;    /* Brown     -> Red      */
    default: return c;
  }
}

static unsigned char
dark_fg(unsigned char c)
{
  switch (c)
  {
    case 0:  return 7;    /* Black     -> Lightgray */
    case 7:  return 15;   /* Lightgray -> White     */
    case 8:  return 7;    /* Darkgray  -> Lightgray */
    case 1:  return 9;
    case 2:  return 10;
    case 3:  return 11;
    case 4:  return 12;
    case 5:  return 13;
    case 6:  return 14;
    default: return c;
  }
}

static void
apply_dark_theme(void)
{
  unsigned char *base = NULL, *buf;
  int len = 0, i;

  GetThemePalette(&base, &len);
  if (len <= 0)
    return;
  buf = new unsigned char[len];
  for (i = 0; i < len; i++)
  {
    unsigned char c = base[i];

    buf[i] = (unsigned char) ((dark_bg(c >> 4) << 4) | dark_fg(c & 0x0f));
  }
  ApplyThemePalette(buf, len);
  delete[] buf;
}

static void
theme_changed(void)
{
  TIDEFileEditor::colorsCached = 0;
  save_active_theme();
  Repaint();
}

static void
scan_theme_dir(const char *dir, TStringCollection * names,
               TStringCollection * paths)
{
  DIR *dp;
  struct dirent *e;

  if (!dir || !*dir)
    return;
  dp = opendir(dir);
  if (!dp)
    return;
  while ((e = readdir(dp)) != NULL)
  {
    int l = strlen(e->d_name);

    if (l > 6 && strcmp(e->d_name + l - 6, ".theme") == 0)
    {
      char *full = NULL, *disp = NULL;

      string_dup(full, dir);
      string_cat(full, "/", e->d_name, NULL);
      string_dup(disp, e->d_name);
      disp[l - 6] = 0;
      names->insert(newStr(disp));
      paths->insert(newStr(full));
      string_free(full);
      string_free(disp);
    }
  }
  closedir(dp);
}

void
IDEColorTheme(void)
{
  TStringCollection *names, *paths;
  char *udir, *sdir = NULL, *src;
  TDialog *d;
  TListBox *box;
  TScrollBar *sb;
  TRect r, bt;
  ushort result;
  int sel = -1;

  names = new TStringCollection(20, 5);
  paths = new TStringCollection(20, 5);
  names->insert(newStr(_("Classic")));
  names->insert(newStr(_("Dark")));
  udir = user_theme_dir();
  scan_theme_dir(udir, names, paths);
  string_free(udir);
  src = expand_rhide_spec("$(RHIDESRC)");
  string_dup(sdir, src);
  string_free(src);
  if (sdir && *sdir)
  {
    string_cat(sdir, "/themes");
    scan_theme_dir(sdir, names, paths);
  }
  string_free(sdir);

  d = new TDialog(TRect(0, 0, 50, 16), _("Color theme"));
  d->options |= ofCentered;
  r = d->getExtent();
  r.grow(-1, -1);
  r.b.y -= 2;
  sb = new TScrollBar(TRect(r.b.x - 1, r.a.y, r.b.x, r.b.y));
  box = new TListBox(r, 1, sb);
  box->newList(names);
  d->insert(sb);
  d->insert(box);
  bt = d->getExtent();
  bt.a.y = bt.b.y - 2;
  bt.b.y = bt.a.y + 1;
  bt.a.x = bt.a.x + 3;
  bt.b.x = bt.a.x + 12;
  d->insert(new TLButton(bt, _("O~K~"), cmOK, bfDefault));
  bt.a.x = bt.b.x + 2;
  bt.b.x = bt.a.x + 12;
  d->insert(new TLButton(bt, _("~C~ancel"), cmCancel, bfNormal));
  TProgram::deskTop->insert(d);
  d->setState(sfModal, True);
  result = d->execute();
  if (result == cmOK)
    sel = box->focused;
  TProgram::deskTop->remove(d);
  destroy(d);

  if (sel == 0)
    ResetThemePalette();
  else if (sel == 1)
    apply_dark_theme();
  else if (sel >= 2 && sel - 2 < paths->getCount())
    apply_theme_file((char *) paths->at(sel - 2));
  if (sel >= 0)
    theme_changed();

  destroy(names);
  destroy(paths);
}

void
IDELoadTheme(void)
{
  TFileDialog *dialog;
  ushort result;

  InitHistoryID(RHIDE_History_theme);
  dialog = new TFileDialog("*.theme", _("Load color theme"), _("~N~ame"),
                           fdOpenButton, RHIDE_History_theme);
  TProgram::deskTop->insert(dialog);
  dialog->setState(sfModal, True);
  result = dialog->execute();
  if (result != cmCancel)
  {
    char fname[512];

    dialog->getData(fname);
    if (!apply_theme_file(fname))
      messageBox(mfError | mfOKButton,
                 _("Could not read any colors from '%s'"), fname);
    else
      theme_changed();
  }
  TProgram::deskTop->remove(dialog);
  destroy(dialog);
}

void
IDESaveTheme(void)
{
  TFileDialog *dialog;
  ushort result;

  InitHistoryID(RHIDE_History_theme);
  dialog = new TFileDialog("*.theme", _("Save color theme"), _("~N~ame"),
                           fdOKButton, RHIDE_History_theme);
  TProgram::deskTop->insert(dialog);
  dialog->setState(sfModal, True);
  result = dialog->execute();
  if (result != cmCancel)
  {
    char fname[512];

    dialog->getData(fname);
    if (!save_theme_file(fname))
      messageBox(mfError | mfOKButton, _("Could not write '%s'"), fname);
    else
      save_active_theme();
  }
  TProgram::deskTop->remove(dialog);
  destroy(dialog);
}

void
IDELoadThemeAtStartup(void)
{
  char *f = active_theme_file();

  if (f && *f)
  {
    struct stat st;

    if (stat(f, &st) == 0)
      apply_theme_file(f);
  }
  string_free(f);
}
