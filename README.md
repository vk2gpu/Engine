Engine
======

After working on my old engine Psybrus for several years, I got a little bored and frustrated with how large it was becoming, as well as some of my poor choices along the way that seemed to be a'ok at the time. As my engines are only for my own personal learning experience, plus having a framework I can experiment inside of, I decided to start again.

Some of the goals I intend to keep in mind for this engine:
- Unit tests:
-- Test as much as possible in isolation outside of a full engine instance, save myself time fixing bugs later.
- Quick to build:
-- My previous engine was taking on the scale of several minutes to build. I would prefer to keep the build times sub-minute to improve iteration time. I intend to remain dilligent here.
- Be more selective with 3rd party libraries:
-- As much as I love using code others have wrote to save myself time, sometimes dependencies can be pretty large. All dependencies will be committed straight to this repo and built as part of the engine solution, so they need to build quickly. Single header/one or two file libraries that do exactly what I need will always be preferenced to larger extendable frameworks which offer a much larger range of features.
- Hot reloadable C++ support:
-- This builds on the previous point. In scenarios such as game jams it'd be very convenient to change code as it runs. Should also have the ability to statically link against the final build.
- Very selective STL usage:
-- STL headers weigh heavily on build times, and STL containers can be very slow in debug builds. Prefer C standard library and platform libraries where appropriate.
- High performance debug build:
-- Previous engine wasn't too bad, but could be improved upon.
- Don't go as wide on platform support...yet:
-- Psybrus supported Windows, Linux, OSX, Android, and Web. On top of this it supported from GL ES 2.0, and GL 3.3 up to GL 4.3. This is a lot to maintain for one person. My focus will be on Windows, and DX11+ level graphics hardware.
