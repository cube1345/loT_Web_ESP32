param(
    [string]$Port = "COM9",
    [string]$ProjectDir = "E:\EmbeddedDevelopment\WorkSpace\ESPWROOM32\HTTP_Demo",
    [string]$IdfPath = "E:\EmbeddedDevelopment\SoftWare\Espressif\frameworks\esp-idf-v5.5.1"
)

function Write-Info { param([string]$msg) Write-Host "[INFO] $msg" -ForegroundColor Cyan }
function Write-Warn { param([string]$msg) Write-Host "[WARN] $msg" -ForegroundColor Yellow }
function Write-Err  { param([string]$msg) Write-Host "[ERROR] $msg" -ForegroundColor Red }

$env:IDF_PATH = $IdfPath
$exportScript = Join-Path $IdfPath 'export.ps1'
$idfPy = Join-Path $IdfPath 'tools\idf.py'

# Try to load ESP-IDF environment
if (Test-Path $exportScript) {
    try {
        . $exportScript
        Write-Info "ESP-IDF environment loaded from: $exportScript"
    }
    catch {
        Write-Warn "Failed to source export.ps1: $($_.Exception.Message)"
    }
} else {
    Write-Warn "export.ps1 not found at $exportScript"
}

# Navigate to project directory
if (-not (Test-Path $ProjectDir)) {
    Write-Err "Project directory not found: $ProjectDir"
    exit 1
}

Push-Location $ProjectDir
try {
    if ([string]::IsNullOrWhiteSpace($Port)) {
        Write-Warn "Serial port not specified. Use -Port COMx (e.g., COM9)."
        Write-Warn "Tip: Check Device Manager and install USB-UART driver (CP210x/CH340) if needed."
    }

    # Determine idf.py availability
    $idfAvailable = $false
    try {
        $null = Get-Command idf.py -ErrorAction Stop
        $idfAvailable = $true
    } catch { $idfAvailable = $false }

    if ($idfAvailable) {
        Write-Info "Using idf.py from PATH"
        idf.py build
        if ($LASTEXITCODE -ne 0) { Write-Warn "idf.py build failed with exit code $LASTEXITCODE" }
        idf.py -p $Port flash monitor
    }
    else {
        # Fallback to python running tools\idf.py
        $pythonCmd = $null
        try { $pythonCmd = (Get-Command python -ErrorAction Stop).Source } catch { }

        if (-not (Test-Path $idfPy)) {
            Write-Err "Cannot find idf.py at: $idfPy. Check -IdfPath or ESP-IDF installation."
            exit 1
        }
        if (-not $pythonCmd) {
            Write-Err "Python not found in PATH. Install Python and ensure 'python' is available."
            exit 1
        }

        Write-Info "Using Python fallback: $pythonCmd $idfPy"
        & $pythonCmd $idfPy build
        if ($LASTEXITCODE -ne 0) { Write-Warn "python idf.py build failed with exit code $LASTEXITCODE" }
        & $pythonCmd $idfPy -p $Port flash monitor
    }
}
finally {
    Pop-Location
}
