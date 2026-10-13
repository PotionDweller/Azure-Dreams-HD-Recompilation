# Apply this project's fixes to the pinned psxrecomp submodule (v0.3.3-alpha,
# 2da25c18). Run once after `git submodule update --init --recursive`.
# Safe to re-run: it reports and does nothing if the patch is already applied.
#
# Without the patch Azure Dreams black-screens before the intro movie (CD
# SeekL status), and the widescreen minimap, memory-card fast loading, the
# PGXP mod toggle and Alt+Enter fullscreen do not work. See patches/README.md.
# Not 'Stop': Windows PowerShell 5.1 turns a native command's stderr into a
# terminating error under 'Stop', and the "already applied?" probe below is
# expected to print an error on a fresh clone. Exit codes are checked instead.
$ErrorActionPreference = 'Continue'
$root  = Split-Path -Parent $PSScriptRoot
$patch = Join-Path $root 'patches\psxrecomp-local.patch'
$fw    = Join-Path $root 'psxrecomp'

git -C $fw apply --reverse --check $patch 2>$null
if ($LASTEXITCODE -eq 0) { "psxrecomp patch already applied."; exit 0 }

git -C $fw apply --check $patch
if ($LASTEXITCODE -ne 0) {
    throw "psxrecomp patch does not apply. Is the submodule at the pinned commit (git submodule update)?"
}
git -C $fw apply $patch
"psxrecomp patch applied."
