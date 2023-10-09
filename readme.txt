
This is the version of the game submitted to Ludum Dare 54 Jam.

There are a few remaining issues, the biggest of which is that the WASAPI API fails to initialise on some systems.
You can still play the game, however, just without any audio.

We got the collisions and physics working pretty well, but it could be made more robust, with smaller steps to make it more accurate.
If we had more time, we would have liked to do some kind of level editor instead of relying on the RNG to create nice levels. That worked well for the time we had, but given the choice, we would create the levels to make them fair and progress nicely.

The version of my core library used for the submission is included in the lib folder.
As I continually update the core library, this ensures this version of the game will always compile.
I rushed this version of the library out for Ludum Dare and cut lots of corners, so please don't use this as a reference for how to do things well.

As long as you have MinGW or equivalent installed,

Run this for a debug build:
sh build.sh

Run this for a release build:
sh build_release.sh

