# Clang build script for unit tests
# PowerShell script to build and run unit tests with Clang

param(
    [switch]$Clean,
    [switch]$Test,
    [switch]$Verbose
)

$BuildDir = "build"

# Clean build directory if requested
if ($Clean) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) {
        Remove-Item $BuildDir -Recurse -Force
    }
}

# Create build directory if it doesn't exist
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Change to build directory
Push-Location $BuildDir

try {
    # Configure with CMake using Clang
    Write-Host "Configuring with CMake (Clang compiler)..." -ForegroundColor Green
    $configArgs = @(
        "-DCMAKE_C_COMPILER=clang",
        "-DCMAKE_CXX_COMPILER=clang++",
        "-DCMAKE_BUILD_TYPE=Release",
        "-G", "MinGW Makefiles",
        ".."
    )

    if ($Verbose) {
        $configArgs += "--verbose"
    }

    & cmake @configArgs
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }

    # Build the project
    Write-Host "Building project..." -ForegroundColor Green
    $buildArgs = @("--build", ".")

    if ($Verbose) {
        $buildArgs += "--verbose"
    }

    & cmake @buildArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }

    # Run tests if requested
    if ($Test) {
        Write-Host "Running tests..." -ForegroundColor Green
        & ctest
        if ($LASTEXITCODE -ne 0) {
            throw "Tests failed"
        }

        Write-Host "Running test executable directly for detailed output..." -ForegroundColor Cyan
        & .\UnitTest01.exe
    }

    Write-Host "Build completed successfully!" -ForegroundColor Green
}
catch {
    Write-Host "Error: $_" -ForegroundColor Red
    exit 1
}
finally {
    # Return to original directory
    Pop-Location
}
