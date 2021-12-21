# cctop

## Building

### MacOS

This program is built with cmake.  We use the cmake from homebrew, setting the 
path in the CLion IDE settings.  We also installed ncurses using homebrew and 
set the paths in CMakeLists.txt to use that.  The reason is that the curses
that comes with XCode and MacOS is very old and doesn't support wide characters.

What we really want is libncursesw.dylib (w on the end means wide character support).

## WIDE CHARACTERS (UTF_16)
```c++
    //    0x2581 ▁
    //    0x2582 ▂
    //    0x2583 ▃
    //    0x2584 ▄
    //    0x2585 ▅
    //    0x2586 ▆
    //    0x2587 ▇
    //    0x2588 █

```
## Useful links

### CMAKE
* https://stackoverflow.com/questions/9160335/os-specific-instructions-in-cmake-how-to
* https://stackoverflow.com/questions/26704629/setting-up-curl-library-path-in-cmake
* https://cmake.org/cmake/help/git-stage/guide/using-dependencies/index.html

### CURL
* https://curl.se/libcurl/c/url2file.html
* https://curl.se/libcurl/c/CURLOPT_BUFFERSIZE.html

### PROCESSES ETC.
* https://github.com/giampaolo/psutil/blob/ec1d35e41c288248818388830f0e4f98536b93e4/psutil/_psutil_osx.c#L739
* https://github.com/osquery/osquery/blob/master/osquery/tables/system/darwin/cpu_time.cpp
* https://cpp.hotexamples.com/examples/-/-/proc_pidinfo/cpp-proc_pidinfo-function-examples.html
* https://developer.apple.com/forums/thread/30522
* https://opensource.apple.com/source/adv_cmds/adv_cmds-36/ps.tproj/ps.c
* https://opensource.apple.com/source/xnu/xnu-1228/bsd/sys/proc_info.h
* https://coderedirect.com/questions/136685/determine-process-info-programmatically-in-darwin-osx
* https://libproc.readthedocs.io/en/latest/readme.html#higher-level-apis
* https://ops.tips/blog/macos-pid-absolute-path-and-procfs-exploration/#the-libproc-library-in-macos
* https://stackoverflow.com/questions/31469355/how-to-calculate-app-and-cache-memory-like-activity-monitor-in-objective-c-in-ma
* 

