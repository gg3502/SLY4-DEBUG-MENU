#include <cellstatus.h>
#include <sys/prx.h>
#include <sys/timer.h>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <cell/pad.h>
#include <cell/cell_fs.h>
#include <libpsutil.h>
#include "Tunabledata.h"
#include "Entities.h"
#include <sysutil/sysutil_oskdialog.h>
#include <sysutil\sysutil_oskdialog_ext.h>
#include <sysutil\sysutil_msgdialog.h>
#include <sysutil/sysutil_common.h>
#include <cell/sysmodule.h>
#include <sys/memory.h>

extern "C" void* memcpy(void* dest, const void* src, size_t n)
{
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

extern "C" void* memmove(void* dest, const void* src, size_t n)
{
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    if (d < s) {
        for (size_t i = 0; i < n; i++) d[i] = s[i];
    }
    else {
        for (size_t i = n; i > 0; i--) d[i - 1] = s[i - 1];
    }
    return dest;
}

extern "C" void* memset(void* dest, int value, size_t n)
{
    unsigned char* d = (unsigned char*)dest;
    for (size_t i = 0; i < n; i++) d[i] = (unsigned char)value;
    return dest;
}

struct OPD { void* entry; void* toc; };

static OPD AllocPrim_opd = { (void*)0x5536F4, (void*)0xF50800 };
typedef int (*pfn_AllocPrim)(int kind, unsigned char* rgba, char flag);
pfn_AllocPrim AllocPrim = (pfn_AllocPrim)&AllocPrim_opd;

static OPD QueueText_opd = { (void*)0x553868, (void*)0xF50800 };
typedef int (*pfn_QueueText)(float* pos, int strPtr, unsigned char* color, double scale);
pfn_QueueText QueueText = (pfn_QueueText)&QueueText_opd;

static OPD FormatStr_opd = { (void*)0x5534D0, (void*)0xF50800 };
typedef char* (*pfn_FormatStr)(const char* fmt, ...);
pfn_FormatStr FormatStr = (pfn_FormatStr)&FormatStr_opd;

static OPD HashStr_opd = { (void*)0x56A710, (void*)0xF50800 };
typedef unsigned int (*pfn_HashStr)(const char* name);
pfn_HashStr HashStr = (pfn_HashStr)&HashStr_opd;

typedef int (*pfn_PadGetData)(int port, CellPadData* data);

static char g_PadGetDataHookStorage[sizeof(libpsutil::memory::detour)];
libpsutil::memory::detour* g_PadGetDataHook = nullptr;

typedef int (*pfn_MaterialLookup)(int id);
typedef void (*pfn_BindMaterial)(int context, int material);
typedef void (*pfn_BeginBatch)(int context, void* state);
typedef int (*pfn_AllocVerts)(void* outPtr, int context, int indexCount, int vertexCount);
typedef bool (*pfn_IsButtonPressed)(int playerContext, int buttonCode);
static OPD s_sysTimeOpd = { (void*)0xABF2C4, (void*)0xF50800 };
typedef uint64_t(*sys_time_get_system_time_t)(void);
sys_time_get_system_time_t s_origSysTime = nullptr;

static OPD s_teleportPlayerOpd = { (void*)0x488178, (void*)0xF50800 };
typedef unsigned int (*TeleportPlayer_t)(unsigned int, unsigned int, unsigned int);

static OPD s_hashStringOpd = { (void*)0x56A710, (void*)0xF50800 };
typedef unsigned int (*HashString_t)(const char*);

static OPD s_findByNameOpd = { (void*)0x4D5544, (void*)0xF50800 };

static OPD s_throwCollectibleOpd = { (void*)0x4AED50, (void*)0xF50800 };
typedef int (*ThrowCollectible_t)(void*);

static OPD s_setStateOpd = { (void*)0x261328, (void*)0xF50800 };
typedef int (*SetState_t)(int, unsigned int);

static OPD s_getStrOpd = { (void*)0x4F87C4, (void*)0xF50800 };
typedef const char* (*GetStr_t)(unsigned int, const char*, const char*, int);

static OPD s_startJobByInstOpd = { (void*)0x2E0930, (void*)0xF50800 };
typedef int (*StartJobByInst_t)(int, int, int, char, char, unsigned char, unsigned char, char, int);

typedef int (*FindByName_t)(const char*, int);
FindByName_t FindByName = (FindByName_t)&s_findByNameOpd;

static OPD s_abandonCurrentJobOpd = { (void*)0x2E5060, (void*)0xF50800 };
typedef int (*AbandonCurrentJob_t)(int);

static OPD s_loadLevelOpd = { (void*)0x4674C4, (void*)0xF50800 };
typedef int (*LoadLevel_t)(int, unsigned int, int);

static OPD s_getGoalFromIndexOpd = { (void*)0x2E6A3C, (void*)0xF50800 };
typedef unsigned int (*GetGoalFromIndex_t)(int, unsigned int);

static OPD s_teleportToGoalOpd = { (void*)0x2E0498, (void*)0xF50800 };
typedef int (*TeleportToGoal_t)(int, unsigned int, char, unsigned char);

static OPD s_getScreenSizeOpd = { (void*)0x4D4104, (void*)0xF50800 };
typedef void (*GetScreenSize_t)(float*);

static OPD s_calcRenderMatricesOpd = { (void*)0x5115F0, (void*)0xF50800 };
typedef int (*CalcRenderMatrices_t)(int renderTarget, char a2);
static libpsutil::memory::detour* g_CalcRenderMatricesHook = nullptr;
static char g_CalcRenderMatricesStorage[sizeof(libpsutil::memory::detour)];

static OPD s_setFarZClipOpd = { (void*)0x5202C8, (void*)0xF50800 };
typedef int (*SetFarZClip_t)(int, double);

static OPD s_setSplitScreenModeOpd = { (void*)0x457328 , (void*)0xF50800 };
typedef void* (*SetSplitScreenMode_t)(void*, int);

static OPD s_findInstanceOpd = { (void*)0x4D5544, (void*)0xF50800 };
typedef int (*FindInstance_t)(const char*, int);

static OPD s_spawnByHashOpd = { (void*)0xD6D28,  (void*)0xF50800 };
typedef int (*SpawnByHash_t)(unsigned int hash, void* params);

static OPD s_setScaleOpd = { (void*)0xD7904, (void*)0xF50800 };
typedef void(__fastcall* SetScale_t)(int entity, int scale, unsigned char propagate);

typedef int (*SetJobFailed_t)(unsigned int* episodeMgr, int fakeGoal, unsigned int charIdx);
static OPD s_setJobFailedOpd = { (void*)0x2E5DD8, (void*)0xF50800 };

static OPD s_findJobFailScreenOpd = { (void*)0x365EAC, (void*)0xF50800 };
static OPD s_hudJobFailStartOpd = { (void*)0x36FAD0, (void*)0xF50800 };
typedef int  (*FindJobFailScreen_t)(unsigned int def);
typedef void (*HudJobFailStart_t)(int screen, int a2, int a3, int a4, int msgHi, int msgLo);

static OPD s_setBinocucomVisOpd = { (void*)0x365A54, (void*)0xF50800 };
static OPD s_setCharSlotOpd = { (void*)0x36A5F0, (void*)0xF50800 };

static OPD s_raycastOpd = { (void*)0x4A5000, (void*)0xF50800 };
typedef int (*RaycastGround_t)(int physWorld, float* start, float* end, double length, int flags, float* hitOut, int filterEntity);

static OPD s_calcBBoxOpd = { (void*)0x20E560, (void*)0xF50800 };
typedef void (*CalcBBox_t)(float* aabbOut, int entity, char flag);

static OPD s_findDefOpd = { (void*)0xC9FC8, (void*)0xF50800 };
typedef int (*FindDef_t)(unsigned int hash);

static OPD s_groundRaycastOpd = { (void*)0x1D1E0, (void*)0xF50800 };
typedef int (*GroundRaycast_t)(float* start, float* end, float* hitOut);

typedef void (*EndVertices_t)(int cmdBuf);
static OPD s_endVerticesOpd = { (void*)0x523748, (void*)0xF50800 };
EndVertices_t EndVertices = (EndVertices_t)&s_endVerticesOpd;



typedef int  (*SetBinocucomVis_t)(char visible);
typedef void (*SetCharSlot_t)(int** slotPtr, int* charDef, int parentEntity);

#define SPAWNPARAMS_VTABLE   0xB12260

libpsutil::memory::detour* g_SystemTimeHook;
static char g_SysTimeHookStorage[sizeof(libpsutil::memory::detour)];

libpsutil::memory::detour* g_ButtonCheckHook;
static char g_ButtonCheckHookStorage[sizeof(libpsutil::memory::detour)];

libpsutil::memory::detour* g_IntegratorHook;
static char g_IntegratorHookStorage[sizeof(libpsutil::memory::detour)];

libpsutil::memory::detour* g_CameraUpdatePosHook;
static char g_CameraUpdatePosHookStorage[sizeof(libpsutil::memory::detour)];

typedef float* (*LookAtFn_t)(float*, float*, float*, float*);
libpsutil::memory::detour* g_LookAtHook;
static char g_LookAtHookStorage[sizeof(libpsutil::memory::detour)];

typedef float* (*CalcMatrixFn_t)(int, float*, float*, float*);
libpsutil::memory::detour* g_CalcMatrixHook;
static char g_CalcMatrixHookStorage[sizeof(libpsutil::memory::detour)];

static libpsutil::memory::detour* g_SetActiveHook = nullptr;
static char g_SetActiveStorage[sizeof(libpsutil::memory::detour)];

static libpsutil::memory::detour* g_SetCameraHook = nullptr;
static char g_SetCameraStorage[sizeof(libpsutil::memory::detour)];

typedef int (*SetCamera_t)(int renderTarget, float* viewMat);

static libpsutil::memory::detour* g_Post3DHUDHook = nullptr;
static char g_Post3DHUDStorage[sizeof(libpsutil::memory::detour)];

typedef void (*ToggleCollision_t)(int, int, int);


static OPD MaterialLookup_opd = { (void*)0x523AA8, (void*)0xF50800 };
static OPD BindMaterial_opd = { (void*)0x523310, (void*)0xF50800 };
static OPD BeginBatch_opd = { (void*)0x5218D8, (void*)0xF50800 };
static OPD AllocVerts_opd = { (void*)0x5236A0, (void*)0xF50800 };

pfn_MaterialLookup MaterialLookup = (pfn_MaterialLookup)&MaterialLookup_opd;
pfn_BindMaterial BindMaterial = (pfn_BindMaterial)&BindMaterial_opd;
pfn_BeginBatch BeginBatch = (pfn_BeginBatch)&BeginBatch_opd;
pfn_AllocVerts AllocVerts = (pfn_AllocVerts)&AllocVerts_opd;

typedef void(__fastcall* AddMaterial_t)(void* cmdBuf, int material);
typedef void(__fastcall* AddLtoWMatrix_t)(void* cmdBuf, const float* mtx4x4); // sub_5218D8
typedef void (*BeginVertices_t)(void* outHandle, int cmdBuf, uint8_t primType, unsigned int count);

AddMaterial_t    Sly4_AddMaterial = (AddMaterial_t)0x523310;
AddLtoWMatrix_t  Sly4_AddLtoWMatrix = (AddLtoWMatrix_t)0x5218D8;
BeginVertices_t  Sly4_BeginVertices = (BeginVertices_t)0x5236A0;

void SpawnPropAtPos(unsigned int entityHash, float px, float py, float pz);
void SpawnPropAtPlayer(unsigned int entityHash);
void SpawnEntitySmart(unsigned int entityHash);

#define RENDER_OFFSET 0x1285E4

static volatile bool         g_HasPendingBinocu = false;
static volatile unsigned int g_PendingBinocuLeft = 0;
static volatile unsigned int g_PendingBinocuRight = 0;

void QueueBinocucom(unsigned int leftHash, unsigned int rightHash) {
    g_PendingBinocuLeft = leftHash;
    g_PendingBinocuRight = rightHash;
    g_HasPendingBinocu = true;
}

static bool  g_LevelEditorEnabled = false;
static float g_OrbitPivot[3] = {};   // the point we orbit around
static float g_OrbitYaw = 0.0f;
static float g_OrbitPitch = 0.0f;
static float g_OrbitRadius = 10.0f;
static bool  g_OrbitInitialized = false;

static bool g_SnapToGround = false;
static float g_SpawnRotation[3] = { 0.0f, 0.0f, 0.0f };
static float g_SpawnScale = 1.0f;   // uniform scale
static unsigned int g_SelectedEntityHash = 0;
static char g_SelectedEntityName[64] = {};

// Index of the entity category currently being browsed under the "Entities" menu.
static int g_SelectedEntityCatIndex = -1;

static volatile bool g_HasPendingSceneLoad = false;
static volatile bool g_HasPendingSpawn = false;
static unsigned int   g_PendingSpawnHash = 0;
static bool           g_PendingSpawnSmart = false; // true=SpawnEntitySmart, false=SpawnPropAtPlayer
static int g_SelectedWorldIndex = -1;
static int g_SelectedTypeIndex = -1;

static float g_FreeCamVP[16] = {};
static bool g_FreeCamVPValid = false;
static float g_FreeCamViewMatrix[16] = {};
static bool g_FreeCamViewMatrixValid = false;
static float g_ViewProjMatrix[16] = {};
static bool g_ViewProjValid = false;
static float g_ViewMatrix[16] = {};
static float g_ProjMatrix[16] = {};
static bool g_ViewMatrixValid = false;
static bool g_ProjMatrixValid = false;

static void QueueSpawn(unsigned int hash, bool smart) {
    g_PendingSpawnHash = hash;
    g_PendingSpawnSmart = smart;
    g_HasPendingSpawn = true;
}




struct EntityDefSpawnParams {
    void* vtable;          // +0x00
    uint32_t _zero1[3];       // +0x04..+0x0C
    void* tagArrayPtr;     // +0x10
    uint32_t _pad[3];         // +0x14..+0x1C
    float    m00, m01, m02;   // +0x20,+0x24,+0x28
    float    _pad2;           // +0x2C
    float    m10, m11, m12;   // +0x30,+0x34,+0x38
    float    _pad3;           // +0x3C
    float    m20, m21, m22;   // +0x40,+0x44,+0x48
    float    _pad4;           // +0x4C
    float    posX, posY, posZ; // +0x50,+0x54,+0x58
    float    _pad5;           // +0x5C
    float    scaleX, scaleY, scaleZ; // +0x60,+0x64,+0x68
    uint32_t _pad6;            // +0x6C
    int32_t  zero1, zero2, zero3;    // +0x70,+0x74,+0x78
    float    negOne_a;         // +0x7C
    float    negOne_b;         // +0x80
    uint32_t packedFlags;      // +0x84  <- was 0, now 0x9FE00000
    uint32_t zero4;            // +0x88
    char     name[52];         // +0x8C  (total size 0xC0 = 192)
};

#define ApeDebug_RenderContext (*(int*)0x10DDBB4)
#define ApeDebug_DefaultBatchState (*(unsigned long long*)0x1164BD0)

static volatile uint16_t* const g_capacity = (uint16_t*)0x1013B04;
static volatile uint16_t* const g_count = (uint16_t*)0x1013B06;
static volatile uint32_t* const g_arrayPtr = (uint32_t*)0x1013B0C;

#define ENTITYDEF_HASH_OFFSET   52
#define ENTITYDEF_NAME_OFFSET   56
#define ENTITYDEF_NAME_MAXLEN   64

struct RegistryEntry {
    uint32_t hash;
    uint32_t defPtr;
};

CellPadData g_RealPadData = {};

struct FakeColorTunable {
    unsigned char _pad[72]; // unused — just pushes r/g/b/a to the expected +72 offset
    unsigned char r, g, b, a;
};
static FakeColorTunable g_MenuBgColorTunable = { {0}, 0, 0, 0, 180 }; // matches current hardcoded color

struct ThrowCollectibleParams {
    float posX, posY, posZ;
    float unused;
    float value;
    float field5, field6, field7, field8, field9;
    unsigned char byte10, byte11, byte12;
};

bool g_MenuVisible = false;

static int g_oskMode = 0;  // 0=idle, 1=open, 2=running, 3=close, 4=exit
static char g_oskResult[128] = {};
static char g_oskPrompt[128] = {};

#define OSK_MODE_IDLE    0
#define OSK_MODE_OPEN    1
#define OSK_MODE_RUNNING 2
#define OSK_MODE_CLOSE   3
#define OSK_MODE_EXIT    4

static uint16_t g_oskResultBuffer[CELL_OSKDIALOG_STRING_SIZE + 1] = {};
static CellOskDialogCallbackReturnParam g_oskOutputInfo = {};



static int g_ActiveCameraRenderTarget = 0;
static int g_ActiveCameraEntity = 0;

static bool g_FreeCamEnabled = false;
static float g_FreeCamPos[3] = { 0.0f, 0.0f, 0.0f };
static bool g_FreeCamInitialized = false;

static float g_FreeCamYaw = 0.0f;   // radians
static float g_FreeCamPitch = 0.0f; // radians
const double kOriginalFarZClip = 5000.0; // matches Init's default

static float g_FreeCamMoveSpeed = 0.15f; // was a const, now adjustable

static float g_PrevPitchForwardY = 0;

static float g_LastForwardX = 0.0f, g_LastForwardY = 0.0f, g_LastForwardZ = 1.0f;
static bool g_HasLastForward = false;

static bool g_FlyModeEnabled = false;
static const float kFlySpeed = 0.3f; 


static bool g_HasPrintedThisCategory = false;
int g_CurrentMenuMaxLen = 0;   // widest label in the current display list, computed once per category

const int MENU_WINDOW_SIZE = 20;   // max items drawn per frame — keeps text draws safely under whatever buffer limit was breaking
int g_ScrollOffset = 0;
int g_ScrollOffsetStack[16];   // parallel to g_SelectedIndexStack, for restoring scroll position when backing out

// --- Input edge-detection state ---
int g_LeftHoldFrames = 0;
int g_RightHoldFrames = 0;
bool g_PrevLeft = false;
bool g_PrevRight = false;
bool g_PrevUp = false;
bool g_PrevDown = false;
bool g_PrevL3 = false;
bool g_PrevStart = false;
bool g_PrevSquare = false;
bool g_PrevCross = false;
bool g_PrevCircle = false;
bool g_PrevSpawnCombo = false;

// --- Menu drag-position state ---
static float g_MenuDragX = 0.0f;
static float g_MenuDragY = 0.0f;

static const int   kStickDeadzone = 24;     // ignore drift around center (128)
static const float kMenuMoveSpeed = 6.0f;   // pixels per frame at full stick deflection
static const float kMenuDragClamp = 2500.0f;
static float kDragToTextX = 1.0f;
static float kDragToTextY = 1.0f;
static float kDragToRectX = 0.025f;  // ≈ 1 / 40   (rough starting guess)
static float kDragToRectY = 0.015f;  // ≈ 1 / 66.7 (rough starting guess)



float RECT_LEFT_ANCHOR = -8.8f;
float RECT_TOP_ANCHOR = 5.15f;
float RECT_LINE_HEIGHT = 0.24f;
float RECT_CHAR_WIDTH = 0.20f;
float RECT_PADDING_X = 0.1f;
float RECT_PADDING_Y = 0.05f;
float RECT_CHAR_WIDTH_TEXT = 8.0f;
static const float kRectScaleX = 75.0f;    // = 8.0 / 0.20, computed by hand instead of at runtime
static const float kRectScaleY = 75.6667f; // = 16.0 / 0.24, computed by hand instead of at runtime

#define TUNABLE_REGISTRY_ARRAY 0x10C2DBCu
#define TUNABLE_REGISTRY_COUNT 0x10C597Cu

#define VTABLE_BOOL   0x00B0D934u
#define VTABLE_INT    0x00B0D968u
#define VTABLE_FLOAT  0x00B0D99Cu
#define VTABLE_VEC3   0x00B0D9D0u
#define VTABLE_VEC2   0x00B13940u
#define VTABLE_STRING 0x00B0DA38u
#define VTABLE_COLOR 0x00B0DA04u
#define VTABLE_STRING_HASH 0x00B0DA6Cu

#define PLAYER_COINS_LIVEADDR (0xF65CA0 + 4056 * 0 + 112 - 72)

const char* g_CanonicalRootCategories[] = {
    "AI", "Animation", "ArtViewer", "Audio", "Camera", "Cheat", "Debug",
    "Entity", "EntityComp", "FX", "Game", "Goal", "Havok", "Hud",
    "LevelEditor", "Mechanic", "Radar", "UI", "Control"
};
const int g_CanonicalRootCategoryCount = 19;

enum TunableType { TT_UNKNOWN, TT_BOOL, TT_INT, TT_FLOAT, TT_VEC2, TT_VEC3, TT_STRING, TT_COLOR, TT_STRING_HASH, TT_ACTION};

static const char* kSaveDir = "/dev_hdd0/tmp/Sly4DebugMenu";
static const char* kCfgPath = "/dev_hdd0/tmp/Sly4DebugMenu/debugmenu.cfg";
static int g_SavedTunableCount = 0;

#define MAX_SAVED_ENTITIES 256

struct SavedEntity {
    unsigned int hash;
    float posX, posY, posZ;
    float rotX, rotY, rotZ;
    float scale;
};

static SavedEntity g_SavedEntities[MAX_SAVED_ENTITIES];
static int         g_SavedEntityCount = 0;
static char        g_ScenePath[128] = {};
static bool g_IsLoadingScene = false;
static char        g_DisplaySceneName[64] = {};


void DrawOrbitPivotWith3D(float* cmdBuf);
void DrawOrbitPivot();
int StrLen(const char* s);
static int BuildAndSpawn(unsigned int entityHash, float px, float py, float pz);
static bool EnsureSaveDirectory(const char* path);
void GetCurrentSceneName(char* outName, int maxLen);
bool LoadSceneFile();
void SaveSceneFile();
void BuildScenePath();
void LoadSaveFile();
void Action_DeleteCfgFile();
void Action_ReloadCfgFile();

static float g_CameraFwdX = 0, g_CameraFwdZ = 0;
static float g_CameraRightX = 0.0f, g_CameraRightZ = 0.0f;

unsigned int FindLiveTunable(const char* name)
{
    unsigned int hash = HashStr(name);
    unsigned int* registry = (unsigned int*)TUNABLE_REGISTRY_ARRAY;
    unsigned int count = *(unsigned int*)TUNABLE_REGISTRY_COUNT;

    for (unsigned int i = 0; i < count; i++) {
        unsigned int structAddr = registry[i];
        unsigned int structHash = *(unsigned int*)(structAddr + 68);
        if (structHash == hash) return structAddr;
    }
    return 0;
}

struct GridVertex {
    float   x, y, z;
    uint8_t color[4];
    float   pad1, pad2;
};

#define GAMEMGR_RENDERSTATE_OFFSET 0x1285E4


static int g_DebugFrameCounter = 0;

static const char* g_EpisodeKeys[] = {
    "ParisPrologueTitleName",
    "JapanTitleName",
    "WestTitleName",
    "IceAgeTitleName",
    "EnglandTitleName",
    "ArabiaTitleName",
    "ParisEpilogueTitleName",
    "MinigameTitleName"
};
static const int g_EpisodeCount = 8;

static const char* g_JapanJobKeys[] = {
    "PhotoOpTitleName", "BreakoutTitleName", "SushiStartupTitleName", "FishyTitleName",
    "PrettyPinkTitleName", "MadameGeishaTitleName", "SpikedSushiTitleName", "OpAltSickTitleName"
};
static const int g_JapanJobCount = 8;

static const char* g_ArabianJobKeys[] = {
    "LostAndFoundTitleName",
    "SpecialDeliveryTitleName",
    "CrazyClimberTitleName",
    "TaxiDriverTitleName",
    "UpInSmokeTitleName",
    "RockAndRollTitleName",
    "AllRolledUpTitleName",
};
static const int g_ArabianJobCount = 7;

static const char* g_EnglandJobKeys[] = {
    "ShoppingSpreeTitleName",
    "JugglingActTitleName",
    "CaneSwipeTitleName",
    "RobbinHoodsTitleName",
    "MechanicalMenaceTitleName",
    "ShortSupplyTitleName",
    "ShellShockedTitleName",
    "HardTargetTitleName",
    "OperationMousetrapTitleName",
};
static const int g_EnglandJobCount = 9;

static const char* g_IceAgeJobKeys[] = {
    "StoneAgeReconnaissanceTitleName",
    "CavemanCoopersRescueTitleName",
    "GettingStrongerTitleName",
    "GoingUpTitleName",
    "HungryHungryHippoTitleName",
    "IceIceBentleyTitleName",
    "DutyCallsTitleName",
    "OperationJurassicThieveryTitleName",
};
static const int g_IceAgeJobCount = 8;

static const char* g_MiniGameJobKeys[] = {
    "AlterEgo06TitleName",
    "AlterEgo06TitleName",
    "AlterEgo07TitleName",
    "AlterEgo07TitleName",
    "AlterEgo08TitleName",
    "SparkRunner06TitleName",
    "SystemCracker08TitleName",
    "SystemCracker09TitleName",
    "SystemCracker09TitleName",
    "SparkRunner01TitleName",
    "SparkRunner02TitleName",
    "SparkRunner03TitleName",
    "SparkRunner04TitleName",
    "SparkRunner05TitleName",
    "SparkRunner07TitleName",
    "AlterEgo01TitleName",
    "AlterEgo02TitleName",
    "AlterEgo03TitleName",
    "AlterEgo04TitleName",
    "AlterEgo05TitleName",
    "SystemCracker01TitleName",
    "SystemCracker02TitleName",
    "SystemCracker03TitleName",
    "SystemCracker04TitleName",
    "SystemCracker05TitleName",
    "SystemCracker07TitleName",
};
static const int g_MiniGameJobCount = 26;

static const char* g_ParisPrologueJobKeys[] = {
    "PrologueTitleName",
};
static const int g_ParisPrologueJobCount = 1;

static const char* g_ParisEpilogueJobKeys[] = {
    "EpilogueTitleName",
};
static const int g_ParisEpilogueJobCount = 1;

static const char* g_WestJobKeys[] = {
    "UnderArrestTitleName",
    "JailhouseBluesTitleName",
    "ToothpicksVaultTitleName",
    "SaloonBugTitleName",
    "BlindDateTitleName",
    "JailBreakTitleName",
    "GrandKeyLarcenyTitleName",
    "OperationGoldDiggerTitleName",
};
static const int g_WestJobCount = 8;

struct JobEntry {
    int globalIndex;
    const char* jobKey;
    int episodeIndex;
};

static const JobEntry g_AllJobs[] = {
    { 0,  "PrologueTitleName",              0 },
    { 1,  "PhotoOpTitleName",               1 },
    { 2,  "BreakoutTitleName",              1 },
    { 3,  "SushiStartupTitleName",          1 },
    { 4,  "FishyTitleName",                 1 },
    { 5,  "PrettyPinkTitleName",            1 },
    { 6,  "MadameGeishaTitleName",          1 },
    { 7,  "SpikedSushiTitleName",           1 },
    { 8,  "OpAltSickTitleName",             1 },
    { 9,  "UnderArrestTitleName",           2 },
    { 10, "JailhouseBluesTitleName",        2 },
    { 11, "ToothpicksVaultTitleName",       2 },
    { 12, "SaloonBugTitleName",             2 },
    { 13, "BlindDateTitleName",             2 },
    { 14, "JailBreakTitleName",             2 },
    { 15, "GrandKeyLarcenyTitleName",       2 },
    { 16, "OperationGoldDiggerTitleName",   2 },
    { 17, "StoneAgeReconnaissanceTitleName",3 },
    { 18, "CavemanCoopersRescueTitleName",  3 },
    { 19, "GettingStrongerTitleName",       3 },
    { 20, "GoingUpTitleName",               3 },
    { 21, "HungryHungryHippoTitleName",     3 },
    { 22, "IceIceBentleyTitleName",         3 },
    { 23, "DutyCallsTitleName",             3 },
    { 24, "OperationJurassicThieveryTitleName", 3 },
    { 25, "ShoppingSpreeTitleName",         4 },
    { 26, "JugglingActTitleName",           4 },
    { 27, "CaneSwipeTitleName",             4 },
    { 28, "RobbinHoodsTitleName",           4 },
    { 29, "MechanicalMenaceTitleName",      4 },
    { 30, "ShortSupplyTitleName",           4 },
    { 31, "ShellShockedTitleName",          4 },
    { 32, "HardTargetTitleName",            4 },
    { 33, "OperationMousetrapTitleName",    4 },
    { 34, "LostAndFoundTitleName",          5 },
    { 35, "SpecialDeliveryTitleName",       5 },
    { 36, "CrazyClimberTitleName",          5 },
    { 37, "TaxiDriverTitleName",            5 },
    { 38, "UpInSmokeTitleName",             5 },
    { 39, "RockAndRollTitleName",           5 },
    { 40, "AllRolledUpTitleName",           5 },
    { 41, "EpilogueTitleName",              6 },
    { 42, "AlterEgo06TitleName",            7 },
    { 43, "AlterEgo06TitleName",            7 },
    { 44, "AlterEgo07TitleName",            7 },
    { 45, "AlterEgo07TitleName",            7 },
    { 46, "AlterEgo08TitleName",            7 },
    { 47, "SparkRunner06TitleName",         7 },
    { 48, "SystemCracker08TitleName",       7 },
    { 49, "SystemCracker09TitleName",       7 },
    { 50, "SystemCracker09TitleName",       7 },
    { 51, "SparkRunner01TitleName",         7 },
    { 52, "SparkRunner02TitleName",         7 },
    { 53, "SparkRunner03TitleName",         7 },
    { 54, "SparkRunner04TitleName",         7 },
    { 55, "SparkRunner05TitleName",         7 },
    { 56, "SparkRunner07TitleName",         7 },
    { 57, "AlterEgo01TitleName",            7 },
    { 58, "AlterEgo02TitleName",            7 },
    { 59, "AlterEgo03TitleName",            7 },
    { 60, "AlterEgo04TitleName",            7 },
    { 61, "AlterEgo05TitleName",            7 },
    { 62, "SystemCracker01TitleName",       7 },
    { 63, "SystemCracker02TitleName",       7 },
    { 64, "SystemCracker03TitleName",       7 },
    { 65, "SystemCracker04TitleName",       7 },
    { 66, "SystemCracker05TitleName",       7 },
    { 67, "SystemCracker07TitleName",       7 },
};
static const int g_AllJobsCount = 68;

struct EpisodeJobList {
    const char** jobKeys;
    int jobCount;
};

static const EpisodeJobList g_EpisodeJobLists[] = {
    { g_ParisPrologueJobKeys, g_ParisPrologueJobCount }, // episode 0
    { g_JapanJobKeys,         g_JapanJobCount },          // episode 1
    { g_WestJobKeys,          g_WestJobCount },            // episode 2
    { g_IceAgeJobKeys,        g_IceAgeJobCount },          // episode 3
    { g_EnglandJobKeys,       g_EnglandJobCount },         // episode 4
    { g_ArabianJobKeys,       g_ArabianJobCount },         // episode 5
    { g_ParisEpilogueJobKeys, g_ParisEpilogueJobCount },   // episode 6
    { g_MiniGameJobKeys,      g_MiniGameJobCount },        // episode 7
};

struct JobGoalList {
    const char* jobKey;
    const char** goals;
    int goalCount;
};



static const char* g_Goals_AllRolledUp[] = {
    "Chalk Talk: Operation All Rolled Up",  // 0
    "Enter Lamp Shop",  // 1
    "IGC: Belly Outfit",  // 2
    "IGC: Bellydance Intro",  // 3
    "Bellydance MiniGame!",  // 4
    "Mouse Car:Follow the Guard to the Control Room",  // 5
    "IGC: Hey Guards!",  // 6
    "MouseCar: Lead the Guards to the Air Duct",  // 7
    "IGC: Confused Guards",  // 8
    "RCCar: Survive the Air Duct and disable the Laser Door",  // 9
    "IGC: Laser Wall Deactivated",  // 10
    "Hack the Control Room Terminal",  // 11
    "Minigame: Reach the Goal before time runs out",  // 12
    "win",  // 13
    "IGC: Red Alert",  // 14
    "Throw Enemies Into the Laser Turrets",  // 15
    "IGC: Confront Paradox",  // 16
    "Decibel Fight Start",  // 17
    "IGC: Decibel Blast Off 00",  // 18
    "Decibel Glass Fight",  // 19
    "Decibel Platform 1",  // 20
    "Decibel Music Fight",  // 21
    "Decibel Platform 2",  // 22
    "Get to the next platform",  // 23
    "Decibel Glass Fight2",  // 24
    "Decibel Platform 3",  // 25
    "Decibel Music Fight2",  // 26
    "IGC: Decibel Blast Off 04",  // 27
    "Final Fight Decibel",  // 28
    "IGC: Back To The Future",  // 29
};
static const int g_GoalCount_AllRolledUp = 30;

static const char* g_Goals_AlterEgo01[] = {
    "Reach the CPU Core",  // 0
    "Blow up the Core!",  // 1
    "WinAlterEgo",  // 2
};
static const int g_GoalCount_AlterEgo01 = 3;

static const char* g_Goals_AlterEgo02[] = {
    "Reach the CPU Core",  // 0
    "Blow up the Core!",  // 1
    "WinAlterEgo",  // 2
};
static const int g_GoalCount_AlterEgo02 = 3;

static const char* g_Goals_AlterEgo03[] = {
    "Reach the CPU Core",  // 0
    "Blow up the Core!",  // 1
    "WinAlterEgo",  // 2
};
static const int g_GoalCount_AlterEgo03 = 3;

static const char* g_Goals_AlterEgo04[] = {
    "Reach the CPU Core",  // 0
    "Blow up the Core!",  // 1
    "WinAlterEgo",  // 2
};
static const int g_GoalCount_AlterEgo04 = 3;

static const char* g_Goals_AlterEgo05[] = {
    "Reach the CPU Core",  // 0
    "Blow up the Core!",  // 1
    "WinAlterEgo",  // 2
};
static const int g_GoalCount_AlterEgo05 = 3;

static const char* g_Goals_AlterEgo06[] = {
    "Reach the Portal 06A",  // 0
    "Reach the Portal 06A_Main",  // 1
    "Enter the Portal! 06X",  // 2
    "win",  // 3
    "Enter the Portal! 06A",  // 4
    "win",  // 5
    "Reach the Portal 06B",  // 6
    "Enter the Portal! 06B",  // 7
    "win",  // 8
    "Reach the Portal 06C",  // 9
    "Enter the Portal! 06C",  // 10
    "win",  // 11
    "Reach the CPU Core",  // 12
    "Blow up the Core!",  // 13
    "WinAlterEgo",  // 14
};
static const int g_GoalCount_AlterEgo06 = 15;

static const char* g_Goals_AlterEgo06Multi[] = {
    "Reach the Portal 06A (2 player)",  // 0
    "Reach the Portal 06A (2 player)_Main",  // 1
    "Enter the Portal! 06X",  // 2
    "win",  // 3
    "Enter the Portal! 06A",  // 4
    "win",  // 5
    "Reach the Portal 06B",  // 6
    "Enter the Portal! 06B",  // 7
    "win",  // 8
    "Reach the Portal 06C",  // 9
    "Enter the Portal! 06C",  // 10
    "win",  // 11
    "Reach the CPU Core",  // 12
    "Blow up the Core!",  // 13
    "WinAlterEgo",  // 14
};
static const int g_GoalCount_AlterEgo06Multi = 15;

static const char* g_Goals_AlterEgo06xMulti[] = {
    "Reach the Portal",  // 0
    "Enter the Portal!",  // 1
    "WinAlterEgo",  // 2
};
static const int g_GoalCount_AlterEgo06xMulti = 3;

static const char* g_Goals_AlterEgo07[] = {
    "Reach the Portal 07A",  // 0
    "Reach the Portal 07A Main",  // 1
    "Enter the Portal! 07A",  // 2
    "win",  // 3
    "Reach the Portal 07B",  // 4
    "Enter the Portal! 07X",  // 5
    "Enter the Portal! 07B",  // 6
    "Reach the Portal 07C",  // 7
    "Enter the Portal! 07C",  // 8
    "Reach the CPU Core 07D",  // 9
    "Blow up the Core! 07D",  // 10
    "WinAlterEgo",  // 11
};
static const int g_GoalCount_AlterEgo07 = 12;

static const char* g_Goals_AlterEgo07Multi[] = {
    "Reach the Portal 07A (2 player)",  // 0
    "Enter the Portal! 07A",  // 1
    "win",  // 2
    "Reach the Portal 07B",  // 3
    "Enter the Portal! 07X",  // 4
    "Enter the Portal! 07B",  // 5
    "Reach the Portal 07C",  // 6
    "Enter the Portal! 07C",  // 7
    "Reach the CPU Core 07D",  // 8
    "Blow up the Core! 07D",  // 9
    "WinAlterEgo",  // 10
};
static const int g_GoalCount_AlterEgo07Multi = 11;

static const char* g_Goals_AlterEgo07xMulti[] = {
    "Reach the Portal",  // 0
    "Enter the Portal!",  // 1
};
static const int g_GoalCount_AlterEgo07xMulti = 2;

static const char* g_Goals_AlterEgo08[] = {
    "Alter Ego: Reach the CPU Core",  // 0
    "Alter Ego: Reach the CPU Core Main",  // 1
    "Alter Ego: Destroy the CPU Core",  // 2
    "WinAlterEgo",  // 3
    "Spark Runner: Reach the Goal before the timer runs out",  // 4
    "win",  // 5
    "System Cracker: Reach the Docking Gate",  // 6
    "win",  // 7
};
static const int g_GoalCount_AlterEgo08 = 8;

static const char* g_Goals_BlindDate[] = {
    "Blind Date Start",  // 0
    "Follow Toothpick to the Canyon",  // 1
    "Follow that Stagecoach",  // 2
    "Lower the first bridge",  // 3
    "Get to the Jackalope TNT area",  // 4
    "Clear the path for the Stagecoach",  // 5
    "Protect the stagecoach from the Jackalopes",  // 6
    "Get to the Furnaces",  // 7
    "Protect the Stagecoach and knock down the bridge",  // 8
    "Get to the Train Graveyard",  // 9
    "Fight off the Train Graveyard Jackalopes",  // 10
    "Protect the Stagecoach in the Train Graveyard",  // 11
    "Catch up to the Stagecoach again",  // 12
    "Protect the Stagecoach from traps",  // 13
    "Get to the end",  // 14
    "Blind Date End",  // 15
    "IGC: Carmelita Rescue",  // 16
};
static const int g_GoalCount_BlindDate = 17;

static const char* g_Goals_Breakout[] = {
    "Collect the Armor Pieces",  // 0
    "Enter the Prison",  // 1
    "Meet Rioichi",  // 2
    "Sneak under table",  // 3
    "Use the Samurai Armor as a disguise",  // 4
    "Use the Samurai Armor to Block Fire",  // 5
    "Use the Samurai Armor to Block Fire Part Deux",  // 6
    "Navigate through the Prison Traps",  // 7
    "Steal the Key",  // 8
    "Open the Gate",  // 9
    "Cross the Bridge",  // 10
    "Free Rioichi!",  // 11
};
static const int g_GoalCount_Breakout = 12;

static const char* g_Goals_CaneSwipe[] = {
    "Job Start IGC / Binocucom",  // 0
    "Take a photo of the Black Knight",  // 1
    "Take a picture of the cane case",  // 2
    "Take out the Black Knight's guards",  // 3
    "Hack the cane case",  // 4
    "Minigame: Reach the CPU Core",  // 5
    "Minigame: Destroy the CPU Core",  // 6
    "WinAlterEgo",  // 7
    "Collect Sir Galleth's cane",  // 8
};
static const int g_GoalCount_CaneSwipe = 9;

static const char* g_Goals_CavemanCoopersRescue[] = {
    "Job Start IGC",  // 0
    "Sneak into the Arena",  // 1
    "Enter the crawl space",  // 2
    "Approach the Bridge",  // 3
    "IGC: Rescue Eavesdrop",  // 4
    "Sneak above the Arena Gate",  // 5
    "IGC: Grizzle B Intro",  // 6
    "Head to the Arena Viewing Chamber",  // 7
    "IGC: Grizzle Painting",  // 8
    "Steal Bismarque's Lair Key",  // 9
    "IGC: GrabKey",  // 10
    "Open the Gate",  // 11
    "IGC: Open Ice Gate",  // 12
    "Acquire the Sabertooth Costume",  // 13
    "IGC: Costume Collect",  // 14
    "Walk to Pounce the Post",  // 15
    "Pounce the Post Tutorial",  // 16
    "Pounce the Post",  // 17
    "Sneak Past Fighters",  // 18
    "Pounce the Guard",  // 19
    "IGC: Cane Swipe",  // 20
    "IGC: Rescue_BIN_Pounce",  // 21
    "Free Caveman Cooper",  // 22
    "IGC: Caveman Outro",  // 23
};
static const int g_GoalCount_CavemanCoopersRescue = 24;

static const char* g_Goals_ClosedForRemodeling[] = {
    "Job Start Binocucom",  // 0
    "Enter the Blacksmith",  // 1
    "Pre-Entrance VO",  // 2
    "IGC Blacksmith Alarms",  // 3
    "Destroy the control consoles! (room 1)",  // 4
    "Go to room 2!",  // 5
    "Destroy the control consoles! (room 2)",  // 6
    "Go to room 3!",  // 7
    "Destroy the control consoles! (room 3)",  // 8
    "Job Complete",  // 9
};
static const int g_GoalCount_ClosedForRemodeling = 10;

static const char* g_Goals_CrazyClimber[] = {
    "Carpet Shop IGC",  // 0
    "Go to the Carpet Shop",  // 1
    "SuperClimb Tutorial",  // 2
    "Bino: Backroom Reveal",  // 3
    "Cobra Climb",  // 4
    "Break Rope",  // 5
    "Reach Base of Tower",  // 6
    "Enter Tower",  // 7
    "Hatch CLose IGC",  // 8
    "Use Mechanism",  // 9
    "Snake Plat",  // 10
    "Tower Checkpoint01",  // 11
    "Tower Checkpoint01A",  // 12
    "Tower Checkpoint02",  // 13
    "Tower Checkpoint02A",  // 14
    "Tower Checkpoint03",  // 15
    "Tower Checkpoint03A",  // 16
    "Tower Checkpoint04",  // 17
    "Tower Checkpoint05",  // 18
    "Enter Penthouse",  // 19
    "Minigame: Hack the Garage Terminal",  // 20
    "Minigame: Reach the Goal before time runs out",  // 21
    "win",  // 22
    "Rescue the Thief CutScene",  // 23
};
static const int g_GoalCount_CrazyClimber = 24;

static const char* g_Goals_DutyCalls[] = {
    "IGC: Duty Calls Start",  // 0
    "Follow Grizz",  // 1
    "Follow Griz (Start call 1)",  // 2
    "Follow Griz (After call 1)",  // 3
    "Follow Griz (Start call 2)",  // 4
    "Follow Griz (After call 2)",  // 5
    "Follow Griz (Start call 3)",  // 6
    "Follow Griz (After call 3)",  // 7
    "Follow Griz (Start call 4)",  // 8
    "Follow Griz (After call 4)",  // 9
    "Enter Arena Follow Grizzle B",  // 10
};
static const int g_GoalCount_DutyCalls = 11;

static const char* g_Goals_Epilogue[] = {
    "Chalk Talk: Epilogue",  // 0
    "Sly Enters Blimp IGC",  // 1
    "Sly: Blimp Start",  // 2
    "Sly Carmelita Captured IGC",  // 3
    "Rioichi: Get Cane Start",  // 4
    "Rio Cane Recover IGC",  // 5
    "Big Brawl 1",  // 6
    "Hallway 1 IGC",  // 7
    "Caveman Cane Path IGC",  // 8
    "Rioichi: Recover Caveman Cane",  // 9
    "Rio Leaves IGC",  // 10
    "Caveman: Recover Salim Cane Part 1",  // 11
    "Caveman: Recover Salim Cane Part 2",  // 12
    "Caveman Leaves IGC",  // 13
    "Big Brawl 2",  // 14
    "Hallway 2 IGC",  // 15
    "Galleth Cane Path IGC",  // 16
    "Salim: Recover Galleth Cane",  // 17
    "Salim Leaves IGC",  // 18
    "Kid Cane Path IGC",  // 19
    "Sir Galleth: Destroy Fuse Set 1",  // 20
    "Sir Galleth: Destroy Fuse Set 2",  // 21
    "Sir Galleth: Recover Kid Cooper Cane",  // 22
    "Galleth Leaves IGC",  // 23
    "Big Brawl 3",  // 24
    "Hallway 3 IGC",  // 25
    "Kid Boss Path IGC",  // 26
    "Kid Cooper: Start",  // 27
    "Kid Cooper: Spin Platforms",  // 28
    "Kid Cooper: Boss Door IGC",  // 29
    "Kid Cooper: Boss Door Puzzle",  // 30
    "Boss Intro IGC",  // 31
    "Boss Fight Round 1",  // 32
    "Boss Fight Round 2",  // 33
    "Blimp Going Down IGC",  // 34
    "Boss Fight Round 3",  // 35
    "Boss Fight Round 3 IGC",  // 36
    "Boss Fight Fencing 3",  // 37
    "Paradox Defeated IGC",  // 38
};
static const int g_GoalCount_Epilogue = 39;

static const char* g_Goals_Fishy[] = {
    "Play Cutscene Caves Exterior",  // 0
    "Minigame: Reach the CPU Core",  // 1
    "Minigame: Destroy the CPU Core",  // 2
    "WinAlterEgo",  // 3
    "Enter The Cave",  // 4
    "Bentley goes to the gate",  // 5
    "Check out the first cave",  // 6
    "Binocucom:Bentley tells Murray to hit the plant",  // 7
    "Follow Markers to Fishing Spot",  // 8
    "Open Door 04",  // 9
    "Cross over the Lily Pads",  // 10
    "Get to Rickety Bridge!",  // 11
    "Get to Fishing Spot!",  // 12
    "Catch Fish!",  // 13
    "Get the fish back to the cart while it is still fresh!",  // 14
    "Get the fish back to the final cart",  // 15
};
static const int g_GoalCount_Fishy = 16;

static const char* g_Goals_GettingStronger[] = {
    "Talk to Murray",  // 0
    "IGC: Intro",  // 1
    "Train with Murray",  // 2
    "You think you bad?!",  // 3
};
static const int g_GoalCount_GettingStronger = 4;

static const char* g_Goals_GoingUp[] = {
    "IGC: Intro",  // 0
    "Defeat the Dodo Birds",  // 1
    "IGC: MadMurray",  // 2
    "Climb the Mountain",  // 3
    "Wall Smash",  // 4
    "Wall Leap",  // 5
    "Wall Slide",  // 6
    "Enter the Pipe",  // 7
    "IGC: CavemanOpening",  // 8
    "IGC: BINO Rockcliff intro",  // 9
    "Find the Temporal Sprocket",  // 10
    "Reach the Ice Wall",  // 11
    "Avoid the Eggs!",  // 12
    "Continue up the mountain",  // 13
    "Grab Van Piece",  // 14
    "IGC: PteryFakey",  // 15
    "IGC: HatcheryIntro",  // 16
    "Climb but don't wake the babies",  // 17
    "Climb but don't wake the babies 2",  // 18
    "Climb but don't wake the babies 3",  // 19
    "Climb but don't wake the babies 4",  // 20
    "IGC: Outro",  // 21
};
static const int g_GoalCount_GoingUp = 22;

static const char* g_Goals_GrandKeyLarceny[] = {
    "Chalk Talk: Grand Key Larceny!",  // 0
    "Find the Secret Boxing Arena!",  // 1
    "IGC: Boxing Start",  // 2
    "Beat Up 20 Coyotes!",  // 3
    "IGC: Brawl Champ",  // 4
    "Defeat LeMoo!",  // 5
    "IGC: Boxing Finish",  // 6
    "Start the Ball Race IGC",  // 7
    "Get to the Finish!",  // 8
    "Race Finish IGC",  // 9
    "Start the Shooting Match IGC",  // 10
    "Start Shooting!",  // 11
    "Win the Shooting Match IGC",  // 12
    "IGC: Grand Key Larceny Completed!",  // 13
};
static const int g_GoalCount_GrandKeyLarceny = 14;

static const char* g_Goals_HardTarget[] = {
    "IGC Hard Target Setup",  // 0
    "Gather three Fire Bulbs",  // 1
    "Go to the archery game",  // 2
    "IGC Carnival Game",  // 3
    "Shoot the Targets",  // 4
};
static const int g_GoalCount_HardTarget = 5;

static const char* g_Goals_HungryHungryHippo[] = {
    "IGC: Binocucom Hippo",  // 0
    "Squeeze the Penguins",  // 1
    "Play the final VO",  // 2
};
static const int g_GoalCount_HungryHungryHippo = 3;

static const char* g_Goals_IceIceBentley[] = {
    "IGC: Intro",  // 0
    "Find the Grizz",  // 1
    "Locate Bismarque in the art room",  // 2
    "Sculpture Room Intro",  // 3
    "Solve the Sculpture Room puzzle",  // 4
    "Sculpture Room Outro",  // 5
    "Sculpture Room Outro Close Door",  // 6
    "Navigate the tunnel and avoid the egg",  // 7
    "Art Gallery Intro",  // 8
    "Collect the gems from the guards",  // 9
    "Use the gems to open the door",  // 10
    "Art Gallery Outro",  // 11
    "Locate Grizz through the tunnel",  // 12
    "Burial Room Intro",  // 13
    "Tag Grizz",  // 14
    "Burial Room Outro",  // 15
};
static const int g_GoalCount_IceIceBentley = 16;

static const char* g_Goals_JailBreak[] = {
    "IGC: Raft Start Down the River",  // 0
    "IGC: JailBreakStart",  // 1
    "Get to the first dock!",  // 2
    "Clear Out the First Dock!",  // 3
    "IGC: exit to dock A",  // 4
    "Get to the first dock Lock panel!",  // 5
    "IGC: JailBreak_IGC_DockAOpen",  // 6
    "Get Back to the raft after first lock!",  // 7
    "Get to the second dock!",  // 8
    "Protect the Raft From Jackelopes!",  // 9
    "IGC: exit to dock B",  // 10
    "Get to the second dock Lock panel!",  // 11
    "IGC: JailBreak_IGC_DockBOpen",  // 12
    "Get Back to the raft after second lock!",  // 13
    "Get to the Prison dock!",  // 14
    "IGC: exit to dock C",  // 15
    "Get to the Granary!",  // 16
    "IGC: Rescue Gang Enter",  // 17
    "Rescue the team!",  // 18
    "IGC: Rescue Gang Exit",  // 19
};
static const int g_GoalCount_JailBreak = 20;

static const char* g_Goals_JailhouseBlues[] = {
    "Get to location for picture of the Presidio Tower",  // 0
    "Bentley takes picture of Presidio Tower",  // 1
    "Get to location for picture of the Presidio Arsenal",  // 2
    "Bentley takes picture of Presidio Arsenal",  // 3
    "Get to location for picture of the Presidio Wall",  // 4
    "Bentley takes picture of Presidio Wall",  // 5
    "IGC: Sly Meets Tennessee Kid Cooper",  // 6
    "Destroy Prison Wall",  // 7
    "Get Out of the Tower!",  // 8
    "IGC: Show the Tower Bottom",  // 9
    "Help Tennessee escape!",  // 10
    "What is Tennessee looking at?",  // 11
    "First Block Puzzle",  // 12
    "Get to Tnt",  // 13
    "IGC: Kid Cooper picks up the TNT!",  // 14
    "Get Across the water",  // 15
    "IGC: Granary Start",  // 16
    "Granary Block Puzzle",  // 17
    "IGC: Granary Puzzle finished",  // 18
    "Get To Fire Hatch Alley",  // 19
    "IGC: Fire Hatch Alley",  // 20
    "Get To Fourth Block Puzzle End",  // 21
    "Catch up to Tennessee",  // 22
    "What is that strange Glow?",  // 23
    "IGC: Roll Ball Hall Start",  // 24
    "Use the Convict Suit Ball Roll",  // 25
    "IGC: Ball Roll End",  // 26
    "Get to shuffle puzzle",  // 27
    "IGC: Intro the Yard",  // 28
    "Solve the Shuffle Puzzle",  // 29
    "Get to the Presidio Back Wall!",  // 30
    "IGC: Presidio End",  // 31
    "Disable the Laser Fields!",  // 32
    "IGC: Disable the Laser Fields",  // 33
    "Push the final block!",  // 34
    "Plant TNT at crack",  // 35
    "IGC: Sly and Tennessee ending jump cutscene",  // 36
};
static const int g_GoalCount_JailhouseBlues = 37;

static const char* g_Goals_JugglingAct[] = {
    "Job Start Binocucom",  // 0
    "Climb the Tower",  // 1
    "Switch to the Archer Costume",  // 2
    "Grab an arrow",  // 3
    "Shoot the target",  // 4
    "Enter the circus tent",  // 5
    "CircusTent Intro (Binoccucom)",  // 6
    "Find Sir Galleth (tent start)",  // 7
    "Find Sir Galleth (triple rings)",  // 8
    "Find Sir Galleth (trampoline)",  // 9
    "Find Sir Galleth (frog set 1)",  // 10
    "Find Sir Galleth (trapeze set 1)",  // 11
    "Find Sir Galleth (cannon set 1)",  // 12
    "Find Sir Galleth (trapeze set 2)",  // 13
    "Find Sir Galleth (cannon set 2)",  // 14
    "Find Sir Galleth (frog set 2)",  // 15
    "Find Sir Galleth (dragon)",  // 16
    "Find Sir Galleth (rescue galleth)",  // 17
};
static const int g_GoalCount_JugglingAct = 18;

static const char* g_Goals_LeapingLizard[] = {
    "IGC: Intro Binocucom",  // 0
    "Get inside the distribution area",  // 1
    "IGC: Lighthouse",  // 2
    "Get to the top of the spotlight",  // 3
    "Cut the ice into a penguin shape",  // 4
    "Melt the dingle arm free",  // 5
    "Steal the Dingle Arm",  // 6
    "Outro IGC",  // 7
};
static const int g_GoalCount_LeapingLizard = 8;

static const char* g_Goals_LostAndFound[] = {
    "Start Lost and Found",  // 0
    "Enter Lamp Shop",  // 1
    "Bentley talking about Lamps",  // 2
    "Find Clues in Lamp Shop",  // 3
    "Photo Strange Door",  // 4
    "Exit to Village",  // 5
    "Find Clues to Salim",  // 6
    "Go talk to Salim al Kupar",  // 7
    "IGC Sly Meets Salim",  // 8
};
static const int g_GoalCount_LostAndFound = 9;

static const char* g_Goals_MadameGeisha[] = {
    "RC car IGC Intro",  // 0
    "Follow El Jefe to prison",  // 1
    "El Jefe around corner to fishing cave",  // 2
    "Follow El Jefe to geisha house",  // 3
};
static const int g_GoalCount_MadameGeisha = 4;

static const char* g_Goals_MechanicalMenace[] = {
    "Job Start Binocucom",  // 0
    "Find the Moat Monster's Lair",  // 1
    "Find the Moat Monster (Cave Entrance)",  // 2
    "Find the Moat Monster (Wall 1)",  // 3
    "Find the Moat Monster (Wall 2)",  // 4
    "Find the Moat Monster (Wall 3)",  // 5
    "Find the Moat Monster (Wall 4)",  // 6
    "Unplug the Moat Monster",  // 7
    "Defeat the Moat Monster (Phase 1)",  // 8
    "Defeat the Moat Monster (Phase 2)",  // 9
    "IGC Final",  // 10
};
static const int g_GoalCount_MechanicalMenace = 11;

static const char* g_Goals_OpAltSick[] = {
    "Operation Chalk Talk",  // 0
    "Pickpocket the key to the rollers",  // 1
    "Unlock the rollers",  // 2
    "Enter Dragon Gate",  // 3
    "Play BridgeView IGC",  // 4
    "Go to the bridge",  // 5
    "IGC ElJefeConfrontation",  // 6
    "Bridge Rat Fight",  // 7
    "Cross under the bridge",  // 8
    "Follow the stone path",  // 9
    "Open the sword gate",  // 10
    "Pursue El Jefe",  // 11
    "Cross the broken bridge",  // 12
    "Find El Jefe",  // 13
    "Play Cutscene Face To Face",  // 14
    "Get to Tower1",  // 15
    "Defeat El Jefe (Tower1)",  // 16
    "Get to Tower2",  // 17
    "Defeat El Jefe (Tower2)",  // 18
    "Get to Tower3",  // 19
    "Defeat El Jefe",  // 20
    "Play Cutscene El Jefe Defeat",  // 21
};
static const int g_GoalCount_OpAltSick = 22;

static const char* g_Goals_OpenSesame[] = {
    "Open Sesame Start Point",  // 0
    "Binocucom:Decimal and Paradox Walking",  // 1
    "Collect the Map Pieces",  // 2
};
static const int g_GoalCount_OpenSesame = 3;

static const char* g_Goals_OperationGoldDigger[] = {
    "chalk talk",  // 0
    "IGC: Gang boards train",  // 1
    "Free The Van!",  // 2
    "IGC: Murray frees train",  // 3
    "Escort Bentley to the Hacking Terminal",  // 4
    "IGC: Approach the Hacking Terminal",  // 5
    "Gain Access to the Hack Terminal!",  // 6
    "Hack the Terminal",  // 7
    "Reach the CPU Core",  // 8
    "Destroy the CPU Core",  // 9
    "WinAlterEgo",  // 10
    "IGC: Carmelita and Bentley Escape",  // 11
    "Get into the first gold car!",  // 12
    "Release the first gold stash!",  // 13
    "IGC: First Gold Release",  // 14
    "Get to the next train car A",  // 15
    "Release the second gold stash!",  // 16
    "IGC: Second Gold Release",  // 17
    "Get to the next train car B",  // 18
    "IGC: Kid Cooper Cane Stolen",  // 19
    "Defeat Toothpick Tutorial",  // 20
    "Defeat Toothpick Round 1",  // 21
    "Defeat Toothpick Round 2",  // 22
    "Defeat Toothpick Round 3",  // 23
    "IGC: Toothpick Defeated",  // 24
};
static const int g_GoalCount_OperationGoldDigger = 25;

static const char* g_Goals_OperationJurassicThievery[] = {
    "IGC: Intro Chalk Talk",  // 0
    "Enter the Distribution Area",  // 1
    "Jump on the Conveyor Belt",  // 2
    "IGC: Coveyor Belt Intro",  // 3
    "Navigate to the First Egg Vat (pre)",  // 4
    "IGC: Chamber Intro",  // 5
    "Navigate to the First Egg Vat",  // 6
    "Geysers",  // 7
    "Near Bridge",  // 8
    "Destroy the First Egg Vat",  // 9
    "IGC: EggVat1Destroyed",  // 10
    "Navigate to the next Egg Vat",  // 11
    "Hack the Hammers",  // 12
    "Minigame: Reach the Goal before time runs out",  // 13
    "win",  // 14
    "IGC: HammerHack",  // 15
    "Navigate to the next egg vat2",  // 16
    "Destroy the Second Egg Vat",  // 17
    "IGC: EggVat2Destroyed",  // 18
    "Loosen the valves1",  // 19
    "Loosen the valves2",  // 20
    "Loosen the valves3",  // 21
    "IGC: Sly Intro",  // 22
    "Activate the Egg Transporters",  // 23
    "Glider",  // 24
    "Take out the foreman",  // 25
    "IGC: SlyOutro",  // 26
    "switch to murray",  // 27
    "IGC: Grizzle BossFight Intro",  // 28
    "Defeat Grizz Round 1",  // 29
    "Defeat Grizz Phase 2",  // 30
    "Defeat Grizz Phase 3",  // 31
    "IGC: Grizzle BossFight Outro",  // 32
};
static const int g_GoalCount_OperationJurassicThievery = 33;

static const char* g_Goals_OperationMousetrap[] = {
    "Chalk Talk: Operation Mousetrap",  // 0
    "IGC Drawbridge Open",  // 1
    "IGC Lose Cane",  // 2
    "BOSS: Mega Knight Phase1",  // 3
    "BOSS: Mega Knight Phase2",  // 4
    "IGC Mega Knight Defeated",  // 5
    "BOSS: Penelope",  // 6
    "Job Complete",  // 7
};
static const int g_GoalCount_OperationMousetrap = 8;

static const char* g_Goals_PhotoOp[] = {
    "Take a picture of boarguard",  // 0
    "Take a picture of Prison",  // 1
    "Take a picture of Dragon Gate",  // 2
    "Take a picture of Sushi Shop",  // 3
    "Take a picture of Geisha house",  // 4
    "Photograph El Jefe",  // 5
};
static const int g_GoalCount_PhotoOp = 6;

static const char* g_Goals_PrettyPink[] = {
    "Play Cutscene Geisha Exterior",  // 0
    "Enter the hatch on Geisha roof",  // 1
    "Play IGC RioichiEnterGeisha",  // 2
    "Enter the back area of the Geisha house",  // 3
    "Pass through the garden ponds",  // 4
    "Cross the Cat Crushers",  // 5
    "Cross the Broken bridge",  // 6
    "Hack the terminal",  // 7
    "Reach the Goal before time runs out",  // 8
    "win",  // 9
    "Disable Geisha Lock",  // 10
    "Exit to Mainroom",  // 11
    "Meet Murray On top of the Geisha Suit Case!",  // 12
    "Play Dance MiniGame!",  // 13
};
static const int g_GoalCount_PrettyPink = 14;

static const char* g_Goals_Prologue[] = {
    "Intro Movies",  // 0
    "Intro Cinematic",  // 1
    "autosave warning",  // 2
    "Sly Tutorial: Heli IGC",  // 3
    "Sly Tutorial: Movement",  // 4
    "Sly Tutorial: Jump",  // 5
    "Sly Tutorial: Double Jump",  // 6
    "Sly Tutorial: Jump Attach",  // 7
    "Sly Tutorial: IGC Bus",  // 8
    "Sly Tutorial: Before Sidle",  // 9
    "Sly Tutorial: Sidle",  // 10
    "Sly Tutorial: Helicopter 2 IGC",  // 11
    "Sly Zipline To Flagpole",  // 12
    "Sly Pole Flashback",  // 13
    "Sly Tutorial: Balcony",  // 14
    "Sly Tutorial: Le Paradox Window",  // 15
    "Sly Tutorial: Heli Setup 4a",  // 16
    "Sly Tutorial: Heli Setup 4b",  // 17
    "Sly Tutorial: Heli Setup 4c",  // 18
    "Sly: Search for Skylight",  // 19
    "Sly: Sneak up on rat",  // 20
    "Sly: pick pocket",  // 21
    "Sly: Stealth Slam",  // 22
    "Sly: Open Skylight",  // 23
    "Bentley: Bomb",  // 24
    "Bentley: Trains",  // 25
    "Bentley: Hover Jump",  // 26
    "Bentley: Bomb Throw",  // 27
    "Bentley: Train Part 2",  // 28
    "Bentley Bomb Floor",  // 29
    "Bentley Flashback",  // 30
    "Bentley: Control Room",  // 31
    "Bentley: Hacking",  // 32
    "Minigame: Reach the First Docking Gate",  // 33
    "Minigame: Destroy the Firewall",  // 34
    "Minigame: Use the Key to Disable the Lasers",  // 35
    "Minigame: Activate the Panzer Code",  // 36
    "Minigame: Destroy the Barriers",  // 37
    "Minigame: Reach the final Docking Gate",  // 38
    "win",  // 39
    "Bentley Outro Murray Intro",  // 40
    "Murray Tutorial: Basic Combat",  // 41
    "Murray Tutorial: Front of warehouse fight",  // 42
    "Murray Before FlashBack",  // 43
    "Murray Tutorial: Destroy Junction Boxes",  // 44
};
static const int g_GoalCount_Prologue = 45;

static const char* g_Goals_RedFlag[] = {
    "Red Flag Start",  // 0
    "Steal Flags",  // 1
    "PlayerTrans IGC",  // 2
    "Sneak into Harem",  // 3
    "Find Silk inside Furniture",  // 4
};
static const int g_GoalCount_RedFlag = 5;

static const char* g_Goals_RiceRocket[] = {
    "Start Job Pick-Up Sushi",  // 0
    "Drop off First Sushi",  // 1
    "Pick Up Sushi For 2nd Time",  // 2
    "Drop off Second Sushi",  // 3
    "End Rice RocketJob",  // 4
};
static const int g_GoalCount_RiceRocket = 5;

static const char* g_Goals_RobbinHoods[] = {
    "Job Start Binocucom",  // 0
    "Go to the first tower",  // 1
    "Break the first balloon",  // 2
    "Go to the second tower",  // 3
    "Break the second balloon",  // 4
    "Go to the third tower",  // 5
    "Break the third balloon",  // 6
};
static const int g_GoalCount_RobbinHoods = 7;

static const char* g_Goals_RockAndRoll[] = {
    "ExtSweep IGC",  // 0
    "Enter Library",  // 1
    "IntSweep IGC",  // 2
    "Salim Plants Radios",  // 3
    "wait for vo",  // 4
    "Bentley Trans IGC",  // 5
    "Bentley Shoots Radios",  // 6
    "wait for Bentley vo",  // 7
    "rock on IGC",  // 8
    "Plant Bug",  // 9
};
static const int g_GoalCount_RockAndRoll = 10;

static const char* g_Goals_SaloonBug[] = {
    "Binocucom: Check Out Saloon",  // 0
    "Enter the Saloon From Town",  // 1
    "IGC: Sly and Bentley Enter the Saloon From Town",  // 2
    "IGC: Sly and Bentley Enter the Saloon",  // 3
    "Minigame: Get the Steers a Drink!",  // 4
    "Find A Way Into the Casino!",  // 5
    "Sly Enters the Casino",  // 6
    "IGC: Casino Start",  // 7
    "Knock the hooch vat into place",  // 8
    "Get To the Money Drop Without Being Seen!",  // 9
    "IGC: Sly Places the RC Truck",  // 10
    "Plant the Bug At Toothpick's Office!",  // 11
    "IGC: RC Car At Toothpick's Office",  // 12
};
static const int g_GoalCount_SaloonBug = 13;

static const char* g_Goals_ShellShocked[] = {
    "Job Start Binocucom",  // 0
    "Follow the Black Knight",  // 1
    "IGC Black Knight Enters Blacksmith",  // 2
    "Enter the Blacksmith",  // 3
    "Binocucom Blacksmith Interior Sweep",  // 4
    "Follow the Black Knight (1a)",  // 5
    "Follow the Black Knight (1b)",  // 6
    "IGC Black Knight Gate 1",  // 7
    "Hack the gate! (room 1)",  // 8
    "Reach the Docking Gate 04",  // 9
    "Win System Cracker 04",  // 10
    "Move to Room 2",  // 11
    "Follow the Black Knight (2a)",  // 12
    "Follow the Black Knight (2b)",  // 13
    "IGC Black Knight Gate 2",  // 14
    "Hack the gate! (room 2)",  // 15
    "Reach the CPU Core",  // 16
    "Destroy the CPU Core",  // 17
    "WinAlterEgo",  // 18
    "Move to Room 3",  // 19
    "Follow the Black Knight (3a)",  // 20
    "Follow the Black Knight (3b)",  // 21
    "IGC Black Knight Reveal",  // 22
};
static const int g_GoalCount_ShellShocked = 23;

static const char* g_Goals_ShhhhhQuiet[] = {
    "Go to ShhhhhQuiet Start",  // 0
    "IGC:Bentley Flies in the RCCopter",  // 1
    "Distract Guards with Food",  // 2
    "IGC:Sly Gets in Position",  // 3
    "Free the Thief",  // 4
    "IGC:Sly Frees the Thief",  // 5
};
static const int g_GoalCount_ShhhhhQuiet = 6;

static const char* g_Goals_ShoppingSpree[] = {
    "Job Start Binocucom",  // 0
    "Eavesdrop on Guards (Tavern)",  // 1
    "Enter the Tavern",  // 2
    "Tavern Binocucom Reveal",  // 3
    "Find Metal Material",  // 4
    "Exit the Tavern",  // 5
    "Eavesdrop on Guards (Bakery)",  // 6
    "Enter the Bakery",  // 7
    "Reach the First Docking Gate",  // 8
    "Reach the Second Docking Gate",  // 9
    "Reach the Final Docking Gate",  // 10
    "win",  // 11
    "Find Wood Material (part 1)",  // 12
    "Find Wood Material (part 2)",  // 13
    "Exit the Bakery",  // 14
    "Eavesdrop on Guards (Shoemaker)",  // 15
    "Enter the Shoemaker",  // 16
    "Find Leather Material (part 1)",  // 17
    "Find Leather Material (part 2)",  // 18
};
static const int g_GoalCount_ShoppingSpree = 19;

static const char* g_Goals_ShortSupply[] = {
    "Chalk Talk: Short Supply",  // 0
    "IGC Split Up",  // 1
    "Enter the Tavern",  // 2
    "Destroy the machines",  // 3
    "Enter the Shoemaker",  // 4
    "Locate the shutdown switch (part 1)",  // 5
    "Locate the shutdown switch (part 2)",  // 6
    "Locate the shutdown switch (part 3)",  // 7
    "Minigame: Reach the Goal before time runs out",  // 8
    "win",  // 9
    "IGC Shoemaker Shut Down",  // 10
    "Enter the Bakery",  // 11
    "Find a way to the upper floor",  // 12
    "Bakery Interior Binocucom",  // 13
    "Disable all three nodes",  // 14
    "IGC Bakery Shut Down",  // 15
};
static const int g_GoalCount_ShortSupply = 16;

static const char* g_Goals_SparkRunner01[] = {
    "Reach the end before the timer runs out!",  // 0
    "start",  // 1
    "win",  // 2
};
static const int g_GoalCount_SparkRunner01 = 3;

static const char* g_Goals_SparkRunner02[] = {
    "Reach the end before the timer runs out!",  // 0
    "Reach the goal before time runs out!",  // 1
    "win",  // 2
};
static const int g_GoalCount_SparkRunner02 = 3;

static const char* g_Goals_SparkRunner03[] = {
    "Reach the end before the timer runs out!",  // 0
    "Reach the end before the timer runs out!",  // 1
    "win",  // 2
};
static const int g_GoalCount_SparkRunner03 = 3;

static const char* g_Goals_SparkRunner04[] = {
    "Reach the end before the timer runs out!",  // 0
    "Reach the end before the timer runs out!",  // 1
    "win",  // 2
};
static const int g_GoalCount_SparkRunner04 = 3;

static const char* g_Goals_SparkRunner05[] = {
    "Reach the end before the timer runs out!",  // 0
    "Reach the end before the timer runs out!",  // 1
    "win",  // 2
};
static const int g_GoalCount_SparkRunner05 = 3;

static const char* g_Goals_SparkRunner06[] = {
    "Reach the end before the timer runs out!",  // 0
    "Reach the end before the timer runs out!  Main",  // 1
    "win",  // 2
};
static const int g_GoalCount_SparkRunner06 = 3;

static const char* g_Goals_SparkRunner07[] = {
    "Reach the end before the timer runs out!",  // 0
    "Reach the end before the timer runs out!",  // 1
    "win",  // 2
};
static const int g_GoalCount_SparkRunner07 = 3;

static const char* g_Goals_SpecialDelivery[] = {
    "Bino Gems",  // 0
    "Find and Collect Gems",  // 1
    "Deliver The Gems",  // 2
    "Reach the gate",  // 3
    "Sneak under the gate using the costume",  // 4
    "Use Sword to smash gate controls",  // 5
    "Follow LeParadox and Miss Decibel A",  // 6
    "Follow LeParadox and Miss Decibel B",  // 7
    "Follow LeParadox and Miss Decibel C",  // 8
    "Combo Slo Time Sword",  // 9
    "Use Lever and Costume B",  // 10
    "Sneak Under Gate B",  // 11
    "Use SwordB",  // 12
    "Meet Decibel and Paradox",  // 13
};
static const int g_GoalCount_SpecialDelivery = 14;

static const char* g_Goals_SpikedSushi[] = {
    "Enter Cave",  // 0
    "Play Caves Binocucom Intro",  // 1
    "Pick Poison Plants",  // 2
};
static const int g_GoalCount_SpikedSushi = 3;

static const char* g_Goals_StoneAgeReconnaissance[] = {
    "IGC: Intro Chalk Talk",  // 0
    "Take photos in the village",  // 1
    "Wait for Photo VO",  // 2
    "Investigate the Arena",  // 3
    "IGC Biz Intro",  // 4
    "Photograph Grizzle B",  // 5
    "IGC Job Complete",  // 6
};
static const int g_GoalCount_StoneAgeReconnaissance = 7;

static const char* g_Goals_SushiStartup[] = {
    "Play Cutscene Sushi Exterior",  // 0
    "Sneak into the Sushi Shop",  // 1
    "Play Cutscene Rioichi Enter",  // 2
    "Play Cutscene Binocucom Int Sweep",  // 3
    "Climb bamboo to get a better vantage point",  // 4
    "Knife guard01 check point",  // 5
    "Pickpocket the First Sushi Knife",  // 6
    "Unlock the first sushi knife door",  // 7
    "Pickpocket the Second Sushi Knife",  // 8
    "Unlock the second sushi knife door",  // 9
    "Get through second Laser Hall",  // 10
    "Reach the top rafters",  // 11
    "Find a way through the Banquet Room",  // 12
    "Pickpocket the Third Sushi Knife",  // 13
    "Unlock the third sushi knife door",  // 14
    "Turn on the Power to the Sushi restaurant",  // 15
};
static const int g_GoalCount_SushiStartup = 16;

static const char* g_Goals_SystemCracker01[] = {
    "Reach the First Docking Gate",  // 0
    "Destroy the Firewall",  // 1
    "Use the Key to Disable the Lasers",  // 2
    "Switch to the Tank Avatar",  // 3
    "Destroy the Tank Barriers",  // 4
    "Reach the Final Docking Gate",  // 5
    "win",  // 6
};
static const int g_GoalCount_SystemCracker01 = 7;

static const char* g_Goals_SystemCracker02[] = {
    "Reach the First Docking Gate",  // 0
    "Reach the Second Docking Gate",  // 1
    "Reach the Final Docking Gate",  // 2
    "win",  // 3
};
static const int g_GoalCount_SystemCracker02 = 4;

static const char* g_Goals_SystemCracker03[] = {
    "Reach the First Docking Gate",  // 0
    "Reach the Second Docking Gate",  // 1
    "Reach the Final Docking Gate",  // 2
    "win",  // 3
};
static const int g_GoalCount_SystemCracker03 = 4;

static const char* g_Goals_SystemCracker04[] = {
    "Reach the Docking Gate",  // 0
    "win",  // 1
};
static const int g_GoalCount_SystemCracker04 = 2;

static const char* g_Goals_SystemCracker05[] = {
    "Reach the Docking Gate",  // 0
    "win",  // 1
};
static const int g_GoalCount_SystemCracker05 = 2;

static const char* g_Goals_SystemCracker07[] = {
    "Reach the Docking Gate",  // 0
    "win",  // 1
};
static const int g_GoalCount_SystemCracker07 = 2;

static const char* g_Goals_SystemCracker08[] = {
    "Reach the Docking Gate",  // 0
    "Reach the Docking Gate 08b",  // 1
    "Reach the Docking Gate 08c",  // 2
    "Reach the Docking Gate 08d",  // 3
    "win",  // 4
};
static const int g_GoalCount_SystemCracker08 = 5;

static const char* g_Goals_SystemCracker09[] = {
    "Get to the Final Docking Gate!",  // 0
    "win",  // 1
};
static const int g_GoalCount_SystemCracker09 = 2;

static const char* g_Goals_SystemCracker09Multi[] = {
    "Get to the Final Docking Gate! (2P)",  // 0
    "win",  // 1
};
static const int g_GoalCount_SystemCracker09Multi = 2;

static const char* g_Goals_TaxiDriver[] = {
    "Go to Taxi Shop",  // 0
    "IGC:Sly and Murray Plunge into Sewers",  // 1
    "Lift the Sunken Ship",  // 2
    "Murray blocked",  // 3
    "Open Door",  // 4
    "Sneak through Door",  // 5
    "TrapA Checkpoint",  // 6
    "Conquer TrapA",  // 7
    "Navigate to TrapB",  // 8
    "Halfway TrapB",  // 9
    "TrapRoomB Door Checkpoint",  // 10
    "Conquer TrapB",  // 11
    "Use Sword",  // 12
    "Cannon Fortress Battle",  // 13
    "Use the Crank A",  // 14
    "Lift Ship B",  // 15
    "Advance to Grinders",  // 16
    "Gold Room",  // 17
    "Gold Room Part Deux",  // 18
    "Gold Room Checkpoint",  // 19
    "Gold Room Checkpoint Part Deux",  // 20
    "Sword KeyB",  // 21
    "Grinders Stop",  // 22
    "Navigate to island",  // 23
    "Use the Crank B",  // 24
    "Cannon Fight B",  // 25
    "Lift ShipC",  // 26
    "Navigate to Ship",  // 27
    "Ship is Sinking",  // 28
    "Hack the Garage Terminal",  // 29
    "Minigame: Reach the Docking Gate",  // 30
    "win",  // 31
    "Rescue cutscene",  // 32
};
static const int g_GoalCount_TaxiDriver = 33;

static const char* g_Goals_ToothpicksVault[] = {
    "Chalk Talk: Toothpick's Vault",  // 0
    "Follow Toothpick to the Vault!",  // 1
    "IGC: Toothpick Enters the Vault",  // 2
    "Protect Bentley From the Scorpions!",  // 3
    "IGC: Bentley Hack Start",  // 4
    "Minigame: Reach the First Docking Gate",  // 5
    "Minigame: Reach the Second Docking Gate",  // 6
    "Minigame: Reach the Final Docking Gate",  // 7
    "win",  // 8
    "IGC: Kid Cooper Enters the Vault",  // 9
    "IGC: Kid Cooper Gets His Cane Back",  // 10
    "Use Your Gun to Open the Vault Door!",  // 11
    "Explore the Vault!",  // 12
    "IGC: Show Laser Field!",  // 13
    "Shoot the Switch to Turn Off the Grid!",  // 14
    "Open Up the Big Vault Door!",  // 15
    "IGC: Crackshot Vault Door",  // 16
    "Use Crack Shot On Multiple Locks!",  // 17
    "Ride the Rails to the Miner's Camp!",  // 18
    "Use Crack Shot On the Big Vault Door!",  // 19
    "Find a way out of the Mine",  // 20
    "Escape Route! Unlock the door",  // 21
    "Open the Exit Door!",  // 22
    "Escape the mines!",  // 23
    "IGC: Vault Exit",  // 24
};
static const int g_GoalCount_ToothpicksVault = 25;

static const char* g_Goals_UnderArrest[] = {
    "Binocucom: Deface Posters",  // 0
    "Deface First Toothpick Poster",  // 1
    "Deface Second Toothpick Poster",  // 2
    "Deface Third Toothpick Poster",  // 3
    "Deface Fourth Toothpick Poster",  // 4
    "Deface Fifth Toothpick Poster",  // 5
    "Deface Final Toothpick Poster",  // 6
    "Binocucom: Steal Lollipop!",  // 7
    "Steal Toothpick's Lollipop!",  // 8
    "Get a look at the Toothpick's Banner",  // 9
    "Binocucom shows the banner",  // 10
    "Tear Down Toothpick's Banner!",  // 11
    "Toothpick's Banner Fall Cutscene",  // 12
};
static const int g_GoalCount_UnderArrest = 13;

static const char* g_Goals_UpInSmoke[] = {
    "IGC Up In Smoke Start",  // 0
    "Go To TurretA",  // 1
    "IGC TurretA Reveal",  // 2
    "RCCopter Smoke BombA",  // 3
    "IGC On The Move A",  // 4
    "Go To TurretB",  // 5
    "IGC TurretB Reveal",  // 6
    "RCCopter Smoke BombB",  // 7
    "IGC On The Move B",  // 8
    "Go To TurretC",  // 9
    "IGC TurretC Reveal",  // 10
    "RCCopter Smoke BombC",  // 11
    "IGC On The Move C",  // 12
    "Free the Thief",  // 13
    "Minigame: Reach the CPU Core",  // 14
    "Minigame: Destroy the CPU Core",  // 15
    "WinAlterEgo",  // 16
    "IGC Rescue",  // 17
};
static const int g_GoalCount_UpInSmoke = 18;

static const JobGoalList g_AllJobGoalLists[] = {
    { "AllRolledUpTitleName", g_Goals_AllRolledUp, g_GoalCount_AllRolledUp },
    { "AlterEgo01TitleName", g_Goals_AlterEgo01, g_GoalCount_AlterEgo01 },
    { "AlterEgo02TitleName", g_Goals_AlterEgo02, g_GoalCount_AlterEgo02 },
    { "AlterEgo03TitleName", g_Goals_AlterEgo03, g_GoalCount_AlterEgo03 },
    { "AlterEgo04TitleName", g_Goals_AlterEgo04, g_GoalCount_AlterEgo04 },
    { "AlterEgo05TitleName", g_Goals_AlterEgo05, g_GoalCount_AlterEgo05 },
    { "AlterEgo06TitleName", g_Goals_AlterEgo06, g_GoalCount_AlterEgo06 },
    { "AlterEgo06MultiTitleName", g_Goals_AlterEgo06Multi, g_GoalCount_AlterEgo06Multi },
    { "AlterEgo06xMultiTitleName", g_Goals_AlterEgo06xMulti, g_GoalCount_AlterEgo06xMulti },
    { "AlterEgo07TitleName", g_Goals_AlterEgo07, g_GoalCount_AlterEgo07 },
    { "AlterEgo07MultiTitleName", g_Goals_AlterEgo07Multi, g_GoalCount_AlterEgo07Multi },
    { "AlterEgo07xMultiTitleName", g_Goals_AlterEgo07xMulti, g_GoalCount_AlterEgo07xMulti },
    { "AlterEgo08TitleName", g_Goals_AlterEgo08, g_GoalCount_AlterEgo08 },
    { "BlindDateTitleName", g_Goals_BlindDate, g_GoalCount_BlindDate },
    { "BreakoutTitleName", g_Goals_Breakout, g_GoalCount_Breakout },
    { "CaneSwipeTitleName", g_Goals_CaneSwipe, g_GoalCount_CaneSwipe },
    { "CavemanCoopersRescueTitleName", g_Goals_CavemanCoopersRescue, g_GoalCount_CavemanCoopersRescue },
    { "ClosedForRemodelingTitleName", g_Goals_ClosedForRemodeling, g_GoalCount_ClosedForRemodeling },
    { "CrazyClimberTitleName", g_Goals_CrazyClimber, g_GoalCount_CrazyClimber },
    { "DutyCallsTitleName", g_Goals_DutyCalls, g_GoalCount_DutyCalls },
    { "EpilogueTitleName", g_Goals_Epilogue, g_GoalCount_Epilogue },
    { "FishyTitleName", g_Goals_Fishy, g_GoalCount_Fishy },
    { "GettingStrongerTitleName", g_Goals_GettingStronger, g_GoalCount_GettingStronger },
    { "GoingUpTitleName", g_Goals_GoingUp, g_GoalCount_GoingUp },
    { "GrandKeyLarcenyTitleName", g_Goals_GrandKeyLarceny, g_GoalCount_GrandKeyLarceny },
    { "HardTargetTitleName", g_Goals_HardTarget, g_GoalCount_HardTarget },
    { "HungryHungryHippoTitleName", g_Goals_HungryHungryHippo, g_GoalCount_HungryHungryHippo },
    { "IceIceBentleyTitleName", g_Goals_IceIceBentley, g_GoalCount_IceIceBentley },
    { "JailBreakTitleName", g_Goals_JailBreak, g_GoalCount_JailBreak },
    { "JailhouseBluesTitleName", g_Goals_JailhouseBlues, g_GoalCount_JailhouseBlues },
    { "JugglingActTitleName", g_Goals_JugglingAct, g_GoalCount_JugglingAct },
    { "LeapingLizardTitleName", g_Goals_LeapingLizard, g_GoalCount_LeapingLizard },
    { "LostAndFoundTitleName", g_Goals_LostAndFound, g_GoalCount_LostAndFound },
    { "MadameGeishaTitleName", g_Goals_MadameGeisha, g_GoalCount_MadameGeisha },
    { "MechanicalMenaceTitleName", g_Goals_MechanicalMenace, g_GoalCount_MechanicalMenace },
    { "OpAltSickTitleName", g_Goals_OpAltSick, g_GoalCount_OpAltSick },
    { "OpenSesameTitleName", g_Goals_OpenSesame, g_GoalCount_OpenSesame },
    { "OperationGoldDiggerTitleName", g_Goals_OperationGoldDigger, g_GoalCount_OperationGoldDigger },
    { "OperationJurassicThieveryTitleName", g_Goals_OperationJurassicThievery, g_GoalCount_OperationJurassicThievery },
    { "OperationMousetrapTitleName", g_Goals_OperationMousetrap, g_GoalCount_OperationMousetrap },
    { "PhotoOpTitleName", g_Goals_PhotoOp, g_GoalCount_PhotoOp },
    { "PrettyPinkTitleName", g_Goals_PrettyPink, g_GoalCount_PrettyPink },
    { "PrologueTitleName", g_Goals_Prologue, g_GoalCount_Prologue },
    { "RedFlagTitleName", g_Goals_RedFlag, g_GoalCount_RedFlag },
    { "RiceRocketTitleName", g_Goals_RiceRocket, g_GoalCount_RiceRocket },
    { "RobbinHoodsTitleName", g_Goals_RobbinHoods, g_GoalCount_RobbinHoods },
    { "RockAndRollTitleName", g_Goals_RockAndRoll, g_GoalCount_RockAndRoll },
    { "SaloonBugTitleName", g_Goals_SaloonBug, g_GoalCount_SaloonBug },
    { "ShellShockedTitleName", g_Goals_ShellShocked, g_GoalCount_ShellShocked },
    { "ShhhhhQuietTitleName", g_Goals_ShhhhhQuiet, g_GoalCount_ShhhhhQuiet },
    { "ShoppingSpreeTitleName", g_Goals_ShoppingSpree, g_GoalCount_ShoppingSpree },
    { "ShortSupplyTitleName", g_Goals_ShortSupply, g_GoalCount_ShortSupply },
    { "SparkRunner01TitleName", g_Goals_SparkRunner01, g_GoalCount_SparkRunner01 },
    { "SparkRunner02TitleName", g_Goals_SparkRunner02, g_GoalCount_SparkRunner02 },
    { "SparkRunner03TitleName", g_Goals_SparkRunner03, g_GoalCount_SparkRunner03 },
    { "SparkRunner04TitleName", g_Goals_SparkRunner04, g_GoalCount_SparkRunner04 },
    { "SparkRunner05TitleName", g_Goals_SparkRunner05, g_GoalCount_SparkRunner05 },
    { "SparkRunner06TitleName", g_Goals_SparkRunner06, g_GoalCount_SparkRunner06 },
    { "SparkRunner07TitleName", g_Goals_SparkRunner07, g_GoalCount_SparkRunner07 },
    { "SpecialDeliveryTitleName", g_Goals_SpecialDelivery, g_GoalCount_SpecialDelivery },
    { "SpikedSushiTitleName", g_Goals_SpikedSushi, g_GoalCount_SpikedSushi },
    { "StoneAgeReconnaissanceTitleName", g_Goals_StoneAgeReconnaissance, g_GoalCount_StoneAgeReconnaissance },
    { "SushiStartupTitleName", g_Goals_SushiStartup, g_GoalCount_SushiStartup },
    { "SystemCracker01TitleName", g_Goals_SystemCracker01, g_GoalCount_SystemCracker01 },
    { "SystemCracker02TitleName", g_Goals_SystemCracker02, g_GoalCount_SystemCracker02 },
    { "SystemCracker03TitleName", g_Goals_SystemCracker03, g_GoalCount_SystemCracker03 },
    { "SystemCracker04TitleName", g_Goals_SystemCracker04, g_GoalCount_SystemCracker04 },
    { "SystemCracker05TitleName", g_Goals_SystemCracker05, g_GoalCount_SystemCracker05 },
    { "SystemCracker07TitleName", g_Goals_SystemCracker07, g_GoalCount_SystemCracker07 },
    { "SystemCracker08TitleName", g_Goals_SystemCracker08, g_GoalCount_SystemCracker08 },
    { "SystemCracker09TitleName", g_Goals_SystemCracker09, g_GoalCount_SystemCracker09 },
    { "SystemCracker09MultiTitleName", g_Goals_SystemCracker09Multi, g_GoalCount_SystemCracker09Multi },
    { "TaxiDriverTitleName", g_Goals_TaxiDriver, g_GoalCount_TaxiDriver },
    { "ToothpicksVaultTitleName", g_Goals_ToothpicksVault, g_GoalCount_ToothpicksVault },
    { "UnderArrestTitleName", g_Goals_UnderArrest, g_GoalCount_UnderArrest },
    { "UpInSmokeTitleName", g_Goals_UpInSmoke, g_GoalCount_UpInSmoke },
};
static const int g_AllJobGoalListsCount = 76;

static char g_SelectedJobKeyBuf[64];

static bool g_GoalTeleportCooldown = false;
static unsigned char g_GoalStableState = 0;

enum PendingJobStage { STAGE_NONE, STAGE_WAIT_FOR_RESET, STAGE_WAIT_FOR_REAL_START };

static PendingJobStage g_PendingStage = STAGE_NONE;

static bool g_PendingJobStart = false;
static int g_PendingJobIdx = -1;
static int g_PendingGoalIdx = 0;
static unsigned char g_StableGameMgrState = 0;

static int g_ActiveJobGlobalIndex = -1;
static const int g_AllJobGoalsCount = 1;

static int g_SelectedEpisodeIndex = -1;
static char g_SelectedEpisodeNameBuf[64];

static bool g_EnteredTuningViaLoad = false;

typedef void (*ActionFn)();

struct SyntheticItem {
    const char* name;
    unsigned int liveAddr; // unused for TT_ACTION, kept 0
    int type;
    ActionFn action;       // unused for non-action types, kept nullptr
};

static SyntheticItem g_SyntheticItems[] = {
    { "Menu.BackgroundColor", (unsigned int)&g_MenuBgColorTunable, TT_COLOR, nullptr },
    { "Menu.DeleteCfgFile",   0, TT_ACTION, Action_DeleteCfgFile },
    { "Menu.ReloadCfgFile",   0, TT_ACTION, Action_ReloadCfgFile },
};
static const int g_SyntheticItemCount = sizeof(g_SyntheticItems) / sizeof(g_SyntheticItems[0]);

struct SavedTunable {
    char name[128];
    unsigned char data[64];
    int dataSize;
};
static SavedTunable g_SavedTunables[256];


static int GetTunableByteSize(int type)
{
    switch (type) {
    case TT_BOOL:        return 1;
    case TT_INT:         return 4;
    case TT_FLOAT:       return 4;
    case TT_COLOR:       return 4;
    case TT_STRING_HASH: return 4;
    case TT_VEC2:        return 8;
    case TT_VEC3:        return 12;
    case TT_STRING:      return 64;
    default:             return 4;
    }
}

static int g_CallCount = 0;

enum MenuState { MENU_STATE_TOP, MENU_STATE_LOAD, MENU_STATE_TUNABLES, MENU_STATE_VECTOR_EDIT, MENU_STATE_COLOR_EDIT, MENU_STATE_SCENE};
MenuState g_MenuState = MENU_STATE_TOP;

const char* g_MenuItems[] = {
    "Tuning...",
    "Anim Preview Mode...",
    "Load...",
    "Scene..."
};
const int g_TopMenuItemCount = 4;

const char* g_LoadMenuItems[] = {
    "Levels...",
    "Episodes...",
    "Save/Load..."
};
const int g_LoadMenuItemCount = 3;

static const char* g_SceneMenuItems[] = {
    "Save Scene",
    "Load Scene",
    "Clear List",
    "Spawn Entity",
    "Level Editor Cam",
};
static const int g_SceneMenuItemCount = 5;

static bool g_EnteredTuningViaScene = false;

static inline int AbsInt(int v) { return v < 0 ? -v : v; }




int DrawAndSweepHook();

static char g_DrawSweepHookStorage[sizeof(libpsutil::memory::detour)];
libpsutil::memory::detour* g_DrawSweepHook = nullptr;

unsigned int g_VectorEditAddr = 0;
char g_VectorEditLabel[64];
int g_VectorEditComponentCount = 3;
int g_VectorEditSelectedComponent = 0;

static unsigned int s_savedFlags232 = 0;

//const int g_MenuItemCount = 3;
int g_SelectedIndex = 0;

// --- Navigation state ---
char g_CurrentPath[256] = "";
char g_PathStack[16][256];
int g_SelectedIndexStack[16];
int g_PathStackDepth = 0;

struct DisplayItem {
    char label[64];
    bool isCategory;
    int tunableIndex;
    unsigned int liveAddr;   // 0 if not found in live registry
    TunableType type;
};

DisplayItem g_DisplayItems[256];
int g_DisplayItemCount = 0;

float identityMtx[16] = {
    1,0,0,0,
    0,1,0,0,
    0,0,1,0,
    0,0,0,1
};

TunableType GetTunableType(unsigned int vtable)
{
    if (vtable == VTABLE_BOOL) return TT_BOOL;
    if (vtable == VTABLE_INT) return TT_INT;
    if (vtable == VTABLE_FLOAT) return TT_FLOAT;
    if (vtable == VTABLE_VEC3) return TT_VEC3;
    if (vtable == VTABLE_VEC2) return TT_VEC2;
    if (vtable == VTABLE_STRING) return TT_STRING;
    if (vtable == VTABLE_COLOR) return TT_COLOR;
    if (vtable == VTABLE_STRING_HASH) return TT_STRING_HASH;
    return TT_UNKNOWN;
}


int Post3DHUDHook(int a1) {
    // Draw BEFORE original — our draws get flushed by original's Flush call
    if (g_LevelEditorEnabled) {
        int gameMgr = *(int*)0xF65CA0;
        if (gameMgr) {
            int renderObj = *(int*)(gameMgr + 0x1285E4);
            if (renderObj) {
                float* cmdBuf = *(float**)(renderObj + 12);
                if (cmdBuf && (unsigned int)cmdBuf > 0x10000) {
                    DrawOrbitPivotWith3D(cmdBuf);
                }
            }
        }
    }

    return g_Post3DHUDHook->invoke<int>(a1);
}

int SetActiveHook(int entity, int active) {
    int ret = g_SetActiveHook->invoke<int>(entity, active);

    if (active) {
        int rt = *(int*)(entity + 364);
        if (rt) {
            g_ActiveCameraEntity = entity;
            g_ActiveCameraRenderTarget = rt;
        }
    }
    else if (entity == g_ActiveCameraEntity) {
        g_ActiveCameraEntity = 0;
        g_ActiveCameraRenderTarget = 0;
    }

    return ret;
}

bool GetGroundY(float x, float y, float z, float& outY) {
    // Start well above, end well below
    float start[3] = { x, y + 100.0f, z };
    float end[3] = { x, y - 500.0f, z };
    float hitPos[3] = {};

    GroundRaycast_t Raycast = (GroundRaycast_t)&s_groundRaycastOpd;
    int hit = Raycast(start, end, hitPos);

    if (hit) {
        outY = hitPos[1];
        return true;
    }
    return false;
}

const char* FindJobNameByHash(unsigned int hash) {
    for (int e = 0; e < g_EpisodeCount; e++) {
        const EpisodeJobList& jl = g_EpisodeJobLists[e];
        for (int j = 0; j < jl.jobCount; j++) {
            if (HashStr(jl.jobKeys[j]) == hash)
                return jl.jobKeys[j];
        }
    }
    return nullptr;
}

void BuildScenePath() {
    char sceneName[64] = {};
    GetCurrentSceneName(sceneName, sizeof(sceneName));
    printf("[Scene] sceneName='%s'\n", sceneName);

    // Sanitize sceneName directly
    char safeName[64] = {};
    int k = 0;
    for (int i = 0; sceneName[i] && k < 60; i++) {
        char c = sceneName[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-') {
            safeName[k++] = c;
        }
        else {
            safeName[k++] = '_';
        }
    }
    safeName[k] = '\0';

    // Build full path
    const char* prefix = "/dev_hdd0/tmp/Sly4DebugMenu/scene_";
    const char* suffix = ".cfg";
    int i = 0;
    while (prefix[i]) { g_ScenePath[i] = prefix[i]; i++; }
    int j = 0;
    while (safeName[j]) { g_ScenePath[i++] = safeName[j++]; }
    j = 0;
    while (suffix[j]) { g_ScenePath[i++] = suffix[j++]; }
    g_ScenePath[i] = '\0';

    printf("[Scene] path: %s\n", g_ScenePath);
}

void SaveSceneFile() {
    BuildScenePath();
    if (!EnsureSaveDirectory(kSaveDir)) return;

    int fd;
    int ret = cellFsOpen(g_ScenePath, CELL_FS_O_CREAT | CELL_FS_O_WRONLY | CELL_FS_O_TRUNC, &fd, NULL, 0);
    if (ret != CELL_FS_SUCCEEDED) { printf("[Scene] open-for-write failed err=0x%X\n", ret); return; }

    uint64_t written;
    const char* header = "# Sly4 Debug Menu - Saved Scene\n";
    cellFsWrite(fd, header, StrLen(header), &written);

    // Human readable comment
    unsigned int episodeMgr = *(unsigned int*)0xF64C20;
    if (episodeMgr) {
        unsigned int currentJob = ((unsigned int*)episodeMgr)[10];
        if (currentJob) {
            unsigned int jobHash = *(unsigned int*)(currentJob + 468);
            const char* jobName = FindJobNameByHash(jobHash);
            if (jobName) {
                char* infoLine = FormatStr("# Job: %s\n", jobName);
                cellFsWrite(fd, infoLine, StrLen(infoLine), &written);
            }
        }
        else {
            unsigned int currentEpisode = ((unsigned int*)episodeMgr)[9];
            if (currentEpisode) {
                const char* episodeName = (const char*)currentEpisode;
                if (episodeName[0] == '$') episodeName++;
                char* infoLine = FormatStr("# Level: %s\n", episodeName);
                cellFsWrite(fd, infoLine, StrLen(infoLine), &written);
            }
        }
    }

    for (int i = 0; i < g_SavedEntityCount; i++) {
        SavedEntity& e = g_SavedEntities[i];
        char* line = FormatStr(
            "entity 0x%08X %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",
            e.hash,
            e.posX, e.posY, e.posZ,
            e.rotX, e.rotY, e.rotZ,
            e.scale
        );
        cellFsWrite(fd, line, StrLen(line), &written);
    }

    cellFsClose(fd);
    printf("[Scene] saved %d entities to %s\n", g_SavedEntityCount, g_ScenePath);

    // Clear list after saving
   // g_SavedEntityCount = 0;
}

bool LoadSceneFile() {
    BuildScenePath();

    // Clear existing list before attempting load
    g_SavedEntityCount = 0;

    int fd;
    int ret = cellFsOpen(g_ScenePath, CELL_FS_O_RDONLY, &fd, NULL, 0);
    if (ret != CELL_FS_SUCCEEDED) {
        printf("[Scene] no scene file for '%s' (err=0x%X)\n", g_ScenePath, ret);
        return false;
    }

    static char buffer[8192];
    uint64_t bytesRead = 0;
    cellFsRead(fd, buffer, sizeof(buffer) - 1, &bytesRead);
    cellFsClose(fd);
    buffer[bytesRead] = '\0';

    char* p = buffer;
    while (*p) {
        while (*p == '\r' || *p == '\n' || *p == ' ') p++;
        if (!*p) break;

        if (*p == '#') { while (*p && *p != '\n') p++; continue; }

        if (p[0] == 'e' && p[1] == 'n' && p[2] == 't' && p[3] == 'i' && p[4] == 't' && p[5] == 'y' && p[6] == ' ') {
            p += 7;

            unsigned int hash = 0;
            if (p[0] == '0' && p[1] == 'x') {
                p += 2;
                while ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')) {
                    hash <<= 4;
                    if (*p >= '0' && *p <= '9')      hash |= *p - '0';
                    else if (*p >= 'a' && *p <= 'f') hash |= *p - 'a' + 10;
                    else                        hash |= *p - 'A' + 10;
                    p++;
                }
            }
            while (*p == ' ') p++;

            float vals[7] = {};
            for (int i = 0; i < 7; i++) {
                float sign = 1.0f;
                if (*p == '-') { sign = -1.0f; p++; }
                float intPart = 0.0f;
                while (*p >= '0' && *p <= '9') { intPart = intPart * 10.0f + (*p - '0'); p++; }
                float fracPart = 0.0f;
                if (*p == '.') {
                    p++;
                    float div = 10.0f;
                    while (*p >= '0' && *p <= '9') {
                        fracPart += (*p - '0') / div;
                        div *= 10.0f;
                        p++;
                    }
                }
                vals[i] = sign * (intPart + fracPart);
                while (*p == ' ') p++;
            }

            if (hash && g_SavedEntityCount < MAX_SAVED_ENTITIES) {
                SavedEntity& e = g_SavedEntities[g_SavedEntityCount++];
                e.hash = hash;
                e.posX = vals[0]; e.posY = vals[1]; e.posZ = vals[2];
                e.rotX = vals[3]; e.rotY = vals[4]; e.rotZ = vals[5];
                e.scale = vals[6];
            }
        }

        while (*p && *p != '\n') p++;
    }

    printf("[Scene] loaded %d entities from %s\n", g_SavedEntityCount, g_ScenePath);
    return true;
}

void SpawnLoadedScene() {
    int count = g_SavedEntityCount;
    printf("[Scene] spawning %d entities count=%d\n", count, count);

    g_IsLoadingScene = true;

    for (int i = 0; i < count; i++) {
        SavedEntity& e = g_SavedEntities[i];
        printf("[Scene] entity %d hash=0x%08X\n", i, e.hash);

        g_SpawnRotation[0] = e.rotX;
        g_SpawnRotation[1] = e.rotY;
        g_SpawnRotation[2] = e.rotZ;
        g_SpawnScale = e.scale;
        BuildAndSpawn(e.hash, e.posX, e.posY, e.posZ);
    }

    g_IsLoadingScene = false;
    printf("[Scene] spawn complete\n");
}

void ClearScene() {
    int prev = g_SavedEntityCount;
    g_SavedEntityCount = 0;
    printf("[Scene] cleared %d entities\n", prev);
}

void GetCurrentSceneName(char* outName, int maxLen) {
    unsigned int episodeMgr = *(unsigned int*)0xF64C20;
    unsigned int gameMgr = *(unsigned int*)0xF65CA0;
    unsigned int levelHash = gameMgr ? *(unsigned int*)(gameMgr + 1214388) : 0;

    if (episodeMgr) {
        // Current job
        unsigned int currentJob = ((unsigned int*)episodeMgr)[10];
        if (currentJob) {
            unsigned int jobHash = *(unsigned int*)(currentJob + 468);
            if (jobHash) {
                const char* jobName = FindJobNameByHash(jobHash);
                if (jobName) {
                    int i = 0;
                    while (jobName[i] && i < maxLen - 1) { outName[i] = jobName[i]; i++; }
                    outName[i] = '\0';
                    return;
                }
                const char* prefix = "job_";
                int i = 0;
                while (prefix[i]) { outName[i] = prefix[i]; i++; }
                for (int b = 7; b >= 0; b--) {
                    int nibble = (jobHash >> (b * 4)) & 0xF;
                    outName[i++] = nibble < 10 ? '0' + nibble : 'a' + nibble - 10;
                }
                outName[i] = '\0';
                return;
            }
        }

        // Hub — try level hash first
        if (levelHash) {
            const char* prefix = "level_";
            int i = 0;
            while (prefix[i]) { outName[i] = prefix[i]; i++; }
            for (int b = 7; b >= 0; b--) {
                int nibble = (levelHash >> (b * 4)) & 0xF;
                outName[i++] = nibble < 10 ? '0' + nibble : 'a' + nibble - 10;
            }
            outName[i] = '\0';
            return;
        }

        // Hub — fall back to episode hash from episodeMgr[9]
        unsigned int currentEpisode = ((unsigned int*)episodeMgr)[9];
        if (currentEpisode) {
            unsigned int episodeHash = *(unsigned int*)(currentEpisode + 468);
            if (episodeHash) {
                const char* prefix = "episode_";
                int i = 0;
                while (prefix[i]) { outName[i] = prefix[i]; i++; }
                for (int b = 7; b >= 0; b--) {
                    int nibble = (episodeHash >> (b * 4)) & 0xF;
                    outName[i++] = nibble < 10 ? '0' + nibble : 'a' + nibble - 10;
                }
                outName[i] = '\0';
                return;
            }
        }
    }

    const char* unk = "unknown";
    int i = 0; while (unk[i]) { outName[i] = unk[i]; i++; } outName[i] = '\0';
}

void* GetCmdBuf() {
    int gameMgr = *(int*)0xF65CA0;
    int renderObj = *(int*)(gameMgr + RENDER_OFFSET);
    return *(void**)(renderObj + 12);
}

static void OskSysutilCallback(uint64_t status, uint64_t param, void* userdata) {
    switch (status) {
    case CELL_SYSUTIL_OSKDIALOG_FINISHED:
        g_oskMode = OSK_MODE_CLOSE;
        break;
    case CELL_SYSUTIL_OSKDIALOG_INPUT_ENTERED:
        cellOskDialogGetInputText(&g_oskOutputInfo);
        break;
    default:
        break;
    }
}




static void CopyNameSafe(char* dest, const char* src, int maxLen) {
    int i = 0;
    for (; i < maxLen - 1; i++) {
        char c = src[i];
        if (c == 0) break;
        dest[i] = c;
    }
    dest[i] = 0;
}

float MySqrtf(float x) {
    if (x <= 0.0f) return 0.0f;
    float guess = x * 0.5f;
    for (int i = 0; i < 8; i++)
        guess = (guess + x / guess) * 0.5f;
    return guess;
}

float MyFabsf(float x) {
    return (x < 0.0f) ? -x : x;
}

float MyFmodf(float x, float y) {
    float n = (float)((int)(x / y));
    return x - n * y;
}

// accurate Taylor series, valid only near zero - same as before
static float TaylorSin(float x) {
    float x2 = x * x;
    return x * (1.0f - x2 / 6.0f + (x2 * x2) / 120.0f - (x2 * x2 * x2) / 5040.0f); // added 7th-order term for safety
}

float MySinf(float x) {
    const float PI = 3.14159265f;
    const float HALF_PI = 1.57079633f;
    const float TWO_PI = 6.28318530f;

    // wrap to [-pi, pi]
    x = MyFmodf(x, TWO_PI);
    if (x > PI) x -= TWO_PI;
    if (x < -PI) x += TWO_PI;

    // reduce to [-pi/2, pi/2] using quadrant identities, so Taylor series stays accurate
    if (x > HALF_PI) {
        return TaylorSin(PI - x);   // sin(x) = sin(pi - x) for x in (pi/2, pi]
    }
    if (x < -HALF_PI) {
        return TaylorSin(-PI - x);  // sin(x) = sin(-pi - x) for x in [-pi, -pi/2)
    }
    return TaylorSin(x);
}

float MyCosf(float x) {
    const float PI = 3.14159265f;
    const float TWO_PI = 6.28318530f;
    float shifted = x + 1.57079633f;
    shifted = MyFmodf(shifted, TWO_PI);
    if (shifted > PI) shifted -= TWO_PI;
    if (shifted < -PI) shifted += TWO_PI;
    return MySinf(shifted);
}

static unsigned int GetPlayerPositionRef()
{
    unsigned int base = *(unsigned int*)0xF65CA0;
    unsigned int playerRef = *(unsigned int*)(base + 4216);
    if (!playerRef) return 0;

    unsigned int flags = *(unsigned int*)(playerRef + 192);
    if (!(flags & 2)) return 0; // not valid/live right now

    return playerRef;
}

int SetCameraHook(int renderTarget, float* viewMat) {
    bool anyMode = g_FreeCamEnabled || g_LevelEditorEnabled;

    if (anyMode &&
        g_ActiveCameraRenderTarget &&
        renderTarget == g_ActiveCameraRenderTarget) {

        // Initialize from current view on first enable
        if (!g_FreeCamInitialized && g_FreeCamEnabled) {
            float tx = viewMat[12], ty = viewMat[13], tz = viewMat[14];
            g_FreeCamPos[0] = -(viewMat[0] * tx + viewMat[4] * ty + viewMat[8] * tz);
            g_FreeCamPos[1] = -(viewMat[1] * tx + viewMat[5] * ty + viewMat[9] * tz);
            g_FreeCamPos[2] = -(viewMat[2] * tx + viewMat[6] * ty + viewMat[10] * tz);
            g_FreeCamYaw = 0.0f;
            g_FreeCamPitch = 0.0f;
            g_FreeCamInitialized = true;
        }

        if (!g_OrbitInitialized && g_LevelEditorEnabled) {
            unsigned int playerPosRef = GetPlayerPositionRef();
            if (playerPosRef) {
                g_OrbitPivot[0] = *(float*)(playerPosRef + 144);
                g_OrbitPivot[1] = *(float*)(playerPosRef + 148);
                g_OrbitPivot[2] = *(float*)(playerPosRef + 152);
            }
            else {
                float tx = viewMat[12], ty = viewMat[13], tz = viewMat[14];
                g_OrbitPivot[0] = -(viewMat[0] * tx + viewMat[4] * ty + viewMat[8] * tz);
                g_OrbitPivot[1] = -(viewMat[1] * tx + viewMat[5] * ty + viewMat[9] * tz);
                g_OrbitPivot[2] = -(viewMat[2] * tx + viewMat[6] * ty + viewMat[10] * tz);
            }
            g_OrbitYaw = 0.0f;
            g_OrbitPitch = 0.3f;
            g_OrbitRadius = 10.0f;
            g_OrbitInitialized = true;
        }

        float ex, ey, ez;
        float mat[16];

        if (g_FreeCamEnabled) {
            float cp = MyCosf(g_FreeCamPitch), sp = MySinf(g_FreeCamPitch);
            float cy = MyCosf(g_FreeCamYaw), sy = MySinf(g_FreeCamYaw);

            float rx = cy, ry = 0.0f, rz = -sy;
            float ux = sy * sp, uy = cp, uz = cy * sp;
            float fx = sy * cp, fy = -sp, fz = cy * cp;

            ex = g_FreeCamPos[0];
            ey = g_FreeCamPos[1];
            ez = g_FreeCamPos[2];

            mat[0] = rx;  mat[1] = ux;  mat[2] = fx;  mat[3] = 0;
            mat[4] = ry;  mat[5] = uy;  mat[6] = fy;  mat[7] = 0;
            mat[8] = rz;  mat[9] = uz;  mat[10] = fz; mat[11] = 0;
            mat[12] = -(rx * ex + ry * ey + rz * ez);
            mat[13] = -(ux * ex + uy * ey + uz * ez);
            mat[14] = -(fx * ex + fy * ey + fz * ez);
            mat[15] = 1;

        }
        else {
            // Level Editor orbit camera
            float cp = MyCosf(g_OrbitPitch), sp = MySinf(g_OrbitPitch);
            float cy = MyCosf(g_OrbitYaw), sy = MySinf(g_OrbitYaw);

            ex = g_OrbitPivot[0] - g_OrbitRadius * cp * sy;
            ey = g_OrbitPivot[1] - g_OrbitRadius * sp;
            ez = g_OrbitPivot[2] - g_OrbitRadius * cp * cy;

            float viewYaw = g_OrbitYaw + 3.14159265f;
            float viewPitch = -g_OrbitPitch;

            float vcp = MyCosf(viewPitch), vsp = MySinf(viewPitch);
            float vcy = MyCosf(viewYaw), vsy = MySinf(viewYaw);

            float rx = vcy, ry = 0.0f, rz = -vsy;
            float ux = vsy * vsp, uy = vcp, uz = vcy * vsp;
            float fx = vsy * vcp, fy = -vsp, fz = vcy * vcp;

            mat[0] = rx;  mat[1] = ux;  mat[2] = fx;  mat[3] = 0;
            mat[4] = ry;  mat[5] = uy;  mat[6] = fy;  mat[7] = 0;
            mat[8] = rz;  mat[9] = uz;  mat[10] = fz; mat[11] = 0;
            mat[12] = -(rx * ex + ry * ey + rz * ez);
            mat[13] = -(ux * ex + uy * ey + uz * ez);
            mat[14] = -(fx * ex + fy * ey + fz * ez);
            mat[15] = 1;
        }

        int ret = g_SetCameraHook->invoke<int>(renderTarget, mat);
        return ret;
    }

    if (!g_FreeCamEnabled && !g_LevelEditorEnabled) {
        g_FreeCamInitialized = false;
        g_OrbitInitialized = false;
        g_FreeCamVPValid = false;
    }

    return g_SetCameraHook->invoke<int>(renderTarget, viewMat);
}




static unsigned int GetWritablePlayerPosBase()
{
    unsigned int playerRef = GetPlayerPositionRef(); // existing function, unchanged
    if (!playerRef) return 0;

    unsigned int havokController = *(unsigned int*)(playerRef + 396);
    return havokController;
}

void Action_DeleteCfgFile()
{
    int ret = cellFsUnlink(kCfgPath);
    if (ret == CELL_FS_SUCCEEDED)
        printf("[Save] Deleted cfg file\n");
    else
        printf("[Save] Delete failed err=0x%X\n", ret);

    // Clear in-memory saved list + un-star everything, since the file is gone
    g_SavedTunableCount = 0;
}

void Action_ReloadCfgFile()
{
    g_SavedTunableCount = 0; // clear current stars before reloading from disk
    LoadSaveFile();
    printf("[Save] Reloaded cfg file\n");
}

bool StringStartsWith(const char* str, const char* prefix)
{
    int i = 0;
    while (prefix[i] != '\0') {
        if (str[i] != prefix[i]) return false;
        i++;
    }
    return true;
}

int GetNextSegment(const char* name, int startPos, char* outSegment, int maxLen)
{
    int i = 0;
    while (name[startPos + i] != '\0' && name[startPos + i] != '.' && i < maxLen - 1) {
        outSegment[i] = name[startPos + i];
        i++;
    }
    outSegment[i] = '\0';
    return i;
}


static inline bool StrEqual(const char* a, const char* b) {
    while (*a && *b) { if (*a != *b) return false; a++; b++; }
    return *a == *b;
}

int StrLen(const char* s)
{
    int len = 0;
    while (s[len] != '\0') len++;
    return len;
}

int FindGlobalJobIndex(const char* jobKey)
{
    for (int i = 0; i < g_AllJobsCount; i++) {
        if (StrEqual(g_AllJobs[i].jobKey, jobKey)) return g_AllJobs[i].globalIndex;
    }
    return -1;
}

static int FindSavedTunable(const char* name)
{
    for (int i = 0; i < g_SavedTunableCount; i++) {
        if (StrEqual(g_SavedTunables[i].name, name))
            return i;
    }
    return -1;
}

static void MarkTunableSaved(const char* name, unsigned int liveAddr, int type)
{
    int size = GetTunableByteSize(type);
    if (size > 64) size = 64;

    int idx = FindSavedTunable(name);
    if (idx < 0) {
        if (g_SavedTunableCount >= 256) return;
        idx = g_SavedTunableCount++;
        int k = 0;
        while (name[k] != '\0' && k < 127) { g_SavedTunables[idx].name[k] = name[k]; k++; }
        g_SavedTunables[idx].name[k] = '\0';
    }

    unsigned char* src = (unsigned char*)(liveAddr + 72);
    for (int b = 0; b < size; b++)
        g_SavedTunables[idx].data[b] = src[b];
    g_SavedTunables[idx].dataSize = size;
}

static void RemoveSavedTunable(const char* name)
{
    int idx = FindSavedTunable(name);
    if (idx < 0) return;
    for (int i = idx; i < g_SavedTunableCount - 1; i++)
        g_SavedTunables[i] = g_SavedTunables[i + 1];
    g_SavedTunableCount--;
}

static void ToggleTunableSaved(const char* name, unsigned int liveAddr, int type)
{
    if (FindSavedTunable(name) >= 0)
        RemoveSavedTunable(name);
    else
        MarkTunableSaved(name, liveAddr, type);
}

static const char* GetItemSaveName(DisplayItem& item)
{
    if (item.tunableIndex >= 0)
        return g_TunableTable[item.tunableIndex].name;
    for (int i = 0; i < g_SyntheticItemCount; i++) {
        if (g_SyntheticItems[i].liveAddr == item.liveAddr)
            return g_SyntheticItems[i].name;
    }
    return "";
}
static bool IsItemSaved(DisplayItem& item)
{
    const char* name = GetItemSaveName(item);
    if (name[0] == '\0') return false;
    return FindSavedTunable(name) >= 0;
}

static bool ResolveItemByName(const char* name, unsigned int& outLiveAddr, int& outType)
{
    for (int i = 0; i < g_SyntheticItemCount; i++) {
        if (StrEqual(g_SyntheticItems[i].name, name)) {
            outLiveAddr = g_SyntheticItems[i].liveAddr;
            outType = g_SyntheticItems[i].type;
            return true;
        }
    }
    for (int i = 0; i < g_TunableCount; i++) {
        if (StrEqual(g_TunableTable[i].name, name)) {
            unsigned int liveAddr = FindLiveTunable(g_TunableTable[i].name);
            if (!liveAddr) return false;
            outLiveAddr = liveAddr;
            outType = GetTunableType(*(unsigned int*)liveAddr);
            return true;
        }
    }
    return false; 
}

static int ParseIntManual(const char* s)
{
    int sign = 1, result = 0;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') { result = result * 10 + (*s - '0'); s++; }
    return result * sign;
}

static float ParseFloatManual(const char* s)
{
    int sign = 1;
    if (*s == '-') { sign = -1; s++; }
    float intPart = 0.0f;
    while (*s >= '0' && *s <= '9') { intPart = intPart * 10.0f + (*s - '0'); s++; }
    float frac = 0.0f;
    if (*s == '.') {
        s++;
        float scale = 0.1f;
        while (*s >= '0' && *s <= '9') { frac += (*s - '0') * scale; scale *= 0.1f; s++; }
    }
    return sign * (intPart + frac);
}

static char* FormatTunableValue(unsigned int liveAddr, int type)
{
    switch (type) {
    case TT_BOOL:  return FormatStr("%d", (*(unsigned char*)(liveAddr + 72)) ? 1 : 0);
    case TT_INT:   return FormatStr("%d", *(int*)(liveAddr + 72));
    case TT_FLOAT: return FormatStr("%.6f", *(float*)(liveAddr + 72));
    case TT_COLOR: {
        unsigned char r = *(unsigned char*)(liveAddr + 72);
        unsigned char g = *(unsigned char*)(liveAddr + 73);
        unsigned char b = *(unsigned char*)(liveAddr + 74);
        unsigned char a = *(unsigned char*)(liveAddr + 75);
        return FormatStr("%d,%d,%d,%d", r, g, b, a);
    }
    case TT_VEC2: return FormatStr("%.6f,%.6f", *(float*)(liveAddr + 72), *(float*)(liveAddr + 76));
    case TT_VEC3: return FormatStr("%.6f,%.6f,%.6f", *(float*)(liveAddr + 72), *(float*)(liveAddr + 76), *(float*)(liveAddr + 80));
    case TT_STRING: return FormatStr("\"%s\"", (const char*)(liveAddr + 72));
    case TT_STRING_HASH: return FormatStr("0x%08X", *(unsigned int*)(liveAddr + 72));
    default: return FormatStr("0");
    }
}

static void ApplyTunableValue(unsigned int liveAddr, int type, const char* valueStr)
{
    switch (type) {
    case TT_BOOL: *(unsigned char*)(liveAddr + 72) = (ParseIntManual(valueStr) != 0) ? 1 : 0; break;
    case TT_INT:  *(int*)(liveAddr + 72) = ParseIntManual(valueStr); break;
    case TT_FLOAT:*(float*)(liveAddr + 72) = ParseFloatManual(valueStr); break;
    case TT_COLOR: {
        int vals[4] = { 0,0,0,0 }; int idx = 0; const char* p = valueStr;
        while (*p && idx < 4) {
            vals[idx++] = ParseIntManual(p);
            while (*p && *p != ',') p++;
            if (*p == ',') p++;
        }
        *(unsigned char*)(liveAddr + 72) = (unsigned char)vals[0];
        *(unsigned char*)(liveAddr + 73) = (unsigned char)vals[1];
        *(unsigned char*)(liveAddr + 74) = (unsigned char)vals[2];
        *(unsigned char*)(liveAddr + 75) = (unsigned char)vals[3];
        break;
    }
    case TT_VEC2:
    case TT_VEC3: {
        float vals[3] = { 0,0,0 }; int count = (type == TT_VEC3) ? 3 : 2; int idx = 0; const char* p = valueStr;
        while (*p && idx < count) {
            vals[idx++] = ParseFloatManual(p);
            while (*p && *p != ',') p++;
            if (*p == ',') p++;
        }
        for (int k = 0; k < count; k++) *(float*)(liveAddr + 72 + k * 4) = vals[k];
        break;
    }
    case TT_STRING: {
        const char* p = valueStr;
        if (*p == '"') p++;
        char* dst = (char*)(liveAddr + 72);
        int k = 0;
        while (*p && *p != '"' && k < 63) dst[k++] = *p++;
        dst[k] = '\0';
        break;
    }
    case TT_STRING_HASH: {
        const char* p = valueStr;
        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) p += 2;
        unsigned int result = 0;
        while (*p) {
            char c = *p; unsigned int digit;
            if (c >= '0' && c <= '9') digit = c - '0';
            else if (c >= 'A' && c <= 'F') digit = 10 + (c - 'A');
            else if (c >= 'a' && c <= 'f') digit = 10 + (c - 'a');
            else break;
            result = result * 16 + digit; p++;
        }
        *(unsigned int*)(liveAddr + 72) = result;
        break;
    }
    }
}

static bool EnsureSaveDirectory(const char* path)
{
    int ret = cellFsMkdir(path, CELL_FS_DEFAULT_CREATE_MODE_1);
    if (ret == CELL_FS_SUCCEEDED || ret == CELL_FS_EEXIST) return true;
    printf("[Save] mkdir failed '%s' err=0x%X\n", path, ret);
    return false;
}

void WriteSaveFile()
{
    if (!EnsureSaveDirectory(kSaveDir)) return;

    int fd;
    int ret = cellFsOpen(kCfgPath, CELL_FS_O_CREAT | CELL_FS_O_WRONLY | CELL_FS_O_TRUNC, &fd, NULL, 0);
    if (ret != CELL_FS_SUCCEEDED) { printf("[Save] open-for-write failed err=0x%X\n", ret); return; }

    const char* header = "# Sly4 Debug Menu - Saved Settings\n";
    uint64_t written;
    cellFsWrite(fd, header, StrLen(header), &written);

    for (int i = 0; i < g_SavedTunableCount; i++) {
        unsigned int liveAddr; int type;
        if (!ResolveItemByName(g_SavedTunables[i].name, liveAddr, type)) continue;
        char* valueStr = FormatTunableValue(liveAddr, type);
        char* line = FormatStr("\"%s\" = %s\n", g_SavedTunables[i].name, valueStr);
        cellFsWrite(fd, line, StrLen(line), &written);
    }
    cellFsClose(fd);
    printf("[Save] Wrote %d entries\n", g_SavedTunableCount);
}

void LoadSaveFile()
{
    int fd;
    int ret = cellFsOpen(kCfgPath, CELL_FS_O_RDONLY, &fd, NULL, 0);
    if (ret != CELL_FS_SUCCEEDED) { printf("[Save] no save file yet (err=0x%X)\n", ret); return; }

    static char buffer[8192];
    uint64_t bytesRead = 0;
    cellFsRead(fd, buffer, sizeof(buffer) - 1, &bytesRead);
    cellFsClose(fd);
    buffer[bytesRead] = '\0';

    char* p = buffer;
    while (*p) {
        while (*p == '\r' || *p == '\n' || *p == ' ') p++;
        if (*p == '\0') break;
        if (*p == '#' || *p != '"') { while (*p && *p != '\n') p++; continue; }
        p++; // skip opening quote

        char name[128]; int k = 0;
        while (*p && *p != '"' && k < 127) name[k++] = *p++;
        name[k] = '\0';
        if (*p == '"') p++;

        while (*p == ' ') p++;
        if (*p == '=') p++;
        while (*p == ' ') p++;

        char value[128]; k = 0;
        while (*p && *p != '\n' && *p != '\r' && k < 127) value[k++] = *p++;
        value[k] = '\0';

        unsigned int liveAddr; int type;
        if (ResolveItemByName(name, liveAddr, type)) {
            ApplyTunableValue(liveAddr, type, value);
            MarkTunableSaved(name, liveAddr, type); // re-star immediately on load
            printf("[Save] Loaded '%s' = %s\n", name, value);
        }
        else {
            printf("[Save] Skipped unknown '%s'\n", name);
        }
    }
}

// --- FPS tracking state ---
static uint32_t s_lastTime = 0;
static uint32_t s_frameCount = 0;
static float s_currentFps = 0.0f;
static uint32_t s_timebaseFreq = 0;
static uint32_t s_lastObservedTime = 0;

int SysTimeWrapperHook()  // matches sub_704198's own signature/return width
{
    int result = g_SystemTimeHook->invoke<int>();
    s_lastObservedTime = (uint32_t)result;
    return result;
}

static void ActivateEntity(int entity) {
    if (!entity) return;
    unsigned int* vtable = *(unsigned int**)entity;
    typedef void(__fastcall* Activate_t)(int, int, int, int, int);
    Activate_t activate = (Activate_t)vtable[22];  // vtable[+88] = slot 22
    activate(entity, 0, 1, 1, 1);
}



void UpdateFps()
{
    uint32_t now = s_lastObservedTime; // updated passively whenever the game calls it

    s_frameCount++;
    if (s_lastTime == 0)
        s_lastTime = now;

    uint32_t elapsed = now - s_lastTime;
    if (elapsed >= 1000000 && now != 0)
    {
        s_currentFps = (float)s_frameCount * 1000000.0f / (float)elapsed;
        s_frameCount = 0;
        s_lastTime = now;
    }
}



bool IsCanonicalRootCategory(const char* name)
{
    for (int i = 0; i < g_CanonicalRootCategoryCount; i++) {
        bool match = true;
        int k = 0;
        while (true) {
            char a = name[k];
            char b = g_CanonicalRootCategories[i][k];
            if (a != b) { match = false; break; }
            if (a == '\0') break;
            k++;
        }
        if (match) return true;
    }
    return false;
}



bool ButtonCheckHook(int playerContext, int buttonCode)
{
    if (g_MenuVisible) {
        return 0;
    }
    return g_ButtonCheckHook->invoke<bool>(playerContext, buttonCode);
}



void DrawDebugRect(float x, float y, float width, float height, float z, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    int context = ApeDebug_RenderContext;
    int material = MaterialLookup(0);
    BindMaterial(context, material);
    BeginBatch(context, &ApeDebug_DefaultBatchState);

    struct { void* ptr; short count; } vertHandle;
    AllocVerts(&vertHandle, context, 6, 4);   // 6 indices, 4 vertices — same as the original (broken) rect path

    float x2 = x + width;
    float y2 = y + height;

    float* v = (float*)vertHandle.ptr;

    // Vertex 0: top-left, UV (0,0)
    v[0] = x;  v[1] = y;  v[2] = z;
    unsigned char* c0 = (unsigned char*)(v + 3);
    c0[0] = r; c0[1] = g; c0[2] = b; c0[3] = a;
    v[4] = 0.0f; v[5] = 0.0f;   // UV — written into the bytes the original code left as zero, but as REAL UV this time

    v += 6;   // 24-byte stride = 6 floats per vertex

    // Vertex 1: top-right, UV (1,0)
    v[0] = x2; v[1] = y;  v[2] = z;
    unsigned char* c1 = (unsigned char*)(v + 3);
    c1[0] = r; c1[1] = g; c1[2] = b; c1[3] = a;
    v[4] = 1.0f; v[5] = 0.0f;

    v += 6;

    // Vertex 2: bottom-left, UV (0,1)
    v[0] = x;  v[1] = y2; v[2] = z;
    unsigned char* c2 = (unsigned char*)(v + 3);
    c2[0] = r; c2[1] = g; c2[2] = b; c2[3] = a;
    v[4] = 0.0f; v[5] = 1.0f;

    v += 6;

    // Vertex 3: bottom-right, UV (1,1)
    v[0] = x2; v[1] = y2; v[2] = z;
    unsigned char* c3 = (unsigned char*)(v + 3);
    c3[0] = r; c3[1] = g; c3[2] = b; c3[3] = a;
    v[4] = 1.0f; v[5] = 1.0f;
}

float CalcMenuWidth()
{
    const char* header = g_CurrentPath[0] ? g_CurrentPath : "Debug Menu";
    int maxLen = StrLen(header) + 2;

    for (int i = 0; i < g_DisplayItemCount; i++) {
        int len = StrLen(g_DisplayItems[i].label) + 4;
        len += g_DisplayItems[i].isCategory ? 3 : 8;
        if (len > maxLen) maxLen = len;
    }

    return maxLen * RECT_CHAR_WIDTH + RECT_PADDING_X * 2;
}


void DrawFpsCounter()
{
    char fpsStr[16];
    sprintf(fpsStr, "FPS: %.1f", s_currentFps);

    // Top-right anchor — same normalized 0-1 space as your QueueText calls
    float posX = 0.82f;
    float posY = 0.05f;

    // Rough box sized for "FPS: XX.X" — tune width/height once you see
    // actual text metrics at your render scale.
    float boxWidth = 0.13f;
    float boxHeight = 0.035f;
    float padding = 0.005f;

    // Draw the rect FIRST, slightly behind in Z, so the real text paints on top
    DrawDebugRect(
        posX - padding,
        posY - padding,
        boxWidth + padding * 2.0f,
        boxHeight + padding * 2.0f,
        0.01f,                       // z — push back slightly behind text's z (0.0f)
        20, 20, 20, 200              // dark, semi-opaque
    );

    // Real FPS text, drawn after (on top), same anchor as before
    unsigned char textColor[4] = { 0, 255, 120, 255 }; // green for visibility
    float textPos[3] = { posX, posY, 0.0f };
    QueueText(textPos, (int)fpsStr, textColor, 1.0f);
}

void DrawMenuBackground() {
    int itemCount;
    int maxLen;

    if (StrEqual(g_CurrentPath, "EpisodeJobs")) {
        float width = g_CurrentMenuMaxLen * RECT_CHAR_WIDTH + RECT_PADDING_X * 2;
        int lineCount = 1 + 1 + g_DisplayItemCount + 1 + 2 + 2;
        float height = (lineCount + 1) * RECT_LINE_HEIGHT + RECT_PADDING_Y * 2;
        float originX = RECT_LEFT_ANCHOR + (g_MenuDragX / kRectScaleX);
        float originY = (RECT_TOP_ANCHOR - (g_MenuDragY / kRectScaleY)) - height;
        DrawDebugRect(originX, originY, width, height, 0.0f,
            g_MenuBgColorTunable.r, g_MenuBgColorTunable.g, g_MenuBgColorTunable.b, g_MenuBgColorTunable.a);
        return;
    }

    if (StrEqual(g_CurrentPath, "JobGoals")) {
        float width = g_CurrentMenuMaxLen * RECT_CHAR_WIDTH + RECT_PADDING_X * 2;
        int lineCount = 1 + g_DisplayItemCount;
        float height = (lineCount + 1) * RECT_LINE_HEIGHT * (20.0f / 16.0f) + RECT_PADDING_Y * 2;
        float originX = RECT_LEFT_ANCHOR + (g_MenuDragX / kRectScaleX);
        float originY = (RECT_TOP_ANCHOR - (g_MenuDragY / kRectScaleY)) - height;
        DrawDebugRect(originX, originY, width, height, 0.0f,
            g_MenuBgColorTunable.r, g_MenuBgColorTunable.g, g_MenuBgColorTunable.b, g_MenuBgColorTunable.a);
        return;
    }

    if (g_MenuState == MENU_STATE_TOP) {
        itemCount = g_TopMenuItemCount;
        maxLen = StrLen("Debug Menu") + 2;
        for (int i = 0; i < g_TopMenuItemCount; i++) {
            int len = StrLen(g_MenuItems[i]) + 4 + 3;
            if (len > maxLen) maxLen = len;
        }
    }
    else if (g_MenuState == MENU_STATE_LOAD) {
        itemCount = g_LoadMenuItemCount;
        maxLen = StrLen("Load") + 2;
        for (int i = 0; i < g_LoadMenuItemCount; i++) {
            int len = StrLen(g_LoadMenuItems[i]) + 4 + 3;
            if (len > maxLen) maxLen = len;
        }
    }
    else if (g_MenuState == MENU_STATE_SCENE) {
        itemCount = g_SceneMenuItemCount + 1;
        maxLen = StrLen("Scene") + 2;
        for (int i = 0; i < g_SceneMenuItemCount; i++) {
            int len = StrLen(g_SceneMenuItems[i]) + 4 + 3;
            if (len > maxLen) maxLen = len;
        }
        // Use cached name — not recalculated every frame
        int countLen = StrLen("  Entities: 000  [") + StrLen(g_DisplaySceneName) + 2;
        if (countLen > maxLen) maxLen = countLen;
    }
    else {
        const char* header = g_CurrentPath[0] ? g_CurrentPath : "Tuning";
        maxLen = StrLen(header) + 2;
        itemCount = g_DisplayItemCount;
        for (int i = 0; i < g_DisplayItemCount; i++) {
            int len = StrLen(g_DisplayItems[i].label) + 4;
            len += g_DisplayItems[i].isCategory ? 3 : 8;
            if (len > maxLen) maxLen = len;
        }
    }

    int extraLines = 0;
    if (g_MenuState != MENU_STATE_SCENE) {
        if (g_ScrollOffset > 0) extraLines++;
        if (g_ScrollOffset + (g_DisplayItemCount > MENU_WINDOW_SIZE ? MENU_WINDOW_SIZE : g_DisplayItemCount) < g_DisplayItemCount) extraLines++;
    }

    int visibleItems = (g_MenuState == MENU_STATE_SCENE) ? itemCount :
        ((g_DisplayItemCount > MENU_WINDOW_SIZE) ? MENU_WINDOW_SIZE : g_DisplayItemCount);
    int lineCount = 1 + visibleItems + extraLines;

    float width = maxLen * RECT_CHAR_WIDTH + RECT_PADDING_X * 2;
    float height = (lineCount + 1) * RECT_LINE_HEIGHT + RECT_PADDING_Y * 2;
    float originX = RECT_LEFT_ANCHOR + (g_MenuDragX / kRectScaleX);
    float originY = (RECT_TOP_ANCHOR - (g_MenuDragY / kRectScaleY)) - height;

    DrawDebugRect(originX, originY, width, height, 0.0f,
        g_MenuBgColorTunable.r, g_MenuBgColorTunable.g, g_MenuBgColorTunable.b, g_MenuBgColorTunable.a);
}

void DrawSelectionHighlight()
{
    if (g_DisplayItemCount == 0) return;

    float width = CalcMenuWidth();
    float rowTop = RECT_TOP_ANCHOR - (g_SelectedIndex + 2) * RECT_LINE_HEIGHT;   // was +1, now +2
    float rowBottom = rowTop - RECT_LINE_HEIGHT;

    DrawDebugRect(RECT_LEFT_ANCHOR, rowBottom, width, RECT_LINE_HEIGHT, 0.0f, 180, 180, 180, 255);
}



void DrawDebugLine(float x1, float y1, float x2, float y2, float z, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    int context = ApeDebug_RenderContext;
    int material = MaterialLookup(0);
    BindMaterial(context, material);
    BeginBatch(context, &ApeDebug_DefaultBatchState);

    struct { void* ptr; short count; } vertHandle;
    AllocVerts(&vertHandle, context, 2, 2);

    float* v = (float*)vertHandle.ptr;
    v[0] = x1; v[1] = y1; v[2] = z;
    unsigned char* c1 = (unsigned char*)(v + 3);
    c1[0] = r; c1[1] = g; c1[2] = b; c1[3] = a;

    v[6] = x2; v[7] = y2; v[8] = z;
    unsigned char* c2 = (unsigned char*)(v + 9);
    c2[0] = r; c2[1] = g; c2[2] = b; c2[3] = a;
    // gap bytes (16-23, 40-47) intentionally left untouched, matching the working radar function exactly
}

void DrawDebugLine3D(float x1, float y1, float z1,
    float x2, float y2, float z2,
    unsigned char r, unsigned char g,
    unsigned char b, unsigned char a) {

    unsigned int episodeMgr = *(unsigned int*)0xF64C20;
    if (!episodeMgr) return;

    int gameMgr = *(int*)0xF65CA0;
    if (!gameMgr) return;
    int renderObj = *(int*)(gameMgr + 0x1285E4);
    if (!renderObj) return;
    float* cmdBuf = *(float**)(renderObj + 12);
    if (!cmdBuf) return;
    if ((unsigned int)cmdBuf < 0x10000) return;

    static OPD s_endVerticesOpd = { (void*)0x523748, (void*)0xF50800 };
    typedef void (*EndVertices_t)(int);
    EndVertices_t EndVertices = (EndVertices_t)&s_endVerticesOpd;

    int material = MaterialLookup(0);
    BindMaterial((int)cmdBuf, material);
    BeginBatch((int)cmdBuf, (float*)&ApeDebug_DefaultBatchState);

    struct { void* ptr; short count; } vertHandle;
    AllocVerts(&vertHandle, (int)cmdBuf, 2, 2);

    float* v = (float*)vertHandle.ptr;
    if (!v) return;
    if ((unsigned int)v < 0x10000) return;

    v[0] = x1; v[1] = y1; v[2] = z1;
    unsigned char* c1 = (unsigned char*)(v + 3);
    c1[0] = r; c1[1] = g; c1[2] = b; c1[3] = a;

    v[6] = x2; v[7] = y2; v[8] = z2;
    unsigned char* c2 = (unsigned char*)(v + 9);
    c2[0] = r; c2[1] = g; c2[2] = b; c2[3] = a;

    EndVertices((int)cmdBuf);
}

void DrawSpawnRotationIndicator() {
    if (!StrEqual(g_CurrentPath, "EntitySpawn")) return;

    float rx = g_SpawnRotation[0] * 3.14159265f / 180.0f;
    float ry = g_SpawnRotation[1] * 3.14159265f / 180.0f;
    float rz = g_SpawnRotation[2] * 3.14159265f / 180.0f;
    float cx = MyCosf(rx), sx = MySinf(rx);
    float cy = MyCosf(ry), sy = MySinf(ry);
    float cz = MyCosf(rz), sz = MySinf(rz);

    float m00 = cy * cz + sy * sx * sz;   float m01 = -cy * sz + sy * sx * cz;  float m02 = sy * cx;
    float m10 = cx * sz;               float m11 = cx * cz;             float m12 = -sx;
    float m20 = -sy * cz + cy * sx * sz;  float m21 = sy * sz + cy * sx * cz;  float m22 = cy * cx;

    float cxs = 0.82f;   // right side, away from menu
    float cys = 0.15f;   // near top
    float baseLen = 1.0f;   // shorter lines
    float perspFactor = 2.0f;
    float z = 5.0f;

    // X axis — red
    {
        float depth = m01;
        float len = baseLen / (1.0f + depth * perspFactor);
        // Clamp to prevent blowup near the flip point
        if (len > baseLen * 2.0f) len = baseLen * 2.0f;
        if (len < baseLen * 0.3f) len = baseLen * 0.3f;
       // unsigned char alpha = depth > 0 ? 255 : 60;
        float ex = cxs + m00 * len;
        float ey = cys - m02 * len;
        DrawDebugLine(cxs, cys, ex, ey, z, 255, 50, 50, 255);
        float labelPos[3] = { ex + 0.008f, ey - 0.008f, 0.0f };
        unsigned char col[4] = { 255, 50, 50, 255 };
        
    }

    // Y axis — green
    {
        float depth = m11;
        float len = baseLen / (1.0f + depth * perspFactor);
        if (len > baseLen * 2.0f) len = baseLen * 2.0f;
        if (len < baseLen * 0.3f) len = baseLen * 0.3f;
       // unsigned char alpha = depth > 0 ? 255 : 60;
        float ex = cxs + m10 * len;
        float ey = cys - m12 * len;
        DrawDebugLine(cxs, cys, ex, ey, z, 50, 255, 50, 255);
        float labelPos[3] = { ex + 0.008f, ey - 0.008f, 0.0f };
        unsigned char col[4] = { 50, 255, 50, 255 };
       
    }

    // Z axis — blue
    {
        float depth = m21;
        float len = baseLen / (1.0f + depth * perspFactor);
        if (len > baseLen * 2.0f) len = baseLen * 2.0f;
        if (len < baseLen * 0.3f) len = baseLen * 0.3f;
       // unsigned char alpha = depth > 0 ? 255 : 60;
        float ex = cxs + m20 * len;
        float ey = cys - m22 * len;
        DrawDebugLine(cxs, cys, ex, ey, z, 50, 100, 255, 255);
        float labelPos[3] = { ex + 0.008f, ey - 0.008f, 0.0f };
        unsigned char col[4] = { 50, 100, 255, 255 };
        
    }
}

void DrawSpawnRotationGizmo() {
    if (!StrEqual(g_CurrentPath, "EntitySpawn")) return;
    if (!g_FreeCamEnabled) return;

    int gameMgr = *(int*)0xF65CA0;
    if (!gameMgr) return;

    int renderObj = *(int*)(gameMgr + 0x1285E4);
    if (!renderObj) return;

    int* cmdBuf = *(int**)(renderObj + 12);
    if (!cmdBuf) return;

    static int* lastCmdBuf = nullptr;
    if (cmdBuf == lastCmdBuf) return;
    lastCmdBuf = cmdBuf;

    // 1. Material first
    typedef int (*MaterialLookup_t)(int);
    MaterialLookup_t MaterialLookup = (MaterialLookup_t)&MaterialLookup_opd;
    int material = MaterialLookup(0);

    typedef int* (*AddMaterial_t)(int* cmdBuf, int material);
    AddMaterial_t BindMaterial = (AddMaterial_t)&BindMaterial_opd;
    BindMaterial(cmdBuf, material);

    // 2. Matrix second
    float rx = g_SpawnRotation[0] * 3.14159265f / 180.0f;
    float ry = g_SpawnRotation[1] * 3.14159265f / 180.0f;
    float rz = g_SpawnRotation[2] * 3.14159265f / 180.0f;
    float cx = MyCosf(rx), sx = MySinf(rx);
    float cy = MyCosf(ry), sy = MySinf(ry);
    float cz = MyCosf(rz), sz = MySinf(rz);

    float mat[16] = {
         cy * cz + sy * sx * sz, -cy * sz + sy * sx * cz,  sy * cx, 0,
         cx * sz,             cx * cz,             -sx,    0,
        -sy * cz + cy * sx * sz,  sy * sz + cy * sx * cz,   cy * cx, 0,
         g_FreeCamPos[0],   g_FreeCamPos[1],    g_FreeCamPos[2], 1
    };

    typedef int* (*AddLtoWMatrix_t)(int* cmdBuf, const float* mtx);
    AddLtoWMatrix_t BeginBatch = (AddLtoWMatrix_t)&BeginBatch_opd;
    BeginBatch(cmdBuf, mat);

    // 3. BeginVertices last
    int outBuf[8] = {};
    BeginVertices_t AllocVerts = (BeginVertices_t)&AllocVerts_opd;
    AllocVerts(outBuf, (int)cmdBuf, 1, 6);

    float* vertPtr = (float*)outBuf[0];
    if (!vertPtr || (unsigned int)vertPtr < 0x10000) return;

    float len = 5.0f;
    float* v = vertPtr;

    // X axis — red, stride 6 floats per vertex
    v[0] = 0;   v[1] = 0; v[2] = 0;   *(unsigned int*)&v[3] = 0xFF0000FF;
    v[6] = len; v[7] = 0; v[8] = 0;   *(unsigned int*)&v[9] = 0xFF0000FF;
    // Y axis — green
    v[12] = 0; v[13] = 0;   v[14] = 0; *(unsigned int*)&v[15] = 0x00FF00FF;
    v[18] = 0; v[19] = len; v[20] = 0; *(unsigned int*)&v[21] = 0x00FF00FF;
    // Z axis — blue
    v[24] = 0; v[25] = 0; v[26] = 0;   *(unsigned int*)&v[27] = 0x0000FFFF;
    v[30] = 0; v[31] = 0; v[32] = len; *(unsigned int*)&v[33] = 0x0000FFFF;
}

void Action_DumpLevelEditorVtable()
{
    FindByName_t FindByName = (FindByName_t)&s_findByNameOpd;

    int modeObj = FindByName("LevelEditor", 0);
    printf("[Debug] LevelEditor mode obj = 0x%08X\n", modeObj);
    if (modeObj) {
        unsigned int* vtable = *(unsigned int**)modeObj;
        for (int i = 0; i < 60; i += 4)
            printf("[Debug] LE vtable+%d = 0x%08X\n", i, vtable[i / 4]);
    }
}

void Action_DumpInGameVtable()
{
    FindByName_t FindByName = (FindByName_t)&s_findByNameOpd;

    int modeObj = FindByName("InGame", 0);
    printf("[Debug] InGame mode obj = 0x%08X\n", modeObj);
    if (modeObj) {
        unsigned int* vtable = *(unsigned int**)modeObj;
        for (int i = 0; i < 60; i += 4)
            printf("[Debug] IG vtable+%d = 0x%08X\n", i, vtable[i / 4]);
    }
}

void Action_DumpArtViewerVtable()
{
    FindByName_t FindByName = (FindByName_t)&s_findByNameOpd;

    int modeObj = FindByName("ArtViewer", 0);
    printf("[Debug] ArtViewer mode obj = 0x%08X\n", modeObj);
}

void Action_DumpSlyLookup()
{
    FindByName_t FindByName = (FindByName_t)&s_findByNameOpd;

    int entityObj = FindByName("Sly", 0);
    printf("[Debug] 'Sly' lookup = 0x%08X\n", entityObj);
}

void Action_ScanPlayerStructForPointers()
{
    unsigned int base = *(unsigned int*)0xF65CA0;
    printf("[Debug] Scanning player struct at base 0x%08X\n", base);

    for (int offset = 0; offset < 4056; offset += 4) {
        unsigned int value = *(unsigned int*)(base + offset);

        // PS3 user-space heap addresses are typically in this broad range —
        // adjust if you know tighter bounds from this session's confirmed addresses
        if (value >= 0x10000000 && value < 0x60000000) {
            printf("[Debug] +%d (0x%X) = 0x%08X  <- candidate pointer\n", offset, offset, value);
        }
    }
    printf("[Debug] Scan complete\n");
}

void Action_ScanForSlyHash()
{
    HashString_t HashString = (HashString_t)&s_hashStringOpd;
    unsigned int slyHash = HashString("Sly");
    printf("[Debug] hash(\"Sly\") = 0x%08X\n", slyHash);

    // Scan the known live entity cluster we already confirmed this session
    unsigned int base = 0x49DFE8D0;
    for (int offset = 0; offset < 0x400; offset += 4) {
        unsigned int value = *(unsigned int*)(base + offset);
        if (value == slyHash) {
            printf("[Debug] MATCH at base+%d (0x%X)\n", offset, offset);
        }
    }
    printf("[Debug] Scan complete\n");
}

void Action_CheckPlayerRefGated()
{
    unsigned int base = *(unsigned int*)0xF65CA0;
    unsigned int v18 = *(unsigned int*)(base + 4216);

    if (v18 == 0) {
        printf("[Debug] v18 is null\n");
        return;
    }

    unsigned int flags = *(unsigned int*)(v18 + 192);
    printf("[Debug] v18=0x%08X flags=0x%08X (bit2=%d)\n", v18, flags, (flags & 2) != 0);

    if (flags & 2) {
        float x = *(float*)(v18 + 144);
        float y = *(float*)(v18 + 148);
        float z = *(float*)(v18 + 152);
        printf("[Debug] gated position*100 = (%d, %d, %d)\n",
            (int)(x * 100), (int)(y * 100), (int)(z * 100));
    }
}

void Action_SpawnCollectible()
{
    unsigned int playerPosRef = GetPlayerPositionRef();
    if (playerPosRef == 0) {
        printf("[Debug] Player position not available right now\n");
        return;
    }

    ThrowCollectible_t ThrowCollectible = (ThrowCollectible_t)&s_throwCollectibleOpd;

    float px = *(float*)(playerPosRef + 144);
    float py = *(float*)(playerPosRef + 148);
    float pz = *(float*)(playerPosRef + 152);

    ThrowCollectibleParams params = { 0 }; // zero all unknown fields for now

    params.posX = px + 1.0f; // fixed offset — adjust once you see where it lands
    params.posY = py;
    params.posZ = pz;
    params.value = 1.0f;      // unconfirmed units — start small

    ThrowCollectible(&params);
    printf("[Debug] Spawned collectible at (%d, %d, %d)\n",
        (int)(params.posX * 100), (int)(params.posY * 100), (int)(params.posZ * 100));
}

void Action_ScanPlayerRefForHavokPointer(unsigned int targetValue)
{
    unsigned int playerRef = GetPlayerPositionRef();
    if (!playerRef) { printf("[Debug] playerRef is 0\n"); return; }

    printf("[Debug] Scanning playerRef=0x%08X for value 0x%08X\n", playerRef, targetValue);
    for (int offset = 0; offset < 512; offset += 4) {
        unsigned int value = *(unsigned int*)(playerRef + offset);
        if (value == targetValue) {
            printf("[Debug] MATCH at +%d (0x%X)\n", offset, offset);
        }
    }
    printf("[Debug] Scan complete\n");
}

void Action_ComparePositionAddresses()
{
    unsigned int playerRef = GetPlayerPositionRef();
    unsigned int havokController = GetWritablePlayerPosBase();

    unsigned int cachedPosAddr = playerRef + 144;
    unsigned int writablePosAddr = havokController + 0x1E0;

    printf("[Debug] playerRef=0x%08X  havokController=0x%08X\n", playerRef, havokController);
    printf("[Debug] cached pos addr=0x%08X  writable pos addr=0x%08X\n", cachedPosAddr, writablePosAddr);

    float cx = *(float*)cachedPosAddr;
    float wx = *(float*)writablePosAddr;
    printf("[Debug] cached X*100=%d  writable X*100=%d\n", (int)(cx * 100), (int)(wx * 100));
}

void Action_DumpTeleportMatrix()
{
    unsigned int playerBase = *(unsigned int*)0xF65CA0;
    unsigned int matrixAddr = playerBase + 4208 + 16;

    unsigned int havokBase = GetWritablePlayerPosBase();
    float realX = 0.0f, realY = 0.0f, realZ = 0.0f;
    if (havokBase != 0) {
        realX = *(float*)(havokBase + 0x1E0);
        realY = *(float*)(havokBase + 0x1E4);
        realZ = *(float*)(havokBase + 0x1E8);
    }

    printf("[Debug] matrixAddr=0x%08X realPos=(%d,%d,%d)\n",
        matrixAddr, (int)(realX * 100), (int)(realY * 100), (int)(realZ * 100));

    for (int i = 0; i < 16; i++) {
        float v = *(float*)(matrixAddr + i * 4);
        printf("[Debug] matrix[%d] (+%d) = %d\n", i, i * 4, (int)(v * 100));
    }
}

void Action_ToggleNoclip()
{
    static bool noclipOn = false;
    noclipOn = !noclipOn;

    unsigned int playerBase2 = *(unsigned int*)0xF65CA0;
    unsigned int playerPtr = playerBase2 + 4208;
    unsigned int entityPtr = *(unsigned int*)(playerPtr + 8);

    printf("[Debug] playerPtr=0x%08X entityPtr=0x%08X\n", playerPtr, entityPtr);

    if (entityPtr != 0) {
        unsigned int vtable = *(unsigned int*)entityPtr;
        unsigned int func = *(unsigned int*)(vtable + 92);
        printf("[Debug] vtable=0x%08X func@+92=0x%08X\n", vtable, func);

        ToggleCollision_t ToggleCollision = (ToggleCollision_t)func;
        ToggleCollision((int)entityPtr, noclipOn ? 0 : 1, 0);
    }
    else {
        printf("[Debug] entityPtr is NULL — can't proceed\n");
    }

    printf("[Debug] Noclip = %d\n", noclipOn);
}

void Action_LookupAllEpisodeTitles()
{
    unsigned int stringTable = *(unsigned int*)0xF6687C;
    GetStr_t GetStr = (GetStr_t)&s_getStrOpd;

    const char* keys[] = {
        "$ParisPrologueTitleName",
        "$JapanTitleName",
        "$WestTitleName",
        "$IceAgeTitleName",
        "$EnglandTitleName",
        "$ArabiaTitleName",
        "$ParisEpilogueTitleName",
        "$MinigameTitleName",
        "ParisPrologueTitleName",
        "JapanTitleName",
        "WestTitleName",
        "IceAgeTitleName",
        "EnglandTitleName",
        "ArabiaTitleName",
        "ParisEpilogueTitleName",
        "MinigameTitleName"
    };

    for (int i = 0; i < 16; i++) {
        const char* result = GetStr(stringTable, keys[i], "[Missing]", 0);
        printf("[Debug] %s -> %s\n", keys[i], result);
    }
}

void Action_LookupGoalKeys()
{
    unsigned int stringTable = *(unsigned int*)0xF6687C;
    GetStr_t GetStr = (GetStr_t)&s_getStrOpd;

    const char* keys[] = {
        "Japan_SushiShop_Goal01",
        "Japan_SushiShop_Goal02",
        "Japan_SushiShop_Goal03",
        "Japan_SushiShop_Goal04",
        "Japan_SushiShop_Goal05"
    };

    for (int i = 0; i < 5; i++) {
        const char* result = GetStr(stringTable, keys[i], "[Missing]", 0);
        printf("[Debug] %s -> %s\n", keys[i], result);
    }
}

void Action_FindJobIndex()
{
    unsigned int episodeMgr = *(unsigned int*)0xF64C20;
    unsigned int jobArrayBase = *(unsigned int*)(episodeMgr + 28);
    unsigned int jobCount = *(unsigned int*)(episodeMgr + 32);

    for (unsigned int i = 0; i < jobCount; i++) {
        unsigned int jobPtr = jobArrayBase + i * 536;
        const char* name = (const char*)jobPtr; // same convention as Episode's name-at-offset-0
        if (StrEqual(name, "SushiStartup")) { // adjust if the internal id differs
            printf("[Debug] Found job at index %d\n", i);
        }
    }
}

void Action_DumpAllJobs()
{
    unsigned int episodeMgr = *(unsigned int*)0xF64C20;
    unsigned int jobArrayBase = *(unsigned int*)(episodeMgr + 28);
    unsigned int jobCount = *(unsigned int*)(episodeMgr + 32);
    unsigned int episodeArrayBase = *(unsigned int*)(episodeMgr + 20);

    printf("[Debug] jobCount=%d\n", jobCount);
    for (unsigned int i = 0; i < jobCount; i++) {
        unsigned int jobPtr = jobArrayBase + i * 536;
        const char* jobName = (const char*)jobPtr;
        unsigned char episodeIdx = *(unsigned char*)(jobPtr + 500);
        unsigned int episodePtr = episodeArrayBase + episodeIdx * 860;
        const char* episodeName = (const char*)episodePtr;
        printf("[Debug] job[%d] = '%s'  (episode[%d] = '%s')\n", i, jobName, episodeIdx, episodeName);
    }
}

void ShowJobFailScreen(const char* message) {
    unsigned int def = *(unsigned int*)0xF64D04;
    if (!def) return;

    FindJobFailScreen_t FindJobFailScreen = (FindJobFailScreen_t)&s_findJobFailScreenOpd;
    int screen = FindJobFailScreen(def);
    printf("[JobFail] screen=0x%08X\n", screen);
    if (!screen) return;

    // Check the fields Start() reads before crashing
    printf("[JobFail] a1[40]=0x%08X (state, >>1 should not be 1)\n", *(unsigned int*)(screen + 160));
    printf("[JobFail] a1[5] =0x%08X (parent HUD)\n", *(unsigned int*)(screen + 20));
    printf("[JobFail] a1[6] =0x%08X (prev screen)\n", *(unsigned int*)(screen + 24));
    printf("[JobFail] a1[7] =0x%08X (text obj)\n", *(unsigned int*)(screen + 28));
    printf("[JobFail] a1[44]=0x%08X (entity A)\n", *(unsigned int*)(screen + 176));
    printf("[JobFail] a1[45]=0x%08X (entity B)\n", *(unsigned int*)(screen + 180));
    printf("[JobFail] a1[46]=0x%08X (entity C)\n", *(unsigned int*)(screen + 184));
    printf("[JobFail] a1[47]=0x%08X\n", *(unsigned int*)(screen + 188));
    printf("[JobFail] a1[48]=0x%08X\n", *(unsigned int*)(screen + 192));
    printf("[JobFail] calling Start...\n");

    HudJobFailStart_t Start = (HudJobFailStart_t)&s_hudJobFailStartOpd;
    Start(screen, 0, 0, 0, 0, (int)message);
    printf("[JobFail] done\n");
}

void TriggerJobFailed(const char* message, unsigned int charIdx) {
    unsigned int episodeMgr = *(unsigned int*)0xF64C20;
    if (!episodeMgr) return;

    // Already failed check
    if ((((unsigned int*)episodeMgr)[25] >> 29) & 1) return;

    static char s_fakeGoal[1180 + 64] = {};
    for (int i = 0; i < 64; i++) s_fakeGoal[1180 + i] = 0;
    for (int i = 0; message[i] && i < 63; i++) s_fakeGoal[1180 + i] = message[i];

    SetJobFailed_t SetJobFailed = (SetJobFailed_t)&s_setJobFailedOpd;
    SetJobFailed((unsigned int*)episodeMgr, (int)s_fakeGoal, charIdx);

    // Null out the stored fake goal pointer before anything dereferences it
    ((unsigned int*)episodeMgr)[19] = 0;

    printf("[JobFail] done\n");
}

void ShowBinocucom(unsigned int leftEntityHash, unsigned int rightEntityHash) {
    // Read count as uint16 — it's a short at +2 of the struct at 0x106C228
    unsigned short instCount = *(unsigned short*)0x106C22A;
    printf("[Binocu] instCount=%u\n", (unsigned int)instCount);
    if (!instCount) return;

    // Array pointer at +8 of struct
    unsigned int instArrayPtr = *(unsigned int*)0x106C230;
    printf("[Binocu] instArrayPtr=0x%08X\n", instArrayPtr);
    if (!instArrayPtr) return;

    // First entry: each slot is 8 bytes {vtable*, inst*}, inst is at +4
    int* binocuInst = *(int**)(instArrayPtr + 4);
    printf("[Binocu] inst=0x%08X\n", (unsigned int)binocuInst);
    if (!binocuInst) return;

    // Slot pointers within the inst
    int** slot0 = (int**)((char*)binocuInst + 68);
    int** slot1 = (int**)((char*)binocuInst + 68 + 24);
    printf("[Binocu] slot0=0x%08X slot1=0x%08X\n",
        (unsigned int)slot0, (unsigned int)slot1);
    printf("[Binocu] *slot0=0x%08X *slot1=0x%08X\n",
        (unsigned int)*slot0, (unsigned int)*slot1);

    static int s_leftDef[8] = {};
    static int s_rightDef[8] = {};
    s_leftDef[1] = (int)leftEntityHash;
    s_rightDef[1] = (int)rightEntityHash;

    printf("[Binocu] leftDef=0x%08X rightDef=0x%08X\n",
        (unsigned int)s_leftDef, (unsigned int)s_rightDef);

    printf("[Binocu] slot0[0]=0x%08X slot0[1]=0x%08X slot0[4]=0x%08X\n",
        *(unsigned int*)slot0,
        *(unsigned int*)((char*)slot0 + 4),
        *(unsigned int*)((char*)slot0 + 16));

    SetCharSlot_t SetCharSlot = (SetCharSlot_t)&s_setCharSlotOpd;
    printf("[Binocu] calling SetCharSlot slot0...\n");
    SetCharSlot(slot0, s_leftDef, 0);
    printf("[Binocu] slot0 done, calling slot1...\n");
    SetCharSlot(slot1, s_rightDef, 0);
    printf("[Binocu] slot1 done\n");

    SetBinocucomVis_t SetVis = (SetBinocucomVis_t)&s_setBinocucomVisOpd;
    SetVis(1);
    printf("[Binocu] done\n");
}



bool Action_StartJobByIndex(int jobIdx, int startGoalIdx, bool useHub, bool useHideout)
{
    unsigned int episodeMgr = *(unsigned int*)0xF64C20;
    unsigned int jobArrayBase = *(unsigned int*)(episodeMgr + 28);
    unsigned int jobPtr = jobArrayBase + jobIdx * 536;

    bool sameJobAlreadyActive = (g_ActiveJobGlobalIndex == jobIdx && !useHub && !useHideout);
    bool differentJobActive = (g_ActiveJobGlobalIndex != -1 && g_ActiveJobGlobalIndex != jobIdx && !useHub && !useHideout);

    if (sameJobAlreadyActive) {
        printf("[Debug] Job %d already active, just reopening goals\n", jobIdx);
        return true; // pretend success, no actual call needed
    }

    if (differentJobActive) {
        StartJobByInst_t StartJobByInst = (StartJobByInst_t)&s_startJobByInstOpd;
        StartJobByInst((int)episodeMgr, (int)jobPtr, 0, 0, 1, 0, 0, 0, 1);

        g_ActiveJobGlobalIndex = -1;
        printf("[Debug] Different job active in same episode -> sent to hideout, click again once there\n");
        return false;
    }

    StartJobByInst_t StartJobByInst = (StartJobByInst_t)&s_startJobByInstOpd;
    int result = StartJobByInst((int)episodeMgr, (int)jobPtr, startGoalIdx,
        useHub ? 1 : 0, useHideout ? 1 : 0,
        0, 0, 0, 1);

    if (!useHub && !useHideout && result) {
        g_ActiveJobGlobalIndex = jobIdx;
    }

    printf("[Debug] StartJobByInst result=%d jobPtr=0x%08X\n", result, jobPtr);
    return (!useHub && !useHideout && result != 0);
}

void Action_TeleportToGoalIndex(int goalIdx)
{
    if (g_GoalTeleportCooldown) {
        printf("[Debug] Still settling from last teleport, ignoring\n");
        return;
    }

    unsigned int episodeMgr = *(unsigned int*)0xF64C20;
    GetGoalFromIndex_t GetGoalFromIndex = (GetGoalFromIndex_t)&s_getGoalFromIndexOpd;
    unsigned int goalPtr = GetGoalFromIndex((int)episodeMgr, (unsigned int)goalIdx);

    if (goalPtr == 0) {
        printf("[Debug] goalPtr is NULL\n");
        return;
    }

    g_GoalStableState = *(unsigned char*)(episodeMgr + 48); // capture before

    TeleportToGoal_t TeleportToGoal = (TeleportToGoal_t)&s_teleportToGoalOpd;
    int result = TeleportToGoal((int)episodeMgr, goalPtr, 1, 0);

    g_GoalTeleportCooldown = true; // block further clicks until settled
    printf("[Debug] TeleportToGoal(%d) result=%d, cooling down\n", goalIdx, result);
}

void UpdateGoalTeleportCooldown()
{
    if (!g_GoalTeleportCooldown) return;

    unsigned int episodeMgr = *(unsigned int*)0xF64C20;
    unsigned char currentState = *(unsigned char*)(episodeMgr + 48);

    if (currentState == g_GoalStableState) {
        g_GoalTeleportCooldown = false;
        printf("[Debug] Goal teleport settled, clicks allowed again\n");
    }
}

void GetGoalColorIndices(int goalCount, int* outCurrentIdx)
{
    GetGoalFromIndex_t GetGoalFromIndex = (GetGoalFromIndex_t)&s_getGoalFromIndexOpd;
    unsigned int episodeMgr = *(unsigned int*)0xF64C20;

    *outCurrentIdx = -1; // -1 = none found yet (shouldn't normally happen)

    for (int i = 0; i < goalCount; i++) {
        unsigned int goalPtr = GetGoalFromIndex((int)episodeMgr, (unsigned int)i);
        if (goalPtr == 0) continue;

        unsigned char state = *(unsigned char*)(goalPtr + 18);
        if (state != 5) {
            *outCurrentIdx = i; // first non-complete goal = the current one
            break;
        }
    }
}

int CalcRenderMatricesHook(int renderTarget, char a2) {
    int ret = g_CalcRenderMatricesHook->invoke<int>(renderTarget, a2);

    // Capture VP after CalcRenderMatrices runs — now includes our FreeCam view
    if (g_ActiveCameraRenderTarget && renderTarget == g_ActiveCameraRenderTarget) {
        float* vp = (float*)(renderTarget + 880);
        for (int i = 0; i < 16; i++) {
            g_ViewProjMatrix[i] = vp[i];
            g_FreeCamVP[i] = vp[i];
        }
        g_ViewProjValid = true;
        g_FreeCamVPValid = true;
    }

    return ret;
}

bool WorldToScreen(float wx, float wy, float wz, float* sx, float* sy) {
    if (!g_ViewProjValid) return false;

    float* m = g_ViewProjMatrix;

    float cx = m[0] * wx + m[4] * wy + m[8] * wz + m[12];
    float cy = m[1] * wx + m[5] * wy + m[9] * wz + m[13];
    float cw = m[3] * wx + m[7] * wy + m[11] * wz + m[15];

    if (cw < 0.001f) return false;  // behind camera only

    *sx = (cx / cw + 1.0f) * 0.5f;
    *sy = (1.0f - cy / cw) * 0.5f;

    return true;  // don't clip to screen bounds
}


void Action_TestWorldToScreenOnPlayer() {
    unsigned int playerPosRef = GetPlayerPositionRef();
    if (playerPosRef == 0) return;

    float px = *(float*)(playerPosRef + 144);
    float py = *(float*)(playerPosRef + 148);
    float pz = *(float*)(playerPosRef + 152);

    printf("[Debug] player world pos bits: x=0x%08X y=0x%08X z=0x%08X\n",
        *(unsigned int*)&px, *(unsigned int*)&py, *(unsigned int*)&pz);

    printf("[Debug] ViewProjValid=%d\n", g_ViewProjValid ? 1 : 0);
    printf("[Debug] ViewMatrixValid=%d ProjMatrixValid=%d\n",    // <- add here
        g_ViewMatrixValid ? 1 : 0, g_ProjMatrixValid ? 1 : 0);

    printf("[Debug] VP matrix row0: 0x%08X 0x%08X 0x%08X 0x%08X\n",
        *(unsigned int*)&g_ViewProjMatrix[0], *(unsigned int*)&g_ViewProjMatrix[1],
        *(unsigned int*)&g_ViewProjMatrix[2], *(unsigned int*)&g_ViewProjMatrix[3]);
    printf("[Debug] VP matrix row1: 0x%08X 0x%08X 0x%08X 0x%08X\n",
        *(unsigned int*)&g_ViewProjMatrix[4], *(unsigned int*)&g_ViewProjMatrix[5],
        *(unsigned int*)&g_ViewProjMatrix[6], *(unsigned int*)&g_ViewProjMatrix[7]);
    printf("[Debug] VP matrix row2: 0x%08X 0x%08X 0x%08X 0x%08X\n",
        *(unsigned int*)&g_ViewProjMatrix[8], *(unsigned int*)&g_ViewProjMatrix[9],
        *(unsigned int*)&g_ViewProjMatrix[10], *(unsigned int*)&g_ViewProjMatrix[11]);
    printf("[Debug] VP matrix row3: 0x%08X 0x%08X 0x%08X 0x%08X\n",
        *(unsigned int*)&g_ViewProjMatrix[12], *(unsigned int*)&g_ViewProjMatrix[13],
        *(unsigned int*)&g_ViewProjMatrix[14], *(unsigned int*)&g_ViewProjMatrix[15]);

    float sx = 0, sy = 0;
    bool ok = WorldToScreen(px, py, pz, &sx, &sy);

    printf("[Debug] ok=%d screen bits: sx=0x%08X sy=0x%08X\n",
        ok, *(unsigned int*)&sx, *(unsigned int*)&sy);
}

void Action_DumpEntityScreenPositions()
{
    unsigned int instListPtr = *(unsigned int*)0x1013B34; // dereference to get 0x409EFE00
    unsigned short entityCount = *(unsigned short*)0x1013B3C;

    printf("[Debug] instListPtr=0x%08X entityCount=%d\n", instListPtr, entityCount);

    for (int i = 0; i < 10; i++) {
        unsigned int entityPtr = *(unsigned int*)(instListPtr + i * 4);
        printf("[Debug] entity[%d] ptr=0x%08X\n", i, entityPtr);
    }
}

/*void UpdateWorldToScreenTest()
{
    
    g_DebugFrameCounter++;

    unsigned int base = GetWritablePlayerPosBase();
    if (base == 0) return;

    float px = *(float*)(base + 0x1E0);
    float py = *(float*)(base + 0x1E4);
    float pz = *(float*)(base + 0x1E8);

    unsigned int gameMgr = *(unsigned int*)0xF65CA0;
    unsigned int renderTarget = *(unsigned int*)(gameMgr + 0x128168);
    float* mtx = (float*)(renderTarget + 1328);

    float clipW = px * mtx[12] + py * mtx[13] + pz * mtx[14] + 1.0f * mtx[15];

    float sx, sy;
    bool ok = WorldToScreenSly4(px, py, pz, &sx, &sy);

    if (g_DebugFrameCounter % 15 == 0) {
        printf("[Debug] clipW=%d.%03d ok=%d sx=%d.%03d sy=%d.%03d\n",
            (int)clipW, (int)((clipW - (int)clipW) * 1000),
            ok,
            (int)sx, (int)((sx - (int)sx) * 1000),
            (int)sy, (int)((sy - (int)sy) * 1000));
    }

    if (!ok) return;

    float screenSize[2];
    GetScreenSize_t GetScreenSize = (GetScreenSize_t)&s_getScreenSizeOpd;
    GetScreenSize(screenSize);

    float pos[3] = { sx * screenSize[0], sy * screenSize[1], 0.0f };
    unsigned char color[4] = { 255, 0, 255, 255 };
    QueueText(pos, (int)"PLAYER", color, 1.0);
}

inline void writeColor(GridVertex* v, uint32_t packedColor) {
    *(uint32_t*)v->color = packedColor; // direct copy, no byte-swap needed per Hackpack's own unpack order
}*/

/*int DrawGrid(void* cmdBuf, const float* localToWorldMtx, int lineCountX, int lineCountZ, float spacing, uint32_t color) {
    int gridMaterial = MaterialLookup(0);   // confirmed pattern, slot 0 = flat/default
    Sly4_AddMaterial(cmdBuf, gridMaterial);
    Sly4_AddLtoWMatrix(cmdBuf, localToWorldMtx);

    void* vertHandle;
    unsigned int totalVerts = 2 * ((lineCountX + 1) + (lineCountZ + 1));
    GridVertex* v = (GridVertex*)Sly4_BeginVertices(&vertHandle, cmdBuf, 1, totalVerts);

    for (int z = 0; z <= lineCountZ; z++) {
        v->x = 0.0f;                 v->y = 0.0f; v->z = z * spacing; writeColor(v, color); v++;
        v->x = lineCountX * spacing; v->y = 0.0f; v->z = z * spacing; writeColor(v, color); v++;
    }
    for (int x = 0; x <= lineCountX; x++) {
        v->x = x * spacing; v->y = 0.0f; v->z = 0.0f;                 writeColor(v, color); v++;
        v->x = x * spacing; v->y = 0.0f; v->z = lineCountZ * spacing; writeColor(v, color); v++;
    }
    return 0;
}*/


void Action_ToggleFreeCam()
{
    g_FreeCamEnabled = !g_FreeCamEnabled;
    g_FreeCamInitialized = false;
    //g_PlayerPosFrozenCaptured = false;

    unsigned int gameMgr = *(unsigned int*)0xF65CA0;
    unsigned int renderTarget = *(unsigned int*)(gameMgr + 0x128168);
    if (renderTarget != 0) {
        SetFarZClip_t SetFarZClip = (SetFarZClip_t)&s_setFarZClipOpd;
        if (g_FreeCamEnabled) {
            SetFarZClip((int)renderTarget, 5000.0); // max draw distance
        }
        else {
            SetFarZClip((int)renderTarget, kOriginalFarZClip); // restore normal
        }
    }

    printf("[Debug] FreeCam = %d\n", g_FreeCamEnabled);
}

void Action_SetSloMo()
{
    *(int*)0x10B7A5C = 386;
    *(int*)0x10B7A10 = 291;
}

void Action_SetNormalSpeed()
{
    *(int*)0x10B7A5C = 60;
    *(int*)0x10B7A10 = 1;
}

void Action_TestSplitScreen2Player()
{
    unsigned int gameMgr = *(unsigned int*)0xF65CA0;
    SetSplitScreenMode_t SetSplitScreenMode = (SetSplitScreenMode_t)&s_setSplitScreenModeOpd;
    SetSplitScreenMode((void*)gameMgr, 2); // 2-player split
}


void DumpAllEntityDefs() {
    uint32_t arrayBase = *g_arrayPtr;
    uint16_t count = *g_count;

    printf("EntityDef registry dump - %u entries\n", count);
    printf("%-12s %-12s %s\n", "Hash", "DefPtr", "Name");
    printf("------------------------------------------------------\n");

    RegistryEntry* entries = (RegistryEntry*)arrayBase;

    for (uint16_t i = 0; i < count; i++) {
        uint32_t hash = entries[i].hash;
        uint32_t defPtr = entries[i].defPtr;
        if (!defPtr) continue;

        char nameBuf[ENTITYDEF_NAME_MAXLEN + 1] = { 0 };
        CopyNameSafe(nameBuf, (const char*)(defPtr + ENTITYDEF_NAME_OFFSET), ENTITYDEF_NAME_MAXLEN);
        nameBuf[ENTITYDEF_NAME_MAXLEN] = 0;

        uint32_t storedHash = *(uint32_t*)(defPtr + ENTITYDEF_HASH_OFFSET);

        printf("0x%08X   0x%08X   %s%s\n",
            hash, defPtr, nameBuf,
            (hash != storedHash) ? "  [hash mismatch!]" : "");
    }
}

static int BuildAndSpawn(unsigned int entityHash, float px, float py, float pz) {
    printf("[Spawn] A hash=0x%08X\n", entityHash);
    float spawnY = py;
    printf("[Spawn] B spawnY set\n");

    if (g_SnapToGround && !g_IsLoadingScene) {
        printf("[Spawn] C snap\n");
        FindDef_t FindDef = (FindDef_t)&s_findDefOpd;
        int entityDef = FindDef(entityHash);
        if (entityDef) {
            float groundY;
            if (GetGroundY(px, py, pz, groundY)) {
                float aabb[8] = {};
                CalcBBox_t CalcBBox = (CalcBBox_t)&s_calcBBoxOpd;
                CalcBBox(aabb, entityDef, 0);
                float minY = aabb[1];
                float maxY = aabb[5];
                float height = maxY - minY;
                spawnY = (height > 0.0f && height < 100.0f) ?
                    groundY + (height * 0.5f) : groundY + 0.5f;
            }
        }
        printf("[Spawn] C snap done spawnY=%d\n", (int)spawnY);
    }

    printf("[Spawn] D rotation\n");
    float rx = g_SpawnRotation[0] * 3.14159265f / 180.0f;
    float ry = g_SpawnRotation[1] * 3.14159265f / 180.0f;
    float rz = g_SpawnRotation[2] * 3.14159265f / 180.0f;
    float cx = MyCosf(rx), sx = MySinf(rx);
    float cy = MyCosf(ry), sy = MySinf(ry);
    float cz = MyCosf(rz), sz = MySinf(rz);

    printf("[Spawn] E params\n");
    EntityDefSpawnParams params = {};
    params.vtable = (void*)0xB12260;
    params.m00 = cy * cz + sy * sx * sz;  params.m01 = -cy * sz + sy * sx * cz;  params.m02 = sy * cx;
    params.m10 = cx * sz;              params.m11 = cx * cz;             params.m12 = -sx;
    params.m20 = -sy * cz + cy * sx * sz;  params.m21 = sy * sz + cy * sx * cz;  params.m22 = cy * cx;
    params.posX = px; params.posY = spawnY; params.posZ = pz;
    params.scaleX = params.scaleY = params.scaleZ = 1.0f;
    params.negOne_a = -1.0f; params.negOne_b = -1.0f;
    params.packedFlags = 0x9FE00000;

    printf("[Spawn] F SpawnByHash\n");
    SpawnByHash_t SpawnByHash = (SpawnByHash_t)&s_spawnByHashOpd;
    int entity = SpawnByHash(entityHash, &params);
    printf("[Spawn] G entity=0x%08X\n", entity);

    if (entity && !g_IsLoadingScene && g_SavedEntityCount < MAX_SAVED_ENTITIES) {
        printf("[Spawn] H tracking\n");
        SavedEntity& e = g_SavedEntities[g_SavedEntityCount++];
        e.hash = entityHash;
        e.posX = px; e.posY = spawnY; e.posZ = pz;
        e.rotX = g_SpawnRotation[0];
        e.rotY = g_SpawnRotation[1];
        e.rotZ = g_SpawnRotation[2];
        e.scale = 1.0f;
    }

    printf("[Spawn] I done\n");
    return entity;
}

void SpawnPropAtPlayer(unsigned int entityHash) {
    unsigned int base = GetWritablePlayerPosBase();
    if (!base) { printf("[Spawn] ABORT: base null\n"); return; }

    float px = *(float*)(base + 0x1E0);
    float py = *(float*)(base + 0x1E4);
    float pz = *(float*)(base + 0x1E8);

    BuildAndSpawn(entityHash, px, py, pz);
}

void UpdateFreeCamInput()
{
    if (!g_FreeCamEnabled) return;
    if (g_MenuVisible) return;

    int lx = (int)g_RealPadData.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X] - 128;
    int ly = (int)g_RealPadData.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y] - 128;
    int rx = (int)g_RealPadData.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X] - 128;
    int ry = (int)g_RealPadData.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y] - 128;

    uint16_t digital2 = g_RealPadData.button[CELL_PAD_BTN_OFFSET_DIGITAL2];
    bool curL1 = (digital2 & CELL_PAD_CTRL_L1) != 0;
    bool curR1 = (digital2 & CELL_PAD_CTRL_R1) != 0;
    bool curL2 = (digital2 & CELL_PAD_CTRL_L2) != 0;
    bool curR2 = (digital2 & CELL_PAD_CTRL_R2) != 0;

    const float kLookSpeed = 0.03f;
    const float kBaseMoveSpeed = 0.15f;   // slower default (was 0.3f)
    const float kFastMoveSpeed = 0.3f;    // R2: old default speed becomes "fast"
    const float kSlowMoveSpeed = 0.05f;   // L2: slow precision movement
    const float kHeightSpeed = 0.2f;
    const float kPitchLimitUp = 1.4f; // was 0.3
    const float kPitchLimitDown = 1.4f;

    float moveSpeed = g_FreeCamMoveSpeed;
    if (curL2) moveSpeed = g_FreeCamMoveSpeed * 0.33f;      // slow modifier, relative now
    else if (curR2) moveSpeed = g_FreeCamMoveSpeed * 2.0f;  // fast modifier, relative now

    if (AbsInt(rx) > kStickDeadzone) {
        g_FreeCamYaw -= (rx / 128.0f) * kLookSpeed;
        if (g_FreeCamYaw > 3.14159265f) g_FreeCamYaw -= 6.28318530f;
        if (g_FreeCamYaw < -3.14159265f) g_FreeCamYaw += 6.28318530f;
    }
    if (AbsInt(ry) > kStickDeadzone) {
        g_FreeCamPitch -= (ry / 128.0f) * kLookSpeed;
        if (g_FreeCamPitch > kPitchLimitUp) g_FreeCamPitch = kPitchLimitUp;
        if (g_FreeCamPitch < -kPitchLimitDown) g_FreeCamPitch = -kPitchLimitDown;
    }

    float forwardX = MySinf(g_FreeCamYaw) * MyCosf(g_FreeCamPitch);
    float forwardY = -MySinf(g_FreeCamPitch);
    float forwardZ = MyCosf(g_FreeCamYaw) * MyCosf(g_FreeCamPitch);

    // --- diagnostic, right after forwardY is computed ---
    /*if (MyFabsf(forwardY - g_PrevPitchForwardY) > 0.3f) {
        printf("[Debug] PITCH JUMP pitch=%d.%03d prevY=%d cury=%d\n",
            (int)g_FreeCamPitch, (int)((g_FreeCamPitch - (int)g_FreeCamPitch) * 1000),
            (int)(g_PrevPitchForwardY * 1000), (int)(forwardY * 1000));
    }*/

    float rightX = MyCosf(g_FreeCamYaw);
    float rightZ = -MySinf(g_FreeCamYaw);

    if (AbsInt(ly) > kStickDeadzone) {
        float move = (ly / 128.0f) * moveSpeed;
        g_FreeCamPos[0] += forwardX * move;
        g_FreeCamPos[1] += forwardY * move;
        g_FreeCamPos[2] += forwardZ * move;
    }
    if (AbsInt(lx) > kStickDeadzone) {
        float move = (lx / 128.0f) * moveSpeed;
        g_FreeCamPos[0] += rightX * move;
        g_FreeCamPos[2] += rightZ * move;
    }

    // R1/L1: height up/down, also respects the L2/R2 speed modifier
    if (curR1) g_FreeCamPos[1] += kHeightSpeed * (moveSpeed / kBaseMoveSpeed);
    if (curL1) g_FreeCamPos[1] -= kHeightSpeed * (moveSpeed / kBaseMoveSpeed);
}

void UpdateLevelEditorInput() {
    if (!g_LevelEditorEnabled) return;
    if (g_MenuVisible) return;

    int lx = (int)g_RealPadData.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X] - 128;
    int ly = (int)g_RealPadData.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y] - 128;
    int rx = (int)g_RealPadData.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X] - 128;
    int ry = (int)g_RealPadData.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y] - 128;

    uint16_t digital2 = g_RealPadData.button[CELL_PAD_BTN_OFFSET_DIGITAL2];
    bool curL1 = (digital2 & CELL_PAD_CTRL_L1) != 0;
    bool curL2 = (digital2 & CELL_PAD_CTRL_L2) != 0;
    bool curR1 = (digital2 & CELL_PAD_CTRL_R1) != 0;
    bool curR2 = (digital2 & CELL_PAD_CTRL_R2) != 0;

    const float kOrbitSpeed = 0.008f;  // was 0.03f
    const float kZoomSpeed = 0.1f;    // was 0.3f
    const float kMoveSpeed = 0.05f;   // was 0.15f

    
    // Right stick — orbit yaw/pitch, fixed speed
    
    if (AbsInt(rx) > kStickDeadzone)
        g_OrbitYaw -= (rx / 128.0f) * kOrbitSpeed;
    if (AbsInt(ry) > kStickDeadzone) {
        g_OrbitPitch -= (ry / 128.0f) * kOrbitSpeed;  // negated — stick down = camera down
        if (g_OrbitPitch > 1.5f) g_OrbitPitch = 1.5f;
        if (g_OrbitPitch < -1.5f) g_OrbitPitch = -1.5f;
    }

    // R1/R2 — zoom in/out (radius only, no pivot movement)
    
    if (curR1) { g_OrbitRadius -= kZoomSpeed; if (g_OrbitRadius < 1.0f)   g_OrbitRadius = 1.0f; }
    if (curR2) { g_OrbitRadius += kZoomSpeed; if (g_OrbitRadius > 500.0f) g_OrbitRadius = 500.0f; }

    // Left stick — move pivot on XZ plane relative to orbit yaw
    
    float fwdX = MySinf(g_OrbitYaw);
    float fwdZ = MyCosf(g_OrbitYaw);
    float rightX = MyCosf(g_OrbitYaw);
    float rightZ = -MySinf(g_OrbitYaw);

    if (AbsInt(ly) > kStickDeadzone) {
        float move = -(ly / 128.0f) * kMoveSpeed;
        g_OrbitPivot[0] += fwdX * move;
        g_OrbitPivot[2] += fwdZ * move;
    }
    if (AbsInt(lx) > kStickDeadzone) {
        float move = -(lx / 128.0f) * kMoveSpeed;
        g_OrbitPivot[0] += rightX * move;
        g_OrbitPivot[2] += rightZ * move;
    }

    // L1/L2 — move pivot up/down
    if (curL2) g_OrbitPivot[1] -= kMoveSpeed;
    if (curL1) g_OrbitPivot[1] += kMoveSpeed;
}


float* LookAtHook(float* result, float* eyePos, float* targetPos, float* upVec)
{
    if (g_FreeCamEnabled) {
        if (!g_FreeCamInitialized) {
            g_FreeCamPos[0] = eyePos[0];
            g_FreeCamPos[1] = eyePos[1];
            g_FreeCamPos[2] = eyePos[2];
            g_FreeCamInitialized = true;
        }

        // also shift the look-target by the same delta, so we keep looking the same relative direction
        float dx = g_FreeCamPos[0] - eyePos[0];
        float dy = g_FreeCamPos[1] - eyePos[1];
        float dz = g_FreeCamPos[2] - eyePos[2];

        float newEye[3] = { g_FreeCamPos[0], g_FreeCamPos[1], g_FreeCamPos[2] };
        float newTarget[3] = { targetPos[0] + dx, targetPos[1] + dy, targetPos[2] + dz };

        return g_LookAtHook->invoke<float*>(result, newEye, newTarget, upVec);
    }
    else {
        g_FreeCamInitialized = false; // re-sync next time it's enabled
        return g_LookAtHook->invoke<float*>(result, eyePos, targetPos, upVec);
    }
}

float* CalcMatrixHook(int a1, float* eyePos, float* targetPos, float* result) {
    float* ret;

    if (g_FreeCamEnabled) {
        if (!g_FreeCamInitialized) {
            g_FreeCamPos[0] = eyePos[0];
            g_FreeCamPos[1] = eyePos[1];
            g_FreeCamPos[2] = eyePos[2];
            g_FreeCamYaw = 0.0f;
            g_FreeCamPitch = 0.0f;
            g_FreeCamInitialized = true;
        }

        float forwardX = MySinf(g_FreeCamYaw) * MyCosf(g_FreeCamPitch);
        float forwardY = MySinf(g_FreeCamPitch);
        float forwardZ = MyCosf(g_FreeCamYaw) * MyCosf(g_FreeCamPitch);

        if (g_HasLastForward) {
            float dot = forwardX * g_LastForwardX + forwardY * g_LastForwardY + forwardZ * g_LastForwardZ;
            if (dot < 0.85f) {
                forwardX = g_LastForwardX;
                forwardY = g_LastForwardY;
                forwardZ = g_LastForwardZ;
            }
        }
        g_LastForwardX = forwardX;
        g_LastForwardY = forwardY;
        g_LastForwardZ = forwardZ;
        g_HasLastForward = true;

        float newEye[3] = { g_FreeCamPos[0], g_FreeCamPos[1], g_FreeCamPos[2] };
        float newTarget[3] = {
            g_FreeCamPos[0] + forwardX,
            g_FreeCamPos[1] + forwardY,
            g_FreeCamPos[2] + forwardZ
        };

        // Let the engine compute the correct matrix format
        ret = g_CalcMatrixHook->invoke<float*>(a1, newEye, newTarget, result);

        // Save the resulting matrix for CalcRenderMatricesHook to use
        if (result) {
            for (int i = 0; i < 16; i++)
                g_FreeCamViewMatrix[i] = result[i];
            g_FreeCamViewMatrixValid = true;
        }
    }
    else {
        g_FreeCamInitialized = false;
        g_HasLastForward = false;
        g_FreeCamViewMatrixValid = false;
        ret = g_CalcMatrixHook->invoke<float*>(a1, eyePos, targetPos, result);
    }

    return ret;
}

void Action_ToggleHUD()
{
    FindInstance_t FindInstance = (FindInstance_t)&s_findInstanceOpd;

    int common = FindInstance("Common", 0);
    if (common) {
        void (*toggleFn)(int) = (void(*)(int))(*(unsigned int*)common + 88);
        toggleFn(common);
    }

    int inGame = FindInstance("InGame", 0);
    if (inGame) {
        void (*toggleFn)(int) = (void(*)(int))(*(unsigned int*)inGame + 88);
        toggleFn(inGame);
    }
}


void UpdateFlyMode()
{
    if (!g_FlyModeEnabled) return;
    unsigned int base = GetWritablePlayerPosBase();
    if (base == 0) return;

    SetState_t SetState = (SetState_t)&s_setStateOpd;
    SetState((int)base, 2); // force/maintain airborne state every frame

    //*(float*)(base + 404) = 0.0f;        // suspected air-time accumulator reset
   // *(unsigned char*)(base + 0x412) = 0; // suppress flailing-animation trigger byte

    int lx = (int)g_RealPadData.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X] - 128;
    int ly = (int)g_RealPadData.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y] - 128;

    uint16_t digital2 = g_RealPadData.button[CELL_PAD_BTN_OFFSET_DIGITAL2];
    bool curL2 = (digital2 & CELL_PAD_CTRL_L2) != 0;
    bool curR2 = (digital2 & CELL_PAD_CTRL_R2) != 0;

    float vx = 0.0f, vy = 0.0f, vz = 0.0f;

    if (AbsInt(lx) > kStickDeadzone) vx = (lx / 128.0f) * kFlySpeed * 30.0f;
    if (AbsInt(ly) > kStickDeadzone) vz = (ly / 128.0f) * kFlySpeed * 30.0f;
    if (curR2) vy = kFlySpeed * 30.0f;
    else if (curL2) vy = -kFlySpeed * 30.0f;

    *(float*)(base + 592) = vx;
    *(float*)(base + 596) = vy;
    *(float*)(base + 600) = vz;
}


void Action_ToggleFlyMode()
{
    g_FlyModeEnabled = !g_FlyModeEnabled;

    printf("[Debug] Fly mode = %d\n", g_FlyModeEnabled);
}

void Action_TryTeleport(float newX, float newY, float newZ)
{
    unsigned int playerBase = *(unsigned int*)0xF65CA0;
    unsigned int playerPtr = playerBase + 4208;
    unsigned int matrixAddr = playerPtr + 16;

    // Leave rows 0-2 (rotation) completely untouched — only overwrite row 3 (translation)
    *(float*)(matrixAddr + 48) = newX;
    *(float*)(matrixAddr + 52) = newY;
    *(float*)(matrixAddr + 56) = newZ;
    *(float*)(matrixAddr + 60) = 0.0f; // matches what's actually there, not assumed

    typedef unsigned int (*TeleportPlayer_t)(unsigned int, unsigned int, unsigned int);
    TeleportPlayer_t TeleportPlayer = (TeleportPlayer_t)&s_teleportPlayerOpd; // needs OPD struct, TOC TBD

    TeleportPlayer(playerBase, playerPtr, matrixAddr);
}

static void SanitizeForFont(char* str)
{
    int read = 0, write = 0;
    while (str[read] != '\0') {
        unsigned char c0 = (unsigned char)str[read];
        unsigned char c1 = (unsigned char)str[read + 1];

        if (c0 == 0xC3) { // UTF-8 lead byte for Latin-1 Supplement block (À-ÿ)
            char replacement = 0;
            switch (c1) {
            case 0x84: replacement = 'A'; break; // Ä
            case 0x96: replacement = 'O'; break; // Ö
            case 0x85: replacement = 'A'; break; // Å
            case 0xA4: replacement = 'a'; break; // ä
            case 0xB6: replacement = 'o'; break; // ö
            case 0xA5: replacement = 'a'; break; // å
            }
            if (replacement != 0) {
                str[write++] = replacement;
                read += 2; // skip both bytes of the UTF-8 sequence
                continue;
            }
        }

        str[write++] = str[read++];
    }
    str[write] = '\0';
}

const JobGoalList* FindJobGoals(const char* jobKey)
{
    for (int i = 0; i < g_AllJobGoalListsCount; i++) {
        if (StrEqual(g_AllJobGoalLists[i].jobKey, jobKey)) return &g_AllJobGoalLists[i];
    }
    return nullptr;
}


void RebuildDisplayItems()
{
    g_DisplayItemCount = 0;
    int pathLen = 0;
    while (g_CurrentPath[pathLen] != '\0') pathLen++;

    bool inMenuCategory = StrEqual(g_CurrentPath, "Menu");
    bool inPlayerCategory = StrEqual(g_CurrentPath, "Player");
    bool inEpisodesCategory = StrEqual(g_CurrentPath, "Episodes");
    bool inEpisodeJobsCategory = StrEqual(g_CurrentPath, "EpisodeJobs");
    bool inJobGoalsCategory = StrEqual(g_CurrentPath, "JobGoals");
    bool inFreeCamCategory = StrEqual(g_CurrentPath, "FreeCam");
    bool inEntitiesCategory = StrEqual(g_CurrentPath, "Entities");
    bool inEntityWorldCategory = StrEqual(g_CurrentPath, "EntityWorld");
    bool inEntityListCategory = StrEqual(g_CurrentPath, "EntityList");
    bool inEntitySpawnCategory = StrEqual(g_CurrentPath, "EntitySpawn");
    bool inMemesCategory = StrEqual(g_CurrentPath, "Memes");  // NEW
    unsigned int playerBase = *(unsigned int*)0xF65CA0;
    unsigned int playerPosRef = GetPlayerPositionRef();

    if (!inMenuCategory && !inPlayerCategory && !inEpisodesCategory && !inEpisodeJobsCategory && !inJobGoalsCategory && !inFreeCamCategory && !inEntitiesCategory && !inEntityWorldCategory && !inEntityListCategory && !inEntitySpawnCategory && !inMemesCategory) {
        for (int i = 0; i < g_TunableCount; i++) {
            const char* name = g_TunableTable[i].name;
            bool matches;
            int segStart;

            if (pathLen == 0) {
                matches = true;
                segStart = 0;
            }
            else {
                matches = StringStartsWith(name, g_CurrentPath) && name[pathLen] == '.';
                segStart = pathLen + 1;
            }
            if (!matches) continue;

            char segment[64];
            int segLen = GetNextSegment(name, segStart, segment, sizeof(segment));
            bool hasMore = (name[segStart + segLen] == '.');

            if (hasMore) {
                bool alreadyAdded = false;
                for (int j = 0; j < g_DisplayItemCount; j++) {
                    if (g_DisplayItems[j].isCategory) {
                        bool same = true;
                        for (int k = 0; k <= segLen; k++) {
                            if (g_DisplayItems[j].label[k] != segment[k]) { same = false; break; }
                        }
                        if (same) { alreadyAdded = true; break; }
                    }
                }

                bool passesRootFilter = (pathLen != 0) || IsCanonicalRootCategory(segment);

                if (!alreadyAdded && passesRootFilter && g_DisplayItemCount < 256) {
                    DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
                    for (int k = 0; k <= segLen; k++) item.label[k] = segment[k];
                    item.isCategory = true;
                    item.tunableIndex = -1;
                }
            }
            else {
                if (g_DisplayItemCount < 256) {
                    DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
                    for (int k = 0; k <= segLen; k++) item.label[k] = segment[k];
                    item.isCategory = false;
                    item.tunableIndex = i;
                    item.liveAddr = FindLiveTunable(g_TunableTable[i].name);
                    item.type = item.liveAddr ? GetTunableType(*(unsigned int*)item.liveAddr) : TT_UNKNOWN;
                }
            }
        }
    }

    // --- Synthetic "Menu" category at root ---
    if (pathLen == 0 && g_DisplayItemCount < 256) {
        DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
        const char* label = "Menu";
        int k = 0;
        while (label[k] != '\0') { item.label[k] = label[k]; k++; }
        item.label[k] = '\0';
        item.isCategory = true;
        item.tunableIndex = -1;
    }

    if (inMenuCategory && g_DisplayItemCount < 256) {
        DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
        const char* label = "Background Color";
        int k = 0; while (label[k] != '\0') { item.label[k] = label[k]; k++; } item.label[k] = '\0';
        item.isCategory = false; item.tunableIndex = -1;
        item.liveAddr = (unsigned int)&g_MenuBgColorTunable;
        item.type = TT_COLOR;
    }
    if (inMenuCategory && g_DisplayItemCount < 256) {
        DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
        const char* label = "Delete Saved Settings";
        int k = 0; while (label[k] != '\0') { item.label[k] = label[k]; k++; } item.label[k] = '\0';
        item.isCategory = false; item.tunableIndex = -1;
        item.liveAddr = 0;
        item.type = TT_ACTION;
    }
    if (inMenuCategory && g_DisplayItemCount < 256) {
        DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
        const char* label = "Reload Saved Settings";
        int k = 0; while (label[k] != '\0') { item.label[k] = label[k]; k++; } item.label[k] = '\0';
        item.isCategory = false; item.tunableIndex = -1;
        item.liveAddr = 0;
        item.type = TT_ACTION;
    }
    if (inMenuCategory && g_DisplayItemCount < 256) {
        DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
        const char* label = "Toggle Fly Mode";
        int k = 0; while (label[k] != '\0') { item.label[k] = label[k]; k++; } item.label[k] = '\0';
        item.isCategory = false; item.tunableIndex = -1;
        item.liveAddr = 0;
        item.type = TT_ACTION;
    }

    // --- Synthetic "Player" category at root ---
    if (pathLen == 0 && g_DisplayItemCount < 256) {
        DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
        const char* label = "Player";
        int k = 0; while (label[k] != '\0') { item.label[k] = label[k]; k++; } item.label[k] = '\0';
        item.isCategory = true;
        item.tunableIndex = -1;
    }

    if (inPlayerCategory && g_DisplayItemCount < 256) {
        DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
        const char* label = "Coins";
        int k = 0; while (label[k] != '\0') { item.label[k] = label[k]; k++; } item.label[k] = '\0';
        item.isCategory = false; item.tunableIndex = -1;
        item.liveAddr = playerBase + 112 - 72;
        item.type = TT_INT;
    }

    if (inPlayerCategory && playerPosRef != 0 && g_DisplayItemCount < 256) {
        DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
        const char* label = "Position";
        int k = 0; while (label[k] != '\0') { item.label[k] = label[k]; k++; } item.label[k] = '\0';
        item.isCategory = false; item.tunableIndex = -1;
        item.liveAddr = playerPosRef + 144 - 72;
        item.type = TT_VEC3;
    }

    // --- Synthetic "FreeCam" category at root ---
    if (pathLen == 0 && g_DisplayItemCount < 256) {
        DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
        const char* label = "FreeCam";
        int k = 0; while (label[k] != '\0') { item.label[k] = label[k]; k++; } item.label[k] = '\0';
        item.isCategory = true;
        item.tunableIndex = -1;
    }

    // --- Synthetic "Memes" category at root ---  // NEW
    if (pathLen == 0 && g_DisplayItemCount < 256) {
        DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
        const char* label = "Memes";
        int k = 0; while (label[k] != '\0') { item.label[k] = label[k]; k++; } item.label[k] = '\0';
        item.isCategory = true;
        item.tunableIndex = -1;
    }

    // --- "Memes" contents ---  // NEW
    if (inMemesCategory && g_DisplayItemCount < 256) {
        DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
        const char* label = "Fake Job Fail";
        int k = 0; while (label[k] != '\0') { item.label[k] = label[k]; k++; } item.label[k] = '\0';
        item.isCategory = false;
        item.tunableIndex = -1;
        item.liveAddr = 0;
        item.type = TT_ACTION;
    }

   /* if (inMemesCategory && g_DisplayItemCount < 256) {
        DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
        const char* label = "Fake Binocucom Conversation";
        int k = 0; while (label[k]) { item.label[k] = label[k]; k++; } item.label[k] = '\0';
        item.isCategory = false; item.tunableIndex = -1;
        item.liveAddr = 0;
        item.type = TT_ACTION;
    }*/

    // --- "Entities" -> one sub-category per hub world ---
    if (inEntitiesCategory) {
        unsigned int stringTable = *(unsigned int*)0xF6687C;
        GetStr_t GetStr = (GetStr_t)&s_getStrOpd;
        for (int i = 0; i < g_EntityWorldCount && g_DisplayItemCount < 256; i++) {
            const char* resolved = GetStr(stringTable, g_EntityWorlds[i].locKey, g_EntityWorlds[i].fallback, 0);
            DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
            int k = 0; while (resolved[k] != '\0' && k < 60) { item.label[k] = resolved[k]; k++; } item.label[k] = '\0';
            SanitizeForFont(item.label);
            item.isCategory = true;
            item.tunableIndex = 4000 + i;
        }
    }

    // --- "EntityWorld" -> type-groups inside the selected world ---
    if (inEntityWorldCategory && g_SelectedWorldIndex >= 0 && g_SelectedWorldIndex < g_EntityWorldCount) {
        const EntityWorld& w = g_EntityWorlds[g_SelectedWorldIndex];
        for (int i = 0; i < w.groupCount && g_DisplayItemCount < 256; i++) {
            DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
            const char* label = w.groups[i].name;
            int k = 0; while (label[k] != '\0' && k < 60) { item.label[k] = label[k]; k++; } item.label[k] = '\0';
            item.isCategory = true;
            item.tunableIndex = 4100 + i;
        }
    }

    // --- "EntityList" -> entity leaves, clicking navigates to EntitySpawn ---
    if (inEntityListCategory && g_SelectedWorldIndex >= 0 && g_SelectedWorldIndex < g_EntityWorldCount) {
        const EntityWorld& w = g_EntityWorlds[g_SelectedWorldIndex];
        if (g_SelectedTypeIndex >= 0 && g_SelectedTypeIndex < w.groupCount) {
            const EntityTypeGroup& g = w.groups[g_SelectedTypeIndex];
            for (int i = 0; i < g.count && g_DisplayItemCount < 256; i++) {
                DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
                const char* label = g.items[i].name;
                int k = 0; while (label[k] != '\0' && k < 60) { item.label[k] = label[k]; k++; } item.label[k] = '\0';
                item.isCategory = false;
                item.tunableIndex = 5000;
                item.liveAddr = g.items[i].hash;
                item.type = TT_ACTION;
            }
        }
    }

    // --- "EntitySpawn" -> rotation, scale, spawn ---
    if (inEntitySpawnCategory) {
        if (g_DisplayItemCount < 256) {
            DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
            const char* label = "Rotation XYZ";
            int k = 0; while (label[k]) { item.label[k] = label[k]; k++; } item.label[k] = '\0';
            item.isCategory = false; item.tunableIndex = -1;
            item.liveAddr = (unsigned int)&g_SpawnRotation[0] - 72;
            item.type = TT_VEC3;
        }
        if (g_DisplayItemCount < 256) {
            DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
            const char* label = g_SnapToGround ? "Snap to Ground: ON" : "Snap to Ground: OFF";
            int k = 0; while (label[k]) { item.label[k] = label[k]; k++; } item.label[k] = '\0';
            item.isCategory = false; item.tunableIndex = 5002;
            item.liveAddr = 0;
            item.type = TT_ACTION;
        }
        if (g_DisplayItemCount < 256) {
            DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
            const char* label = "Spawn";
            int k = 0; while (label[k]) { item.label[k] = label[k]; k++; } item.label[k] = '\0';
            item.isCategory = false; item.tunableIndex = 5001;
            item.liveAddr = g_SelectedEntityHash;
            item.type = TT_ACTION;
        }
    }

    if (inFreeCamCategory && g_DisplayItemCount < 256) {
        DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
        const char* label = g_FreeCamEnabled ? "FreeCam: ON" : "FreeCam: OFF";
        int k = 0; while (label[k] != '\0') { item.label[k] = label[k]; k++; } item.label[k] = '\0';
        item.isCategory = false; item.tunableIndex = -1;
        item.liveAddr = 0;
        item.type = TT_ACTION;
    }
    if (inFreeCamCategory && g_DisplayItemCount < 256) {
        DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
        const char* label = "Speed";
        int k = 0; while (label[k] != '\0') { item.label[k] = label[k]; k++; } item.label[k] = '\0';
        item.isCategory = false; item.tunableIndex = -1;
        item.liveAddr = (unsigned int)&g_FreeCamMoveSpeed - 72;
        item.type = TT_FLOAT;
    }

    // --- Episode leaves ---
    if (inEpisodesCategory) {
        unsigned int stringTable = *(unsigned int*)0xF6687C;
        GetStr_t GetStr = (GetStr_t)&s_getStrOpd;
        for (int i = 0; i < g_EpisodeCount && g_DisplayItemCount < 256; i++) {
            const char* resolved = GetStr(stringTable, g_EpisodeKeys[i], "[Missing]", 0);
            DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
            int k = 0;
            while (resolved[k] != '\0' && k < 60) { item.label[k] = resolved[k]; k++; }
            item.label[k] = '\0';
            SanitizeForFont(item.label);
            item.isCategory = true;
            item.tunableIndex = 1000 + i;
        }
    }

    // --- "EpisodeJobs" ---
    if (inEpisodeJobsCategory) {
        unsigned int stringTable = *(unsigned int*)0xF6687C;
        GetStr_t GetStr = (GetStr_t)&s_getStrOpd;
        if (g_SelectedEpisodeIndex >= 0 && g_SelectedEpisodeIndex < g_EpisodeCount) {
            const EpisodeJobList& jl = g_EpisodeJobLists[g_SelectedEpisodeIndex];
            for (int i = 0; i < jl.jobCount && g_DisplayItemCount < 256; i++) {
                const char* resolved = GetStr(stringTable, jl.jobKeys[i], "[Missing]", 0);
                DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
                int k = 0;
                while (resolved[k] != '\0' && k < 60) { item.label[k] = resolved[k]; k++; }
                item.label[k] = '\0';
                SanitizeForFont(item.label);
                item.isCategory = false;
                item.tunableIndex = 2000 + i;
                item.liveAddr = 0;
                item.type = TT_ACTION;
            }
        }
        g_CurrentMenuMaxLen = StrLen(g_SelectedEpisodeNameBuf) + 2;
        for (int i = 0; i < g_DisplayItemCount; i++) {
            int len = StrLen(g_DisplayItems[i].label) + 12;
            if (len > g_CurrentMenuMaxLen) g_CurrentMenuMaxLen = len;
        }
        return;
    }

    // --- "JobGoals" ---
    if (inJobGoalsCategory) {
        const JobGoalList* jg = FindJobGoals(g_SelectedJobKeyBuf);
        if (jg != nullptr) {
            for (int i = 0; i < jg->goalCount && g_DisplayItemCount < 256; i++) {
                DisplayItem& item = g_DisplayItems[g_DisplayItemCount++];
                int k = 0;
                while (jg->goals[i][k] != '\0' && k < 60) { item.label[k] = jg->goals[i][k]; k++; }
                item.label[k] = '\0';
                SanitizeForFont(item.label);
                item.isCategory = false;
                item.tunableIndex = 3000 + i;
                item.liveAddr = 0;
                item.type = TT_ACTION;
            }
        }
        g_CurrentMenuMaxLen = StrLen(g_SelectedJobKeyBuf) + 2;
        for (int i = 0; i < g_DisplayItemCount; i++) {
            int len = StrLen(g_DisplayItems[i].label) + 12;
            if (len > g_CurrentMenuMaxLen) g_CurrentMenuMaxLen = len;
        }
        return;
    }

    // --- EntitySpawn: skip sort, early return ---
    if (inEntitySpawnCategory) {
        g_CurrentMenuMaxLen = StrLen(g_SelectedEntityName) + 2;
        for (int i = 0; i < g_DisplayItemCount; i++) {
            int len = StrLen(g_DisplayItems[i].label) + 12;
            if (len > g_CurrentMenuMaxLen) g_CurrentMenuMaxLen = len;
        }
        return;
    }

    for (int i = 1; i < g_DisplayItemCount; i++) {
        DisplayItem key = g_DisplayItems[i];
        int j = i - 1;
        while (j >= 0) {
            bool greater;
            if (g_DisplayItems[j].isCategory != key.isCategory) {
                greater = key.isCategory && !g_DisplayItems[j].isCategory;
            }
            else {
                greater = false;
                int k = 0;
                while (true) {
                    char a = g_DisplayItems[j].label[k];
                    char b = key.label[k];
                    if (a != b) { greater = (a > b); break; }
                    if (a == '\0') { greater = false; break; }
                    k++;
                }
            }
            if (!greater) break;
            g_DisplayItems[j + 1] = g_DisplayItems[j];
            j--;
        }
        g_DisplayItems[j + 1] = key;
    }

    const char* header = g_CurrentPath[0] ? g_CurrentPath : "Tuning";
    g_CurrentMenuMaxLen = StrLen(header) + 2;
    for (int i = 0; i < g_DisplayItemCount; i++) {
        int len = StrLen(g_DisplayItems[i].label) + 4;
        len += g_DisplayItems[i].isCategory ? 3 : 8;
        if (len > g_CurrentMenuMaxLen) g_CurrentMenuMaxLen = len;
    }
}

void EnterCategory(const char* categoryName)
{
    if (g_PathStackDepth < 16) {
        int i = 0;
        while (g_CurrentPath[i] != '\0') { g_PathStack[g_PathStackDepth][i] = g_CurrentPath[i]; i++; }
        g_PathStack[g_PathStackDepth][i] = '\0';
        g_SelectedIndexStack[g_PathStackDepth] = g_SelectedIndex;
        g_ScrollOffsetStack[g_PathStackDepth] = g_ScrollOffset;
        g_PathStackDepth++;
    }

    char newPath[256];
    int pos = 0;
    if (g_CurrentPath[0] != '\0') {
        int i = 0;
        while (g_CurrentPath[i] != '\0') { newPath[pos++] = g_CurrentPath[i]; i++; }
        newPath[pos++] = '.';
    }
    int i = 0;
    while (categoryName[i] != '\0') { newPath[pos++] = categoryName[i]; i++; }
    newPath[pos] = '\0';

    for (int k = 0; k <= pos; k++) g_CurrentPath[k] = newPath[k];
    g_SelectedIndex = 0;
    g_ScrollOffset = 0;
    RebuildDisplayItems();
    g_HasPrintedThisCategory = false;
}

bool GoBack()
{
    if (g_PathStackDepth == 0) return false;
    g_PathStackDepth--;
    int i = 0;
    while (g_PathStack[g_PathStackDepth][i] != '\0') {
        g_CurrentPath[i] = g_PathStack[g_PathStackDepth][i];
        i++;
    }
    g_CurrentPath[i] = '\0';
    g_SelectedIndex = g_SelectedIndexStack[g_PathStackDepth];
    g_ScrollOffset = g_ScrollOffsetStack[g_PathStackDepth];
    RebuildDisplayItems();
    g_HasPrintedThisCategory = false;
    return true;
}

void SysutilGameExitCallback(uint64_t status, uint64_t param, void* userdata) {
    if (status == CELL_SYSUTIL_REQUEST_EXITGAME) {
        printf("[SPRX] exit requested, cleaning up\n");

        // Remove all detour hooks first
        if (g_DrawSweepHook) { g_DrawSweepHook->~detour();   g_DrawSweepHook = nullptr; }
        if (g_PadGetDataHook) { g_PadGetDataHook->~detour();  g_PadGetDataHook = nullptr; }
        

        // Abort OSK if open
        if (g_oskMode == OSK_MODE_RUNNING)
            cellOskDialogAbort();

        // Unregister callbacks
        cellSysutilUnregisterCallback(0);
        cellSysutilUnregisterCallback(1);
        cellSysutilUnregisterCallback(2);

        printf("[SPRX] cleanup done\n");
    }

    // Forward OSK events to OSK callback
    OskSysutilCallback(status, param, userdata);
}

void OpenOSK(const char* prompt) {
    if (g_oskMode != OSK_MODE_IDLE) return;

    static uint16_t promptUtf16[128] = {};
    memset(promptUtf16, 0, sizeof(promptUtf16));
    for (int i = 0; i < 127 && prompt[i]; i++)
        promptUtf16[i] = (uint16_t)(unsigned char)prompt[i];

    cellOskDialogAddSupportLanguage(CELL_OSKDIALOG_PANELMODE_FINNISH);
    cellOskDialogSetKeyLayoutOption(CELL_OSKDIALOG_10KEY_PANEL | CELL_OSKDIALOG_FULLKEY_PANEL);

    static uint16_t initText[1] = { 0 };

    CellOskDialogInputFieldInfo inputInfo = {};
    inputInfo.message = promptUtf16;
    inputInfo.init_text = initText;
    inputInfo.limit_length = 64;

    CellOskDialogParam dialogParam = {};
    dialogParam.allowOskPanelFlg = CELL_OSKDIALOG_PANELMODE_ALPHABET_FULL_WIDTH |
        CELL_OSKDIALOG_PANELMODE_NUMERAL_FULL_WIDTH |
        CELL_OSKDIALOG_PANELMODE_ENGLISH |
        CELL_OSKDIALOG_PANELMODE_FINNISH;
    dialogParam.firstViewPanel = CELL_OSKDIALOG_PANELMODE_FINNISH;
    dialogParam.controlPoint.x = 0.0f;
    dialogParam.controlPoint.y = 0.0f;
    dialogParam.prohibitFlgs = 0;

    memset(g_oskResultBuffer, 0, sizeof(g_oskResultBuffer));
    g_oskOutputInfo.result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;
    g_oskOutputInfo.numCharsResultString = 128;
    g_oskOutputInfo.pResultString = g_oskResultBuffer;

    g_MenuVisible = false;
    g_oskMode = OSK_MODE_RUNNING;  // set BEFORE LoadAsync so callback sees correct state

    cellSysutilRegisterCallback(1, OskSysutilCallback, NULL);  // register once here

    int ret = cellOskDialogLoadAsync(SYS_MEMORY_CONTAINER_ID_INVALID, &dialogParam, &inputInfo);
    printf("[OSK] LoadAsync ret=%d (0x%08X)\n", ret, (unsigned int)ret);
}

void ProcessOSK() {
    cellSysutilCheckCallback();  // pump events — this is all ProcessOSK needs to do while RUNNING

    if (g_oskMode == OSK_MODE_CLOSE) {
        cellOskDialogUnloadAsync(&g_oskOutputInfo);

        memset(g_oskResult, 0, sizeof(g_oskResult));
        for (int i = 0; i < 127 && g_oskResultBuffer[i]; i++)
            g_oskResult[i] = (char)(g_oskResultBuffer[i] & 0x7F);

        cellSysutilUnregisterCallback(1);
        g_oskMode = OSK_MODE_IDLE;

        if (g_oskResult[0])
            TriggerJobFailed(g_oskResult, 0);

        g_MenuVisible = true;
        RebuildDisplayItems();
    }
}






void PollInput()
{
    if (g_RealPadData.len <= 0) return;

    uint16_t digital1 = g_RealPadData.button[CELL_PAD_BTN_OFFSET_DIGITAL1];
    uint16_t digital2 = g_RealPadData.button[CELL_PAD_BTN_OFFSET_DIGITAL2];

    bool curL1 = (digital2 & CELL_PAD_CTRL_L1) != 0;
    bool curL3 = (digital1 & CELL_PAD_CTRL_L3) != 0;
    bool curStart = (digital1 & CELL_PAD_CTRL_START) != 0;
    bool curUp = (digital1 & CELL_PAD_CTRL_UP) != 0;
    bool curDown = (digital1 & CELL_PAD_CTRL_DOWN) != 0;
    bool curLeft = (digital1 & CELL_PAD_CTRL_LEFT) != 0;
    bool curRight = (digital1 & CELL_PAD_CTRL_RIGHT) != 0;
    bool curSquare = (digital2 & CELL_PAD_CTRL_SQUARE) != 0;
    bool curCross = (digital2 & CELL_PAD_CTRL_CROSS) != 0;
    bool curCircle = (digital2 & CELL_PAD_CTRL_CIRCLE) != 0;
    bool curL2 = (digital2 & CELL_PAD_CTRL_L2) != 0;
    bool curR2 = (digital2 & CELL_PAD_CTRL_R2) != 0;

    if (g_MenuVisible) {

        // --- Left stick: drag menu position ---
        int lx = (int)g_RealPadData.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X] - 128;
        int ly = (int)g_RealPadData.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y] - 128;

        if (AbsInt(lx) > kStickDeadzone)
            g_MenuDragX += (lx / 128.0f) * kMenuMoveSpeed;
        if (AbsInt(ly) > kStickDeadzone)
            g_MenuDragY += (ly / 128.0f) * kMenuMoveSpeed;

        if (g_MenuDragX > kMenuDragClamp) g_MenuDragX = kMenuDragClamp;
        if (g_MenuDragX < -kMenuDragClamp) g_MenuDragX = -kMenuDragClamp;
        if (g_MenuDragY > kMenuDragClamp) g_MenuDragY = kMenuDragClamp;
        if (g_MenuDragY < -kMenuDragClamp) g_MenuDragY = -kMenuDragClamp;

        if (g_MenuState != MENU_STATE_VECTOR_EDIT) {
            int itemCount;
            if (g_MenuState == MENU_STATE_TOP)        itemCount = g_TopMenuItemCount;
            else if (g_MenuState == MENU_STATE_LOAD)  itemCount = g_LoadMenuItemCount;
            else if (g_MenuState == MENU_STATE_SCENE) itemCount = g_SceneMenuItemCount;
            else                                       itemCount = g_DisplayItemCount;

            if (curUp && !g_PrevUp && itemCount > 0) {
                g_SelectedIndex--;
                if (g_SelectedIndex < 0) g_SelectedIndex = itemCount - 1;
            }
            if (curDown && !g_PrevDown && itemCount > 0) {
                g_SelectedIndex++;
                if (g_SelectedIndex >= itemCount) g_SelectedIndex = 0;
            }

            if (g_MenuState == MENU_STATE_TUNABLES) {
                if (g_SelectedIndex == 0) {
                    g_ScrollOffset = 0;
                }
                else if (g_SelectedIndex == itemCount - 1) {
                    g_ScrollOffset = (itemCount > MENU_WINDOW_SIZE) ? (itemCount - MENU_WINDOW_SIZE) : 0;
                }
                else if (g_SelectedIndex < g_ScrollOffset) {
                    g_ScrollOffset = g_SelectedIndex;
                }
                else if (g_SelectedIndex >= g_ScrollOffset + MENU_WINDOW_SIZE) {
                    g_ScrollOffset = g_SelectedIndex - MENU_WINDOW_SIZE + 1;
                }
            }
        }

        if (g_MenuState == MENU_STATE_TOP) {
            if (curCross && !g_PrevCross) {
                if (g_SelectedIndex == 0) {
                    g_MenuState = MENU_STATE_TUNABLES;
                    g_CurrentPath[0] = '\0';
                    g_PathStackDepth = 0;
                    g_SelectedIndex = 0;
                    g_ScrollOffset = 0;
                    RebuildDisplayItems();
                }
                else if (g_SelectedIndex == 2) {
                    g_MenuState = MENU_STATE_LOAD;
                    g_SelectedIndex = 0;
                }
                else if (g_SelectedIndex == 3) {
                    g_MenuState = MENU_STATE_SCENE;
                    g_SelectedIndex = 0;
                    g_ScrollOffset = 0;
                }
                else {
                    printf("[Menu] %s not implemented yet\n", g_MenuItems[g_SelectedIndex]);
                }
            }
            if (curSquare && !g_PrevSquare) {
                printf("[Menu] Back -> closing menu\n");
                g_MenuVisible = false;
            }
        }
        else if (g_MenuState == MENU_STATE_SCENE) {
            // Left/right for FreeCam Speed
            if (StrEqual(g_SceneMenuItems[g_SelectedIndex], "FreeCam Speed")) {
                if (curLeft) g_LeftHoldFrames++; else g_LeftHoldFrames = 0;
                if (curRight) g_RightHoldFrames++; else g_RightHoldFrames = 0;
                bool triggerLeft = (curLeft && !g_PrevLeft) || (g_LeftHoldFrames > 20 && (g_LeftHoldFrames % 3 == 0));
                bool triggerRight = (curRight && !g_PrevRight) || (g_RightHoldFrames > 20 && (g_RightHoldFrames % 3 == 0));
                float step = curL1 ? 1.0f : 0.1f;
                if (triggerLeft)  g_FreeCamMoveSpeed -= step;
                if (triggerRight) g_FreeCamMoveSpeed += step;
                if (g_FreeCamMoveSpeed < 0.1f)  g_FreeCamMoveSpeed = 0.1f;
                if (g_FreeCamMoveSpeed > 20.0f) g_FreeCamMoveSpeed = 20.0f;
            }

            if (curCross && !g_PrevCross) {
                if (g_SelectedIndex >= g_SceneMenuItemCount) return;
                if (StrEqual(g_SceneMenuItems[g_SelectedIndex], "Save Scene")) {
                    SaveSceneFile();
                }
                else if (StrEqual(g_SceneMenuItems[g_SelectedIndex], "Load Scene")) {
                    if (LoadSceneFile()) {
                        g_HasPendingSceneLoad = true;  // defer to next frame
                    }
                }
                else if (StrEqual(g_SceneMenuItems[g_SelectedIndex], "Clear List")) {
                    ClearScene();
                }
                else if (StrEqual(g_SceneMenuItems[g_SelectedIndex], "Spawn Entity")) {
                    g_MenuState = MENU_STATE_TUNABLES;
                    const char* path = "Entities";
                    int k = 0;
                    while (path[k]) { g_CurrentPath[k] = path[k]; k++; }
                    g_CurrentPath[k] = '\0';
                    g_PathStackDepth = 0;
                    g_SelectedIndex = 0;
                    g_ScrollOffset = 0;
                    g_EnteredTuningViaScene = true;
                    RebuildDisplayItems();
                }
                else if (StrEqual(g_SceneMenuItems[g_SelectedIndex], "Level Editor Cam")) {
                    g_LevelEditorEnabled = !g_LevelEditorEnabled;
                    if (g_LevelEditorEnabled) {
                        g_FreeCamEnabled = false;
                        g_FreeCamInitialized = false;
                        g_OrbitInitialized = false;
                    }
                    else {
                        g_OrbitInitialized = false;
                    }
                }
            }

            if (curSquare && !g_PrevSquare) {
                g_MenuState = MENU_STATE_TOP;
                g_SelectedIndex = 3;
            }
        }

        else if (g_MenuState == MENU_STATE_LOAD) {
            if (curCross && !g_PrevCross) {
                if (StrEqual(g_LoadMenuItems[g_SelectedIndex], "Episodes...")) {
                    g_MenuState = MENU_STATE_TUNABLES;
                    g_EnteredTuningViaLoad = true;
                    const char* path = "Episodes";
                    int k = 0;
                    while (path[k] != '\0') { g_CurrentPath[k] = path[k]; k++; }
                    g_CurrentPath[k] = '\0';
                    g_PathStackDepth = 0;
                    g_SelectedIndex = 0;
                    g_ScrollOffset = 0;
                    RebuildDisplayItems();
                }
                else {
                    printf("[Menu] %s not implemented yet\n", g_LoadMenuItems[g_SelectedIndex]);
                }
            }
            if (curSquare && !g_PrevSquare) {
                g_MenuState = MENU_STATE_TOP;
                g_SelectedIndex = 2;
            }
        }
        else if (g_MenuState == MENU_STATE_VECTOR_EDIT) {
            if (curUp && !g_PrevUp) {
                g_VectorEditSelectedComponent--;
                if (g_VectorEditSelectedComponent < 0) g_VectorEditSelectedComponent = g_VectorEditComponentCount - 1;
            }
            if (curDown && !g_PrevDown) {
                g_VectorEditSelectedComponent++;
                if (g_VectorEditSelectedComponent >= g_VectorEditComponentCount) g_VectorEditSelectedComponent = 0;
            }

            if (curLeft) g_LeftHoldFrames++; else g_LeftHoldFrames = 0;
            if (curRight) g_RightHoldFrames++; else g_RightHoldFrames = 0;
            bool triggerLeft = (curLeft && !g_PrevLeft) || (g_LeftHoldFrames > 20 && (g_LeftHoldFrames % 3 == 0));
            bool triggerRight = (curRight && !g_PrevRight) || (g_RightHoldFrames > 20 && (g_RightHoldFrames % 3 == 0));

            if (triggerLeft || triggerRight) {
                int dir = triggerRight ? 1 : -1;
                float multiplier = curL1 ? 10.0f : 1.0f;
                float* component = (float*)(g_VectorEditAddr + 72 + g_VectorEditSelectedComponent * 4);
                *component += dir * 0.1f * multiplier;
            }

            if (curSquare && !g_PrevSquare) {
                g_MenuState = MENU_STATE_TUNABLES;
            }
        }
        else if (g_MenuState == MENU_STATE_COLOR_EDIT) {
            if (curUp && !g_PrevUp) {
                g_VectorEditSelectedComponent--;
                if (g_VectorEditSelectedComponent < 0) g_VectorEditSelectedComponent = 3;
            }
            if (curDown && !g_PrevDown) {
                g_VectorEditSelectedComponent++;
                if (g_VectorEditSelectedComponent >= 4) g_VectorEditSelectedComponent = 0;
            }

            if (curLeft) g_LeftHoldFrames++; else g_LeftHoldFrames = 0;
            if (curRight) g_RightHoldFrames++; else g_RightHoldFrames = 0;
            bool triggerLeft = (curLeft && !g_PrevLeft) || (g_LeftHoldFrames > 20 && (g_LeftHoldFrames % 3 == 0));
            bool triggerRight = (curRight && !g_PrevRight) || (g_RightHoldFrames > 20 && (g_RightHoldFrames % 3 == 0));

            if (triggerLeft || triggerRight) {
                int dir = triggerRight ? 1 : -1;
                unsigned char* component = (unsigned char*)(g_VectorEditAddr + 72 + g_VectorEditSelectedComponent);
                int newVal = (int)*component + dir * (curL1 ? 10 : 1);
                if (newVal < 0) newVal = 0;
                if (newVal > 255) newVal = 255;
                *component = (unsigned char)newVal;
            }

            if (curSquare && !g_PrevSquare) {
                g_MenuState = MENU_STATE_TUNABLES;
            }
        }
        else {
            if (curLeft) g_LeftHoldFrames++; else g_LeftHoldFrames = 0;
            if (curRight) g_RightHoldFrames++; else g_RightHoldFrames = 0;

            bool triggerLeft = (curLeft && !g_PrevLeft) || (g_LeftHoldFrames > 20 && (g_LeftHoldFrames % 3 == 0));
            bool triggerRight = (curRight && !g_PrevRight) || (g_RightHoldFrames > 20 && (g_RightHoldFrames % 3 == 0));

            if (triggerLeft || triggerRight) {
                if (g_DisplayItemCount > 0) {
                    DisplayItem& item = g_DisplayItems[g_SelectedIndex];
                    if (!item.isCategory && item.liveAddr) {
                        int dir = triggerRight ? 1 : -1;
                        float multiplier = curL1 ? 10.0f : 1.0f;
                        if (item.type == TT_INT) {
                            *(int*)(item.liveAddr + 72) += (int)(dir * multiplier);
                        }
                        else if (item.type == TT_FLOAT) {
                            *(float*)(item.liveAddr + 72) += dir * 0.1f * multiplier;
                        }
                    }
                }
            }

            if (curCross && !g_PrevCross && g_DisplayItemCount > 0) {
                DisplayItem& item = g_DisplayItems[g_SelectedIndex];

                if (item.isCategory && item.tunableIndex >= 4000 && item.tunableIndex < 4000 + g_EntityWorldCount) {
                    g_SelectedWorldIndex = item.tunableIndex - 4000;
                    g_SelectedTypeIndex = -1;

                    if (g_PathStackDepth < 16) {
                        int pi = 0;
                        while (g_CurrentPath[pi] != '\0') { g_PathStack[g_PathStackDepth][pi] = g_CurrentPath[pi]; pi++; }
                        g_PathStack[g_PathStackDepth][pi] = '\0';
                        g_SelectedIndexStack[g_PathStackDepth] = g_SelectedIndex;
                        g_ScrollOffsetStack[g_PathStackDepth] = g_ScrollOffset;
                        g_PathStackDepth++;
                    }

                    const char* path = "EntityWorld";
                    int k = 0;
                    while (path[k] != '\0') { g_CurrentPath[k] = path[k]; k++; }
                    g_CurrentPath[k] = '\0';
                    g_SelectedIndex = 0;
                    g_ScrollOffset = 0;
                    RebuildDisplayItems();
                }
                else if (item.isCategory && item.tunableIndex >= 4100 && item.tunableIndex < 4100 + 64) {
                    g_SelectedTypeIndex = item.tunableIndex - 4100;

                    if (g_PathStackDepth < 16) {
                        int pi = 0;
                        while (g_CurrentPath[pi] != '\0') { g_PathStack[g_PathStackDepth][pi] = g_CurrentPath[pi]; pi++; }
                        g_PathStack[g_PathStackDepth][pi] = '\0';
                        g_SelectedIndexStack[g_PathStackDepth] = g_SelectedIndex;
                        g_ScrollOffsetStack[g_PathStackDepth] = g_ScrollOffset;
                        g_PathStackDepth++;
                    }

                    const char* path = "EntityList";
                    int k = 0;
                    while (path[k] != '\0') { g_CurrentPath[k] = path[k]; k++; }
                    g_CurrentPath[k] = '\0';
                    g_SelectedIndex = 0;
                    g_ScrollOffset = 0;
                    RebuildDisplayItems();
                }
                // --- CHANGED: navigate to EntitySpawn instead of spawning directly ---
                else if (!item.isCategory && item.tunableIndex == 5000) {
                    g_SelectedEntityHash = item.liveAddr;
                    int k = 0;
                    while (item.label[k] != '\0' && k < 63) {
                        g_SelectedEntityName[k] = item.label[k]; k++;
                    }
                    g_SelectedEntityName[k] = '\0';

                    if (g_PathStackDepth < 16) {
                        int pi = 0;
                        while (g_CurrentPath[pi]) { g_PathStack[g_PathStackDepth][pi] = g_CurrentPath[pi]; pi++; }
                        g_PathStack[g_PathStackDepth][pi] = '\0';
                        g_SelectedIndexStack[g_PathStackDepth] = g_SelectedIndex;
                        g_ScrollOffsetStack[g_PathStackDepth] = g_ScrollOffset;
                        g_PathStackDepth++;
                    }

                    const char* path = "EntitySpawn";
                    k = 0;
                    while (path[k]) { g_CurrentPath[k] = path[k]; k++; } g_CurrentPath[k] = '\0';
                    g_SelectedIndex = 0;   // start at Rotation XYZ
                    g_ScrollOffset = 0;
                    RebuildDisplayItems();
                }
                // --- NEW: actual spawn trigger from EntitySpawn screen ---
                else if (!item.isCategory && item.tunableIndex == 5001) {
                    QueueSpawn(g_SelectedEntityHash, true);
                }

                else if (item.tunableIndex == 5002) {
                    g_SnapToGround = !g_SnapToGround;
                    RebuildDisplayItems();  // refresh to update ON/OFF label
                }
                else if (item.isCategory && item.tunableIndex >= 1000 && item.tunableIndex < 1000 + g_EpisodeCount) {
                    g_SelectedEpisodeIndex = item.tunableIndex - 1000;
                    int k = 0;
                    while (item.label[k] != '\0' && k < 63) { g_SelectedEpisodeNameBuf[k] = item.label[k]; k++; }
                    g_SelectedEpisodeNameBuf[k] = '\0';

                    if (g_PathStackDepth < 16) {
                        int i = 0;
                        while (g_CurrentPath[i] != '\0') { g_PathStack[g_PathStackDepth][i] = g_CurrentPath[i]; i++; }
                        g_PathStack[g_PathStackDepth][i] = '\0';
                        g_SelectedIndexStack[g_PathStackDepth] = g_SelectedIndex;
                        g_ScrollOffsetStack[g_PathStackDepth] = g_ScrollOffset;
                        g_PathStackDepth++;
                    }

                    const char* path = "EpisodeJobs";
                    k = 0;
                    while (path[k] != '\0') { g_CurrentPath[k] = path[k]; k++; }
                    g_CurrentPath[k] = '\0';
                    g_SelectedIndex = 0;
                    g_ScrollOffset = 0;
                    RebuildDisplayItems();
                }
                else if (!item.isCategory && item.tunableIndex >= 2000 && item.tunableIndex < 2000 + 100) {
                    int localJobIdx = item.tunableIndex - 2000;

                    if (g_SelectedEpisodeIndex >= 0 && g_SelectedEpisodeIndex < g_EpisodeCount) {
                        const EpisodeJobList& jl = g_EpisodeJobLists[g_SelectedEpisodeIndex];

                        if (localJobIdx < jl.jobCount) {
                            const char* key = jl.jobKeys[localJobIdx];
                            int globalIdx = FindGlobalJobIndex(key);

                            if (globalIdx >= 0) {
                                if (curL2) {
                                    Action_StartJobByIndex(globalIdx, 0, true, false);
                                }
                                else if (curR2) {
                                    Action_StartJobByIndex(globalIdx, 0, false, true);
                                }
                                else {
                                    bool started = Action_StartJobByIndex(globalIdx, 0, false, false);

                                    if (started) {
                                        int k = 0;
                                        while (key[k] != '\0' && k < 63) { g_SelectedJobKeyBuf[k] = key[k]; k++; }
                                        g_SelectedJobKeyBuf[k] = '\0';

                                        if (g_PathStackDepth < 16) {
                                            int i = 0;
                                            while (g_CurrentPath[i] != '\0') { g_PathStack[g_PathStackDepth][i] = g_CurrentPath[i]; i++; }
                                            g_PathStack[g_PathStackDepth][i] = '\0';
                                            g_SelectedIndexStack[g_PathStackDepth] = g_SelectedIndex;
                                            g_ScrollOffsetStack[g_PathStackDepth] = g_ScrollOffset;
                                            g_PathStackDepth++;
                                        }

                                        const char* path = "JobGoals";
                                        k = 0;
                                        while (path[k] != '\0') { g_CurrentPath[k] = path[k]; k++; }
                                        g_CurrentPath[k] = '\0';
                                        g_SelectedIndex = 0;
                                        g_ScrollOffset = 0;
                                        RebuildDisplayItems();
                                    }
                                }
                            }
                            else {
                                printf("[Menu] Could not resolve global index for job '%s'\n", key);
                            }
                        }
                    }
                }
                else if (!item.isCategory && item.tunableIndex >= 3000 && item.tunableIndex < 3000 + 100) {
                    int goalIdx = item.tunableIndex - 3000;
                    Action_TeleportToGoalIndex(goalIdx);
                }
                else if (item.isCategory) {
                    EnterCategory(item.label);
                }
                else if (item.type == TT_ACTION) {
                    if (StrEqual(item.label, "Delete Saved Settings")) Action_DeleteCfgFile();
                    else if (StrEqual(item.label, "Reload Saved Settings")) Action_ReloadCfgFile();
                    else if (StrEqual(item.label, "Toggle Fly Mode")) Action_ToggleFlyMode();
                    else if (StrEqual(item.label, "FreeCam: ON") || StrEqual(item.label, "FreeCam: OFF")) Action_ToggleFreeCam();
                    else if (StrEqual(item.label, "Fake Job Fail")) { g_MenuVisible = false; OpenOSK("Enter fail message:"); }
                   // else if (StrEqual(item.label, "Fake Binocucom Conversation")) { QueueBinocucom(0x00014120, 0x88397820); }
                    }
                
                else if (item.liveAddr && item.type == TT_BOOL) {
                    unsigned char* valPtr = (unsigned char*)(item.liveAddr + 72);
                    *valPtr = !(*valPtr);
                }
                else if (item.liveAddr && (item.type == TT_VEC3 || item.type == TT_VEC2)) {
                    g_VectorEditAddr = item.liveAddr;
                    int k = 0;
                    while (item.label[k] != '\0') { g_VectorEditLabel[k] = item.label[k]; k++; }
                    g_VectorEditLabel[k] = '\0';
                    g_VectorEditComponentCount = (item.type == TT_VEC3) ? 3 : 2;
                    g_VectorEditSelectedComponent = 0;
                    g_MenuState = MENU_STATE_VECTOR_EDIT;
                }
                else if (item.liveAddr && item.type == TT_COLOR) {
                    g_VectorEditAddr = item.liveAddr;
                    int k = 0;
                    while (item.label[k] != '\0') { g_VectorEditLabel[k] = item.label[k]; k++; }
                    g_VectorEditLabel[k] = '\0';
                    g_VectorEditComponentCount = 4;
                    g_VectorEditSelectedComponent = 0;
                    g_MenuState = MENU_STATE_COLOR_EDIT;
                }
            }

            if (curCircle && !g_PrevCircle && g_DisplayItemCount > 0) {
                DisplayItem& item = g_DisplayItems[g_SelectedIndex];
                if (!item.isCategory && item.liveAddr && item.type != TT_ACTION) {
                    ToggleTunableSaved(GetItemSaveName(item), item.liveAddr, item.type);
                    WriteSaveFile();
                    printf("[Menu] %s saved = %d\n", GetItemSaveName(item), IsItemSaved(item) ? 1 : 0);
                }
            }

            if (curSquare && !g_PrevSquare) {
                if (!GoBack()) {
                    if (g_EnteredTuningViaLoad) {
                        g_EnteredTuningViaLoad = false;
                        g_MenuState = MENU_STATE_LOAD;
                        g_SelectedIndex = 1;
                    }
                    else if (g_EnteredTuningViaScene) {
                        g_EnteredTuningViaScene = false;
                        g_MenuState = MENU_STATE_SCENE;
                        g_SelectedIndex = 3;
                    }
                    else {
                        g_MenuState = MENU_STATE_TOP;
                        g_SelectedIndex = 0;
                    }
                }
            }
        }
    }

    g_PrevL3 = curL3;
    g_PrevStart = curStart;
    g_PrevUp = curUp;
    g_PrevDown = curDown;
    g_PrevLeft = curLeft;
    g_PrevRight = curRight;
    g_PrevSquare = curSquare;
    g_PrevCross = curCross;
    g_PrevCircle = curCircle;
}

int PadGetDataHook(int port, CellPadData* data) {
    int ret = g_PadGetDataHook->invoke<int>(port, data);

    cellSysutilCheckCallback();
    ProcessOSK();
    

    if (ret == CELL_OK && data->len > 0 && port == 0) {
        g_RealPadData = *data;

        uint16_t digital1 = data->button[CELL_PAD_BTN_OFFSET_DIGITAL1];
        uint16_t digital2 = data->button[CELL_PAD_BTN_OFFSET_DIGITAL2];

        bool curL3 = (digital1 & CELL_PAD_CTRL_L3) != 0;
        bool curStart = (digital1 & CELL_PAD_CTRL_START) != 0;

        bool comboNow = curL3 && curStart;
        bool comboPrev = g_PrevL3 && g_PrevStart;
        if (comboNow && !comboPrev) {
            g_MenuVisible = !g_MenuVisible;
            if (g_MenuVisible) {
                RebuildDisplayItems();
            }
        }
        g_PrevL3 = curL3;
        g_PrevStart = curStart;

        // --- New: spawn test trigger, R3 + SQUARE ---
        /*bool curR3 = (digital1 & CELL_PAD_CTRL_R3) != 0;
        bool curSquare = (digital2 & CELL_PAD_CTRL_SQUARE) != 0;
        bool spawnComboNow = curR3 && curSquare;
        if (spawnComboNow && !g_PrevSpawnCombo) {
            printf("[Spawn] triggered from PadGetDataHook\n");
            SpawnPropAtPlayer(0xF9F8A58D);   // CoinDynamic
        }
        g_PrevSpawnCombo = spawnComboNow;*/

        // --- Drain queued spawn requests here (safe hook for spawning) ---
        if (g_HasPendingSpawn) {
            g_HasPendingSpawn = false;
            if (g_PendingSpawnSmart) SpawnEntitySmart(g_PendingSpawnHash);
            else                     SpawnPropAtPlayer(g_PendingSpawnHash);
        }

        if (g_HasPendingBinocu) {
            g_HasPendingBinocu = false;
            ShowBinocucom(g_PendingBinocuLeft, g_PendingBinocuRight);
        }

        if (g_HasPendingSceneLoad) {
            g_HasPendingSceneLoad = false;
            SpawnLoadedScene();
        }

    }

    if (g_MenuVisible && port == 0) {
        memset(data->button, 0, sizeof(data->button));
        data->len = 0;
    }
    else if (g_FreeCamEnabled && port == 0) {
        data->button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X] = 128;
        data->button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y] = 128;
        data->button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X] = 128;
        data->button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y] = 128;
        data->button[CELL_PAD_BTN_OFFSET_DIGITAL1] = 0;
        data->button[CELL_PAD_BTN_OFFSET_DIGITAL2] = 0;
    }

    return ret;
}

void DrawMenu()
{
    unsigned char headerColor[4] = { 80, 160, 255, 255 };
    unsigned char normalColor[4] = { 255, 255, 255, 255 };
    unsigned char onColor[4] = { 173, 177, 137, 255 };
    unsigned char savedColor[4] = { 255, 255, 0, 255 };
    unsigned char dimColor[4] = { 150, 150, 150, 255 };

    float pos[3] = { 0.10f + g_MenuDragX * kDragToTextX, 0.08f + g_MenuDragY * kDragToTextY, 0.0f };

    if (g_MenuState == MENU_STATE_TOP) {
        QueueText(pos, (int)"[Debug Menu]", headerColor, 1.0);
        for (int i = 0; i < g_TopMenuItemCount; i++) {
            pos[1] += 16.0f;
            const char* cursor = (i == g_SelectedIndex) ? ">" : " ";
            char* line = FormatStr("%s %s", cursor, g_MenuItems[i]);
            QueueText(pos, (int)line, normalColor, 1.0);
            float arrowPos[3] = { pos[0] + 220.0f, pos[1], 0.0f };
            QueueText(arrowPos, (int)">>", normalColor, 1.0);
        }
        return;
    }

    if (g_MenuState == MENU_STATE_LOAD) {
        QueueText(pos, (int)"[Load]", headerColor, 1.0);
        for (int i = 0; i < g_LoadMenuItemCount; i++) {
            pos[1] += 16.0f;
            const char* cursor = (i == g_SelectedIndex) ? ">" : " ";
            char* line = FormatStr("%s %s", cursor, g_LoadMenuItems[i]);
            QueueText(pos, (int)line, normalColor, 1.0);
            float arrowPos[3] = { pos[0] + 220.0f, pos[1], 0.0f };
            QueueText(arrowPos, (int)">>", normalColor, 1.0);
        }
        return;
    }

    if (g_MenuState == MENU_STATE_SCENE) {
        QueueText(pos, (int)"[Scene]", headerColor, 1.0);

        for (int i = 0; i < g_SceneMenuItemCount; i++) {
            pos[1] += 16.0f;
            const char* cursor = (i == g_SelectedIndex) ? ">" : " ";

            if (StrEqual(g_SceneMenuItems[i], "Spawn Entity")) {
                char* line = FormatStr("%s %s", cursor, g_SceneMenuItems[i]);
                QueueText(pos, (int)line, normalColor, 1.0);
                float arrowPos[3] = { pos[0] + 220.0f, pos[1], 0.0f };
                QueueText(arrowPos, (int)">>", normalColor, 1.0);
            }
            else if (StrEqual(g_SceneMenuItems[i], "Level Editor Cam")) {
                unsigned char onColor[4] = { 255, 180, 50, 255 };
                unsigned char offColor[4] = { 255, 255, 255, 255 };
                char* line = FormatStr("%s Level Editor Cam: %s", cursor, g_LevelEditorEnabled ? "ON" : "OFF");
                QueueText(pos, (int)line, g_LevelEditorEnabled ? onColor : offColor, 1.0);
            }

            else {
                char* line = FormatStr("%s %s", cursor, g_SceneMenuItems[i]);
                QueueText(pos, (int)line, normalColor, 1.0);
            }
        }

        // Use cached scene name — not recalculated every frame
        pos[1] += 16.0f;
        char* countLine = FormatStr("  Entities: %d  [%s]", g_SavedEntityCount, g_DisplaySceneName);
        QueueText(pos, (int)countLine, dimColor, 1.0);

        return;
    }

    if (g_MenuState == MENU_STATE_VECTOR_EDIT) {
        char* header = FormatStr("[%s]", g_VectorEditLabel);
        QueueText(pos, (int)header, headerColor, 1.0);

        unsigned char axisColors[3][4] = {
            { 255, 80,  80,  255 },
            { 80,  255, 80,  255 },
            { 80,  130, 255, 255 },
        };
        const char* axisNames[3] = { "X", "Y", "Z" };

        for (int i = 0; i < g_VectorEditComponentCount; i++) {
            pos[1] += 16.0f;
            const char* cursor = (i == g_VectorEditSelectedComponent) ? ">" : " ";
            float value = *(float*)(g_VectorEditAddr + 72 + i * 4);
            char* line = FormatStr("%s %s: %.3f", cursor, axisNames[i], value);
            QueueText(pos, (int)line, axisColors[i], 1.0);
        }
        return;
    }

    if (g_MenuState == MENU_STATE_COLOR_EDIT) {
        char* header = FormatStr("[%s]", g_VectorEditLabel);
        QueueText(pos, (int)header, headerColor, 1.0);

        const char* channelNames[4] = { "R", "G", "B", "A" };
        unsigned char r = *(unsigned char*)(g_VectorEditAddr + 72);
        unsigned char g = *(unsigned char*)(g_VectorEditAddr + 73);
        unsigned char b = *(unsigned char*)(g_VectorEditAddr + 74);
        unsigned char a = *(unsigned char*)(g_VectorEditAddr + 75);

        for (int i = 0; i < 4; i++) {
            pos[1] += 16.0f;
            const char* cursor = (i == g_VectorEditSelectedComponent) ? ">" : " ";
            unsigned char value = *(unsigned char*)(g_VectorEditAddr + 72 + i);
            char* line = FormatStr("%s %s: %d", cursor, channelNames[i], value);
            QueueText(pos, (int)line, normalColor, 1.0);
        }

        pos[1] += 16.0f;
        char* previewLabel = FormatStr("  Preview:");
        QueueText(pos, (int)previewLabel, normalColor, 1.0);
        unsigned char swatchColor[4] = { r, g, b, 255 };
        float swatchPos[3] = { pos[0] + 220.0f, pos[1], 0.0f };
        QueueText(swatchPos, (int)"####", swatchColor, 1.0);
        return;
    }

    if (StrEqual(g_CurrentPath, "EntitySpawn")) {
        char* header = FormatStr("[%s]", g_SelectedEntityName);
        QueueText(pos, (int)header, headerColor, 1.0);

        for (int i = 0; i < g_DisplayItemCount; i++) {
            pos[1] += 16.0f;
            const char* cursor = (i == g_SelectedIndex) ? ">" : " ";
            DisplayItem& item = g_DisplayItems[i];

            if (item.tunableIndex == 5001) {
                unsigned char spawnColor[4] = { 0, 220, 0, 255 };
                char* line = FormatStr("%s Spawn", cursor);
                QueueText(pos, (int)line, spawnColor, 1.0);
            }

            else if (item.tunableIndex == 5002) {
                unsigned char col[4] = { 80, 255, 80, 255 };
                unsigned char offCol[4] = { 255, 255, 255, 255 };
                char* line = FormatStr("%s %s", cursor, item.label);
                QueueText(pos, (int)line, g_SnapToGround ? col : offCol, 1.0);
            }

            else if (item.type == TT_VEC3 && item.liveAddr) {
                float x = *(float*)(item.liveAddr + 72);
                float y = *(float*)(item.liveAddr + 76);
                float z = *(float*)(item.liveAddr + 80);

                char* label = FormatStr("%s ", cursor);
                QueueText(pos, (int)label, normalColor, 1.0);

                float valueX = pos[0] + StrLen(label) * RECT_CHAR_WIDTH_TEXT;

                unsigned char xCol[4] = { 255, 80, 80, 255 };
                unsigned char yCol[4] = { 80, 255, 80, 255 };
                unsigned char zCol[4] = { 80, 130, 255, 255 };

                char* xStr = FormatStr("X=%.1f ", x);
                float xPos[3] = { valueX, pos[1], 0.0f };
                QueueText(xPos, (int)xStr, xCol, 1.0);

                char* yStr = FormatStr("Y=%.1f ", y);
                float yPos[3] = { valueX + StrLen(xStr) * RECT_CHAR_WIDTH_TEXT, pos[1], 0.0f };
                QueueText(yPos, (int)yStr, yCol, 1.0);

                char* zStr = FormatStr("Z=%.1f", z);
                float zPos[3] = { valueX + (StrLen(xStr) + StrLen(yStr)) * RECT_CHAR_WIDTH_TEXT, pos[1], 0.0f };
                QueueText(zPos, (int)zStr, zCol, 1.0);
            }
            else if (item.type == TT_FLOAT && item.liveAddr) {
                float val = *(float*)(item.liveAddr + 72);
                char* line = FormatStr("%s %s: %.2f", cursor, item.label, val);
                QueueText(pos, (int)line, normalColor, 1.0);
            }
        }
        return;
    }

    if (StrEqual(g_CurrentPath, "EpisodeJobs")) {
        char* header = FormatStr("[%s]", g_SelectedEpisodeNameBuf);
        QueueText(pos, (int)header, headerColor, 1.0);

        float valueX = pos[0] + (g_CurrentMenuMaxLen * RECT_CHAR_WIDTH_TEXT) + 20.0f;
        unsigned char orangeColor[4] = { 255, 165, 0, 255 };

        pos[1] += 16.0f;
        QueueText(pos, (int)"-Start Episode-", orangeColor, 1.0);

        for (int i = 0; i < g_DisplayItemCount; i++) {
            pos[1] += 16.0f;
            const char* cursor = (i == g_SelectedIndex) ? ">" : " ";
            char* line = FormatStr("%s %s", cursor, g_DisplayItems[i].label);
            QueueText(pos, (int)line, normalColor, 1.0);
            float arrowPos[3] = { valueX, pos[1], 0.0f };
            QueueText(arrowPos, (int)">>", normalColor, 1.0);
        }

        pos[1] += 16.0f;
        QueueText(pos, (int)"-End Episode-", orangeColor, 1.0);

        pos[1] += 32.0f;
        QueueText(pos, (int)"<L2+X> start from hub", normalColor, 1.0);
        pos[1] += 16.0f;
        QueueText(pos, (int)"<R2+X> start from hideout", normalColor, 1.0);
        return;
    }

    if (StrEqual(g_CurrentPath, "JobGoals")) {
        char* header = FormatStr("[%s]", g_SelectedJobKeyBuf);
        QueueText(pos, (int)header, headerColor, 1.0);

        unsigned char orangeColor[4] = { 255, 165, 0, 255 };
        unsigned char greenColor[4] = { 0, 220, 0, 255 };
        unsigned char yellowColor[4] = { 230, 230, 0, 255 };
        unsigned char redColor[4] = { 230, 60, 30, 255 };

        int currentGoalIdx;
        GetGoalColorIndices(g_DisplayItemCount, &currentGoalIdx);

        for (int i = 0; i < g_DisplayItemCount; i++) {
            pos[1] += 20.0f;
            const char* cursor = (i == g_SelectedIndex) ? ">" : " ";

            unsigned char* squareColor = redColor;
            if (currentGoalIdx == -1 || i < currentGoalIdx) squareColor = greenColor;
            else if (i == currentGoalIdx) squareColor = yellowColor;

            float squarePos[3] = { pos[0], pos[1] + 2.0f, 0.0f };
            QueueText(squarePos, (int)"\xE2", squareColor, 1.0);

            float textPos[3] = { pos[0] + 18.0f, pos[1], 0.0f };
            char* line = FormatStr("%s %s", cursor, g_DisplayItems[i].label);
            QueueText(textPos, (int)line, orangeColor, 1.0);
        }
        return;
    }

    char* header = g_CurrentPath[0] ? FormatStr("[%s]", g_CurrentPath) : FormatStr("[Tuning]");
    QueueText(pos, (int)header, headerColor, 1.0);

    float valueX = pos[0] + (g_CurrentMenuMaxLen * RECT_CHAR_WIDTH_TEXT) + 20.0f;

    int visibleCount = g_DisplayItemCount;
    if (visibleCount > MENU_WINDOW_SIZE) visibleCount = MENU_WINDOW_SIZE;

    if (g_ScrollOffset > 0) {
        pos[1] += 16.0f;
        QueueText(pos, (int)"-- more above --", normalColor, 1.0);
    }

    for (int row = 0; row < visibleCount; row++) {
        int i = g_ScrollOffset + row;
        pos[1] += 16.0f;
        const char* cursor = (i == g_SelectedIndex) ? ">" : " ";

        if (g_DisplayItems[i].isCategory) {
            char* line = FormatStr("%s %s", cursor, g_DisplayItems[i].label);
            if (!g_HasPrintedThisCategory) printf("[Menu] item %d ptr=%p text='%s'\n", i, line, line);
            QueueText(pos, (int)line, normalColor, 1.0);
            float arrowPos[3] = { valueX, pos[1], 0.0f };
            QueueText(arrowPos, (int)">>", normalColor, 1.0);
            continue;
        }

        DisplayItem& item = g_DisplayItems[i];

        if (item.type == TT_ACTION) {
            char* line = FormatStr("%s %s", cursor, item.label);
            QueueText(pos, (int)line, normalColor, 1.0);
            continue;
        }

        if (item.liveAddr == 0) {
            char* line = FormatStr("%s %s: ???", cursor, item.label);
            if (!g_HasPrintedThisCategory) printf("[Menu] item %d ptr=%p text='%s'\n", i, line, line);
            QueueText(pos, (int)line, normalColor, 1.0);
            continue;
        }

        bool saved = IsItemSaved(item);

        if (item.type == TT_BOOL) {
            bool isOn = (*(unsigned char*)(item.liveAddr + 72)) != 0;
            char* labelPart = FormatStr("%s %s: ", cursor, item.label);
            if (!g_HasPrintedThisCategory) printf("[Menu] item %d label='%s' value='%s'\n", i, item.label, isOn ? "ON" : "OFF");
            QueueText(pos, (int)labelPart, normalColor, 1.0);
            const char* valueStr = isOn ? "ON" : "OFF";
            float valuePos[3] = { valueX, pos[1], 0.0f };
            QueueText(valuePos, (int)valueStr, isOn ? onColor : normalColor, 1.0);
            if (saved) {
                float starPos[3] = { valueX + StrLen(valueStr) * RECT_CHAR_WIDTH_TEXT + 6.0f, pos[1], 0.0f };
                QueueText(starPos, (int)"*", savedColor, 1.0);
            }
            continue;
        }

        if (item.type == TT_COLOR) {
            char* labelPart = FormatStr("%s %s: ", cursor, item.label);
            QueueText(pos, (int)labelPart, normalColor, 1.0);
            unsigned char r = *(unsigned char*)(item.liveAddr + 72);
            unsigned char g = *(unsigned char*)(item.liveAddr + 73);
            unsigned char b = *(unsigned char*)(item.liveAddr + 74);
            unsigned char a = *(unsigned char*)(item.liveAddr + 75);
            char* valueText = FormatStr("%d,%d,%d,%d", r, g, b, a);
            if (!g_HasPrintedThisCategory) printf("[Menu] item %d label='%s' value='%s'\n", i, item.label, valueText);
            unsigned char swatchColor[4] = { r, g, b, 255 };
            float valuePos[3] = { valueX, pos[1], 0.0f };
            QueueText(valuePos, (int)valueText, swatchColor, 1.0);
            if (saved) {
                float starPos[3] = { valueX + StrLen(valueText) * RECT_CHAR_WIDTH_TEXT + 6.0f, pos[1], 0.0f };
                QueueText(starPos, (int)"*", savedColor, 1.0);
            }
            continue;
        }

        if (item.type == TT_STRING) {
            char* labelPart = FormatStr("%s %s: ", cursor, item.label);
            QueueText(pos, (int)labelPart, normalColor, 1.0);
            const char* str = (const char*)(item.liveAddr + 72);
            char* valueText = FormatStr("%s", str);
            if (!g_HasPrintedThisCategory) printf("[Menu] item %d label='%s' value='%s'\n", i, item.label, valueText);
            float valuePos[3] = { valueX, pos[1], 0.0f };
            QueueText(valuePos, (int)valueText, normalColor, 1.0);
            if (saved) {
                float starPos[3] = { valueX + StrLen(valueText) * RECT_CHAR_WIDTH_TEXT + 6.0f, pos[1], 0.0f };
                QueueText(starPos, (int)"*", savedColor, 1.0);
            }
            continue;
        }

        if (item.type == TT_STRING_HASH) {
            char* labelPart = FormatStr("%s %s: ", cursor, item.label);
            QueueText(pos, (int)labelPart, normalColor, 1.0);
            unsigned int hash = *(unsigned int*)(item.liveAddr + 72);
            char* valueText = FormatStr("#%08X", hash);
            if (!g_HasPrintedThisCategory) printf("[Menu] item %d label='%s' value='%s'\n", i, item.label, valueText);
            float valuePos[3] = { valueX, pos[1], 0.0f };
            QueueText(valuePos, (int)valueText, normalColor, 1.0);
            if (saved) {
                float starPos[3] = { valueX + StrLen(valueText) * RECT_CHAR_WIDTH_TEXT + 6.0f, pos[1], 0.0f };
                QueueText(starPos, (int)"*", savedColor, 1.0);
            }
            continue;
        }

        char* labelPart = FormatStr("%s %s: ", cursor, item.label);
        QueueText(pos, (int)labelPart, normalColor, 1.0);

        char* valueText;
        switch (item.type) {
        case TT_INT:
            valueText = FormatStr("%d", *(int*)(item.liveAddr + 72));
            break;
        case TT_FLOAT:
            valueText = FormatStr("%.3f", *(float*)(item.liveAddr + 72));
            break;
        default:
            valueText = FormatStr("(...)");
            break;
        }
        if (!g_HasPrintedThisCategory) printf("[Menu] item %d label='%s' value='%s'\n", i, item.label, valueText);

        float valuePos[3] = { valueX, pos[1], 0.0f };
        QueueText(valuePos, (int)valueText, normalColor, 1.0);

        if (saved) {
            float starPos[3] = { valueX + StrLen(valueText) * RECT_CHAR_WIDTH_TEXT + 6.0f, pos[1], 0.0f };
            QueueText(starPos, (int)"*", savedColor, 1.0);
        }
    }

    if (g_ScrollOffset + visibleCount < g_DisplayItemCount) {
        pos[1] += 16.0f;
        QueueText(pos, (int)"-- more below --", normalColor, 1.0);
    }

    g_HasPrintedThisCategory = true;
}

void SpawnPropAtPos(unsigned int entityHash, float px, float py, float pz) {
    BuildAndSpawn(entityHash, px, py, pz);
}

// Spawn at the freecam point when freecam is active, otherwise at the player.
void SpawnEntitySmart(unsigned int entityHash) {
    if (g_FreeCamEnabled) {
        SpawnPropAtPos(entityHash, g_FreeCamPos[0], g_FreeCamPos[1], g_FreeCamPos[2]);
    }
    else {
        SpawnPropAtPlayer(entityHash);
    }
}

void InitOSKContext() {
    // Load the OSK module explicitly
    cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_OSK_EXT);

    // Register our sysutil callback on slot 1
    // (slot 0 might be taken by system)
    cellSysutilRegisterCallback(1, OskSysutilCallback, NULL);

    printf("[OSK] context initialized\n");
}

void DrawOrbitPivot() {
    if (!g_LevelEditorEnabled) return;

    float px = g_OrbitPivot[0];
    float py = g_OrbitPivot[1];
    float pz = g_OrbitPivot[2];

    float len = g_OrbitRadius * 0.15f;
    if (len < 0.5f)  len = 0.5f;
    if (len > 30.0f) len = 30.0f;

    // X axis — red
    DrawDebugLine3D(px - len, py, pz, px + len, py, pz, 255, 50, 50, 255);
    // Y axis — green
    DrawDebugLine3D(px, py - len, pz, px, py + len, pz, 50, 255, 50, 255);
    // Z axis — blue
    DrawDebugLine3D(px, py, pz - len, px, py, pz + len, 50, 100, 255, 255);
}

void DrawLine3DOnCmdBuf(float* cmdBuf,
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    unsigned char r, unsigned char g,
    unsigned char b, unsigned char a) {

    // Skip BindMaterial — context might already have material bound
    BeginBatch((int)cmdBuf, (float*)&ApeDebug_DefaultBatchState);

    struct { void* ptr; short count; } vertHandle;
    AllocVerts(&vertHandle, (int)cmdBuf, 2, 2);

    float* v = (float*)vertHandle.ptr;
    if (!v || (unsigned int)v < 0x10000) return;

    v[0] = x1; v[1] = y1; v[2] = z1;
    unsigned char* c1 = (unsigned char*)(v + 3);
    c1[0] = r; c1[1] = g; c1[2] = b; c1[3] = a;

    v[6] = x2; v[7] = y2; v[8] = z2;
    unsigned char* c2 = (unsigned char*)(v + 9);
    c2[0] = r; c2[1] = g; c2[2] = b; c2[3] = a;
}

void DrawOrbitPivotWith3D(float* cmdBuf) {
    float px = g_OrbitPivot[0];
    float py = g_OrbitPivot[1];
    float pz = g_OrbitPivot[2];

    float len = g_OrbitRadius * 0.15f;
    if (len < 0.5f)  len = 0.5f;
    if (len > 30.0f) len = 30.0f;

    // X — red
    //DrawLine3DOnCmdBuf(cmdBuf, px - len, py, pz, px + len, py, pz, 255, 50, 50, 255);
    // Y — green
    //DrawLine3DOnCmdBuf(cmdBuf, px, py - len, pz, px, py + len, pz, 50, 255, 50, 255);
    // Z — blue
  //  DrawLine3DOnCmdBuf(cmdBuf, px, py, pz - len, px, py, pz + len, 50, 100, 255, 255);


    DrawLine3DOnCmdBuf(cmdBuf, -100, 0, 0, 100, 0, 0, 255, 0, 0, 255);  // giant red X line at world origin
    DrawLine3DOnCmdBuf(cmdBuf, 0, -100, 0, 0, 100, 0, 0, 255, 0, 255);  // giant green Y line

}

int DrawAndSweepHook()
{
    //printf("Inside DrawAndSweep Hook!");

    // In DrawAndSweepHook, always draw when valid:
    // In DrawAndSweepHook:
   /* if (g_ViewProjValid) {
        unsigned int playerPosRef = GetPlayerPositionRef();
        if (playerPosRef) {
            float px = *(float*)(playerPosRef + 144);
            float py = *(float*)(playerPosRef + 148);
            float pz = *(float*)(playerPosRef + 152);
            float sx, sy;
            if (WorldToScreen(px, py + 2.0f, pz, &sx, &sy)) {
                // Clamp to screen
                if (sx < 0.0f) sx = 0.0f; if (sx > 1.0f) sx = 1.0f;
                if (sy < 0.0f) sy = 0.0f; if (sy > 1.0f) sy = 1.0f;

                float size = 0.05f;
                DrawDebugLine(sx - size, sy - size, sx + size, sy - size, 0.0f, 255, 255, 0, 255);
                DrawDebugLine(sx + size, sy - size, sx + size, sy + size, 0.0f, 255, 255, 0, 255);
                DrawDebugLine(sx + size, sy + size, sx - size, sy + size, 0.0f, 255, 255, 0, 255);
                DrawDebugLine(sx - size, sy + size, sx - size, sy - size, 0.0f, 255, 255, 0, 255);
            }
        }
    }*/

   // UpdateFps();
  //  DrawFpsCounter();
    UpdateFlyMode();
    //UpdatePendingJobStart();
    UpdateGoalTeleportCooldown();
    UpdateFreeCamInput();
    UpdateLevelEditorInput();
    DrawSpawnRotationIndicator();
    //DrawOrbitPivot();

   // DrawSpawnRotationGizmo();
    //void* cmdBuf = GetCmdBuf();
    //DrawGrid(cmdBuf, identityMtx, 20, 20, 1.0f, 0xFFFFFFFFu);
   // UpdateWorldToScreenTest();
    
    if (g_MenuVisible) {
        DrawMenuBackground();
     //   DrawSelectionHighlight();
    }

    PollInput();
    if (g_MenuVisible) {
        DrawMenu();
    }

    int ret = g_DrawSweepHook->invoke<int>();
    return ret;
}

SYS_MODULE_INFO(Sly4_DebugMenu, 0, 1, 1);
SYS_MODULE_START(_Sly4_DebugMenu_prx_entry);

SYS_LIB_DECLARE_WITH_STUB(Sly4_DebugMenu_lib, SYS_LIB_AUTO_EXPORT, Sly4_DebugMenu_stub);
SYS_LIB_EXPORT(_Sly4_DebugMenu_export_function, Sly4_DebugMenu_lib);

extern "C" int _Sly4_DebugMenu_export_function(void)
{
    return CELL_OK;
}

extern "C" int _Sly4_DebugMenu_prx_entry(void)
{
    //printf("[Init] Sly4 debug rect hook installing...\n");
    cellSysutilRegisterCallback(0, SysutilGameExitCallback, NULL);
    InitOSKContext();

    g_DrawSweepHook = new (g_DrawSweepHookStorage) libpsutil::memory::detour(0x56AEFC, (void*)DrawAndSweepHook);
    g_PadGetDataHook = new (g_PadGetDataHookStorage) libpsutil::memory::detour(0xABDEC4, (void*)PadGetDataHook);
   // g_CalcMatrixHook = new (g_CalcMatrixHookStorage) libpsutil::memory::detour(0x101A60, (void*)CalcMatrixHook);
    g_CalcRenderMatricesHook = new (g_CalcRenderMatricesStorage) libpsutil::memory::detour(0x5115F0, (void*)CalcRenderMatricesHook);
    g_SetActiveHook = new (g_SetActiveStorage) libpsutil::memory::detour(0xFA3CC, (void*)SetActiveHook);
    g_SetCameraHook = new (g_SetCameraStorage) libpsutil::memory::detour(0x5201B0, (void*)SetCameraHook);
   // g_Post3DHUDHook = new (g_Post3DHUDStorage) libpsutil::memory::detour(0x459B60, (void*)Post3DHUDHook);
    //g_LookAtHook = new (g_LookAtHookStorage) libpsutil::memory::detour(0x56A0D0, (void*)LookAtHook);
    //g_CameraUpdatePosHook = new (g_CameraUpdatePosHookStorage) libpsutil::memory::detour(0x101CC0, (void*)CameraUpdatePosHook);
   // g_IntegratorHook = new (g_IntegratorHookStorage) libpsutil::memory::detour(0x26C920, (void*)IntegratorHook);
    //g_SystemTimeHook = new (g_SysTimeHookStorage) libpsutil::memory::detour(0x704198, (void*)SysTimeWrapperHook);
    //g_ButtonCheckHook = new (g_ButtonCheckHookStorage) libpsutil::memory::detour(0x74BB98, (void*)ButtonCheckHook);
   // printf("[Init] hook constructed at %p\n", g_DrawSweepHook);
   // InitOSK();
    printf("[Init] Sly4 Debug Menu installed!\n");
    LoadSaveFile(); // restore saved tunables + re-star them
    printf("[Init] Save file load attempted.\n");
    
    return SYS_PRX_RESIDENT;
}