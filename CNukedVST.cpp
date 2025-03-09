/******************************************************************************
 * CNukedVST.cpp (uses Nuked-OPL3 for polyphonic FM)          *
 ******************************************************************************/

#include "aeffect.h"
#include "aeffectx.h"
#include "opl3.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <utility>  // For std::pair and std::make_pair

// Define VSTCALLBACK if not defined (usually from VST SDK)
#ifndef VSTCALLBACK
#if defined(WIN32) || defined(__FLAT__)
#define VSTCALLBACK __cdecl
#else
#define VSTCALLBACK
#endif
#endif

// Helper to avoid unused parameter warnings
template<typename T>
static inline void unused(T&& /*arg*/) {}

// -----------------------------------------------------------------------------
// 1) Some VST constant definitions
// -----------------------------------------------------------------------------
#define kNumPrograms 1
#define kNumInputs   0
#define kNumOutputs  2

// For OPL3, we have up to 18 "channels," each with 2 operators => 36 operators.
static const int OPL3_CHANNEL_COUNT = 18;
static const int OPL3_OPERATORS_PER_CHANNEL = 2;
static const int OPL3_TOTAL_OPERATORS = OPL3_CHANNEL_COUNT * OPL3_OPERATORS_PER_CHANNEL;

// Every single synthesis parameter for OPL3
enum {
    // Each operator has these parameters:
    kParamAM,   // Amplitude Modulation enable
    kParamVIB,  // Vibrato enable
    kParamEGT,  // Envelope Type
    kParamKSR,  // Key Scale Rate
    kParamMULT, // Frequency multiplier
    kParamKSL,  // Key Scale Level
    kParamTL,   // Total Level (volume)
    kParamAR,   // Attack Rate
    kParamDR,   // Decay Rate
    kParamSL,   // Sustain Level
    kParamRR,   // Release Rate
    kParamWS,   // Wave Select
    
    kNumOperatorParams // = 12
};

// Channel parameters
enum {
    kParamFeedback,    // 3-bit feedback
    kParamConnection,  // Connection type (0=FM, 1=AM)
    kParamLeftOutput,  // Enable left output
    kParamRightOutput, // Enable right output
    
    kNumChannelParams // = 4
};

// Additional global parameters
enum {
    kParamTremoloDepth, // Deep vibrato
    kParamVibratoDepth, // Deep tremolo
    kParamRhythmMode,   // Rhythm mode enable
    kParamHH,           // Rhythm mode: Hi-hat
    kParamTC,           // Rhythm mode: Top cymbal
    kParamTOM,          // Rhythm mode: Tom-tom
    kParamSD,           // Rhythm mode: Snare drum
    kParamBD,           // Rhythm mode: Bass drum
    
    kNumGlobalParams // = 8
};

// VST Parameter enumeration - MONOTIMBRAL design (one set of parameters for all voices)
enum {
    // Modulator parameters
    kVST_Mod_AM = 0,
    kVST_Mod_VIB,
    kVST_Mod_EGT,
    kVST_Mod_KSR,
    kVST_Mod_MULT,
    kVST_Mod_KSL,
    kVST_Mod_TL,
    kVST_Mod_AR,
    kVST_Mod_DR,
    kVST_Mod_SL,
    kVST_Mod_RR,
    kVST_Mod_WS,
    
    // Carrier parameters
    kVST_Car_AM,
    kVST_Car_VIB,
    kVST_Car_EGT,
    kVST_Car_KSR,
    kVST_Car_MULT,
    kVST_Car_KSL,
    kVST_Car_TL,
    kVST_Car_AR,
    kVST_Car_DR,
    kVST_Car_SL,
    kVST_Car_RR,
    kVST_Car_WS,
    
    // Channel parameters
    kVST_FB,
    kVST_CON,
    kVST_LEFT,
    kVST_RIGHT,
    
    // Global parameters - only keep essential ones
    kVST_TremoloDepth,
    kVST_VibratoDepth,
    
    // No longer exposing these rhythm mode parameters:
    // kVST_RhythmMode,
    // kVST_HH,
    // kVST_TC,
    // kVST_TOM,
    // kVST_SD,
    // kVST_BD,
    
    kNumVSTParams
};

// So total parameters = OPL3_TOTAL_OPERATORS * kNumOperatorParams + OPL3_CHANNEL_COUNT * kNumChannelParams + kNumGlobalParams
static const int TOTAL_OPERATOR_PARAMETERS = OPL3_TOTAL_OPERATORS * kNumOperatorParams;
static const int TOTAL_CHANNEL_PARAMETERS = OPL3_CHANNEL_COUNT * kNumChannelParams;
static const int TOTAL_INTERNAL_PARAMETERS = TOTAL_OPERATOR_PARAMETERS + TOTAL_CHANNEL_PARAMETERS + kNumGlobalParams;

// Parameter name helpers
static const char* PARAM_NAMES[kNumOperatorParams] = {
    "Tremolo", "Vibrato", "Sustain", "KSR", "Mult", "KSL", "Level", "Attack", "Decay", "Sustain Lv", "Release", "Waveform"
};

static const char* CHANNEL_NAMES[kNumChannelParams] = {
    "Feedback", "Connection", "Left Out", "Right Out"
};

static const char* GLOBAL_NAMES[kNumGlobalParams] = {
    "Tremolo Depth", "Vibrato Depth", "Rhythm Mode", "HH", "TC", "TOM", "SD", "BD"
};

// New descriptive names for the operators
static const char* OPERATOR_TYPES[2] = {
    "Mod", "Car"  // Modulator and Carrier
};

// We manage 16 voices in software, mapped onto 16 out of 18 possible OPL3 channels.
static const int MAX_VOICES = 16;

// -----------------------------------------------------------------------------
// We define a minimal VoiceInfo structure to handle MIDI notes -> channel assignment
// -----------------------------------------------------------------------------
struct VoiceInfo {
    bool active;
    int midiNote;
    float frequency;
    int channelIndex; // which OPL3 channel is being used
};

// -----------------------------------------------------------------------------
// Our main plugin "class." In real VST2 code, you'd typically wrap this in a class
// that you pass to AEffect, but we can do it all in one file for simplicity.
// -----------------------------------------------------------------------------
typedef struct MyOPL3VST {
    AEffect         aeffect;                  // VST2 struct
    float           sampleRate;
    VoiceInfo       voices[MAX_VOICES];
    opl3_chip       chip;                     // Nuked-OPL3 instance (correct type from opl3.h)

    // We store all parameter values in a float array. Each is [0..1], we scale them later.
    float           paramValues[TOTAL_INTERNAL_PARAMETERS];
    
    // For the monotimbral interface, we need to store the current settings that apply to all voices
    float           currentSettings[2*kNumOperatorParams + kNumChannelParams + kNumGlobalParams];

} MyOPL3VST;

// Forward declarations of our function callbacks:
static void        processReplacing(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames);
static void        setParameter(AEffect* effect, int32_t index, float value);
static float       getParameter(AEffect* effect, int32_t index);
static intptr_t    dispatcher(AEffect* effect, int32_t opCode, int32_t index, intptr_t value, void* ptr, float opt);

// Helper to convert parameter index to name
static void getParameterName(MyOPL3VST* vst, int32_t index, char* label);
static void getParameterDisplay(MyOPL3VST* vst, int32_t index, char* text);

// Helper function for MIDI handling
static void handleMidiEvent(MyOPL3VST* vst, VstMidiEvent& midiEvent);

// We'll make a small helper so we can write OPL3 registers for each parameter
static void updateOPL3Parameters(MyOPL3VST* vst);

// Helper to apply current voice settings to all OPL3 channels
static void applyVoiceSettingsToAllChannels(MyOPL3VST* vst);

// -----------------------------------------------------------------------------
// 2) Entry point to create the plugin object
// -----------------------------------------------------------------------------
extern "C" AEffect* VSTPluginMain(audioMasterCallback audioMaster)
{
    // Allocate our plugin struct
    MyOPL3VST* vst = new MyOPL3VST;
    memset(vst, 0, sizeof(MyOPL3VST));

    // Fill out the AEffect
    AEffect& ae = vst->aeffect;
    ae.magic            = CCONST('V', 's', 't', 'P');  // Fixed multi-character constant
    ae.dispatcher       = dispatcher;
    ae.process          = nullptr;            // Not used, we only use processReplacing
    ae.setParameter     = setParameter;
    ae.getParameter     = getParameter;
    ae.processReplacing = processReplacing;
    ae.numPrograms      = kNumPrograms;
    ae.numParams        = kNumVSTParams;  // Monotimbral interface has fewer parameters
    ae.numInputs        = kNumInputs;
    ae.numOutputs       = kNumOutputs;
    
    // Set flags to indicate a synth plugin with replacing process function
    ae.flags            = effFlagsIsSynth | effFlagsCanReplacing | effFlagsProgramChunks;
    ae.initialDelay     = 0;
    ae.uniqueID         = CCONST('O', 'P', 'L', '3');
    ae.version          = 1000;  // 1.0.0.0
    
    // Store pointer to our struct
    ae.object           = vst;
    ae.user             = nullptr;

    // Set defaults
    vst->sampleRate = 44100.f;
    for (int i = 0; i < MAX_VOICES; i++) {
        vst->voices[i].active = false;
        vst->voices[i].midiNote = -1;
        vst->voices[i].frequency = 0.f;
        vst->voices[i].channelIndex = i; // simple 1:1 mapping
    }

    // Initialize paramValues with sensible defaults
    for (int i = 0; i < TOTAL_INTERNAL_PARAMETERS; i++) {
        // Default initialization to prevent undefined behavior
        vst->paramValues[i] = 0.f;
    }
    
    // Initialize VST interface parameters with sensible defaults
    // Modulator (operator 0)
    vst->currentSettings[kVST_Mod_AM]   = 0.0f; // AM off
    vst->currentSettings[kVST_Mod_VIB]  = 0.0f; // VIB off
    vst->currentSettings[kVST_Mod_EGT]  = 0.0f; // EGT non-sustaining
    vst->currentSettings[kVST_Mod_KSR]  = 0.0f; // KSR off
    vst->currentSettings[kVST_Mod_MULT] = 0.2f; // MULT around 2-3
    vst->currentSettings[kVST_Mod_KSL]  = 0.0f; // KSL 0
    vst->currentSettings[kVST_Mod_TL]   = 0.1f; // Modulator level
    vst->currentSettings[kVST_Mod_AR]   = 1.0f; // Fastest attack
    vst->currentSettings[kVST_Mod_DR]   = 0.4f; // Medium decay
    vst->currentSettings[kVST_Mod_SL]   = 0.3f; // Medium sustain level
    vst->currentSettings[kVST_Mod_RR]   = 0.5f; // Medium release
    vst->currentSettings[kVST_Mod_WS]   = 0.0f; // Sine wave
    
    // Carrier (operator 1)
    vst->currentSettings[kVST_Car_AM]   = 0.0f; // AM off
    vst->currentSettings[kVST_Car_VIB]  = 0.0f; // VIB off
    vst->currentSettings[kVST_Car_EGT]  = 0.0f; // EGT non-sustaining
    vst->currentSettings[kVST_Car_KSR]  = 0.0f; // KSR off
    vst->currentSettings[kVST_Car_MULT] = 0.2f; // MULT around 2-3
    vst->currentSettings[kVST_Car_KSL]  = 0.0f; // KSL 0
    vst->currentSettings[kVST_Car_TL]   = 0.0f; // Maximum volume for carrier
    vst->currentSettings[kVST_Car_AR]   = 1.0f; // Fastest attack
    vst->currentSettings[kVST_Car_DR]   = 0.4f; // Medium decay
    vst->currentSettings[kVST_Car_SL]   = 0.3f; // Medium sustain level
    vst->currentSettings[kVST_Car_RR]   = 0.5f; // Medium release
    vst->currentSettings[kVST_Car_WS]   = 0.0f; // Sine wave
    
    // Channel parameters
    vst->currentSettings[kVST_FB]     = 0.0f; // No feedback
    vst->currentSettings[kVST_CON]    = 0.0f; // FM mode
    vst->currentSettings[kVST_LEFT]   = 1.0f; // Left output on
    vst->currentSettings[kVST_RIGHT]  = 1.0f; // Right output on
    
    // Global parameters
    vst->currentSettings[kVST_TremoloDepth] = 0.0f; // Normal tremolo
    vst->currentSettings[kVST_VibratoDepth] = 0.0f; // Normal vibrato
    
    // Apply these settings to the internal OPL3 parameters for all voices
    applyVoiceSettingsToAllChannels(vst);

    // Initialize OPL3 at 44.1k
    OPL3_Reset(&vst->chip, vst->sampleRate);
    
    // Enable OPL3 features (not OPL2 mode)
    OPL3_WriteReg(&vst->chip, 0x105, 1);
    
    // Set waveform select enable bit
    OPL3_WriteReg(&vst->chip, 0x01, 0x20);
    
    // Initialize all parameters to the default values
    updateOPL3Parameters(vst);

    return &ae;
}

// -----------------------------------------------------------------------------
// Helper to apply the current voice settings to all channels
// -----------------------------------------------------------------------------
static void applyVoiceSettingsToAllChannels(MyOPL3VST* vst)
{
    // Apply modulator settings to all modulator operators (operator 0 in each channel)
    for (int ch = 0; ch < MAX_VOICES; ch++) {
        int op = ch * 2; // Modulator operator index
        
        // Copy all modulator parameters
        for (int param = 0; param < kNumOperatorParams; param++) {
            vst->paramValues[op*kNumOperatorParams + param] = vst->currentSettings[param];
        }
        
        // Copy all carrier parameters
        op = ch * 2 + 1; // Carrier operator index
        for (int param = 0; param < kNumOperatorParams; param++) {
            vst->paramValues[op*kNumOperatorParams + param] = vst->currentSettings[kNumOperatorParams + param];
        }
        
        // Copy channel parameters
        int chParamIndex = TOTAL_OPERATOR_PARAMETERS + ch * kNumChannelParams;
        for (int param = 0; param < kNumChannelParams; param++) {
            vst->paramValues[chParamIndex + param] = vst->currentSettings[2*kNumOperatorParams + param];
        }
    }
    
    // Copy global parameters
    int globalBaseIndex = TOTAL_OPERATOR_PARAMETERS + TOTAL_CHANNEL_PARAMETERS;
    for (int param = 0; param < 2; param++) {  // Only copy the first 2 global parameters we expose
        vst->paramValues[globalBaseIndex + param] = 
            vst->currentSettings[2*kNumOperatorParams + kNumChannelParams + param];
    }
}

// -----------------------------------------------------------------------------
// 3) Dispatcher callback
// -----------------------------------------------------------------------------
static intptr_t dispatcher(AEffect* effect, int32_t opCode, int32_t index, intptr_t value, void* ptr, float opt)
{
    MyOPL3VST* vst = (MyOPL3VST*)effect->object;
    char* strPtr = (char*)ptr;
    
    switch (opCode)
    {
        case effGetEffectName:
            strncpy(strPtr, "OPL3 FM Synth", 31);
            return 1;

        case effGetVendorString:
            strncpy(strPtr, "VSTPluginDev", 31);
            return 1;

        case effGetProductString:
            strncpy(strPtr, "OPL3 FM Synthesizer", 31);
            return 1;

        case effGetVendorVersion:
            return 1000; // 1.0.0.0
            
        case effCanDo:
            if (!strcmp(strPtr, "receiveVstEvents") ||
                !strcmp(strPtr, "receiveVstMidiEvent"))
                return 1;
            return 0;
            
        case effGetParamName:
            if (index >= 0 && index < kNumVSTParams)
            {
                getParameterName(vst, index, strPtr);
                return 1;
            }
            return 0;
            
        case effGetParamDisplay:
            if (index >= 0 && index < kNumVSTParams)
            {
                getParameterDisplay(vst, index, strPtr);
                return 1;
            }
            return 0;
        
        case effSetSampleRate:
        {
            // Host is telling us the sample rate changed
            float newRate = opt;
            vst->sampleRate = newRate;
            OPL3_Reset(&vst->chip, newRate);
            // Re-initialize all parameters after reset
            updateOPL3Parameters(vst);
            break;
        }
        
        case effMainsChanged:
        {
            // 0 => stop, 1 => start
            if (value == 0) {
                // Deactivate
                // All notes off
                for (int i = 0; i < MAX_VOICES; i++) {
                    if (vst->voices[i].active) {
                        int ch = vst->voices[i].channelIndex;
                        int bank = (ch < 9) ? 0 : 1;
                        int chInBank = ch % 9;
                        // Turn off note - we need to create a composite register value
                        uint16_t reg = (bank << 8) | (0xB0 + chInBank);
                        OPL3_WriteReg(&vst->chip, reg, 0);
                        vst->voices[i].active = false;
                    }
                }
            } else {
                // Reactivate - nothing special required
            }
            break;
        }
        
        case effProcessEvents:
        {
            // Host is sending events (MIDI, etc.)
            VstEvents* events = (VstEvents*)ptr;
            for (int i = 0; i < events->numEvents; i++)
            {
                if (events->events[i]->type == kVstMidiType) {
                    VstMidiEvent* midi = (VstMidiEvent*)events->events[i];
                    handleMidiEvent(vst, *midi);
                }
            }
            return 1;
        }
        
        default:
            break;
    }
    return 0;
}

// -----------------------------------------------------------------------------
// Helper functions for displaying parameter info
// -----------------------------------------------------------------------------
static void getParameterName(MyOPL3VST* vst, int32_t index, char* label)
{
    // Modulator parameters
    if (index < kNumOperatorParams) {
        sprintf(label, "%s %s", OPERATOR_TYPES[0], PARAM_NAMES[index]);
    }
    // Carrier parameters
    else if (index < 2*kNumOperatorParams) {
        int paramIndex = index - kNumOperatorParams;
        sprintf(label, "%s %s", OPERATOR_TYPES[1], PARAM_NAMES[paramIndex]);
    }
    // Channel parameters
    else if (index < 2*kNumOperatorParams + kNumChannelParams) {
        int paramIndex = index - 2*kNumOperatorParams;
        strcpy(label, CHANNEL_NAMES[paramIndex]);
    }
    // Global parameters
    else {
        int paramIndex = index - (2*kNumOperatorParams + kNumChannelParams);
        strcpy(label, GLOBAL_NAMES[paramIndex]);
    }
}

static void getParameterDisplay(MyOPL3VST* vst, int32_t index, char* text)
{
    float value = vst->currentSettings[index];
    
    // Modulator parameters
    if (index < kNumOperatorParams) {
        int paramType = index;
        
        switch (paramType) {
            case kParamAM:
            case kParamVIB:
            case kParamEGT:
            case kParamKSR:
                sprintf(text, "%s", value > 0.5f ? "On" : "Off");
                break;
                
            case kParamMULT:
                sprintf(text, "%d", static_cast<int>(value * 15.0f));
                break;
                
            case kParamKSL:
                sprintf(text, "%d dB/oct", static_cast<int>(value * 3.0f));
                break;
                
            case kParamTL:
                sprintf(text, "%.1f dB", value * 63.0f);
                break;
                
            case kParamAR:
            case kParamDR:
            case kParamRR:
                sprintf(text, "%d", static_cast<int>(value * 15.0f));
                break;
                
            case kParamSL:
                sprintf(text, "%d", static_cast<int>(value * 15.0f));
                break;
                
            case kParamWS:
                sprintf(text, "%d", static_cast<int>(value * 7.0f));
                break;
                
            default:
                sprintf(text, "%.2f", value);
        }
    }
    // Carrier parameters
    else if (index < 2*kNumOperatorParams) {
        int paramType = index - kNumOperatorParams;
        
        switch (paramType) {
            case kParamAM:
            case kParamVIB:
            case kParamEGT:
            case kParamKSR:
                sprintf(text, "%s", value > 0.5f ? "On" : "Off");
                break;
                
            case kParamMULT:
                sprintf(text, "%d", static_cast<int>(value * 15.0f));
                break;
                
            case kParamKSL:
                sprintf(text, "%d dB/oct", static_cast<int>(value * 3.0f));
                break;
                
            case kParamTL:
                sprintf(text, "%.1f dB", value * 63.0f);
                break;
                
            case kParamAR:
            case kParamDR:
            case kParamRR:
                sprintf(text, "%d", static_cast<int>(value * 15.0f));
                break;
                
            case kParamSL:
                sprintf(text, "%d", static_cast<int>(value * 15.0f));
                break;
                
            case kParamWS:
                sprintf(text, "%d", static_cast<int>(value * 7.0f));
                break;
                
            default:
                sprintf(text, "%.2f", value);
        }
    }
    // Channel parameters
    else if (index < 2*kNumOperatorParams + kNumChannelParams) {
        int paramType = index - 2*kNumOperatorParams;
        
        switch (paramType) {
            case kParamFeedback:
                sprintf(text, "%d", static_cast<int>(value * 7.0f));
                break;
                
            case kParamConnection:
                sprintf(text, "%s", value > 0.5f ? "AM" : "FM");
                break;
                
            case kParamLeftOutput:
            case kParamRightOutput:
                sprintf(text, "%s", value > 0.5f ? "On" : "Off");
                break;
                
            default:
                sprintf(text, "%.2f", value);
        }
    }
    // Global parameters
    else {
        int paramType = index - (2*kNumOperatorParams + kNumChannelParams);
        
        switch (paramType) {
            case kParamTremoloDepth:
            case kParamVibratoDepth:
                sprintf(text, "%s", value > 0.5f ? "On" : "Off");
                break;
                
            default:
                sprintf(text, "%.2f", value);
        }
    }
}

// -----------------------------------------------------------------------------
// 4) setParameter / getParameter
// -----------------------------------------------------------------------------
static void setParameter(AEffect* effect, int32_t index, float value)
{
    MyOPL3VST* vst = (MyOPL3VST*)effect->object;
    if (index < 0 || index >= kNumVSTParams) return;

    // Store in currentSettings array
    vst->currentSettings[index] = value;
    
    // Apply the setting to all voices
    applyVoiceSettingsToAllChannels(vst);

    // Immediately update the OPL3 register(s)
    updateOPL3Parameters(vst);
}

static float getParameter(AEffect* effect, int32_t index)
{
    MyOPL3VST* vst = (MyOPL3VST*)effect->object;
    if (index < 0 || index >= kNumVSTParams) return 0.f;

    return vst->currentSettings[index];
}

// -----------------------------------------------------------------------------
// 5) The big function that updates OPL3 registers from paramValues
// -----------------------------------------------------------------------------
static void updateOPL3Parameters(MyOPL3VST* vst)
{
    // We will recalculate each operator's register from the parameter array.
    // For simplicity, we'll assume OPL3 bank=0 for all writes.

    // Each channel has 2 operators => operator indices
    // The location of each operator's regs in OPL3 is more complicated, but we can
    // do a standard AdLib layout for channels 0..8 in bank 0, 9..17 in bank 1, etc.
    // The code below is a simplified, partial example.

    // Helper function to compute the OPL register offsets for operator i
    auto getOpBase = [&](int opIndex) -> std::pair<int, int> {
        // Convert our opIndex (0, 1, 2, ..., 35) to channel and isCarrier
        int channel = opIndex / 2;
        int isCarrier = opIndex % 2;
        
        // These arrays map from (channel, isCarrier) to actual OPL3 operator index
        // Per the OPL3 programmer's guide mapping table
        static const int actualOpIndex[18][2] = {
            {0, 3},   {1, 4},   {2, 5},   {6, 9},   {7, 10},  {8, 11},
            {12, 15}, {13, 16}, {14, 17}, {18, 21}, {19, 22}, {20, 23},
            {24, 27}, {25, 28}, {26, 29}, {30, 33}, {31, 34}, {32, 35}
        };
        
        // Get the actual OPL3 operator index
        int actualOp = actualOpIndex[channel][isCarrier];
        
        // Map actual operator to register offset
        // Offsets from the OPL3 programming guide
        static const int regOffsets[36] = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15
        };
        
        // Calculate bank and offset
        int bank = (actualOp >= 18) ? 1 : 0;
        int offset = regOffsets[actualOp];
        
        return std::make_pair(bank, offset);
    };

    // First, handle global parameters
    int globalBaseIndex = TOTAL_OPERATOR_PARAMETERS + TOTAL_CHANNEL_PARAMETERS;
    uint8_t tremVib = 0;
    if (vst->paramValues[globalBaseIndex + kParamTremoloDepth] > 0.5f) {
        tremVib |= 0x80; // Deep tremolo
    }
    if (vst->paramValues[globalBaseIndex + kParamVibratoDepth] > 0.5f) {
        tremVib |= 0x40; // Deep vibrato
    }
    
    // Always disable rhythm mode
    uint8_t rhythmBits = 0; // Always keep rhythm mode disabled
    
    // Create composite register value (bank 0, register 0xBD)
    uint16_t bdReg = 0x0BD; // Bank 0, register 0xBD
    OPL3_WriteReg(&vst->chip, bdReg, rhythmBits | tremVib);

    // Now we traverse each operator, read paramValues, and write registers
    for (int op = 0; op < OPL3_TOTAL_OPERATORS; op++)
    {
        // Our param block for this operator is
        //    [op * kNumOperatorParams ... op * kNumOperatorParams + (kNumOperatorParams-1)]
        float amF   = vst->paramValues[op*kNumOperatorParams + kParamAM];
        float vibF  = vst->paramValues[op*kNumOperatorParams + kParamVIB];
        float egtF  = vst->paramValues[op*kNumOperatorParams + kParamEGT];
        float ksrF  = vst->paramValues[op*kNumOperatorParams + kParamKSR];
        float multF = vst->paramValues[op*kNumOperatorParams + kParamMULT];
        float kslF  = vst->paramValues[op*kNumOperatorParams + kParamKSL];
        float tlF   = vst->paramValues[op*kNumOperatorParams + kParamTL];
        float arF   = vst->paramValues[op*kNumOperatorParams + kParamAR];
        float drF   = vst->paramValues[op*kNumOperatorParams + kParamDR];
        float slF   = vst->paramValues[op*kNumOperatorParams + kParamSL];
        float rrF   = vst->paramValues[op*kNumOperatorParams + kParamRR];
        float wsF   = vst->paramValues[op*kNumOperatorParams + kParamWS];

        // Convert each 0..1 float to an integer in the relevant range
        int AM   = (amF  > 0.5f) ? 1 : 0;
        int VIB  = (vibF > 0.5f) ? 1 : 0;
        int EGT  = (egtF > 0.5f) ? 1 : 0;
        int KSR  = (ksrF > 0.5f) ? 1 : 0;
        int MULT = (int)(multF * 15.999f); // 0..15
        int KSL  = (int)(kslF * 3.999f);   // 0..3
        int TL   = (int)(tlF * 63.999f);   // 0..63
        int AR   = (int)(arF * 15.999f);   // 0..15
        int DR   = (int)(drF * 15.999f);   // 0..15
        int SL   = (int)(slF * 15.999f);   // 0..15
        int RR   = (int)(rrF * 15.999f);   // 0..15
        int WS   = (int)(wsF * 7.999f);    // 0..7

        // Get operator's register bank and offset
        std::pair<int, int> opBase = getOpBase(op);
        int bank = opBase.first;
        int opSlot = opBase.second;

        // 0x20 => AM, VIB, EGT, KSR, MULT
        int r20 = (AM << 7) | (VIB << 6) | (EGT << 5) | (KSR << 4) | MULT;
        uint16_t reg20 = (bank << 8) | (0x20 + opSlot);
        OPL3_WriteReg(&vst->chip, reg20, (unsigned char)r20);

        // 0x40 => KSL, TL
        int r40 = (KSL << 6) | TL;
        uint16_t reg40 = (bank << 8) | (0x40 + opSlot);
        OPL3_WriteReg(&vst->chip, reg40, (unsigned char)r40);

        // 0x60 => AR, DR
        int r60 = (AR << 4) | DR;
        uint16_t reg60 = (bank << 8) | (0x60 + opSlot);
        OPL3_WriteReg(&vst->chip, reg60, (unsigned char)r60);

        // 0x80 => SL, RR
        int r80 = (SL << 4) | RR;
        uint16_t reg80 = (bank << 8) | (0x80 + opSlot);
        OPL3_WriteReg(&vst->chip, reg80, (unsigned char)r80);

        // 0xE0 => WS (waveform)
        uint16_t regE0 = (bank << 8) | (0xE0 + opSlot);
        OPL3_WriteReg(&vst->chip, regE0, (unsigned char)WS);
    }

    // Now each channel's parameters
    int channelParamOffset = OPL3_TOTAL_OPERATORS * kNumOperatorParams;
    for (int ch = 0; ch < OPL3_CHANNEL_COUNT; ch++)
    {
        float fbF   = vst->paramValues[channelParamOffset + ch*kNumChannelParams + kParamFeedback];
        float conF  = vst->paramValues[channelParamOffset + ch*kNumChannelParams + kParamConnection];
        float leftF = vst->paramValues[channelParamOffset + ch*kNumChannelParams + kParamLeftOutput];
        float rightF = vst->paramValues[channelParamOffset + ch*kNumChannelParams + kParamRightOutput];
        
        int FB  = (int)(fbF * 7.999f);   // 3 bits of feedback
        int CON = (conF > 0.5f) ? 1 : 0; // 0=FM, 1=AM
        int LEFT = (leftF > 0.5f) ? 1 : 0;
        int RIGHT = (rightF > 0.5f) ? 1 : 0;

        // For channel ch, the feedback/connect bits live in register 0xC0 + chInBank
        int bank = (ch < 9) ? 0 : 1;
        int chInBank = ch % 9;

        // Left/right output enablers
        unsigned char regC0 = (LEFT << 4) | (RIGHT << 5) | (FB << 1) | CON;
        
        // Create composite register value
        uint16_t regAddr = (bank << 8) | (0xC0 + chInBank);
        OPL3_WriteReg(&vst->chip, regAddr, regC0);
    }
}

// -----------------------------------------------------------------------------
// 6) processReplacing callback: Generate audio from OPL3
// -----------------------------------------------------------------------------
static void processReplacing(AEffect* effect, float** inputs, float** outputs, int32_t sampleFrames)
{
    MyOPL3VST* vst = (MyOPL3VST*)effect->object;

    float* outL = outputs[0];
    float* outR = outputs[1];

    int16_t buffer[2];
    for (int i = 0; i < sampleFrames; i++) {
        // Generate one stereo sample
        OPL3_Generate(&vst->chip, buffer);
        
        outL[i] = (float)buffer[0] / 32768.f;
        outR[i] = (float)buffer[1] / 32768.f;
    }
}

// -----------------------------------------------------------------------------
// 7) MIDI Handling
// -----------------------------------------------------------------------------
static void handleMidiEvent(MyOPL3VST* vst, VstMidiEvent& midiEvent)
{
    // Cast from char* to unsigned char* safely with a local array
    unsigned char data[3];
    data[0] = (unsigned char)midiEvent.midiData[0];
    data[1] = (unsigned char)midiEvent.midiData[1];
    data[2] = (unsigned char)midiEvent.midiData[2];
    
    unsigned char status = data[0] & 0xF0;
    // Save this variable even if unused to avoid the warning
    unsigned char channel = data[0] & 0x0F; 
    unused(channel); // Mark as used to suppress warning
    
    unsigned char d1 = data[1];
    unsigned char d2 = data[2];

    switch (status)
    {
        case 0x90: // note on
        {
            if (d2 > 0) {
                // find a free voice
                for (int i = 0; i < MAX_VOICES; i++) {
                    if (!vst->voices[i].active) {
                        vst->voices[i].active = true;
                        vst->voices[i].midiNote = d1;
                        float freq = 440.f * powf(2.f, (d1 - 69) / 12.f);
                        vst->voices[i].frequency = freq;
                        // Turn on channel i (or voices[i].channelIndex).
                        int ch = vst->voices[i].channelIndex;
                        int bank = (ch < 9) ? 0 : 1;
                        int chInBank = ch % 9;

                        // compute a rough F-Number for OPL3
                        // Pick an appropriate block (octave) based on the MIDI note
                        int block = (d1 / 12) - 1;
                        if (block < 0) block = 0;
                        if (block > 7) block = 7;
                        
                        // This formula is approximate
                        double fNumDouble = freq * (1 << (20 - block)) / vst->sampleRate;
                        int fNum = (int)fNumDouble & 0x3FF; // 10 bits
                        
                        unsigned char lowF = (unsigned char)(fNum & 0xFF);
                        unsigned char highF = (unsigned char)(((fNum >> 8) & 3) | (block << 2) | 0x20); // 0x20 => key on

                        // Create composite register values
                        uint16_t regA0 = (bank << 8) | (0xA0 + chInBank);
                        uint16_t regB0 = (bank << 8) | (0xB0 + chInBank);
                        
                        // Write to A0/B0 regs
                        OPL3_WriteReg(&vst->chip, regA0, lowF);
                        OPL3_WriteReg(&vst->chip, regB0, highF);

                        break;
                    }
                }
            }
            else {
                // velocity=0 => treat as note off
                // handle same as 0x80
                for (int i = 0; i < MAX_VOICES; i++) {
                    if (vst->voices[i].active && vst->voices[i].midiNote == d1) {
                        int ch = vst->voices[i].channelIndex;
                        int bank = (ch < 9) ? 0 : 1;
                        int chInBank = ch % 9;
                        
                        // Read existing value to preserve block/fnum
                        unsigned char highF = 0;
                        // Clear the key-on bit
                        highF &= ~0x20;
                        
                        // Create composite register value
                        uint16_t regB0 = (bank << 8) | (0xB0 + chInBank);
                        OPL3_WriteReg(&vst->chip, regB0, highF);
                        vst->voices[i].active = false;
                    }
                }
            }
            break;
        }
        case 0x80: // note off
        {
            for (int i = 0; i < MAX_VOICES; i++) {
                if (vst->voices[i].active && vst->voices[i].midiNote == d1) {
                    int ch = vst->voices[i].channelIndex;
                    int bank = (ch < 9) ? 0 : 1;
                    int chInBank = ch % 9;

                    // Read existing value to preserve block/fnum
                    unsigned char highF = 0;
                    // Clear the key-on bit
                    highF &= ~0x20;
                    
                    // Create composite register value
                    uint16_t regB0 = (bank << 8) | (0xB0 + chInBank);
                    OPL3_WriteReg(&vst->chip, regB0, highF);
                    vst->voices[i].active = false;
                }
            }
            break;
        }
        case 0xB0: // CC
        {
            // Handle common MIDI CCs here
            switch (d1) {
                case 120: // All Sound Off
                case 123: // All Notes Off
                    for (int i = 0; i < MAX_VOICES; i++) {
                        if (vst->voices[i].active) {
                            int ch = vst->voices[i].channelIndex;
                            int bank = (ch < 9) ? 0 : 1;
                            int chInBank = ch % 9;
                            
                            // Create composite register value
                            uint16_t regB0 = (bank << 8) | (0xB0 + chInBank);
                            OPL3_WriteReg(&vst->chip, regB0, 0);
                            vst->voices[i].active = false;
                        }
                    }
                    break;
                // Add other CC handlers as needed
            }
            break;
        }
        case 0xE0: // Pitch bend
        {
            // TODO: Implement pitch bend by adjusting F-number
            break;
        }
        default:
            // Other MIDI events can be handled here
            break;
    }
}