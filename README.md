# Matter Bridge

## Updating Matter library
Rough workflow for updating X-CUBE-MATTER is parsing example `.project` file to get most of the required c/cpp files (they still need to be manually edited and fixed), and looking into `.cproject` for include directories/defines. Then editing the `CMakeLists.txt` in the root directory and also in the `Library/Drivers`, `Library/Middlewares` and `Library/Utilities` subdirectories.