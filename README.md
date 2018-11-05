# libmcmap
C Library for Reading, Writing, Editing, and Creating Minecraft Maps

Originally published on the [Minecraft Forum](https://www.minecraftforum.net/forums/mapping-and-modding-java-edition/minecraft-tools/1266002-libmcmap-c-library-for-map-reading-writing-editing). The precursor to this project was discussed on [Flippeh's cNBT topic](https://www.minecraftforum.net/forums/mapping-and-modding-java-edition/minecraft-tools/1260544-cnbt-a-low-level-nbt-manipulation-library?page=2#c25). Since then it became a much bigger project, and eventually I got a request for the sourcecode.

Inside the repo are two projects; `libnbt` is a dependency of `libmcmap`. (This does not include the code I wrote for generating a POV-Ray scene from the map data.)

Libmcmap & libnbt compile natively on Mac OSX and Linux, and on Windows using [MinGW](http://mingw.org/).

# Screenshots

![ASCII printout of NBT data](screenshots/20140515a-screenshot.png)

https://twitter.com/petermarkley/status/373872343589744640


![Boring a hole through an existing world](screenshots/20140515b-screenshot.png)

https://twitter.com/petermarkley/status/386722644088741888


![Creating a world from scratch](screenshots/20140515c-screenshot.png)

https://twitter.com/petermarkley/status/388499118584115200

# Building & Installing

Each of the two projects must be compiled with `make`, using an argument for your environment:

```
$ cd libnbt/
$ make <osx | linux | mingw>
```

This will produce the corresponding library file:
* `libnbt.so` for Linux
* `libnbt.dylib` for macOS
* `libnbt.dll` for Windows.

Taking Linux as an example, this and the header file should be copied to somewhere useful on your system. Replace `usr/local/lib/` and `/usr/local/include/` with the locations of your choice. (See "Using with an Application" below for how this decision will affect you.)

```
$ sudo cp libnbt.so /usr/local/lib/
$ sudo cp nbt.h /usr/local/include/
```

Repeat the process for `libmcmap`:

```
$ cd ../libmcmap/
$ make linux
$ sudo cp libmcmap.so /usr/local/lib/
$ sudo cp mcmap.h /usr/local/include/
```

# Using with an Application

In order to use the library, your application needs all the Libmcmap symbol definitions. Add this to your source code:

```
#include "mcmap.h"
#include "nbt.h"
```

Then use the following compiler arguments. (These examples use the same locations from the steps above. Adjust as necessary.)
* Tell the preprocessor where to find the header files: `-I/usr/local/include/`.
* Tell the compiler about the binaries and where to find them: `-lmcmap -lnbt -L/usr/local/lib/`

Once compiled, if your application won't run you may have put the libraries outside the default search path for the dynamic linker. If so, you can repeat their location in a temporary environment variable. (How to add it permanently will vary by system.)

```
$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/
```

See the provided test programs for an example implementation:

```
$ mkdir maps
$ ./test  # Generate a test map!
$ ./shownbt "maps/My Super Test/level.dat"  # Dump its level.dat file as ASCII to stdout
Compound:  
 L-- Compound "Data":  
      +-- Int "version":  19133
      +-- Long "RandomSeed":  1541389172
      +-- String "LevelName":  "My Super Test"
      +-- Long "LastPlayed":  1541389173000
      +-- String "generatorName":  "flat"
      +-- String "generatorOptions":  "0"
      +-- Byte "MapFeatures":  00
      +-- Int "GameType":  1
      +-- Byte "hardcore":  00
      +-- Byte "allowCommands":  01
      +-- Int "SpawnX":  0
      +-- Int "SpawnY":  41
      +-- Int "SpawnZ":  0
      L-- Compound "GameRules":  
           +-- String "commandBlockOutput":  "true"
           +-- String "doDaylightCycle":  "false"
           +-- String "doFireTick":  "true"
           +-- String "doMobLoot":  "true"
           +-- String "doMobSpawning":  "true"
           +-- String "doTileDrops":  "true"
           +-- String "keepInventory":  "false"
           +-- String "mobGriefing":  "false"
           L-- String "naturalRegeneration":  "true"
```
