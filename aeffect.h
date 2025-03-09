// Minimal VST2.4 compatible header
// Based on the VST 2.4 API without requiring the Steinberg SDK

#ifndef __aeffect_h__
#define __aeffect_h__

#include <cstdint>   // Added for fixed-width integer types

#define CCONST(a, b, c, d) \
    ((((int)a) << 24) | (((int)b) << 16) | (((int)c) << 8) | (((int)d) << 0))

// VST2 Plugin magic identifier
#define kEffectMagic CCONST('V', 's', 't', 'P')

// VST2 Opcodes
enum {
    effOpen = 0,
    effClose,
    effSetProgram,
    effGetProgram,
    effSetProgramName,
    effGetProgramName,
    effGetParamLabel,
    effGetParamDisplay,
    effGetParamName,
    effGetVu,
    effSetSampleRate,
    effSetBlockSize,
    effMainsChanged,
    effEditGetRect,
    effEditOpen,
    effEditClose,
    effEditDraw,
    effEditMouse,
    effEditKey,
    effEditIdle,
    effEditTop,
    effEditSleep,
    effIdentify,
    effGetChunk,
    effSetChunk,
    effProcessEvents,
    effCanBeAutomated,
    effString2Parameter,
    effGetNumProgramCategories,
    effGetProgramNameIndexed,
    effCopyProgram,
    effConnectInput,
    effConnectOutput,
    effGetInputProperties,
    effGetOutputProperties,
    effGetPlugCategory,
    effGetCurrentPosition,
    effGetDestinationBuffer,
    effOfflineNotify,
    effOfflinePrepare,
    effOfflineRun,
    effProcessVarIo,
    effSetSpeakerArrangement,
    effSetBlockSizeAndSampleRate,
    effSetBypass,
    effGetEffectName,
    effGetErrorText,
    effGetVendorString,
    effGetProductString,
    effGetVendorVersion,
    effVendorSpecific,
    effCanDo,
    effGetTailSize,
    effIdle,
    effGetIcon,
    effSetViewPosition,
    effGetParameterProperties,
    effKeysRequired,
    effGetVstVersion,
    effEditKeyDown,
    effEditKeyUp,
    effSetEditKnobMode,
    effGetMidiProgramName,
    effGetCurrentMidiProgram,
    effGetMidiProgramCategory,
    effHasMidiProgramsChanged,
    effGetMidiKeyName,
    effBeginSetProgram,
    effEndSetProgram,
    effGetSpeakerArrangement,
    effShellGetNextPlugin,
    effStartProcess,
    effStopProcess,
    effSetTotalSampleToProcess,
    effSetPanLaw,
    effBeginLoadBank,
    effBeginLoadProgram,
    effSetProcessPrecision,
    effGetNumMidiInputChannels,
    effGetNumMidiOutputChannels
};

// VST2 flags
enum {
    effFlagsHasEditor     = 1 << 0,
    effFlagsHasClip       = 1 << 1,
    effFlagsHasVu         = 1 << 2,
    effFlagsCanMono       = 1 << 3,
    effFlagsCanReplacing  = 1 << 4,
    effFlagsProgramChunks = 1 << 5,
    effFlagsIsSynth       = 1 << 8,
    effFlagsNoSoundInStop = 1 << 9,
    effFlagsExtIsAsync    = 1 << 10,
    effFlagsExtHasBuffer  = 1 << 11,
    effFlagsCanDoubleReplacing = 1 << 12
};

// Basic types
#ifdef _WIN32
#include <windows.h>
#else
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef intptr_t intptr;
#define __cdecl
#endif

// VST2 Plugin categories
enum {
    kPlugCategUnknown = 0,
    kPlugCategEffect,
    kPlugCategSynth,
    kPlugCategAnalysis,
    kPlugCategMastering,
    kPlugCategSpacializer,
    kPlugCategRoomFx,
    kPlugCategSurroundFx,
    kPlugCategRestoration,
    kPlugCategOfflineProcess,
    kPlugCategShell,
    kPlugCategGenerator
};

// Forward declarations
struct AEffect;

// Function pointer types
typedef intptr (*audioMasterCallback)(AEffect* effect, int32 opcode, int32 index, intptr value, void* ptr, float opt);
typedef intptr (*AEffectDispatcherProc)(AEffect* effect, int32 opcode, int32 index, intptr value, void* ptr, float opt);
typedef void (*AEffectProcessProc)(AEffect* effect, float** inputs, float** outputs, int32 sampleFrames);
typedef void (*AEffectProcessDoubleProc)(AEffect* effect, double** inputs, double** outputs, int32 sampleFrames);
typedef void (*AEffectSetParameterProc)(AEffect* effect, int32 index, float parameter);
typedef float (*AEffectGetParameterProc)(AEffect* effect, int32 index);

// Main AEffect structure
struct AEffect {
    int32 magic;                    // Must be kEffectMagic
    
    AEffectDispatcherProc dispatcher;
    AEffectProcessProc process;
    AEffectSetParameterProc setParameter;
    AEffectGetParameterProc getParameter;
    
    int32 numPrograms;
    int32 numParams;
    int32 numInputs;
    int32 numOutputs;
    
    int32 flags;
    
    intptr resvd1;
    intptr resvd2;
    
    int32 initialDelay;
    
    int32 realQualities;
    int32 offQualities;
    float ioRatio;
    
    void* object;
    void* user;
    
    int32 uniqueID;
    int32 version;
    
    AEffectProcessProc processReplacing;
    AEffectProcessDoubleProc processDoubleReplacing;
    char future[56];
};

// Plugin entry point function prototype
#ifdef _WIN32
    #define VST_EXPORT __declspec(dllexport)
#else
    #define VST_EXPORT __attribute__ ((visibility ("default")))
#endif

typedef AEffect* (*VSTPluginMainProc)(audioMasterCallback audioMaster);

#ifdef __cplusplus
extern "C" {
#endif
    VST_EXPORT AEffect* VSTPluginMain(audioMasterCallback audioMaster);
#ifdef __cplusplus
}
#endif

// Legacy alias
#define main_plugin VSTPluginMain

#endif // __aeffect_h__
