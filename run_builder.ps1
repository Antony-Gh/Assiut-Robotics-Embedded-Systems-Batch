$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$BuilderExe = Join-Path $ScriptDir "builder.exe"

function Log-Error { param($msg) Write-Host "[ERROR] $msg" -ForegroundColor Red }
function Log-Info { param($msg) Write-Host "[INFO] $msg" -ForegroundColor Cyan }
function Log-Success { param($msg) Write-Host "[SUCCESS] $msg" -ForegroundColor Green }
function Log-Run { param($msg) Write-Host "[RUNNING] $msg" -ForegroundColor Yellow }

if (-not (Test-Path $BuilderExe)) {
    Log-Error "builder.exe not found at $BuilderExe"
    Log-Info "Please compile CBuilder first."
    pause
    exit 1
}

while ($true) {
    Clear-Host
    Write-Host "=======================================" -ForegroundColor Cyan
    Write-Host "         CBuilder Interactive          " -ForegroundColor Cyan
    Write-Host "           By: Anthony Gaius           " -ForegroundColor Cyan
    Write-Host "=======================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Please select an action:"
    Write-Host "1. " -ForegroundColor Green -NoNewline; Write-Host "Compile and Run a single C file"
    Write-Host "2. " -ForegroundColor Green -NoNewline; Write-Host "Build a project directory"
    Write-Host "3. " -ForegroundColor Green -NoNewline; Write-Host "Create a new C project"
    Write-Host "4. " -ForegroundColor Green -NoNewline; Write-Host "Exit"
    Write-Host ""

    $Action = Read-Host "Enter your choice (1-4)"

    switch ($Action) {
        "1" {
            Write-Host ""
            $FilePath = Read-Host "Enter the path to the .c file"
            if ([string]::IsNullOrWhiteSpace($FilePath)) {
                Log-Error "Path cannot be empty."
                pause
                continue
            }
            
            # Remove surrounding quotes
            $FilePath = $FilePath -replace '^"|"$',''

            if (-not (Test-Path -LiteralPath $FilePath -PathType Leaf)) {
                Log-Error "File not found: $FilePath"
                pause
                continue
            }
            
            Write-Host ""
            Log-Run "builder.exe compile "$FilePath" --run"
            & $BuilderExe compile $FilePath --run
            
            Write-Host ""
            pause
        }
        
        "2" {
            Write-Host ""
            $ProjPath = Read-Host "Enter the path to the project directory (leave empty for current dir)"
            
            if ([string]::IsNullOrWhiteSpace($ProjPath)) {
                $ProjPath = "."
            }

            # Remove surrounding quotes
            $ProjPath = $ProjPath -replace '^"|"$',''

            if (-not (Test-Path -LiteralPath $ProjPath -PathType Container)) {
                Log-Error "Directory not found: $ProjPath"
                pause
                continue
            }
            
            Write-Host ""
            $RunOpt = Read-Host "Do you want to run the project after building? (Y/N)"
            
            Write-Host ""
            if ($RunOpt -match "^[Yy]") {
                Log-Run "builder.exe run "$ProjPath""
                & $BuilderExe run $ProjPath
            } else {
                Log-Run "builder.exe build "$ProjPath""
                & $BuilderExe build $ProjPath
            }
            
            Write-Host ""
            pause
        }
        
        "3" {
            Write-Host ""
            $ProjName = Read-Host "Enter the name/path for the new project"
            if ([string]::IsNullOrWhiteSpace($ProjName)) {
                Log-Error "Project name cannot be empty."
                pause
                continue
            }

            # Remove surrounding quotes
            $ProjName = $ProjName -replace '^"|"$',''
            
            Write-Host ""
            Log-Run "builder.exe new "$ProjName""
            & $BuilderExe new $ProjName
            
            Write-Host ""
            pause
        }

        "4" {
            exit 0
        }
        
        default {
            Write-Host ""
            Log-Error "Invalid choice. Please try again."
            pause
        }
    }
}
