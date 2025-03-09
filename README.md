# CNukedVST: OPL3 FM Synthesizer VST

A self-contained, dependency-free implementation of an OPL3 FM synthesizer VST instrument for Linux. This plugin emulates the Yamaha YMF262 (OPL3) sound chip using the Nuked OPL3 library, providing classic FM synthesis sounds in a VST format. The plugin is statically linked to avoid glibc version issues and designed to work without requiring any external SDK dependencies.

## Features

* **Accurate OPL3 Emulation**: Uses the highly accurate Nuked OPL3 emulation library
* **Monotimbral/Polyphonic Design**: Up to 16-voice polyphony with a single consistent timbre
* **Complete 2-Operator FM Synthesis**: Full control over both carrier and modulator parameters
* **Simple Parameter Interface**: Easy-to-understand parameters with descriptive names:
  + Separate modulator and carrier controls for all synthesis parameters
  + Feedback and connection type controls
  + Stereo output control
  + Global tremolo and vibrato depth settings
* **Zero External Dependencies**: Doesn't require the Steinberg SDK or any other framework
* **Statically Linked**: Minimizes system dependencies and compatibility issues
* **MIDI Ready**: Responds to note-on, note-off, and other MIDI control messages
* **Cross-Platform Potential**: Core implementation is portable (currently built for Linux)

## Building

### Prerequisites

```bash
sudo apt-get update
sudo apt-get install build-essential
```

### Required External Files

Before building, you need the Nuked OPL3 library files:

1. Download these two files from the [Nuked-OPL3 repository](https://github.com/nukeykt/Nuked-OPL3):
   - `opl3.c`

   - `opl3.h`

2. Place both files in your CNukedVST project directory alongside `CNukedVST.cpp`

You can download them directly using:

```bash
wget https://raw.githubusercontent.com/nukeykt/Nuked-OPL3/master/opl3.c
wget https://raw.githubusercontent.com/nukeykt/Nuked-OPL3/master/opl3.h
```

### Compilation

1. Clone this repository:

```bash
git clone https://github.com/yourusername/CNukedVST.git
cd CNukedVST
```

2. Build the plugin:

```bash
make
```

3. Install to your user's VST directory:

```bash
make install
```

This will place the VST plugin in `~/.vst/` .

### Verification

To verify that the plugin is properly built and statically linked:

```bash
make check-static
```

## Usage

1. Start your favorite VST host application (Reaper, Ardour, Carla, etc.)
2. Scan for VST plugins, ensuring the host is configured to look in `~/.vst/`
3. Add the "OPL3 FM Synth" as an instrument on a MIDI track
4. Play notes via MIDI to control the synthesizer
5. Adjust the various FM synthesis parameters to create different sounds:
   - Modulator parameters affect the tone color/timbre
   - Carrier parameters control the overall envelope and output
   - Feedback creates more complex, richer sounds
   - Connection type toggles between FM and AM synthesis modes

## FM Synthesis Parameters

The plugin provides easy-to-use parameters for FM synthesis, organized into logical groups:

### Modulator Parameters

Each modulator has these parameters:
* **Tremolo**: Enables amplitude modulation effect
* **Vibrato**: Enables frequency vibrato effect
* **Sustain**: Toggles between sustaining and non-sustaining envelope types
* **KSR**: Controls how key scaling affects envelope rates
* **Mult**: Sets the frequency multiple (0-15) relative to the base frequency
* **KSL**: Controls how note frequency affects volume attenuation
* **Level**: Sets the output level of the modulator (0-63 dB)
* **Attack**: Controls how quickly the sound reaches peak volume (0-15)
* **Decay**: Controls how quickly the sound falls to sustain level (0-15)
* **Sustain Lv**: Sets the volume level maintained during note hold (0-15)
* **Release**: Controls how quickly the sound fades after note-off (0-15)
* **Waveform**: Selects between different waveforms (0-7)

### Carrier Parameters

The carrier has the same set of parameters as the modulator, but they affect the final output sound:
* **Level**: Controls the overall volume of the instrument
* **Attack/Decay/Sustain/Release**: Shapes the overall volume envelope
* **Waveform**: Affects the basic tone quality

### Channel Parameters

* **Feedback**: Controls how much of the modulator's output is fed back into itself
* **Connection**: Switches between FM mode (0) and AM mode (1)
* **Left/Right Output**: Controls stereo output routing

### Global Parameters

* **Tremolo Depth**: Sets the intensity of the tremolo effect
* **Vibrato Depth**: Sets the intensity of the vibrato effect

## Technical Details

This implementation:

* Provides a complete implementation of the VST2.4 ABI
* Creates its own VST2.4 header definitions without using the proprietary SDK
* Uses the Nuked OPL3 library for accurate emulation of the YMF262 (OPL3) sound chip
* Implements polyphonic FM synthesis with up to 16 voices
* Maps MIDI note events to OPL3 channels with accurate register handling
* Uses static linking for the C++ standard library to maximize compatibility
* Features a detailed operator-to-register mapping based on the OPL3 programmer's guide

## License

This project uses components with different licenses:

* The VST plugin wrapper code is released under the MIT License.
* The Nuked OPL3 library is licensed under the GNU Lesser General Public License (LGPL).

Please see the source files for specific license details.

## Acknowledgments

* Nuked OPL3 emulator by Nuke. YKT provides the core FM synthesis emulation
* The OPL3 Programmer's Guide provided valuable information about register mapping
* The plugin demonstrates how to create a VST instrument with accurate FM synthesis without relying on external dependencies

While the VST2.4 standard is no longer actively developed, this format remains widely supported and provides a good balance of simplicity and functionality for custom plugin development.

---

*Note: VST is a trademark of Steinberg Media Technologies GmbH*
*Note: This emulation is not affiliated with or endorsed by Yamaha Corporation*
