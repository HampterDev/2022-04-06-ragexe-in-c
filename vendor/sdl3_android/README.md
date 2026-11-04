# SDL3 Android activity

`java/org/libsdl/app/*.java` are SDL's Java sources for Android
(`android-project/app/src/main/java/org/libsdl/app`), unchanged, from the SDL commit
`829a65d769d935c4852f8159e964312c0957260a` that `cmake/SDL3.cmake` builds (SDL 3.4.18). The zlib
licence is in `LICENSE.txt`.

The activity checks that the Java side and `libSDL3.so` are the same version, so these files are
replaced together with the pinned commit. The Android app (`android/`) compiles them next to its
own `ClientActivity`, which extends `SDLActivity`.
