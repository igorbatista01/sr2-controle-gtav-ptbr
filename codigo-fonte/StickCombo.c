/* SR2 StickCombo v2
   1) L3 click centered = crouch, L3 click with left stick pushed up = sprint.
   2) On foot, while L1 (left shoulder) is held (weapon wheel open) the game sees the
      RIGHT stick as the left stick, so the wheel is chosen with R3; the right stick is
      zeroed meanwhile so the camera does not spin.
   Reads the pad through the game's own SDL3.dll. Swaps the game's import slot for
   SDL_GetGamepadAxis inside DFEngine.dll (RVA 0x1102C8) with a wrapper. */
typedef unsigned int u32; typedef unsigned char u8; typedef short s16; typedef int s32;
__declspec(dllimport) void* __stdcall GetModuleHandleA(const char*);
__declspec(dllimport) void* __stdcall GetProcAddress(void*, const char*);
__declspec(dllimport) void  __stdcall Sleep(u32);
__declspec(dllimport) void* __stdcall CreateThread(void*, u32, u32 (__stdcall*)(void*), void*, u32, u32*);
__declspec(dllimport) int   __stdcall DisableThreadLibraryCalls(void*);
__declspec(dllimport) int   __stdcall VirtualProtect(void*, u32, u32, u32*);

#define SPRINT_PAD  ((volatile u32*)0x0234E80C)
#define CROUCH_PAD  ((volatile u32*)0x0234E7F4)
#define PLAYER_STATUS ((volatile u8*)0x00E9A5BC)
#define MENU_STATUS   ((volatile u8*)0x00EBE860)
#define BTN_LS 8u
#define UNBOUND 0xFFFFFFFFu
#define SDL_BTN_LEFT_SHOULDER 9
#define IAT_RVA 0x1102C8u

typedef u32* (__cdecl *fnGetGamepads)(int*);
typedef void* (__cdecl *fnFromID)(u32);
typedef s16 (__cdecl *fnAxis)(void*, int);
typedef u8 (__cdecl *fnButton)(void*, int);
typedef void (__cdecl *fnFree)(void*);

static fnAxis realAxis; static fnButton realButton;

static int onFootWheel(void* pad) {
    if (*MENU_STATUS != 2) return 0;
    u8 st = *PLAYER_STATUS;
    if (st == 3 || st == 5 || st == 6 || st == 8 || st == 22) return 0; /* vehicle, boat, heli, plane, shield */
    return realButton(pad, SDL_BTN_LEFT_SHOULDER) ? 1 : 0;
}

static s16 __cdecl MyAxis(void* pad, int axis) {
    if (axis >= 0 && axis <= 3 && onFootWheel(pad)) {
        if (axis == 0) return realAxis(pad, 2);
        if (axis == 1) return realAxis(pad, 3);
        return 0; /* axis 2/3: right stick muted while the wheel is open */
    }
    return realAxis(pad, axis);
}

static u32 __stdcall Worker(void* p) {
    (void)p;
    void *sdl = 0, *df = 0;
    while (!(sdl = GetModuleHandleA("SDL3.dll")) || !(df = GetModuleHandleA("DFEngine.dll"))) Sleep(500);
    fnGetGamepads getPads = (fnGetGamepads)GetProcAddress(sdl, "SDL_GetGamepads");
    fnFromID fromID = (fnFromID)GetProcAddress(sdl, "SDL_GetGamepadFromID");
    fnAxis axis = (fnAxis)GetProcAddress(sdl, "SDL_GetGamepadAxis");
    realButton = (fnButton)GetProcAddress(sdl, "SDL_GetGamepadButton");
    fnFree sfree = (fnFree)GetProcAddress(sdl, "SDL_free");
    if (!getPads || !fromID || !axis || !realButton || !sfree) return 0;
    realAxis = axis;

    /* hook: replace DFEngine's import slot only if it still points at the real SDL function */
    Sleep(1500);
    void** slot = (void**)((u8*)df + IAT_RVA);
    if (*slot == (void*)axis) {
        u32 old;
        if (VirtualProtect(slot, 4, 0x04 /*PAGE_READWRITE*/, &old)) { *slot = (void*)MyAxis; VirtualProtect(slot, 4, old, &old); }
    }

    void* pad = 0; u32 tick = 0; int sprintOn = 0;
    u32 lastS = 0x12345678, lastC = 0x12345678;
    for (;;) {
        Sleep(4);
        if (!pad || (tick++ % 250) == 0) {
            int n = 0; u32* ids = getPads(&n);
            void* found = 0;
            if (ids) { for (int i = 0; i < n && !found; i++) found = fromID(ids[i]); sfree(ids); }
            pad = found;
        }
        if (!pad) continue;
        s32 lx = axis(pad, 0), ly = axis(pad, 1);
        s32 ax = lx < 0 ? -lx : lx, ay = ly < 0 ? -ly : ly;
        if (!sprintOn && ly < -18000) sprintOn = 1;
        else if (sprintOn && ly > -11500) sprintOn = 0;
        int centered = (ax < 9800 && ay < 9800);
        u32 s = sprintOn ? BTN_LS : UNBOUND;
        u32 c = centered ? BTN_LS : UNBOUND;
        if (s != lastS) { *SPRINT_PAD = s; lastS = s; }
        if (c != lastC) { *CROUCH_PAD = c; lastC = c; }
    }
}

int __stdcall DllMain(void* h, u32 reason, void* r) {
    (void)r;
    if (reason == 1) { DisableThreadLibraryCalls(h); CreateThread(0, 0, Worker, 0, 0, 0); }
    return 1;
}
