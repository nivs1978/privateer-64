param(
    [ValidateSet('kaper', 'main', 'disk', 'launch')]
    [string]$Target = 'disk'
)

$ErrorActionPreference = 'Stop'

$workspace = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $workspace 'build'
$kaperBas = Join-Path $workspace 'KAPER.BAS'
$mainBas = Join-Path $workspace 'MAIN.BAS'
$kaperPrg = Join-Path $buildDir 'kaper.prg'
$mainPrg = Join-Path $buildDir 'main.prg'
$diskImage = Join-Path $buildDir 'kaper.d64'
$viceExe = 'C:\apps\GTK3VICE-3.10-win64\bin\x64sc.exe'
$c1541Exe = 'C:\apps\GTK3VICE-3.10-win64\bin\c1541.exe'

function Get-Vs64Tools {
    $vs64 = Get-ChildItem "$env:USERPROFILE\.vscode\extensions\rosc.vs64-*" -Directory |
        Sort-Object Name -Descending |
        Select-Object -First 1

    if (-not $vs64) {
        throw 'VS64 extension not found.'
    }

    return @{
        Python = Join-Path $vs64.FullName 'resources\python\python.exe'
        BasicCompiler = Join-Path $vs64.FullName 'tools\bc.py'
    }
}

function Ensure-BuildDir {
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
}

function Compile-Basic([string]$sourcePath, [string]$outputPath) {
    $tools = Get-Vs64Tools
    & $tools.Python $tools.BasicCompiler --tsb -I $workspace -I $buildDir -o $outputPath $sourcePath
    if ($LASTEXITCODE -ne 0) {
        throw "Compile failed for $sourcePath"
    }
}

function Build-Kaper {
    Ensure-BuildDir
    Compile-Basic -sourcePath $kaperBas -outputPath $kaperPrg
}

function Build-Main {
    Ensure-BuildDir
    Compile-Basic -sourcePath $mainBas -outputPath $mainPrg
}

function Create-Disk {
    Build-Kaper
    Build-Main

    Write-Host "Creating disk: $diskImage"
    & $c1541Exe -format kaper,01 d64 $diskImage -write $kaperPrg kaper -write $mainPrg main -list $diskImage
    if ($LASTEXITCODE -ne 0) {
        throw 'D64 creation failed'
    }
}

function Launch-Vice {
    Create-Disk
    $autostartArg = '{0}:main' -f $diskImage
    Write-Host "Starting VICE with $autostartArg"
    & $viceExe -autostart $autostartArg
}

switch ($Target) {
    'kaper' { Build-Kaper }
    'main' { Build-Main }
    'disk' { Create-Disk }
    'launch' { Launch-Vice }
}