#!/usr/bin/env python3
# @file    workspace_sync.py
#
# @brief   Synchronize a project based on the dnx RTOS workspace with the
#          workspace. The synchronization is described once in a definition
#          file of the project (TOML, default utils/workspace_sync.toml, see
#          tools/workspace_sync.toml) and done in steps:
#
#            1. files: directories and files of the workspace ([[sync]]
#               entries: tools, common libraries and programs, appsets, ...)
#               are copied to the project (mode "mirror" or "update")
#            2. kernel: the kernel is checked out from git (branch, tag,
#               commit or the commit used by the workspace) or copied from a
#               directory
#            3. config: configurations are synchronized by
#               tools/config_sync.sh and the BSP files of the project are
#               applied to restore the configuration of the project
#
#          Usage: tools/workspace_sync.py [-c FILE] [-n] [-f] [-s STEPS]
#
# @note    The workspace source can be a directory or a git repository (clone
#          to a temporary directory). Branches are never created: the kernel
#          is checked out as detached HEAD.
#
#          MIT License
#
#          Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>
#
#          Permission is hereby granted, free of charge, to any person obtaining
#          a copy of this software and associated documentation files (the
#          "Software"), to deal in the Software without restriction, including
#          without limitation the rights to use, copy, modify, merge, publish,
#          distribute, sublicense, and/or sell copies of the Software, and to
#          permit persons to whom the Software is furnished to do so, subject to
#          the following conditions:
#
#          The above copyright notice and this permission notice shall be included
#          in all copies or substantial portions of the Software.
#
#          THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#          OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#          MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#          IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#          CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#          TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#          SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import argparse
import filecmp
import fnmatch
import os
import shutil
import subprocess
import sys
import tempfile

try:
    import tomllib
except ModuleNotFoundError:
    sys.exit("[error] Python 3.11 or newer is required (tomllib)")

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
DEFAULT_CONFIG = "utils/workspace_sync.toml"
STEPS = ("files", "kernel", "config")
MODES = ("mirror", "update")

# paths never copied and never removed
ALWAYS_EXCLUDE = [".git", "__pycache__", "*.pyc"]

# paths of the kernel not copied (build output) in addition to ALWAYS_EXCLUDE
KERNEL_EXCLUDE = ["build"]


class SyncError(Exception):
    pass


def log(msg):
    print(msg, flush=True)


def run(cmd, cwd=None, capture=False):
    """Run command, raise SyncError on failure, return stdout if capture."""
    try:
        res = subprocess.run(cmd, cwd=cwd, check=True, text=True,
                             stdout=subprocess.PIPE if capture else None)
    except FileNotFoundError:
        raise SyncError(f"command not found: {cmd[0]}")
    except subprocess.CalledProcessError as e:
        raise SyncError(f"command failed ({e.returncode}): {' '.join(cmd)}")
    return res.stdout.strip() if capture else None


def git(args, cwd=None, capture=True):
    return run(["git"] + args, cwd=cwd, capture=capture)


def is_git_repo(path):
    return os.path.exists(os.path.join(path, ".git"))


def abspath(base, path):
    """Path relative to base (or absolute, with ~ expanded), symbolic links are
    resolved (".." of a link is the parent of its target)."""
    path = os.path.expanduser(path)
    return os.path.realpath(path if os.path.isabs(path) else os.path.join(base, path))


def project_link(rel):
    """Target of symbolic link when path of project (or one of its directories
    in the project) is a link, None otherwise."""
    path = ROOT_DIR
    for part in rel.split("/"):
        path = os.path.join(path, part)
        if os.path.islink(path):
            return os.path.realpath(path)
    return None


#-------------------------------------------------------------------------------
# Patterns: pattern without '/' matches name of any path component, pattern
# with '/' matches path relative to the synchronized directory ('*' matches
# also '/'), e.g. "build", "*.bak", "programs/myprog", "libs/*.mk".
#-------------------------------------------------------------------------------
def matches(rel, patterns):
    parts = rel.split("/")
    for pat in patterns:
        pat = pat.strip("/")
        if "/" in pat:
            if fnmatch.fnmatchcase(rel, pat) or fnmatch.fnmatchcase(rel, pat + "/*"):
                return True
        elif any(fnmatch.fnmatchcase(p, pat) for p in parts):
            return True
    return False


def included(rel, include):
    """File is included when include list is empty or file matches it."""
    return not include or matches(rel, include)


def list_files(top, exclude):
    """Relative paths (with '/') of files and links in directory, excluded
    files and directories are skipped."""
    files = set()
    for root, dirs, names in os.walk(top):
        rel_root = os.path.relpath(root, top).replace(os.sep, "/")
        rel_root = "" if rel_root == "." else rel_root + "/"
        keep = []
        for d in dirs:
            rel = rel_root + d
            if matches(rel, exclude):
                continue
            if os.path.islink(os.path.join(root, d)):
                files.add(rel)
            else:
                keep.append(d)
        dirs[:] = keep
        for n in names:
            rel = rel_root + n
            if not matches(rel, exclude):
                files.add(rel)
    return files


def same_file(src, dst):
    if os.path.islink(src) or os.path.islink(dst):
        return (os.path.islink(src) and os.path.islink(dst)
                and os.readlink(src) == os.readlink(dst))
    if not os.path.isfile(dst):
        return False
    if os.stat(src).st_mode & 0o111 != os.stat(dst).st_mode & 0o111:
        return False
    return filecmp.cmp(src, dst, shallow=False)


def copy_file(src, dst):
    if os.path.lexists(dst) and (os.path.isdir(dst) and not os.path.islink(dst)):
        shutil.rmtree(dst)
    elif os.path.lexists(dst):
        os.remove(dst)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if os.path.islink(src):
        os.symlink(os.readlink(src), dst)
    else:
        shutil.copy2(src, dst)


def remove_empty_dirs(top, exclude):
    """Remove empty directories (not excluded) below top."""
    for root, dirs, names in os.walk(top, topdown=False):
        if root == top:
            continue
        rel = os.path.relpath(root, top).replace(os.sep, "/")
        if matches(rel, exclude):
            continue
        if not os.listdir(root):
            os.rmdir(root)


def sync_tree(src, dst, mode, include, exclude, dry_run):
    """Synchronize directory or file src to dst. Returns (added, modified,
    removed) lists of relative paths."""
    added, modified, removed = [], [], []

    if os.path.isfile(src) or os.path.islink(src):
        name = os.path.basename(dst)
        if not os.path.lexists(dst):
            added.append(name)
        elif not same_file(src, dst):
            modified.append(name)
        if (added or modified) and not dry_run:
            copy_file(src, dst)
        return added, modified, removed

    src_files = {f for f in list_files(src, exclude) if included(f, include)}

    for rel in sorted(src_files):
        s = os.path.join(src, rel)
        d = os.path.join(dst, rel)
        if not os.path.lexists(d):
            added.append(rel)
        elif same_file(s, d):
            continue
        else:
            modified.append(rel)
        if not dry_run:
            copy_file(s, d)

    if mode == "mirror" and os.path.isdir(dst):
        for rel in sorted(list_files(dst, exclude)):
            if rel in src_files or not included(rel, include):
                continue
            removed.append(rel)
            if not dry_run:
                os.remove(os.path.join(dst, rel))
        if not dry_run:
            remove_empty_dirs(dst, exclude)

    return added, modified, removed


def report(name, changes, verbose):
    added, modified, removed = changes
    log(f"    {name}: {len(added)} added, {len(modified)} modified, {len(removed)} removed")
    if verbose:
        for tag, files in (("A", added), ("M", modified), ("D", removed)):
            for f in files:
                log(f"      {tag} {f}")


#-------------------------------------------------------------------------------
# Definition file
#-------------------------------------------------------------------------------
def get(table, key, types, default=None, where=""):
    val = table.get(key, default)
    if val is not None and not isinstance(val, types):
        raise SyncError(f"{where}{key}: wrong type")
    return val


def load_config(path):
    try:
        with open(path, "rb") as f:
            cfg = tomllib.load(f)
    except FileNotFoundError:
        raise SyncError(f"definition file not found: {path} (example: tools/workspace_sync.toml)")
    except tomllib.TOMLDecodeError as e:
        raise SyncError(f"{path}: {e}")

    known = {"workspace", "kernel", "sync", "config", "exclude"}
    for key in cfg:
        if key not in known:
            raise SyncError(f"{path}: unknown key or table: {key}")
    return cfg


#-------------------------------------------------------------------------------
# Workspace source
#-------------------------------------------------------------------------------
class Workspace:
    def __init__(self, cfg, cfg_dir):
        ws = get(cfg, "workspace", dict, {})
        self.path = get(ws, "path", str, where="workspace.")
        self.url = get(ws, "git", str, where="workspace.")
        self.ref = get(ws, "ref", str, "develop", where="workspace.")
        self.tmp = None

        if bool(self.path) == bool(self.url):
            raise SyncError("workspace: set one of 'path' or 'git'")
        if self.path:
            self.path = abspath(cfg_dir, self.path)
            if not os.path.isdir(self.path):
                raise SyncError(f"workspace directory not found: {self.path}")

    def describe(self):
        return self.path if self.path else f"{self.url} ({self.ref})"

    def open(self):
        """Directory with files of the workspace (clone of git repository)."""
        if self.path:
            return self.path
        self.tmp = tempfile.mkdtemp(prefix="workspace_sync.")
        log(f"==> Fetch workspace {self.url} ({self.ref})")
        git(["init", "-q", self.tmp], capture=False)
        git(["fetch", "-q", "--depth", "1", self.url, self.ref], cwd=self.tmp, capture=False)
        git(["checkout", "-q", "--detach", "FETCH_HEAD"], cwd=self.tmp, capture=False)
        self.path = self.tmp
        return self.path

    def kernel_commit(self):
        """Commit of the kernel used by the workspace."""
        kdir = os.path.join(self.path, "kernel")
        if not self.tmp and is_git_repo(kdir):
            return git(["rev-parse", "HEAD"], cwd=kdir)
        return git(["rev-parse", "HEAD:kernel"], cwd=self.path)

    def kernel_url(self):
        try:
            return git(["config", "-f", ".gitmodules", "submodule.kernel.url"], cwd=self.path)
        except SyncError:
            return None

    def close(self):
        if self.tmp:
            shutil.rmtree(self.tmp, ignore_errors=True)


#-------------------------------------------------------------------------------
# Steps
#-------------------------------------------------------------------------------
def sync_entries(cfg):
    entries = []
    for i, e in enumerate(get(cfg, "sync", list, [])):
        where = f"sync[{i}]."
        if not isinstance(e, dict):
            raise SyncError(f"sync[{i}]: table expected")
        src = get(e, "src", str, where=where)
        if not src:
            raise SyncError(f"{where}src: required")
        mode = get(e, "mode", str, "mirror", where=where)
        if mode not in MODES:
            raise SyncError(f"{where}mode: one of {', '.join(MODES)}")
        unknown = set(e) - {"src", "dst", "mode", "include", "exclude"}
        if unknown:
            raise SyncError(f"{where}unknown key: {', '.join(sorted(unknown))}")
        entries.append({
            "src": src.strip("/"),
            "dst": get(e, "dst", str, src, where=where).strip("/"),
            "mode": mode,
            "include": get(e, "include", list, [], where=where),
            "exclude": get(e, "exclude", list, [], where=where),
        })
    return entries


def step_files(cfg, ws_dir, args):
    exclude_all = ALWAYS_EXCLUDE + get(cfg, "exclude", list, [])
    entries = sync_entries(cfg)
    log(f"==> Synchronize files ({len(entries)} entries)")
    for e in entries:
        src = os.path.join(ws_dir, e["src"])
        dst = os.path.join(ROOT_DIR, e["dst"])
        if not os.path.lexists(src):
            raise SyncError(f"not found in workspace: {e['src']}")
        name = e["src"] if e["src"] == e["dst"] else f"{e['src']} -> {e['dst']}"
        link = project_link(e["dst"])
        if link:
            log(f"    {name}: skipped, symbolic link to {link}")
            continue
        changes = sync_tree(src, dst, e["mode"], e["include"],
                            exclude_all + e["exclude"], args.dry_run)
        report(f"{name} ({e['mode']})", changes, args.verbose)


def kernel_config(cfg):
    k = get(cfg, "kernel", dict, {})
    source = get(k, "source", str, "git", where="kernel.")
    if source not in ("git", "path", "none"):
        raise SyncError("kernel.source: one of git, path, none")
    return k, source


def step_kernel(cfg, ws, args):
    k, source = kernel_config(cfg)
    kdir = os.path.join(ROOT_DIR, "kernel")

    if source == "none":
        log("==> Kernel: not synchronized (source = \"none\")")
        return

    link = project_link("kernel")
    if link:
        log(f"==> Kernel: skipped, symbolic link to {link}")
        return

    if source == "path":
        path = get(k, "path", str, where="kernel.")
        if not path:
            raise SyncError("kernel.path: required for source = \"path\"")
        src = abspath(args.cfg_dir, path)
        if not os.path.isdir(src):
            raise SyncError(f"kernel directory not found: {src}")
        log(f"==> Copy kernel from {src}")
        exclude = ALWAYS_EXCLUDE + KERNEL_EXCLUDE + get(k, "exclude", list, [], where="kernel.")
        changes = sync_tree(src, kdir, "mirror", [], exclude, args.dry_run)
        report("kernel (mirror)", changes, args.verbose)
        return

    # source = "git"
    ref = get(k, "ref", str, "workspace", where="kernel.")
    url = get(k, "git", str, where="kernel.")
    if ref == "workspace":
        ref = ws.kernel_commit()
    if not url:
        if is_git_repo(kdir):
            try:
                url = git(["remote", "get-url", "origin"], cwd=kdir)
            except SyncError:
                url = None
        url = url or ws.kernel_url()
    if not url:
        raise SyncError("kernel.git: URL not set and not found in kernel or workspace")

    if os.path.isdir(kdir) and not is_git_repo(kdir) and os.listdir(kdir):
        raise SyncError("kernel: directory is not a git repository (use source = \"path\")")

    if is_git_repo(kdir):
        old = git(["rev-parse", "HEAD"], cwd=kdir)
    else:
        old = None

    log(f"==> Kernel from git {url} ({ref})")
    if args.dry_run:
        log(f"    kernel: {old[:12] if old else 'none'} -> {ref}"
            + (" (no change)" if old == ref else ""))
        return

    if not is_git_repo(kdir):
        git(["init", "-q", kdir], capture=False)
    try:
        git(["cat-file", "-e", ref + "^{commit}"], cwd=kdir)
        target = ref if len(ref) >= 7 and all(c in "0123456789abcdef" for c in ref.lower()) else None
    except SyncError:
        target = None
    if not target:
        git(["fetch", "-q", url, ref], cwd=kdir, capture=False)
        target = "FETCH_HEAD"
    git(["checkout", "-q", "--detach", target], cwd=kdir, capture=False)
    new = git(["rev-parse", "HEAD"], cwd=kdir)
    log(f"    kernel: {old[:12] if old else 'none'} -> {new[:12]}"
        + (" (no change)" if old == new else ""))


def step_config(cfg, args):
    c = get(cfg, "config", dict, {})
    sync = get(c, "sync", bool, True, where="config.")
    bsps = get(c, "bsp", list, [], where="config.")
    for bsp in bsps:
        if not isinstance(bsp, str):
            raise SyncError("config.bsp: list of strings expected")

    if sync:
        cmd = ["bash", "tools/config_sync.sh"] + bsps
        log(f"==> {' '.join(cmd)}")
        if not args.dry_run:
            if not bsps:
                raise SyncError("config.bsp: list BSP files of the project to restore configuration")
            run(cmd, cwd=ROOT_DIR)
    else:
        for bsp in bsps:
            cmd = ["bash", "tools/bsp_import.sh", bsp]
            log(f"==> {' '.join(cmd)}")
            if not args.dry_run:
                run(cmd, cwd=ROOT_DIR)


def check_clean(cfg, steps):
    """Refuse to overwrite uncommitted changes of the project."""
    if not is_git_repo(ROOT_DIR):
        return
    paths = []
    if "files" in steps:
        paths += [e["dst"] for e in sync_entries(cfg)]
    if "kernel" in steps and kernel_config(cfg)[1] != "none":
        paths.append("kernel")
    if "config" in steps:
        paths += ["config", "bsp"]
    paths = [p for p in paths if os.path.lexists(os.path.join(ROOT_DIR, p))]
    if not paths:
        return
    out = git(["status", "--porcelain", "--"] + paths, cwd=ROOT_DIR)
    if out:
        raise SyncError("uncommitted changes in synchronized paths (commit them or use --force):\n" + out)


def main():
    parser = argparse.ArgumentParser(
        description="Synchronize the project with the dnx RTOS workspace.")
    parser.add_argument("-c", "--config", default=DEFAULT_CONFIG,
                        help=f"definition file (default {DEFAULT_CONFIG})")
    parser.add_argument("-n", "--dry-run", action="store_true",
                        help="show changes, do not change anything")
    parser.add_argument("-f", "--force", action="store_true",
                        help="synchronize even with uncommitted changes")
    parser.add_argument("-s", "--steps", default=",".join(STEPS),
                        help=f"steps separated by ',' (default {','.join(STEPS)})")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="list changed files")
    args = parser.parse_args()

    steps = [s.strip() for s in args.steps.split(",") if s.strip()]
    for s in steps:
        if s not in STEPS:
            parser.error(f"unknown step: {s}")

    cfg_path = abspath(ROOT_DIR, args.config)
    args.cfg_dir = os.path.dirname(cfg_path)

    ws = None
    try:
        cfg = load_config(cfg_path)
        ws = Workspace(cfg, args.cfg_dir)
        sync_entries(cfg)
        kernel_config(cfg)
        if not args.force and not args.dry_run:
            check_clean(cfg, steps)

        log(f"Workspace: {ws.describe()}")
        if args.dry_run:
            log("Dry run: nothing is changed.")

        ws_dir = ws.open() if ("files" in steps or "kernel" in steps) else None
        if os.path.realpath(ws_dir or "") == os.path.realpath(ROOT_DIR):
            raise SyncError("workspace source is the project itself")

        if "files" in steps:
            step_files(cfg, ws_dir, args)
        if "kernel" in steps:
            step_kernel(cfg, ws, args)
        if "config" in steps:
            step_config(cfg, args)
    except SyncError as e:
        sys.exit(f"[error] {e}")
    finally:
        if ws:
            ws.close()

    log("== Workspace synchronized." if not args.dry_run else "== Dry run finished.")


if __name__ == "__main__":
    main()
