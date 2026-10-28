$gcc = 'C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740\tools\bin'
$make = 'C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.make.win32_2.2.200.202604021615\tools\bin'
$env:PATH = "$gcc;$make;$env:PATH"
Set-Location 'e:\Robotics_Team\New_Embedded_batch\Assiut-Robotics-Embedded-Systems-Batch\STM32\Task2\STM32F103C8TX\STM32F103C8TX\Debug'
& "$make\make.exe" -j4 all
