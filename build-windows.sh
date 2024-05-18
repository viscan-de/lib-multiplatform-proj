#! /bin/bash
rm -rf .libtiff .vcpkg built
git clone https://github.com/microsoft/vcpkg.git ./.vcpkg
cd .vcpkg/
./bootstrap-vcpkg.bat
./vcpkg install tiff:arm64-windows-static
cd ..
cmake -B built -DBUILD_APPS=OFF -DBUILD_PROJSYNC=OFF -DBUILD_SHARED_LIBS=OFF -DTIFF_LIBRARY=./.vcpkg/installed/arm64-windows-static/lib -DTIFF_INCLUDE_DIR=./.vcpkg/installed/arm64-windows-static/include  
cmake --build built --config Release -t proj
cd built/lib/Release
cp ../../.././.vcpkg/installed/arm64-windows-static/lib/*.lib .
rm projcomplete.lib jpeg.lib
lib /OUT:libproj.lib *.lib
