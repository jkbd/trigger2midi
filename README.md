# Trigger2MIDI

Convert electronic drum trigger signals to MIDI events. You need a
sound card with analog inputs and a LV2-plug-in host.

**Still under development.**

## Build
```bash
$ mkdir build && cd build
$ cmake .. -D CMAKE_BUILD_TYPE=Release
$ cmake --build .
```

To install, copy the bundle `trigger2midi.lv2` manually to the
[standard locations](http://lv2plug.in/pages/filesystem-hierarchy-standard.html)
or run

```
$ sudo cmake --install .
```

to install it in `$PREFIX/lib/lv2/`. You could change the `$PREFIX` by running

```
$ cmake -D CMAKE_INSTALL_PREFIX=/your/path ..
```

## Run

For example with `jalv`:
```
$ jalv https://github.com/jkbd/trigger2midi
```