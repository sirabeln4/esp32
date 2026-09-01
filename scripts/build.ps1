[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet(
        'espressif_esp32_devkitc_v4',
        'espressif_esp32s3_devkitc1_n8',
        'espressif_esp32c6_devkitc1_n8',
        'seeed_xiao_esp32c6'
    )]
    [string]$Board,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[A-Za-z0-9_-]+$')]
    [string]$Project,

    [ValidatePattern('^COM[0-9]+$')]
    [string]$Port,

    [switch]$Flash,
    [switch]$Monitor,
    [switch]$Menuconfig,
    [switch]$Fullclean
)

$ErrorActionPreference = 'Stop'
$workspaceRoot = Split-Path -Parent $PSScriptRoot
$projectPath = Join-Path $workspaceRoot (Join-Path 'projects' $Project)
$buildPath = Join-Path $projectPath ("build-" + $Board.Replace('_', '-'))

if (-not (Test-Path -LiteralPath (Join-Path $projectPath 'CMakeLists.txt'))) {
    throw "ESP-IDF project not found: $projectPath"
}

if ((($Flash -or $Monitor) -and [string]::IsNullOrWhiteSpace($Port))) {
    throw '-Port COMx is required when using -Flash or -Monitor.'
}

$baseArguments = @(
    '-C', $projectPath,
    '-B', $buildPath,
    "-DBOARD=$Board"
)

if ($Fullclean) {
    & idf.py @baseArguments fullclean
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if ($Menuconfig) {
    & idf.py @baseArguments menuconfig
    exit $LASTEXITCODE
}

$actions = @('build')
if ($Flash) { $actions += 'flash' }
if ($Monitor) { $actions += 'monitor' }

$portArguments = @()
if (-not [string]::IsNullOrWhiteSpace($Port)) {
    $portArguments = @('-p', $Port)
}

& idf.py @baseArguments @portArguments @actions
exit $LASTEXITCODE

