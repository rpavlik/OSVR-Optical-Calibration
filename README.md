# OSVR-Optical-Calibration
> Maintained at <https://github.com/OSVR/OSVR-Optical-Calibration>
>
> For details, see <http://osvr.github.io>
>
> For support, see <http://support.osvr.com>

**App under development.**

## License and Vendored Projects

This project: Licensed under the Apache License, Version 2.0.

- `/cmake` - Git subtree from <https://github.com/rpavlik/cmake-modules> used at compile-time only. Primarily BSL 1.0, some items under the license used by CMake (BSD-style)
- `/vendor/libSDL2pp` - Git submodule, accessed from <https://github.com/OSVR/libSDL2pp> but with original upstream source at <https://github.com/AMDmi3/libSDL2pp>. zlib license, same as SDL2. [COPYING.txt file](https://github.com/OSVR/libSDL2pp/blob/master/COPYING.txt)
- `/vendor/glm` - Git submodule, accessed from <https://github.com/OSVR/glm> but with original upstream source at <https://github.com/g-truc/glm>. Used under the MIT license. [COPYING.txt file](https://github.com/g-truc/glm/blob/master/copying.txt)

### Dependencies

- [OSVR-Core](https://github.com/OSVR/OSVR-Core) - Apache License, Version 2.0.
- SDL2 - zlib license.