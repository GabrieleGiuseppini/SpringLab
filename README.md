# SpringLab
UI and framework to experiment with two-dimensional mass-spring networks in C++.

# Overview
This project is a laboratory for experimenting with different mathematical formulations and different implementations of mass-spring networks.

The project is a spin-off from [FloatingSandbox](https://github.com/GabrieleGiuseppini/Floating-Sandbox), a 2D physics simulator. This project was born to find a better alternative to the naive Euclidean implementation of the mass-spring network in the simulator.

# Building the Project
I build this project with Visual Studio 2019 (thus full C++ 17 support).
I tried to do my best to craft the CMake files in a platform-independent way, but I'm working on this exclusively in Visual Studio, hence I'm sure some unportable features have slipped in. Feel free to send pull requests for CMake edits for other platforms.

In order to build the game, you will need the following dependencies:
- <a href="https://www.wxwidgets.org/">WxWidgets</a> (cross-platform GUI library)*
- <a href="http://openil.sourceforge.net/">DevIL</a> (cross-platform image manipulation library)*
- <a href="https://github.com/kazuho/picojson">picojson</a> (header-only JSON parser and serializer)
- <a href="https://github.com/google/benchmark">Google Benchmark</a>
- <a href="https://github.com/google/googletest/">Google Test</a> (I'm building out of the _release-1.10.0_ tag)

Dependencies marked with * may be statically linked by using the `MSVC_USE_STATIC_LINKING` option.

A custom `UserSettings.cmake` may be used in order to configure the locations of all dependencies. If you want to use it, copy the `UserSettings.example.cmake` to `UserSettings.cmake` and adapt it to your setup. In case you do not want to use this file, you can use the example to get an overview of all CMake variables you might need to use to configure the dependencies.