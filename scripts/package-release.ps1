# Assemble the public release folder (and optionally a zip) from a Release build.
#   scripts\package-release.ps1                 -> dist\AzureDreams-HD-Recompiled-<VERSION>\
#   scripts\package-release.ps1 -Zip            ... plus the .zip next to it
#   scripts\package-release.ps1 -Force          replace an existing output folder
#
# The package contains no game data: players supply their own disc image
# (picked on first launch and remembered in disc.cfg). Everything else lives
# inside the folder, so it can be unzipped anywhere or added to an app launcher.
# Two launchers pick a preset (presets\<name>\: its own settings + mod choices);
# the exe on its own opens the built-in launcher with the default (HD) setup.
param([string]$Build = 'build-release',
      [string]$Out,
      [switch]$Zip,
      [switch]$Force)
$ErrorActionPreference = 'Stop'

$root    = Split-Path -Parent $PSScriptRoot
$build   = Join-Path $root $Build
$exeName = 'AzureDreams_Recompiled.exe'
$version = (Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
if (-not $Out) { $Out = Join-Path $root "dist\AzureDreams-HD-Recompiled-$version" }
if (-not (Test-Path -LiteralPath (Join-Path $build $exeName))) { throw "Not built: $build\$exeName" }

if (Test-Path -LiteralPath $Out) {
    if (-not $Force) { throw "$Out exists (use -Force to replace it)" }
    Remove-Item -LiteralPath $Out -Recurse -Force
}
New-Item -ItemType Directory -Force $Out | Out-Null

# Runtime files. game.toml comes from the repo: disc stays blank (picked on
# first launch) and its exe path is absent, so the runtime reads the boot EXE
# from the player's disc.
Copy-Item -LiteralPath (Join-Path $build $exeName) -Destination $Out
Copy-Item -LiteralPath (Join-Path $root 'game.toml') -Destination $Out
foreach ($f in 'game_options.toml', 'input.ini', 'keybinds.ini') {
    $src = Join-Path $build $f
    if (Test-Path -LiteralPath $src) { Copy-Item -LiteralPath $src -Destination $Out }
}
Copy-Item -LiteralPath (Join-Path $build 'bios') -Destination $Out -Recurse
# assets\ minus box art: the build stages the local (untracked) cover art,
# which is the publisher's artwork and never ships.
Copy-Item -LiteralPath (Join-Path $build 'assets') -Destination $Out -Recurse
Get-ChildItem -LiteralPath (Join-Path $Out 'assets') -Recurse -File -Filter 'boxart*' |
    Remove-Item -Force
New-Item -ItemType Directory -Force (Join-Path $Out 'mods') | Out-Null
Copy-Item -LiteralPath (Join-Path $build 'mods\bundled') -Destination (Join-Path $Out 'mods') -Recurse
Copy-Item -LiteralPath (Join-Path $root 'presets') -Destination $Out -Recurse
# Overlay captures (recorded because game.toml enables the overlay cache) go to
# diagnostics\ with the runtime's other forensics, not the top of the folder.
[IO.File]::WriteAllText((Join-Path $Out '.psxrecomp.env'),
    "# Read by the runtime at startup; paths are relative to this folder.`r`n" +
    "PSX_OVERLAY_CAPTURES=diagnostics/overlay_captures.json`r`n")

# Preset launchers. %~dp0 is this folder, so they work wherever it is unzipped.
$bat = "@echo off`r`nstart `"`" `"%~dp0$exeName`" --preset {0} --no-launcher`r`n"
[IO.File]::WriteAllText((Join-Path $Out 'Play Azure Dreams (HD).bat'), ($bat -f 'hd'))
[IO.File]::WriteAllText((Join-Path $Out 'Play Azure Dreams (Original).bat'), ($bat -f 'original'))

$readme = @"
Azure Dreams HD Recompiled $version
==================================

A native Windows port of Azure Dreams (PlayStation, USA, SLUS-00614).
This package contains NO game data. You need your own disc image of
Azure Dreams (USA): the .cue with its .bin next to it.

HOW TO PLAY
  Play Azure Dreams (HD).bat
      Widescreen, 4x resolution, perspective-correct textures, smoothed
      movies, fast loading and a faster CPU (removes the original slowdown).
  Play Azure Dreams (Original).bat
      Plays like the PlayStation: 4:3, native resolution, original loading
      and speed.
  $exeName
      Opens the launcher, where you can change settings and mods yourself.

  The first launch asks for your disc image; it is remembered after that.
  Both modes share your memory-card saves (saves\) and controls. Each mode
  keeps its own settings in presets\hd\ or presets\original\.

UPDATING
  Copy the new files over this folder. Do not delete saves\ (your saves),
  disc.cfg (your disc choice) or presets\ (your settings).

BIOS
  Uses OpenBIOS (bios\, see bios\OpenBIOS.LICENSE); no PlayStation BIOS
  file is needed.

Azure Dreams is a trademark of Konami. This is an unofficial fan project,
not affiliated with or endorsed by Konami.
"@
[IO.File]::WriteAllText((Join-Path $Out 'README.txt'), ($readme -replace "`r?`n", "`r`n"))

# Refuse to ship anything that is game data or per-user state.
$bad = Get-ChildItem -LiteralPath $Out -Recurse -File | Where-Object {
    $rel = $_.FullName.Substring($Out.Length + 1)
    ($_.Extension -in '.cue', '.bin', '.img', '.iso', '.chd', '.mcr', '.mcd', '.sav', '.state' -and
        $rel -ne 'bios\openbios.bin') -or
    $_.Name -like 'SLUS*' -or $_.Name -like 'boxart*' -or $_.Name -like 'overlay_captures*' -or
    $_.Name -in 'disc.cfg', 'bios.cfg' -or
    ($_.Name -in 'settings.toml', 'state.toml' -and $rel -notlike 'presets\*')
}
if ($bad) { throw "Refusing to package: $($bad.FullName -join ', ')" }

if ($Zip) {
    $zipPath = "$Out.zip"
    if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
    Compress-Archive -Path (Join-Path $Out '*') -DestinationPath $zipPath
    "Zipped to $zipPath"
}
"Packaged to $Out"
