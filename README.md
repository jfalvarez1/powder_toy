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

---

### Power & Signal Sources

#### VCCS - Power Supply
Voltage scales with cluster size (larger blocks = more power).

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Base voltage | 1-100 |
| `tmp2` | Effective voltage (auto-calculated from cluster size) | Read-only |

#### SGNL - Signal Generator
Generates various waveforms at adjustable frequency.

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Frequency (higher = faster) | 1-100 |
| `tmp2` | Waveform type | 0-6 |
| `tmp3` | Current output value | Read-only |

Waveform types: 0=Square, 1=Sine, 2=Sawtooth, 3=Pulse, 4=Triangle, 5=Ramp-down, 6=Random

**Controls:** Spark PSCN to increase freq, NSCN to decrease, METL to cycle waveform.

#### GRND - Ground
Absorbs electrical current. No configurable properties.

---

### Measurement Instruments

#### VOLT - Voltmeter (7-segment display)
Measures voltage. **Can use PROB probes for remote measurement!**

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Measured voltage in mV | Read-only |

**Display size:** Draw 33x7 pixels for full "XX.XXX V" display.
**Measures from:** VCCS, BTRY, SPRK, SGNL, CAPA, **PROB**

#### AMPR - Ammeter (7-segment display)
Measures current flow. Conducts electricity (wire it in series).

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Measured current in uA | Read-only |

**Display size:** Draw 38x7 pixels for full "XX.XXX mA" display.

#### PROB - Oscilloscope Probe
Samples signals and propagates through connected probe wires.

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Channel number (affects color) | 0-3 |
| `tmp2` | Current signal value | Read-only |

Channel colors: 0=Yellow, 1=Cyan, 2=Magenta, 3=Green

#### OSCI - Oscilloscope Display
Displays waveforms from nearby PROB.

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Time position in display | Auto |
| `tmp2` | Signal brightness | Auto |

**Display modes:**
- **1D trace:** Draw a horizontal row of OSCI. Signal scrolls left.
- **2D waveform:** Draw a grid. Vertical axis = amplitude.

---

### Passive Components

#### RESI - Resistor
Limits current flow and delays spark propagation.

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Resistance (higher = more delay) | 1-100 |

#### CAPA - Capacitor
Stores and releases charge. Blocks DC, passes AC.

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Current charge level | 0-1000 |
| `tmp2` | Capacitance (higher = more storage) | 1-100 |

#### INDC - Inductor
Opposes changes in current, stores energy in magnetic field.

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Inductance value | 1-100 |

#### POTM - Potentiometer
Variable resistor. Spark it to adjust.

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Wiper position (resistance) | 0-100 |

**Controls:** Spark with PSCN to increase, NSCN to decrease.

---

### Semiconductors

#### DIOD - Diode
Current flows one direction only: PSCN -> DIOD -> NSCN

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Forward voltage drop | Default: 0 |

#### ZEND - Zener Diode
Conducts forward, and reverse when above breakdown voltage.

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Breakdown voltage threshold | 1-100 |

**Terminals:** PSCN=anode, NSCN=cathode

#### LEDS - LED
Lights up when powered. Acts as a diode.

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Color | 0-4 |

Colors: 0=Red, 1=Green, 2=Blue, 3=Yellow, 4=White

#### TRNS - NPN Transistor
Current flows Collector->Emitter when Base is triggered.

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Gain/amplification | Default: 1 |

**Terminals:** PSCN=Base, NSCN=Collector, METL/INWR=Emitter

---

### Active Components

#### OPAM - Operational Amplifier
Amplifies the difference between + and - inputs.

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Gain multiplier | 1-100 |

**Terminals:** PSCN=(+) input, NSCN=(-) input, METL/INWR=output

#### RLAY - Relay
Electrically controlled switch. Signal passes when coil is energized.

| Property | Description | Range |
| -------- | ----------- | ----- |
| `tmp` | Coil state (0=off, 1=on) | Read-only |

**Terminals:** PSCN=control coil, NSCN=signal in, METL/INWR=signal out

---

## Example Setups

### 1. Basic Voltage Measurement
```
[VCCS 5x5]----[METL wire]----[VOLT 33x7 display]
```
The voltmeter displays the VCCS output voltage.

### 2. Voltage Measurement with Probes
```
                    [PROB]----[PROB wire]----[VOLT 33x7]
                      |
[VCCS]----[METL]----[circuit under test]----[GRND]
```
Place PROB at the measurement point, wire it to VOLT display.

### 3. Current Measurement (Ammeter in Series)
```
[BTRY]----[METL]----[AMPR 38x7]----[METL]----[LEDS]----[GRND]
                         ^
                    (current flows through)
```

### 4. Signal Generator + Oscilloscope
```
[SGNL]----[PROB]=====[long PROB wire]=====[PROB]----[OSCI 50x10 grid]
   ^
  Set tmp=10 (frequency), tmp2=1 (sine wave)
```

### 5. Complete Test Bench
```
                              [VOLT 33x7]
                                  |
                               [PROB]
                                  |
[SGNL]---[PROB]---[circuit]---[PROB]---[AMPR 38x7]---[GRND]
                                              |
                                           [OSCI]
```

---

## Using the Property Tool

Press **P** to open the Property Tool, then click on an element to modify:

| Property | Common Uses |
| -------- | ----------- |
| `tmp` | Frequency, resistance, channel, color, threshold |
| `tmp2` | Waveform type, capacitance, secondary settings |
| `tmp3` | Signal output values (usually read-only) |
| `tmp4` | Channel info (usually read-only) |

### Console Commands

Press **\`** (backtick) to open the console:
```
!set tmp PROB 1      # Set all probes to channel 1 (cyan)
!set tmp RESI 50     # Set all resistors to 50 ohms
!set tmp SGNL 20     # Set signal generator frequency to 20
!set tmp2 SGNL 1     # Set signal generator to sine wave
!set tmp LEDS 2      # Set all LEDs to blue
!set tmp2 CAPA 50    # Set capacitor capacitance to 50
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
