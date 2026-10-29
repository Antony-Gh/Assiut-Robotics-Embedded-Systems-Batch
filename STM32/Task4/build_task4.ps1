$gcc = 'C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740\tools\bin'
$make = 'C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.make.win32_2.2.200.202604021615\tools\bin'
$env:PATH = "$gcc;$make;$env:PATH"

$projectDir = Join-Path $PSScriptRoot "STM32F103C8TX\STM32F103C8TX\Debug"
Set-Location $projectDir
Write-Host "Building Task 4 in $projectDir..." -ForegroundColor Cyan
& "$make\make.exe" -j4 all
