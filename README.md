# Trigger2MIDI

*Super early development state!*
Convert electronic drum trigger signals to MIDI events.

## Build
```bash
$ mkdir build && cd build
$ cmake .. -D CMAKE_BUILD_TYPE=Release
$ cmake --build .
```

To install, copy the bundle `osc.lv2` manually to the [standard
locations](http://lv2plug.in/pages/filesystem-hierarchy-standard.html)
or run

```
$ sudo cmake --install .
```

to install it in `$PREFIX/lib/lv2/`. You could change the `$PREFIX` by running

```
$ cmake -D CMAKE_INSTALL_PREFIX=/your/path ..
```