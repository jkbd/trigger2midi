# Trigger2MIDI

Convert electronic drum trigger signals to MIDI events.

## Compile and Install
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

## Usage

You need a piezo contact microphon on a drum head, a DI-box and a
sound card with analog amplified inputs. On the software side you need a
LV2-plug-in host.

Connect the piezo to the DI-box and connect the DI-box to the
input. Adjust the gain of the input so the signal does not clip.  Run
the Trigger2MIDI Plug-in, for example with the `jalv` host:

```
$ jalv https://github.com/jkbd/trigger2midi
> dyn = 42
dyn = 42.000000
> mask = 17
mask = 17.000000
```

Connect ports in JACK if needed.

First adjust the "Dynamic Range" parameter (in decibel), so other
noises do not trigger events. Then adjust the "Mask Retrigger"
parameter, so single hits do not trigger multiple Note On
events. Rubber pads are likely to need a small value. Undampened mesh
heads may need a large value. Last set the MIDI note number to your
requirements.

## Notes

* The control voltage output is meant for debugging

* Right now the plug-in only sends MIDI Note On events, not Note Off.