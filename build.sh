
libs="-lopengl32 -lgdi32 -lOle32 -lWinmm"
warnings="-Wno-incompatible-pointer-types -Wno-write-strings"
general="-shared -std=c99 -I../core -g"
gcc ld54.c ../core/audio.cpp -o ld54.dll $general $libs $warnings