# CNukedVST: OPL3 FM Synthesizer VST

A self-contained, dependency-free implementation of an OPL3 FM synthesizer VST instrument for Linux. This plugin emulates the Yamaha YMF262 (OPL3) sound chip using the Nuked OPL3 library, providing classic FM synthesis sounds in a VST format. The plugin is statically linked to avoid glibc version issues and designed to work without requiring any external SDK dependencies.

## Features

* **Authentic OPL3 Emulation**: Uses the highly accurate Nuked OPL3 emulation library
* **Up to 16-Voice Polyphony**: Play multiple notes simultaneously 
* **Complete FM Parameter Control**: Access to all OPL3 FM synthesis parameters including:
  + Operator parameters (AM, VIB, EGT, KSR, MULT, KSL, TL, AR, DR, SL, RR, WS)
  + Channel parameters (Feedback, Connection Type, Output Routing)
  + Global parameters (Tremolo/Vibrato Depth, Rhythm Mode)
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
   - Set operator parameters to control the timbre
   - Adjust ADSR envelope settings for dynamics
   - Experiment with different algorithms and feedback values
   - Try different waveforms for varied sound characteristics

## FM Synthesis Parameters

The plugin provides extensive control over the FM synthesis parameters, similar to the original OPL3 chip:

* **Operator Parameters**:
  + AM: Amplitude Modulation enable
  + VIB: Vibrato enable
  + EGT: Envelope Type
  + KSR: Key Scale Rate
  + MULT: Frequency multiplier
  + KSL: Key Scale Level
  + TL: Total Level (volume)
  + AR: Attack Rate
  + DR: Decay Rate
  + SL: Sustain Level
  + RR: Release Rate
  + WS: Wave Select

* **Channel Parameters**:
  + FB: Feedback amount
  + CON: Connection type (FM or AM)
  + LEFT/RIGHT: Channel output routing

* **Global Parameters**:
  + Tremolo Depth
  + Vibrato Depth
  + Rhythm Mode and percussion sounds

## Technical Details

This implementation:

* Provides a complete implementation of the VST2.4 ABI
* Creates its own VST2.4 header definitions without using the proprietary SDK
* Uses the Nuked OPL3 library for accurate emulation of the YMF262 (OPL3) sound chip
* Implements polyphonic FM synthesis with up to 16 voices
* Maps MIDI note events to OPL3 channels and registers
* Uses static linking for the C++ standard library to maximize compatibility

## License

This project uses components with different licenses:

* The VST plugin wrapper code is released under the MIT License.
* The Nuked OPL3 library is licensed under the GNU Lesser General Public License (LGPL).

Please see the source files for specific license details.

## Acknowledgments

* Nuked OPL3 emulator by nukeykt provides the core FM synthesis emulation
* The plugin demonstrates how to create a VST instrument with accurate FM synthesis without relying on external dependencies
* The VST architecture provides a practical way to use classic FM synthesis sounds in modern DAWs

While the VST2.4 standard is no longer actively developed, this format remains widely supported and provides a good balance of simplicity and functionality for custom plugin development.

## Contributing

Contributions are welcome! Feel free to submit pull requests to improve the plugin or add features:

* Additional parameters and controls
* Presets for classic FM sounds
* GUI improvements
* Performance optimizations

---

*Note: VST is a trademark of Steinberg Media Technologies GmbH*
*Note: This emulation is not affiliated with or endorsed by Yamaha Corporation*
