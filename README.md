The Powder Toy - February 2025
==========================

Get the latest version [from the Powder Toy website](https://powdertoy.co.uk/Download.html).

To use online features such as saving, you need to [register an account](https://powdertoy.co.uk/Register.html).
You can also visit [the official TPT forum](https://powdertoy.co.uk/Discussions/Categories/Index.html).

Have you ever wanted to blow something up? Or maybe you always dreamt of operating an atomic power plant? Do you have a will to develop your own CPU? The Powder Toy lets you to do all of these, and even more!

The Powder Toy is a free physics sandbox game, which simulates air pressure and velocity, heat, gravity and a countless number of interactions between different substances! The game provides you with various building materials, liquids, gases and electronic components which can be used to construct complex machines, guns, bombs, realistic terrains and almost anything else. You can then mine them and watch cool explosions, add intricate wirings, play with little stickmen or operate your machine. You can browse and play thousands of different saves made by the community or upload your own – we welcome your creations!

There is a Lua API – you can automate your work or even make plugins for the game. The Powder Toy is free and the source code is distributed under the GNU General Public License, so you can modify the game yourself or help with development.

Build instructions
===========================================================================

See the _Powder Toy Development Help_ section [on the main page of the wiki](https://powdertoy.co.uk/Wiki/W/Main_Page.html).

Special Thanks
===========================================================================

* Stanislaw K Skowronek - Designed the original Powder Toy
* Simon Robertshaw - Wrote the website, current server owner
* Skresanov Savely
* Pilihp64
* Catelite
* Victoria Hoyle
* Nathan Cousins
* jacksonmj
* Felix Wallin
* Lieuwe Mosch
* Anthony Boot
* Me4502
* MaksProg
* jacob1
* mniip
* LBPHacker

Libraries and other assets used
===========================================================================

* [bzip2](http://www.bzip.org/)
* [FFTW](http://fftw.org/)
* [JsonCpp](https://github.com/open-source-parsers/jsoncpp)
* [libcurl](https://curl.se/libcurl/)
* [libpng](http://www.libpng.org/pub/png/libpng.html)
* [Lua](https://www.lua.org/)
* [LuaJIT](https://luajit.org/)
* [Mallangche](https://github.com/JammPark/Mallangche)
* [mbedtls](https://www.trustedfirmware.org/projects/mbed-tls/)
* [SDL](https://libsdl.org/)

Instructions
===========================================================================

Click on the elements with the mouse and draw in the field, like in MS Paint. The rest of the game is learning what happens next.

Controls
===========================================================================

| Key                     | Action                                                          |
| ----------------------- | --------------------------------------------------------------- |
| TAB                     | Switch between circle/square/triangle brush                     |
| Space                   | Pause                                                           |
| Q / Esc                 | Quit                                                            |
| Z                       | Zoom                                                            |
| S                       | Save stamp (use with Ctrl when STK2 is out)                     |
| L                       | Load last saved stamp                                           |
| K                       | Stamp library                                                   |
| 0-9                     | Set view mode                                                   |
| P / F2                  | Save screenshot as .png                                         |
| E                       | Bring up element search                                         |
| F                       | Pause and step to next frame                                    |
| G                       | Increase grid size                                              |
| Shift + G               | Decrease grid size                                              |
| H                       | Show/Hide HUD                                                   |
| Ctrl + H / F1           | Show intro text                                                 |
| D / F3                  | Debug mode (use with Ctrl when STK2 is out)                     |
| I                       | Invert Pressure and Velocity map                                |
| W                       | Cycle gravity modes (use with Ctrl when STK2 is out)            |
| Y                       | Cycle air modes                                                 |
| Ctrl + E                | Cycle edge modes                                                |
| B                       | Enter decoration editor menu                                    |
| Ctrl + B                | Toggle decorations on/off                                       |
| N                       | Toggle Newtonian Gravity on/off                                 |
| U                       | Toggle ambient heat on/off                                      |
| Ctrl + I                | Install powder toy, for loading saves/stamps by double clicking |
| Backtick                | Toggle console                                                  |
| =                       | Reset pressure and velocity map                                 |
| Ctrl + =                | Reset Electricity                                               |
| \[                      | Decrease brush size                                             |
| \]                      | Increase brush size                                             |
| Alt + \[                | Decrease brush size by 1                                        |
| Alt + \]                | Increase brush size by 1                                        |
| Ctrl + C/V/X            | Copy/Paste/Cut                                                  |
| Ctrl + Z                | Undo                                                            |
| Ctrl + Y                | Redo                                                            |
| Ctrl + Cursor drag      | Rectangle                                                       |
| Shift + Cursor drag     | Line                                                            |
| Middle click            | Sample element                                                  |
| Alt + Left click        | Sample element                                                  |
| Mouse scroll            | Change brush size                                               |
| Ctrl + Mouse scroll     | Change vertical brush size                                      |
| Shift + Mouse scroll    | Change horizontal brush size                                    |
| Shift + R               | Horizontal mirror for selected area when pasting stamps         |
| Ctrl + Shift + R        | Vertical mirror for selected area when pasting stamps           |
| R                       | Rotate selected area counterclockwise when pasting stamps       |
| F11                     | Toggle fullscreen                                               |

Command Line
---------------------------------------------------------------------------

| Command               | Description                                      | Example                                     |
| --------------------- | ------------------------------------------------ | --------------------------------------------|
| `scale:SIZE`          | Change window scale factor                       | `scale:2`                                   |
| `kiosk`               | Fullscreen mode                                  |                                             |
| `proxy:SERVER[:PORT]` | Proxy server to use                              | `proxy:wwwcache.lancs.ac.uk:8080`           |
| `open FILE`           | Opens the file as a stamp or game save           |                                             |
| `ddir DIRECTORY`      | Directory used for saving stamps and preferences |                                             |
| `ptsave:SAVEID`       | Open online save, used by ptsave: URLs           | `ptsave:2198`                               |
| `disable-network`     | Disables internet connections                    |                                             |
| `disable-bluescreen`  | Disable bluescreen handler                       |                                             |
| `redirect`            | Redirects output to stdout.txt / stderr.txt      |                                             |
| `console`             | Redirects output to a new console on Windows     |                                             |
| `cafile:CAFILE`       | Set certificate bundle path                      | `cafile:/etc/ssl/certs/ca-certificates.crt` |
| `capath:CAPATH`       | Set certificate directory path                   | `capath:/etc/ssl/certs`                     |

Custom Elements
===========================================================================

This fork includes many new elements for building circuits and having fun!

## Electronic Components

All electronic components use a **wire-type terminal identification** system:
- **PSCN** = Control/Input terminal (positive/signal input)
- **NSCN** = Power/Secondary terminal (negative/secondary input)
- **METL/INWR** = Output terminal

### Power & Signal Sources

| Element | Description |
| ------- | ----------- |
| **VCCS** | Power supply. Voltage scales with cluster size (larger = more power). Use `tmp` to set base voltage. |
| **SGNL** | Signal Generator. Generates waveforms. `tmp`=frequency (1-100), `tmp2`=waveform type (0-6: Square, Sine, Sawtooth, Pulse, Triangle, Ramp-down, Random). Spark PSCN to increase freq, NSCN to decrease, METL to cycle waveform. |
| **GRND** | Ground. Absorbs electrical current. Essential reference point for circuits. |

### Measurement Instruments

| Element | Description |
| ------- | ----------- |
| **VOLT** | Voltmeter. Measures voltage from nearby VCCS or spark intensity. Color: Green=low, Yellow=medium, Red=high. |
| **AMPR** | Ammeter. Measures current flow (sparks/sec). Conducts electricity. Color: Blue=low, Cyan=medium, White=high. |
| **PROB** | Oscilloscope Probe. Place next to signal source or connect via wire. Use Property Tool (P key) to set `tmp`=channel (0-3 for different colors). Signal propagates through connected probes instantly. |
| **OSCI** | Oscilloscope Display. Draw a horizontal row for 1D time trace, or a grid for 2D waveform display. Place PROB nearby - auto-detects within 10 pixels. |

### Passive Components

| Element | Description |
| ------- | ----------- |
| **RESI** | Resistor. Limits current flow and delays spark propagation. `tmp` sets resistance (1-100). |
| **CAPA** | Capacitor. Stores charge and releases it. Blocks DC, passes AC. `tmp2` sets capacitance. |
| **INDC** | Inductor. Opposes changes in current, stores energy in magnetic field. `tmp` sets inductance. |
| **POTM** | Potentiometer. Variable resistor. `tmp` sets position (0-100). Adjust with SPRK. |

### Semiconductors

| Element | Description |
| ------- | ----------- |
| **DIOD** | Diode. Current flows PSCN->DIOD->NSCN only. Blocks reverse flow. |
| **ZEND** | Zener Diode. PSCN=anode, NSCN=cathode. Conducts forward, and reverse above threshold (`tmp`). |
| **LEDS** | LED. Lights up when powered. `tmp` sets color (0-4: Red, Green, Blue, Yellow, White). Acts as diode. |
| **TRNS** | NPN Transistor. PSCN=Base, NSCN=Collector, METL/INWR=Emitter. Base spark enables current flow. |

### Active Components

| Element | Description |
| ------- | ----------- |
| **OPAM** | Op-Amp. Amplifies difference between inputs. PSCN=(+), NSCN=(-), outputs to METL/INWR. |
| **RLAY** | Relay. PSCN=control coil, NSCN=signal in, METL/INWR=signal out. Passes signal when coil is energized. |

## Building Circuits - Quick Start

1. **Simple LED circuit**: Draw VCCS -> METL wire -> LEDS -> METL wire -> GRND
2. **Signal generator + Oscilloscope**: Draw SGNL -> PROB (long wire) -> place OSCI nearby
3. **Transistor switch**: VCCS -> TRNS (connect PSCN to control signal, NSCN to VCCS, output from METL)

### Using the Property Tool

Press **P** to open the Property Tool. Click on an element to modify its properties:
- `tmp` - Primary parameter (frequency, resistance, channel, etc.)
- `tmp2` - Secondary parameter (waveform type, capacitance, etc.)

### Console Commands

Press **\`** (backtick) to open the console:
```
!set tmp PROB 1    # Set all probes to channel 1
!set tmp RESI 50   # Set all resistors to 50 ohms
```

## Fun/Wacky Elements

| Element | Description |
| ------- | ----------- |
| **BLOB** | Living Blob. A creature that eats organic matter and grows. Gets hungry! |
| **GOST** | Ghost. Spooky! Passes through walls, haunts stickmen, fears light. |
| **MIRR** | Mirror. Reflects particles back in the opposite direction! |
| **RBOW** | Rainbow. Colorful particles that leave beautiful trails everywhere! |
| **TIMZ** | Time Zone. Slows down all particles in its vicinity dramatically! |
| **LUCK** | Lucky Dust. Brings good fortune - random positive effects on nearby particles! |
| **ECHO** | Echo Matter. Copies any particle it touches and creates duplicates! |
| **PRSM** | Prism. Refracts light into rainbow colors! Creates spectacular light shows. |
| **SWRM** | Swarm. Insect swarm that seeks food and attacks! Controlled chaos. |
| **QAKE** | Quake. Causes earthquakes! Shakes nearby particles violently. |

And many more! Explore the Electronics (ELEC) and Special menus to find all new elements.
