"scons" p=windows target=editor module_mono_enabled=yes -j 64
 
bin/godot.windows.editor.dev.x86_64.mono.exe --generate-mono-glue modules/mono/glue
python "./modules/mono/build_scripts/build_assemblies.py" --godot-output-dir=./bin --godot-platform=windows

scons p=windows target=editor profile=./custom.py vsproj="yes" module_mono_enabled=yes module_arkit_enabled=no module_mobile_vr_enabled=no module_webxr_enabled=no debug_symbols="yes" -j 128

C:\ws\Godot\godot>scons p=windows target=editor profile=./custom.py vsproj="yes" module_mono_enabled=yes module_arkit_enabled=no module_mobile_vr_enabled=no module_webxr_enabled=no debug_symbols="yes" use_simd=avx2 opengl3=no -j 128