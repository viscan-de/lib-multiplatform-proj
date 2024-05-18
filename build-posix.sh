#! /bin/bash
rm -rf .libtiff
git clone https://github.com/viscan-de/lib-multiplatform-tiff.git ./.libtiff
cd .libtiff/
make platform=posix
cd ..
cmake -B built -DBUILD_APPS=OFF -DBUILD_PROJSYNC=OFF -DBUILD_SHARED_LIBS=OFF -DTIFF_LIBRARY=./.libtiff/posix/dependencies/lib  -DTIFF_INCLUDE_DIR=./.libtiff/posix/dependencies/include
cmake --build built -t proj
cd built/lib
cp ../../.libtiff/posix/dependencies/lib/*.a .
printf "create libprojcomplete.a\naddlib libjpeg.a\naddlib libpng.a\naddlib libtiffxx.a\naddlib libtiff.a\naddlib libproj.a\nsave\nend" > libprojcomplete.mri
ar -M < libprojcomplete.mri
rm libjpeg.a libpng.a libproj.a libtiff.a libtiffxx.a libprojcomplete.mri
mv libprojcomplete.a libproj.a
