
libs="-lopengl32 -lgdi32 -lOle32"
warnings="-Wno-incompatible-pointer-types -Wno-write-strings"
general="-std=c99 -I../core"
gcc ld54.c ../core/audio.cpp -o ld54.exe $general $libs $warnings -O2