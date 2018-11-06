@ECHO OFF >NUL

if "%~1" == "" (set WORK_PATH=%cd%) else (set WORK_PATH=%~1)

echo.
echo "Vulkan SDK path: "%VULKAN_SDK%
echo "Compiling shaders in: "%WORK_PATH%
echo.

SETLOCAL ENABLEDELAYEDEXPANSION
for /R %WORK_PATH% %%f in (*.vert,*.frag) do (
	set shaderFilePath=%%~ff
	set check=!shaderFilePath:thirdparty=!
	if  !check!==!shaderFilePath! (
		%VULKAN_SDK%/Bin32/glslc.exe -c -mfmt=bin %%f -o %%f.spv
		echo Compiled: %%f.spv
	)
)
