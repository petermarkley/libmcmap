# libmcmap
C Library for Reading, Writing, Editing, and Creating Minecraft Maps

Originally published on the [Minecraft Forum](https://www.minecraftforum.net/forums/mapping-and-modding-java-edition/minecraft-tools/1266002-libmcmap-c-library-for-map-reading-writing-editing). The precursor to this project was discussed on [Flippeh's cNBT topic](https://www.minecraftforum.net/forums/mapping-and-modding-java-edition/minecraft-tools/1260544-cnbt-a-low-level-nbt-manipulation-library?page=2#c25). Since then it became a much bigger project, and eventually I got a request for the sourcecode.

Inside the repo are two projects; `libnbt` is a dependency of `libmcmap`. (This does not include the code I wrote for generating a POV-Ray scene from the map data.)

Libmcmap & libnbt compile natively on Mac OSX and Linux, and on Windows using [MinGW](http://mingw.org/).

# Development & Testing Screenshots

![ASCII printout of NBT data](screenshots/20140515a-screenshot.png)

https://twitter.com/petermarkley/status/373872343589744640


![Boring a hole through an existing world](screenshots/20140515b-screenshot.png)

https://twitter.com/petermarkley/status/386722644088741888


![Creating a world from scratch](screenshots/20140515c-screenshot.png)

https://twitter.com/petermarkley/status/388499118584115200
