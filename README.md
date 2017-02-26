Engine
======

After working on my old engine Psybrus for several years, I got a little bored and frustrated with how large it was becoming, as well as some of my poor choices along the way that seemed to be a'ok at the time. As my engines are only for my own personal learning experience, plus having a framework I can experiment inside of, I decided to start again.

Some of the goals I intend to keep in mind for this engine:
- Unit tests:
  - Test as much as possible in isolation outside of a full engine instance, save myself time fixing bugs later.
- Quick to build:
  - My previous engine was taking on the scale of several minutes to build. I would prefer to keep the build times sub-minute to improve iteration time. I intend to remain dilligent here.
- Be more selective with 3rd party libraries:
  - As much as I love using code others have wrote to save myself time, sometimes dependencies can be pretty large. All dependencies will be committed straight to this repo and built as part of the engine solution, so they need to build quickly. Single header/one or two file libraries that do exactly what I need will always be preferenced to larger extendable frameworks which offer a much larger range of features.
- Hot reloadable C++ support:
  - This builds on the previous point. In scenarios such as game jams it'd be very convenient to change code as it runs. Should also have the ability to statically link against the final build.
- Very selective STL usage:
  - STL headers weigh heavily on build times, and STL containers can be very slow in debug builds. Prefer C standard library, platform libraries, or custom code where appropriate.
- High performance debug build:
  - Previous engine wasn't too bad, but could be improved upon.
- Don't go as wide on platform support...yet:
  - Psybrus supported Windows, Linux, OSX, Android, and Web. On top of this it supported from GL ES 2.0, and GL 3.3 up to GL 4.3. This is a lot to maintain for one person. My focus will be on Windows, and DX11+ level graphics hardware.


Notes about style choices:
- Auto-formatting:
  - I love auto-formatting. Means I spend less time messing with how I want the code to look, and more time coding. Much more valuable on a team, particularly if dealing with a lot of merging, so I figured it's good practice for me to use at home too. See _clang-format in the src folder for how I've set it up.
- Container classes:
  - These don't follow the same naming conventions as the rest of the code base. This was intentional to keep them as close to the STL containers as possible, allowing me to more easily switch later back to STL/EASTL/etc if I desire.
- Namespaces:
  - I've never really liked them much, however my previous codebase used cryptic prefixes on all classes. I figured 1 deep nesting of namespaces is a bit cleaner at avoiding type name collision, with the benefit of being able to use "using namespace" within a file or function scope if I desire.
- Private source + headers.
  - Not all of the code should be visible outside of the library. This provides an easy way to check if code should be used outside of a library.
- Multiple classes in single headers/source files.
  - I've usually kept to "One class, one cpp, one header" in the past, but realistically it just isn't always worth doing. For example, concurrency.h. In the past I'd have had a separate thread, mutex, atomic, etc header. The logic for grouping it all here in this case is if you need one of those constructs, you probably need another. Fewer source files where appropriate should also improve build times, as there are fewer compilation units overall.
- American English:
  - As most APIs use American English for spelling rather than British English, in the name of consistency I will be trying to stick with American English spelling (synchronize, color, etc).


References used during development:
- Bounded MPMC queue
  - http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
- Parallelizing the Naughty Dog Engine Using Fibers
  - http://www.gdcvault.com/play/1022186/Parallelizing-the-Naughty-Dog-Engine
- Hashing algorithms.
  - https://github.com/B-Con/crypto-algorithms