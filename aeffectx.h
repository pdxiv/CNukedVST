// Minimal VST2.4 compatible extended header
// Based on VST 2.4 API without requiring the Steinberg SDK

#ifndef __aeffectx_h__
#define __aeffectx_h__

#include "aeffect.h"

// VST2 host opcodes
enum {
    audioMasterAutomate = 0,
    audioMasterVersion,
    audioMasterCurrentId,
    audioMasterIdle,
    audioMasterPinConnected,
    audioMasterWantMidi = 6,
    audioMasterGetTime,
    audioMasterProcessEvents,
    audioMasterSetTime,
    audioMasterTempoAt,
    audioMasterGetNumAutomatableParameters,
    audioMasterGetParameterQuantization,
    audioMasterIOChanged,
    audioMasterNeedIdle,
    audioMasterSizeWindow,
    audioMasterGetSampleRate,
    audioMasterGetBlockSize,
    audioMasterGetInputLatency,
    audioMasterGetOutputLatency,
    audioMasterGetPreviousPlug,
    audioMasterGetNextPlug,
    audioMasterWillReplaceOrAccumulate,
    audioMasterGetCurrentProcessLevel,
    audioMasterGetAutomationState,
    audioMasterOfflineStart,
    audioMasterOfflineRead,
    audioMasterOfflineWrite,
    audioMasterOfflineGetCurrentPass,
    audioMasterOfflineGetCurrentMetaPass,
    audioMasterSetOutputSampleRate,
    audioMasterGetOutputSpeakerArrangement,
    audioMasterGetVendorString,
    audioMasterGetProductString,
    audioMasterGetVendorVersion,
    audioMasterVendorSpecific,
    audioMasterSetIcon,
    audioMasterCanDo,
    audioMasterGetLanguage,
    audioMasterOpenWindow,
    audioMasterCloseWindow,
    audioMasterGetDirectory,
    audioMasterUpdateDisplay,
    audioMasterBeginEdit,
    audioMasterEndEdit,
    audioMasterOpenFileSelector,
    audioMasterCloseFileSelector,
    audioMasterEditFile,
    audioMasterGetChunkFile,
    audioMasterGetInputSpeakerArrangement
};

// Time info flags
enum {
    kVstTransportChanged = 1,
    kVstTransportPlaying = 1 << 1,
    kVstTransportCycleActive = 1 << 2,
    kVstTransportRecording = 1 << 3,
    kVstAutomationWriting = 1 << 6,
    kVstAutomationReading = 1 << 7,
    kVstNanosValid = 1 << 8,
    kVstPpqPosValid = 1 << 9,
    kVstTempoValid = 1 << 10,
    kVstBarsValid = 1 << 11,
    kVstCyclePosValid = 1 << 12,
    kVstTimeSigValid = 1 << 13,
    kVstSmpteValid = 1 << 14,
    kVstClockValid = 1 << 15
};

// MIDI event types
enum {
    kVstMidiType = 1,
    kVstSysExType = 6
};

// MIDI event structure
struct VstMidiEvent {
    int32 type;
    int32 byteSize;
    int32 deltaFrames;
    int32 flags;
    int32 noteLength;
    int32 noteOffset;
    char midiData[4];
    char detune;
    char noteOffVelocity;
    char reserved1;
    char reserved2;
};

// SysEx event structure
struct VstMidiSysexEvent {
    int32 type;
    int32 byteSize;
    int32 deltaFrames;
    int32 flags;
    int32 dumpBytes;
    intptr resvd1;
    char* sysexDump;
    intptr resvd2;
};

// Generic event structure
struct VstEvent {
    int32 type;
    int32 byteSize;
    int32 deltaFrames;
    int32 flags;
    char data[16];
};

// Events container
struct VstEvents {
    int32 numEvents;
    intptr reserved;
    VstEvent* events[2];    // Variable length
};

// Time info structure
struct VstTimeInfo {
    double samplePos;
    double sampleRate;
    double nanoSeconds;
    double ppqPos;
    double tempo;
    double barStartPos;
    double cycleStartPos;
    double cycleEndPos;
    int32 timeSigNumerator;
    int32 timeSigDenominator;
    int32 smpteOffset;
    int32 smpteFrameRate;
    int32 samplesToNextClock;
    int32 flags;
};

// Plugin capabilities (canDo strings)
#define CANDO_PLUGASINSTSYNTH  "plugAsChannelInsert"
#define CANDO_PLUGASFX         "plugAsFx"
#define CANDO_SENDVSTEVENTS    "sendVstEvents"
#define CANDO_SENDVSTMIDIEVENT "sendVstMidiEvent"
#define CANDO_RECEIVEVSTSEVENTS "receiveVstEvents"
#define CANDO_RECEIVEVSTSENDS  "receiveVstSends"
#define CANDO_RECEIVEVSTMIDIEVENT "receiveVstMidiEvent"
#define CANDO_OFFLINE          "offline"
#define CANDO_MIDIPROGRAM      "midiProgramNames"
#define CANDO_BYPASS           "bypass"

#endif // __aeffectx_h__
