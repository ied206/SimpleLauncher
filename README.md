# SimpleLauncher
Very Simple Executable Launcher for Win32

Designed to require minimum dependency, to work properly in PE environment.

## Usage
Before compile, edit `SimpleLauncher.c`'s `LAUNCH_INI` macro's value to change ini path.  
Default is `PrecCalcPath.ini`, I recommend to adjust this for your need.

Write an executable path into `[LAUNCH_INI]`.  
SimpleLauncher will read file path from `[LAUNCH_INI]` and launch that file.  
Only first line will be processed, so do not try to launch two program at once.

SimpleLauncher supports ANSI, UTF-16 LE, UTF-8 encoding.  
SimpleLauncher can understand environment variable just like cmd shell.

## Compile
Use MinGW-w64 to compile SimpleLauncher.  
Launch `mingw32-make` in SimpleLauncher root directory to build.  
