@for %%I IN (*.vert; *.tesc; *.tese; *.geom; *.frag; *.comp) DO (%VULKAN_SDK%/Bin32/glslangValidator.exe -V "%%I" -o "%%~nI%%~xI.spv")
pause

REM thanks to some guy on the internet who pointed out that the files will
REM get over written, becuase the validator spits out filenames based on the
REM shader stage, heh