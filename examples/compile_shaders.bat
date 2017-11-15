@ECHO OFF >NUL

for /R %cd% %%f in (*.vert,*.frag) do %VULKAN_SDK%/Bin32/glslangValidator.exe -V %%~ff -o %%~ff.spv
