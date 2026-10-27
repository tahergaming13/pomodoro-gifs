# Third-Party Notices

Pomodoro itself is MIT licensed (see [LICENSE](LICENSE)). It bundles or links
against the following third-party components:

## SDL2 (Simple DirectMedia Layer)

Copyright (c) 1997-2025 Sam Lantinga <slouken@libsdl.org>

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the
use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim
   that you wrote the original software. If you use this software in a
   product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

Full text: https://www.libsdl.org/license.php

## Dear ImGui

Copyright (c) 2014-2025 Omar Cornut

MIT License — full text: https://github.com/ocornut/imgui/blob/master/LICENSE.txt

## stb_image

Copyright (c) 2017 Sean Barrett

Public domain / MIT dual license — full text:
https://github.com/nothings/stb/blob/master/stb_image.h

## MinGW-w64 / WinLibs toolchain (build-time only)

GCC is GPLv3 with the GCC Runtime Library Exception; MinGW-w64 runtime is
public domain / permissive. The build statically links the GCC runtime, which
the Runtime Library Exception permits for proprietary and open-source binaries
alike. No GPL obligation is imposed on the produced `Pomodoro.exe`.

https://winlibs.com/ · https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html

## CMake (build-time only)

BSD 3-Clause — https://cmake.org/licensing/

## GIF assets

`assets/rocket.gif`, `assets/clouds.gif`, `assets/beach.gif` are generated from
scratch by `tools/make_gifs.py` (pure Python + Pillow), so they are original
works released under the same MIT license as the rest of this project.
