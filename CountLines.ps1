<#

Count lines in all sgl_*.c/cpp/h/hpp files under src/ and include/
Example (in Windows powershell):

.\CountLines.ps1

#>

Write-Host ""

$totalLines = 0
$filePatterns = @('*.c', '*.cpp', '*.h', '*.hpp')
$targetDirs = @("src", "include")
$matchingFiles = @()

foreach ($dir in $targetDirs) {
    if (Test-Path $dir) {
        $files = Get-ChildItem -Path ".\$dir\" -Recurse -Include $filePatterns -ErrorAction SilentlyContinue | 
                 Where-Object { $_.Name -like "sgl_*" }
        $matchingFiles += $files
    }
    else {
        Write-Warning "Directory '$dir' not found, skipping"
    }
}

if ($matchingFiles.Count -eq 0) {
    Write-Host "No matching files found!" -ForegroundColor Red
    exit
}

# Count and display lines for each file
foreach ($file in $matchingFiles) {
    $lineCount = (Get-Content $file.FullName | Measure-Object -Line).Lines
    $totalLines += $lineCount
    Write-Host "$($file.FullName.PadRight(70)) : $lineCount"
}

# Display summary
Write-Host "`nSummary:"
Write-Host "Files found: $($matchingFiles.Count)"
Write-Host "Total lines: $totalLines" -ForegroundColor Green
Write-Host ""
