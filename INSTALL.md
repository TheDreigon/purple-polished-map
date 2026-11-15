# Install Guide

## Windows

### Install the Purple Polished Map release

There are no downloadable releases yet. Until there are, build Purple Polished Map from source as described below.

Once releases are available:

1. Download **install.bat** and **purple-polished-map.exe** from [the latest release](https://github.com/TheDreigon/purple-polished-map/releases).
2. Hold down Shift, right-click **install.bat**, and click "**Run as administrator**". This will install Purple Polished Map to your account, create a shortcut on your Desktop, and make .blk files open with it.
3. Now you can delete the two downloaded files.
4. Double-click **Purple Polished Map** on your Desktop to open it. If you get an error that "VCRUNTIME140.dll was not found", you need to install the [Microsoft Visual C++ Redistributable](https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist) for x86.

### Build Purple Polished Map from source

You will need [Microsoft Visual Studio 2022](https://visualstudio.microsoft.com/vs/); the Community edition is free.

1. On GitHub, click the green "**Code**" button and click "**Download ZIP**".
2. Unzip the downloaded file. This will create the source folder.
3. Navigate to the source folder in Explorer.
4. Download fltk-1.4.5-source.tar.bz2 or fltk-1.4.5-source.tar.gz from [**fltk.org**](https://www.fltk.org/software.php) to a new **lib** subfolder of the source folder.
5. Extract fltk-1.4.5-source.tar (you may need a program such as [7-Zip](https://www.7-zip.org/)). This will create the lib\\**fltk-1.4.5** folder.
6. Open Visual Studio, select **Open a local folder**, and open the lib\fltk-1.4.5 folder. This will automatically generate the CMake project with a configuration named **x64-Debug** by default.
7. From the Configuration dropdown, select **Manage Configurations...**, click the green plus sign to add a new configuration, and select **x86-Release** from the list. Set the **Configuration type** to **Release** and set the **Toolset** to **msvc_x86_x64**. In the list of CMake variables, uncheck the **FLTK_GRAPHICS_GDIPLUS** option and set the **FLTK_ABI_VERSION** option to **10405**.
8. Set the active Configuration to **x86-Release**.
9. In the **Solution Explorer**, switch to the **CMake Targets View**, right-click on **fltk_images**, and select **Build fltk_images**. This will also build the other required libraries: fltk, fltk_jpeg, fltk_png, and fltk_z.
10. Move all the .lib files from lib\fltk-1.4.5\out\build\x86-Release\lib\\\*.lib up to lib\\\*.lib.
11. Copy the lib\fltk-1.4.5\\**FL** folder to a new include\\**FL** folder. Also copy lib\fltk-1.4.5\out\build\x86-Release\FL\fl_config.h into include\FL.
12. Open ide\polished-map.sln in Visual Studio 2022.
13. If the Solution Configuration dropdown on the toolbar says Debug, set it to **Release**.
14. Go to **Build → Build Solution** or press F7 to build the project. This will create bin\Release\\**purple-polished-map.exe**.
15. Hold down Shift, right-click **install.bat**, and click "**Run as administrator**". This will install Purple Polished Map to your account and create a shortcut on your Desktop.

### Build Purple Polished Map from the command line

If you only have the [Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022), or would rather not click through the IDE, **build-windows.bat** does steps 4 to 14 above for you. You need [Git](https://git-scm.com/) and the "Desktop development with C++" workload, including "C++ CMake tools for Windows".

1. Open a Command Prompt in the folder that contains build-windows.bat.
2. Run `build-windows.bat`. The first run downloads FLTK 1.4.5 to lib\fltk-1.4.5 and builds it (x86 Release, FLTK_GRAPHICS_GDIPLUS off, FLTK_ABI_VERSION 10405); later runs only rebuild the program. This will create bin\Release\\**purple-polished-map.exe**.


## Linux

### Install dependencies

You need at least g++ 7 for C++11 `<regex>` and C++17 `<string_view>` support.

CMake (version 3.15 or later) is required for building FLTK 1.4.

#### Ubuntu/Debian

Run the following commands:

```bash
sudo apt install make g++ git autoconf cmake
sudo apt install zlib1g-dev libpng-dev libxpm-dev libx11-dev libxft-dev libxinerama-dev libfontconfig1-dev x11proto-xext-dev libxrender-dev libxfixes-dev libcairo2-dev libpango1.0-dev
```

#### Fedora

Run the following commands:

```bash
sudo dnf install make g++ git autoconf cmake
sudo dnf install zlib-devel libpng-devel libXpm-devel libX11-devel libXft-devel libXinerama-devel fontconfig-devel libXext-devel libXrender-devel libXfixes-devel libcairo2-devel libpango1.0-devel
```

### Install and build Purple Polished Map

Run the following commands:

```bash
# Clone Purple Polished Map
git clone --branch purple https://github.com/TheDreigon/purple-polished-map.git
cd purple-polished-map

# Build FLTK 1.4.5
git clone --branch release-1.4.5 --depth 1 https://github.com/fltk/fltk.git lib/fltk
pushd lib/fltk
cmake -B build \
	-D CMAKE_INSTALL_PREFIX="$(realpath "$PWD/../..")" \
	-D CMAKE_BUILD_TYPE=Release \
	-D FLTK_ABI_VERSION=10405 \
	-D FLTK_BUILD_GL=0 \
	-D FLTK_BUILD_TEST=0 \
	-D FLTK_GRAPHICS_CAIRO=1 \
	-D FLTK_BACKEND_WAYLAND=0 \
	-D FLTK_USE_SYSTEM_LIBPNG=0 \
	-D FLTK_USE_SYSTEM_ZLIB=0
cmake --build build
cmake --install build
popd

# Build Purple Polished Map
make

# Install Purple Polished Map
# (tested on Ubuntu and Ubuntu derivatives only; it just copies bin/purple-polished-map
#  and res/app.xpm to system directories)
sudo make install
```
