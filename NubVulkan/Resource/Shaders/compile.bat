@for %%i IN (*.vert; *.tesc; *.tese; *.geom; *.frag; *.comp) DO (C:/VulkanSDK/1.0.26.0/Bin32/glslangValidator.exe -V "%%i")
pause