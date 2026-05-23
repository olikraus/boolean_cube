param(
  [string]$MsysRoot = "C:\msys64",
  [switch]$Clean,
  [switch]$SkipBuild,
  [switch]$SkipTest,
  [string]$LogDir = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ProjectRoot = $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($LogDir)) {
  $LogDir = Join-Path $ProjectRoot "test-logs"
}

if (-not (Test-Path $ProjectRoot)) {
  throw "Project root does not exist: $ProjectRoot"
}

if (-not (Test-Path $LogDir)) {
  New-Item -ItemType Directory -Path $LogDir | Out-Null
}

$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$LogFile = Join-Path $LogDir ("bcselftest-" + $timestamp + ".log")
$LatestLog = Join-Path $LogDir "latest.log"

function Write-Log {
  param([string]$Message)
  $line = "[{0}] {1}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss"), $Message
  $line | Tee-Object -FilePath $LogFile -Append | Out-Host
}

function Invoke-External {
  param(
    [string]$File,
    [string[]]$Arguments,
    [string]$StepName
  )

  Write-Log ("STEP: " + $StepName)
  Write-Log ("CMD: " + $File + " " + ($Arguments -join " "))

  & $File @Arguments 2>&1 | Tee-Object -FilePath $LogFile -Append | Out-Host
  $exitCode = $LASTEXITCODE
  if ($exitCode -ne 0) {
    throw ("Command failed with exit code {0}: {1} {2}" -f $exitCode, $File, ($Arguments -join " "))
  }
}

$mingwBin = Join-Path $MsysRoot "mingw64\bin"
$usrBin = Join-Path $MsysRoot "usr\bin"

if (-not (Test-Path $mingwBin)) {
  throw "MSYS2 MinGW bin directory not found: $mingwBin"
}
if (-not (Test-Path $usrBin)) {
  throw "MSYS2 usr bin directory not found: $usrBin"
}

$originalPath = $env:Path
$env:Path = "$mingwBin;$usrBin;$originalPath"

try {
  Write-Log "boolean_cube selftest runner started"
  Write-Log ("ProjectRoot: " + $ProjectRoot)
  Write-Log ("MsysRoot: " + $MsysRoot)
  Write-Log ("LogFile: " + $LogFile)

  $gccCmd = Get-Command gcc -ErrorAction Stop
  $makeCmd = Get-Command make -ErrorAction Stop
  Write-Log ("Using gcc: " + $gccCmd.Source)
  Write-Log ("Using make: " + $makeCmd.Source)

  if ($Clean) {
    Invoke-External -File $makeCmd.Source -Arguments @("-C", (Join-Path $ProjectRoot "bcc"), "clean") -StepName "Clean bcc target"
  }

  if (-not $SkipBuild) {
    Invoke-External -File $makeCmd.Source -Arguments @("-C", (Join-Path $ProjectRoot "bcc")) -StepName "Build bcc target"
  }

  if (-not $SkipTest) {
    $exeCandidates = @(
      (Join-Path $ProjectRoot "bcc\bcc.exe"),
      (Join-Path $ProjectRoot "bcc\bcc")
    )

    $bccExe = $null
    foreach ($candidate in $exeCandidates) {
      if (Test-Path $candidate) {
        $bccExe = $candidate
        break
      }
    }

    if ($null -eq $bccExe) {
      throw "Could not find bcc executable after build. Checked: $($exeCandidates -join ', ')"
    }

    Invoke-External -File $bccExe -Arguments @("-test") -StepName "Run bcc selftests"
  }

  Write-Log "All requested steps completed successfully"
}
finally {
  $env:Path = $originalPath
  if (Test-Path $LogFile) {
    Copy-Item -Path $LogFile -Destination $LatestLog -Force
  }
}
