@ECHO OFF >NUL

if "%~1" == "" (set WORK_PATH=%cd%) else (set WORK_PATH=%~1)

echo.
echo "Vulkan SDK path: "%VULKAN_SDK%
echo "Compiling shaders in: "%WORK_PATH%
echo.
for /R %WORK_PATH% %%f in (*.vert,*.frag) do %VULKAN_SDK%/Bin32/glslangValidator.exe -V %%~ff -o %%~ff.spv
