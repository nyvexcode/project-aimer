// AIMER v1 - Roblox external (Aimbot, Silent, Magic Bullet, Hitbox Expander, ESP, World) + login por key/HWID + Inject
// C++17 | ImGui (win32 + dx11) | ReadProcessMemory | overlay separado
#include "license.h"
#include <dwmapi.h>
#include <d3d11.h>
#include <tlhelp32.h>
#include <mmsystem.h>
#include <cstdlib>
#include <cwchar>
#include <cfloat>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "winmm.lib")

#define APP_NAME  "AIMER"
#define APP_NAMEW L"AIMER"
#define APP_VER   "v1.0"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

struct Vec2 { float x = 0, y = 0; };
struct Vec3 { float x = 0, y = 0, z = 0; };
struct Mat4 { float m[16]{}; };

// ============================================================ offsets
struct Offsets {
    // obrigatorios (vem do offsets.ini)
    uintptr_t fdmPtr = 0, fdmReal = 0, vePtr = 0, viewMatrix = 0;
    uintptr_t nameBox = 0, childStart = 0, childEnd = 0, strLen = 0;
    uintptr_t localPlayer = 0, charModel = 0, team = 0;
    uintptr_t health = 0, maxHealth = 0;
    uintptr_t primitive = 0, primPos = 0, primVel = 0;
    // opcionais (padrao da versao version-9ac8595712b447a7; o offsets.ini pode sobrescrever)
    uintptr_t dmWorkspace = 328, primSize = 452, partTransp = 280, primFlags = 446, canQuery = 32, canCollide = 8;
    uintptr_t classDesc = 24, className = 8, primaryPart = 576, teamColor = 936;
    uintptr_t humWalkspeed = 452, humJumpPower = 408, humUseJumpPower = 469, humJump = 463;
    uintptr_t wsCamera = 1184, camFov = 296;
    uintptr_t litBrightness = 256, litAmbient = 184, litOutAmbient = 232, litClockTime = 176, litSky = 432;
    uintptr_t skyUp = 464, skyDn = 272, skyLf = 368, skyRt = 416, skyBk = 224, skyFt = 320;
    std::string version;
} O;

static std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// aceita "Chave=valor" ou "Chave = valor   [api]" (formato do dump). decimal ou 0xHEX.
static bool LoadOffsets(const char* path, std::string& missing) {
    std::ifstream f(path);
    if (!f) { missing = "offsets.ini nao encontrado"; return false; }
    struct Entry { const char* key; uintptr_t* dst; bool found; bool opt; };
    Entry tbl[] = {
        { "FakeDataModel.Pointer",       &O.fdmPtr,      false, false },
        { "FakeDataModel.RealDataModel", &O.fdmReal,     false, false },
        { "VisualEngine.Pointer",        &O.vePtr,       false, false },
        { "VisualEngine.ViewMatrix",     &O.viewMatrix,  false, false },
        { "Instance.NameContainer",      &O.nameBox,     false, false },
        { "Instance.ChildrenStart",      &O.childStart,  false, false },
        { "Instance.ChildrenEnd",        &O.childEnd,    false, false },
        { "Misc.StringLength",           &O.strLen,      false, false },
        { "Player.LocalPlayer",          &O.localPlayer, false, false },
        { "Player.ModelInstance",        &O.charModel,   false, false },
        { "Player.Team",                 &O.team,        false, false },
        { "Humanoid.Health",             &O.health,      false, false },
        { "Humanoid.MaxHealth",          &O.maxHealth,   false, false },
        { "BasePart.Primitive",          &O.primitive,   false, false },
        { "Primitive.Position",          &O.primPos,     false, false },
        { "Primitive.AssemblyLinearVelocity", &O.primVel, false, false },
        { "DataModel.Workspace",         &O.dmWorkspace, false, true },
        { "Primitive.Size",              &O.primSize,    false, true },
        { "BasePart.Transparency",       &O.partTransp,  false, true },
        { "Primitive.Flags",             &O.primFlags,   false, true },
        { "PrimitiveFlags.CanQuery",     &O.canQuery,    false, true },
        { "Instance.ClassDescriptor",    &O.classDesc,   false, true },
        { "Instance.ClassName",          &O.className,   false, true },
        { "Model.PrimaryPart",           &O.primaryPart, false, true },
        { "Player.TeamColor",            &O.teamColor,   false, true },
        { "PrimitiveFlags.CanCollide",   &O.canCollide,  false, true },
        { "Humanoid.Walkspeed",          &O.humWalkspeed, false, true },
        { "Humanoid.JumpPower",          &O.humJumpPower, false, true },
        { "Humanoid.UseJumpPower",       &O.humUseJumpPower, false, true },
        { "Humanoid.Jump",               &O.humJump,     false, true },
        { "Workspace.CurrentCamera",     &O.wsCamera,    false, true },
        { "Camera.FieldOfView",          &O.camFov,      false, true },
        { "Lighting.Brightness",         &O.litBrightness, false, true },
        { "Lighting.Ambient",            &O.litAmbient,  false, true },
        { "Lighting.OutdoorAmbient",     &O.litOutAmbient, false, true },
        { "Lighting.ClockTime",          &O.litClockTime, false, true },
        { "Lighting.Sky",                &O.litSky,      false, true },
        { "Sky.SkyboxUp",                &O.skyUp,       false, true },
        { "Sky.SkyboxDn",                &O.skyDn,       false, true },
        { "Sky.SkyboxLf",                &O.skyLf,       false, true },
        { "Sky.SkyboxRt",                &O.skyRt,       false, true },
        { "Sky.SkyboxBk",                &O.skyBk,       false, true },
        { "Sky.SkyboxFt",                &O.skyFt,       false, true },
    };
    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = Trim(line.substr(0, eq)), v = Trim(line.substr(eq + 1));
        if (k == "Version") { O.version = v; continue; }
        size_t sp = v.find_first_of(" \t");
        if (sp != std::string::npos) v = v.substr(0, sp);
        if (v.empty()) continue;
        for (auto& e : tbl) {
            if (k != e.key) continue;
            char* end = nullptr;
            unsigned long long n = std::strtoull(v.c_str(), &end, 0);
            if (end != v.c_str()) { *e.dst = (uintptr_t)n; e.found = true; }
        }
    }
    for (auto& e : tbl) if (!e.found && !e.opt) { missing += e.key; missing += "\n"; }
    return missing.empty();
}

// ============================================================ memory
static std::atomic<uint64_t> gReads{ 0 };

struct Memory {
    HANDLE h = nullptr;      // leitura
    HANDLE hw = nullptr;     // escrita (so abre quando Silent / Magic Bullet sao ligados)
    std::mutex wm;
    DWORD pid = 0;
    uintptr_t base = 0;

    // "Inject": anexa ao Roblox (external: nao injeta DLL, so abre o processo para leitura)
    bool Attach(const wchar_t* exe) {
        if (h) { CloseHandle(h); h = nullptr; }
        pid = 0;
        base = 0;
        std::vector<DWORD> pids;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return false;
        PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
        for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe))
            if (!_wcsicmp(pe.szExeFile, exe)) pids.push_back(pe.th32ProcessID);
        CloseHandle(snap);
        if (pids.empty()) return false;
        // varios clientes abertos (ex.: trocou de jogo): usa o mais novo
        ULONGLONG bestT = 0;
        for (DWORD p : pids) {
            HANDLE ph = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, p);
            if (!ph) { if (!pid) pid = p; continue; }
            FILETIME c, e, k, u;
            if (GetProcessTimes(ph, &c, &e, &k, &u)) {
                ULONGLONG t = ((ULONGLONG)c.dwHighDateTime << 32) | c.dwLowDateTime;
                if (t >= bestT) { bestT = t; pid = p; }
            } else if (!pid) pid = p;
            CloseHandle(ph);
        }
        if (!pid) return false;
        HANDLE ms = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (ms == INVALID_HANDLE_VALUE) return false;
        MODULEENTRY32W me{}; me.dwSize = sizeof(me);
        for (BOOL ok = Module32FirstW(ms, &me); ok; ok = Module32NextW(ms, &me))
            if (!_wcsicmp(me.szModule, exe)) { base = (uintptr_t)me.modBaseAddr; break; }
        CloseHandle(ms);
        h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        return h && base;
    }
    void Detach() {
        if (h) { CloseHandle(h); h = nullptr; }
        {
            std::lock_guard<std::mutex> g(wm);
            if (hw) { CloseHandle(hw); hw = nullptr; }
        }
        pid = 0;
        base = 0;
    }
    bool EnsureWrite() {
        std::lock_guard<std::mutex> g(wm);
        if (hw) return true;
        hw = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        return hw != nullptr;
    }
    bool Alive() const {
        DWORD code = 0;
        return h && GetExitCodeProcess(h, &code) && code == STILL_ACTIVE;
    }
    bool Raw(uintptr_t a, void* out, size_t n) const {
        if (a < 0x10000) return false;
        gReads.fetch_add(1, std::memory_order_relaxed);
        SIZE_T got = 0;
        return ReadProcessMemory(h, (LPCVOID)a, out, n, &got) && got == n;
    }
    bool Wr(uintptr_t a, const void* src, size_t n) {
        if (!hw || a < 0x10000) return false;
        SIZE_T put = 0;
        return WriteProcessMemory(hw, (LPVOID)a, src, n, &put) && put == n;
    }
    template <class T> T Rd(uintptr_t a) const {
        T v{};
        Raw(a, &v, sizeof(T));
        return v;
    }
    std::string Str(uintptr_t a) const {
        if (a < 0x10000) return {};
        uint64_t len = Rd<uint64_t>(a + O.strLen);
        if (!len || len > 96) return {};
        uintptr_t src = len >= 16 ? Rd<uintptr_t>(a) : a;
        std::string s(len, '\0');
        if (!Raw(src, s.data(), len)) return {};
        return s;
    }
} M;

// ============================================================ instance helpers
using NameMapT = std::unordered_map<std::string, uintptr_t>;

static std::string InstName(uintptr_t i) { return M.Str(M.Rd<uintptr_t>(i + O.nameBox)); }

static std::vector<uintptr_t> Kids(uintptr_t i) {
    std::vector<uintptr_t> out;
    uintptr_t vec = M.Rd<uintptr_t>(i + O.childStart);
    if (!vec) return out;
    uintptr_t b = M.Rd<uintptr_t>(vec), e = M.Rd<uintptr_t>(vec + O.childEnd);
    if (!b || e <= b || (e - b) > 0x80000) return out;
    std::vector<uintptr_t> raw((e - b) / sizeof(uintptr_t));
    if (!M.Raw(b, raw.data(), raw.size() * sizeof(uintptr_t))) return out;
    for (size_t k = 0; k < raw.size(); k += 2) if (raw[k]) out.push_back(raw[k]); // shared_ptr = 16 bytes
    return out;
}
static uintptr_t Child(uintptr_t i, const char* name) {
    for (auto c : Kids(i)) if (InstName(c) == name) return c;
    return 0;
}
static NameMapT NameMap(uintptr_t i) {
    NameMapT m;
    for (auto c : Kids(i)) {
        std::string n = InstName(c);
        if (!n.empty() && !m.count(n)) m[n] = c;
    }
    return m;
}
static uintptr_t Get(const NameMapT& m, const char* k) {
    auto it = m.find(k);
    return it == m.end() ? 0 : it->second;
}
static bool Plausible(const Vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) &&
           std::fabs(v.x) < 1.0e7f && std::fabs(v.y) < 1.0e7f && std::fabs(v.z) < 1.0e7f;
}
static bool Finite(const Vec3& v) { return Plausible(v); }
static bool ReadPos(uintptr_t prim, Vec3& pos) {
    if (!prim) return false;
    pos = M.Rd<Vec3>(prim + O.primPos);
    return Plausible(pos);
}
// posicao + velocidade numa unica leitura quando os offsets sao vizinhos
static bool ReadPV(uintptr_t prim, Vec3& pos, Vec3& vel) {
    if (!prim) return false;
    if (O.primVel == O.primPos + 12) {
        struct PV { Vec3 p, v; };
        PV b{};
        if (!M.Raw(prim + O.primPos, &b, sizeof(b))) return false;
        pos = b.p; vel = b.v;
    } else {
        pos = M.Rd<Vec3>(prim + O.primPos);
        vel = M.Rd<Vec3>(prim + O.primVel);
    }
    return Plausible(pos);
}
static std::string ClassOf(uintptr_t inst, std::unordered_map<uintptr_t, std::string>& cache) {
    uintptr_t cd = M.Rd<uintptr_t>(inst + O.classDesc);
    if (!cd) return {};
    auto it = cache.find(cd);
    if (it != cache.end()) return it->second;
    std::string n = M.Str(M.Rd<uintptr_t>(cd + O.className));
    if (!n.empty()) cache[cd] = n;
    return n;
}

// ============================================================ config
// ============================================================ config
struct AimCfg {
    // aimbot
    bool enabled = true, drawFov = true, teamCheck = true, predict = false, sticky = false;
    int key = VK_RBUTTON, mode = 0, part = 0, priority = 0;   // mode 0 hold/1 toggle ; part 0 head/1 torso/2 nearest ; priority 0 mira/1 distancia/2 vida
    float smooth = 6.f, maxSpeed = 0.f, fovX = 150.f, fovY = 150.f, deadzone = 0.f;
    float maxDist = 800.f, predictAmt = 0.12f, jitter = 0.f;
    float fovCol[4] = { 0.80f, 0.80f, 0.80f, 1.f };
    // triggerbot (key 0 = sempre ligado)
    bool trigEnabled = false;
    int trigKey = 0;
    float trigDelay = 60.f, trigScale = 80.f;
    // silent (hitbox) (key 0 = sempre ligado)
    bool silent = false, silentClick = false, silentHide = true;
    int silentKey = 0, silentPart = 0;    // part: 0 head, 1 root, 2 both
    float silentSize = 6.f, silentFovX = 200.f, silentFovY = 200.f;
};
static AimCfg PresetLegit() {
    AimCfg a;
    a.smooth = 14.f; a.maxSpeed = 14.f; a.fovX = a.fovY = 90.f; a.part = 2; a.deadzone = 2.f;
    a.jitter = 1.2f; a.maxDist = 600.f; a.silentSize = 4.f; a.silentFovX = a.silentFovY = 90.f;
    return a;
}
static AimCfg PresetRage() {
    AimCfg a;
    a.smooth = 1.f; a.maxSpeed = 0.f; a.fovX = a.fovY = 450.f; a.part = 0;
    a.predict = true; a.predictAmt = 0.14f; a.sticky = true; a.maxDist = 2500.f;
    a.silentSize = 8.f; a.silentFovX = a.silentFovY = 350.f;
    return a;
}

struct EspCfg {
    bool enabled = true, showTeam = false, box = true, name = true, health = true, distance = true, tracer = false, headDot = false;
    int key = 0, boxStyle = 0, tracerOrigin = 0;   // boxStyle 0 full / 1 corners ; origin 0 bottom/1 center/2 top
    float maxDist = 1500.f, textSize = 14.f;
    float boxCol[4]    = { 0.92f, 0.92f, 0.92f, 1.f };
    float nameCol[4]   = { 0.96f, 0.96f, 0.96f, 1.f };
    float tracerCol[4] = { 0.65f, 0.65f, 0.65f, 1.f };
    float teamCol[4]   = { 0.45f, 0.70f, 0.95f, 1.f };
    float friendCol[4] = { 0.40f, 0.90f, 0.50f, 1.f };
    float targetCol[4] = { 1.00f, 0.35f, 0.35f, 1.f };
};

struct MiscCfg {
    // hitbox expander (persistente, diferente do Silent: nao depende de FOV nem de atirar)
    bool hbEnabled = false, hbTeamCheck = true, hbHide = false;
    int hbKey = 0, hbPart = 2;     // part: 0 head, 1 root, 2 both
    float hbSize = 6.f, hbMaxDist = 1500.f;
    float hbCol[4] = { 1.f, 0.25f, 0.55f, 1.f };
    // movimento (jogador local)
    bool speedEnabled = false; float speed = 50.f; int speedKey = 0;
    bool jumpEnabled = false; float jumpPower = 120.f; int jumpKey = 0;
    bool infJump = false; int infJumpKey = 0;
    bool noclip = false; int noclipKey = 0;
    // mundo / camera
    bool fovEnabled = false; float fov = 90.f;
    bool fullbright = false; float brightness = 3.f;
    bool clockEnabled = false; float clockTime = 14.f;
    bool antiAfk = false;
    // ceu
    bool blackSky = false, skyBranding = true;
};

struct Config {
    int magic = 0x41494D08;   // "AIM" + versao
    EspCfg esp;
    MiscCfg misc;
    // ---- desempenho
    float espRate = 90.f, maxTrack = 40.f, headReads = 10.f, overlayFps = 60.f;
    // ---- aim profiles (0 = legit, 1 = rage)
    int profile = 0;
    AimCfg prof[2] = { PresetLegit(), PresetRage() };
    // ---- magic bullet
    bool mbEnabled = false, mbFloors = false;
    int mbKey = 0;
    float mbRange = 250.f;
    // ---- misc (so overlay: seguros)
    bool crossEnabled = false, keybindList = false, watermark = true;
    int crossStyle = 0;                 // 0 cruz, 1 ponto, 2 cruz + ponto
    float crossSize = 9.f, crossGap = 4.f, crossThick = 1.5f;
    float crossCol[4] = { 1.f, 1.f, 1.f, 1.f };
    // ---- geral
    int menuKey = VK_INSERT, profileKey = 0, panicKey = 0;
    int teamMode = 0;                   // 0 auto (Team), 1 TeamColor, 2 nenhum (free-for-all)
    bool autoInject = true, autoSave = true;
    float accent[4] = { 1.f, 0.20f, 0.55f, 1.f };
} C;

static AimCfg& A() { return C.prof[C.profile & 1]; }

static const char* CFG_FILE = "aimer.cfg";
static const char* KEY_FILE = "aimer.key";
static const char* TAG_FILE = "aimer_tags.txt";
static void SaveCfg() { std::ofstream f(CFG_FILE, std::ios::binary); f.write((const char*)&C, sizeof(C)); }
static void LoadCfg() {
    std::ifstream f(CFG_FILE, std::ios::binary | std::ios::ate);
    if (!f || (size_t)f.tellg() != sizeof(Config)) return;
    f.seekg(0);
    Config t;
    f.read((char*)&t, sizeof(t));
    if (t.magic == Config{}.magic) C = t;
}

// ---- marcacoes de jogadores (por nome): 1 = amigo (ignorado pelo aim), 2 = alvo prioritario
static std::mutex gTagLock;
static std::unordered_map<std::string, int> gTags;
static int TagOf(const char* name) {
    std::lock_guard<std::mutex> g(gTagLock);
    auto it = gTags.find(name);
    return it == gTags.end() ? 0 : it->second;
}
static void SaveTags() {
    std::lock_guard<std::mutex> g(gTagLock);
    std::ofstream f(TAG_FILE);
    for (auto& kv : gTags) f << kv.first << "=" << kv.second << "\n";
}
static void SetTag(const std::string& n, int t) {
    {
        std::lock_guard<std::mutex> g(gTagLock);
        if (t == 0) gTags.erase(n); else gTags[n] = t;
    }
    SaveTags();
}
static void ClearTags() {
    { std::lock_guard<std::mutex> g(gTagLock); gTags.clear(); }
    SaveTags();
}
static void LoadTags() {
    std::ifstream f(TAG_FILE);
    std::string line;
    std::lock_guard<std::mutex> g(gTagLock);
    while (std::getline(f, line)) {
        auto eq = line.rfind('=');
        if (eq == std::string::npos || eq == 0) continue;
        int t = std::atoi(line.c_str() + eq + 1);
        if (t == 1 || t == 2) gTags[line.substr(0, eq)] = t;
    }
}


// ============================================================ math
static bool W2S(const Mat4& v, const Vec3& p, float w, float h, Vec2& out) {
    float cx = v.m[0] * p.x + v.m[1] * p.y + v.m[2] * p.z + v.m[3];
    float cy = v.m[4] * p.x + v.m[5] * p.y + v.m[6] * p.z + v.m[7];
    float cw = v.m[12] * p.x + v.m[13] * p.y + v.m[14] * p.z + v.m[15];
    if (cw < 0.1f) return false;
    out.x = (w / 2.f) * (1.f + cx / cw);
    out.y = (h / 2.f) * (1.f - cy / cw);
    return true;
}
static float Dist(const Vec3& a, const Vec3& b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// ============================================================ shared state
struct Ent {
    uintptr_t id = 0;
    char name[32]{};
    Vec3 head, root, vel;
    float hp = 0, maxHp = 100, scale = 1.f;
    bool sameTeam = false, hasHp = true;
    int tag = 0;   // 0 none, 1 friend, 2 target
};
static std::mutex gLock;
static std::shared_ptr<const std::vector<Ent>> gSnap;
static Mat4 gMat;
static Vec3 gLocalPos;
static std::mutex gCharLock;
static std::unordered_set<uintptr_t> gChars;
static std::atomic<bool> gRun{ true };
static std::atomic<bool> gWork{ true };      // threads de trabalho (reiniciam ao reinjetar)
static std::mutex gNamesLock;
static std::vector<std::string> gNames;   // ultimos jogadores vistos (para marcar amigo/alvo)
static std::atomic<bool> gLicensedFlag{ false };
static std::atomic<float> gSW{ 1920.f }, gSH{ 1080.f };
static std::atomic<bool> gGameActive{ false };
static std::atomic<int> gStWrite{ 0 }, gStHb{ 0 }, gStMb{ 0 }, gTracked{ 0 }, gScanHz{ 0 }, gPlayers{ 0 };
static std::atomic<float> gScanMs{ 0.f };
static std::atomic<uint64_t> gReadsPerSec{ 0 };

// ============================================================ silent (hitbox expand)
struct HbPart { uintptr_t prim = 0; Vec3 orig; float tr = 0.f; bool on = false; };
struct Hb { uintptr_t ch = 0; HbPart head, root; };

static void HbSet(uintptr_t part, HbPart& s, bool want, float size, bool hide) {
    if (!part) return;
    if (want) {
        if (!M.hw) return;
        if (!s.on) {
            uintptr_t p = M.Rd<uintptr_t>(part + O.primitive);
            if (!p) return;
            Vec3 sz = M.Rd<Vec3>(p + O.primSize);
            if (!Plausible(sz) || sz.x <= 0.f || sz.y <= 0.f || sz.z <= 0.f || sz.x > 60.f) return;
            s.prim = p;
            s.orig = sz;
            s.tr = M.Rd<float>(part + O.partTransp);
            s.on = true;
        } else if (M.Rd<uintptr_t>(part + O.primitive) != s.prim) {
            s.on = false;      // a parte mudou/foi destruida: nao escreve em ponteiro velho
            return;
        }
        Vec3 ns{ size, size, size };
        Vec3 cur = M.Rd<Vec3>(s.prim + O.primSize);
        if (cur.x != ns.x || cur.y != ns.y || cur.z != ns.z) M.Wr(s.prim + O.primSize, &ns, sizeof(ns));
        if (hide && M.Rd<float>(part + O.partTransp) < 0.99f) {
            float one = 1.f;
            M.Wr(part + O.partTransp, &one, sizeof(one));
        }
    } else if (s.on) {
        if (M.Rd<uintptr_t>(part + O.primitive) == s.prim) {
            M.Wr(s.prim + O.primSize, &s.orig, sizeof(s.orig));
            M.Wr(part + O.partTransp, &s.tr, sizeof(s.tr));
        }
        s.on = false;
    }
}

// ============================================================ scan thread (otimizada)
// - lista de jogadores so a cada 500 ms
// - hp/time/personagem so a cada 250 ms; ponteiros das partes a cada 1 s
// - por tick: 1 leitura por jogador proximo (posicao + velocidade juntas)
// - cabeca exata so para os N mais proximos (resto estimado a partir do root)
// - jogadores longe / alem do limite: so a cada 500 ms (e sem desenhar)
// - no maximo 2 reconstrucoes de personagem por tick (evita picos quando muita gente entra)
// - o intervalo se ajusta sozinho ao tempo gasto (nunca passa de ~40% de um nucleo)
struct PlCache {
    uintptr_t pl = 0, ch = 0, hum = 0, head = 0, root = 0, pHead = 0, pRoot = 0;
    char name[32]{};
    float hp = 100, maxHp = 100, scale = 1.f;
    bool sameTeam = false, alive = false, have = false;
    int tag = 0;
    Vec3 root3, vel3, head3;
    float dist = 1.0e9f;
    ULONGLONG nextSlow = 0, nextPrim = 0, nextFar = 0, nextScale = 0;
};

// suporte universal: personagens sem "HumanoidRootPart" (rigs customizados) usam outra parte / PrimaryPart
static uintptr_t FindRootPart(const NameMapT& m, uintptr_t ch) {
    static const char* names[] = { "HumanoidRootPart", "Torso", "UpperTorso", "LowerTorso", "Head", nullptr };
    for (int i = 0; names[i]; i++) {
        uintptr_t p = Get(m, names[i]);
        if (p) return p;
    }
    return M.Rd<uintptr_t>(ch + O.primaryPart);
}

static void ScanLoop() {
    std::unordered_map<uintptr_t, PlCache> cache;
    std::unordered_map<uintptr_t, Hb> hb;
    std::vector<uintptr_t> plist;
    uintptr_t lastDm = 0, lastFdm = 0, players = 0;
    uintptr_t local = 0, localTeam = 0, localTeamColor = 0, lch = 0, lcCh = 0, lcRoot = 0, lcPrim = 0;
    ULONGLONG nextList = 0, nextLocal = 0, nextChars = 0, nextStat = 0, nextPlayersChk = 0;
    uint64_t lastReads = 0;
    int loops = 0;
    float emaMs = 0.f;
    LARGE_INTEGER qf;
    QueryPerformanceFrequency(&qf);
    Vec3 lpos{};

    // troca de jogo / teleporte: esquece tudo (sem escrever em ponteiros velhos)
    auto resetState = [&]() {
        cache.clear(); hb.clear(); plist.clear();
        local = localTeam = localTeamColor = lch = lcCh = lcRoot = lcPrim = 0;
        nextList = nextLocal = 0;
    };
    auto clearSnap = [&]() {
        std::lock_guard<std::mutex> g(gLock);
        gSnap.reset();
        gTracked = 0;
        gPlayers = 0;
        gStHb = 0;
    };

    while (gRun && gWork) {
        LARGE_INTEGER t0;
        QueryPerformanceCounter(&t0);
        ULONGLONG now = GetTickCount64();

        uintptr_t fdm = M.Rd<uintptr_t>(M.base + O.fdmPtr);
        uintptr_t dm = M.Rd<uintptr_t>(fdm + O.fdmReal);
        uintptr_t ve = M.Rd<uintptr_t>(M.base + O.vePtr);
        if (!fdm || !dm || !ve) {
            if (players) { resetState(); players = 0; clearSnap(); }
            Sleep(400);
            continue;
        }
        if (dm != lastDm || fdm != lastFdm) {      // novo DataModel (outro jogo)
            resetState();
            players = 0;
            lastDm = dm;
            lastFdm = fdm;
            nextPlayersChk = 0;
            clearSnap();
        }
        // o servico Players pode ser recriado mesmo com o mesmo endereco do DataModel: revalida a cada 1 s
        if (!players || now >= nextPlayersChk) {
            nextPlayersChk = now + 1000;
            uintptr_t np = Child(dm, "Players");
            if (np != players) { resetState(); players = np; clearSnap(); }
        }
        if (!players) { Sleep(300); continue; }

        Mat4 mat = M.Rd<Mat4>(ve + O.viewMatrix);

        // ---- jogador local (a cada 250 ms; a posicao a cada tick: 1 leitura)
        if (now >= nextLocal) {
            nextLocal = now + 250;
            local = M.Rd<uintptr_t>(players + O.localPlayer);
            localTeam = local ? M.Rd<uintptr_t>(local + O.team) : 0;
            localTeamColor = local ? M.Rd<uint32_t>(local + O.teamColor) : 0;
            lch = local ? M.Rd<uintptr_t>(local + O.charModel) : 0;
            if (lch != lcCh) {
                lcCh = lch;
                lcRoot = 0;
                if (lch) { NameMapT lm = NameMap(lch); lcRoot = FindRootPart(lm, lch); }
            }
            lcPrim = lcRoot ? M.Rd<uintptr_t>(lcRoot + O.primitive) : 0;
        }
        if (lcPrim) ReadPos(lcPrim, lpos);

        // ---- lista de jogadores (a cada 500 ms)
        if (now >= nextList) {
            nextList = now + 500;
            plist = Kids(players);
            std::unordered_set<uintptr_t> live(plist.begin(), plist.end());
            for (auto it = cache.begin(); it != cache.end();) {
                if (!live.count(it->first)) { hb.erase(it->first); it = cache.erase(it); } else ++it;
            }
        }

        const AimCfg& a = A();
        const bool silentKeyOk = !a.silentKey || (GetAsyncKeyState(a.silentKey) & 0x8000);
        const bool silentOn = a.silent && silentKeyOk;
        if (a.silent || C.misc.hbEnabled) M.EnsureWrite();
        const bool hbKeyOk = !C.misc.hbKey || (GetAsyncKeyState(C.misc.hbKey) & 0x8000);
        const bool hbOn = C.misc.hbEnabled && hbKeyOk;
        const float sw = gSW.load(), sh = gSH.load();
        const bool gameActive = gGameActive.load();
        const bool shooting = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        const int teamMode = C.teamMode;

        // alcance que realmente precisamos (tudo desligado = nada a ler de perto)
        float need = 0.f;
        if (C.esp.enabled) need = std::max(need, C.esp.maxDist);
        if (a.enabled || a.trigEnabled || a.silent) need = std::max(need, a.maxDist);
        const float cull = need > 0.f ? need + 100.f : -1.f;
        const bool headNeeded = a.enabled || a.silent;
        const int maxTrack = (int)std::clamp(C.maxTrack, 4.f, 120.f);
        const int headN = (int)std::clamp(C.headReads, 0.f, 40.f);

        // ordena pela ultima distancia conhecida
        std::vector<PlCache*> order;
        order.reserve(plist.size());
        for (auto pl : plist) {
            if (pl == local) continue;
            PlCache& c = cache[pl];
            c.pl = pl;
            order.push_back(&c);
        }
        std::sort(order.begin(), order.end(), [](const PlCache* x, const PlCache* y) { return x->dist < y->dist; });
        gPlayers = (int)order.size();

        std::vector<Ent> list;
        list.reserve(48);
        std::vector<uintptr_t> chars;
        if (lch) chars.push_back(lch);
        int rebuilds = 0, expanded = 0;

        for (size_t idx = 0; idx < order.size(); idx++) {
            PlCache& c = *order[idx];
            const bool nearSet = cull > 0.f && (int)idx < maxTrack && c.dist <= cull;
            if (c.ch) chars.push_back(c.ch);

            if (!nearSet) {
                // longe / fora do limite: devolve hitbox (se estava expandido) e so atualiza a cada 500 ms
                auto hit = hb.find(c.pl);
                if (hit != hb.end() && (hit->second.head.on || hit->second.root.on)) {
                    HbSet(c.head, hit->second.head, false, 0.f, false);
                    HbSet(c.root, hit->second.root, false, 0.f, false);
                }
                if (now < c.nextFar) continue;
                c.nextFar = now + 500;
            }

            // ---- atualizacao lenta (250 ms): personagem, ponteiros, vida, time
            if (now >= c.nextSlow || !c.ch) {
                c.nextSlow = now + 250;
                uintptr_t ch = M.Rd<uintptr_t>(c.pl + O.charModel);
                if (!ch) { c.ch = 0; c.alive = false; c.have = false; continue; }
                if (ch != c.ch || !c.root) {
                    if (rebuilds >= 2) { c.nextSlow = 0; continue; }     // tenta de novo no proximo tick
                    rebuilds++;
                    NameMapT m = NameMap(ch);
                    c.ch = ch;
                    c.hum = Get(m, "Humanoid");              // pode nao existir (rigs customizados)
                    c.head = Get(m, "Head");
                    c.root = FindRootPart(m, ch);
                    c.pHead = c.pRoot = 0;
                    c.nextPrim = 0;
                    c.have = false;
                    c.scale = 1.f;
                    if (!c.name[0]) {
                        std::string n = InstName(c.pl);
                        strncpy_s(c.name, sizeof(c.name), n.c_str(), _TRUNCATE);
                    }
                    c.tag = TagOf(c.name);
                    chars.push_back(ch);
                }
                if (!c.root) { c.alive = false; continue; }
                if (now >= c.nextPrim || !c.pRoot) {
                    c.pHead = c.head ? M.Rd<uintptr_t>(c.head + O.primitive) : 0;
                    c.pRoot = M.Rd<uintptr_t>(c.root + O.primitive);
                    c.nextPrim = now + 1000;
                }
                // vida e vida maxima numa unica leitura (offsets vizinhos); sem Humanoid = vivo, vida 100
                if (c.hum) {
                    if (O.maxHealth > O.health && O.maxHealth - O.health <= 60) {
                        uint8_t buf[72]{};
                        size_t span = (size_t)(O.maxHealth - O.health) + 4;
                        if (M.Raw(c.hum + O.health, buf, span)) {
                            memcpy(&c.hp, buf, 4);
                            memcpy(&c.maxHp, buf + (O.maxHealth - O.health), 4);
                        }
                    } else {
                        c.hp = M.Rd<float>(c.hum + O.health);
                        c.maxHp = M.Rd<float>(c.hum + O.maxHealth);
                    }
                } else {
                    c.hp = 100.f;
                    c.maxHp = 100.f;
                }
                // deteccao de time: 0 = objeto Team, 1 = TeamColor, 2 = nenhum (free-for-all)
                if (teamMode == 2) c.sameTeam = false;
                else if (teamMode == 1) {
                    uint32_t tc = M.Rd<uint32_t>(c.pl + O.teamColor);
                    c.sameTeam = localTeamColor && tc == (uint32_t)localTeamColor;
                } else {
                    c.sameTeam = localTeam && M.Rd<uintptr_t>(c.pl + O.team) == localTeam;
                }
                c.alive = c.hp > 0.f && c.maxHp > 0.f;
            }
            if (!c.alive || !c.pRoot) continue;

            // ---- posicao (1 leitura)
            if (!ReadPV(c.pRoot, c.root3, c.vel3)) { c.have = false; continue; }
            c.dist = Dist(lpos, c.root3);
            c.have = true;
            if (!nearSet) continue;

            // ---- escala do personagem (rigs gigantes / pequenos): mede a cada 500 ms
            if (c.pHead && now >= c.nextScale) {
                c.nextScale = now + 500;
                Vec3 h3;
                if (ReadPos(c.pHead, h3)) {
                    float d = h3.y - c.root3.y;
                    if (d > 0.4f && d < 14.f) c.scale = c.scale * 0.7f + std::clamp(d / 1.5f, 0.25f, 6.f) * 0.3f;
                }
            }

            // ---- cabeca exata so para os mais proximos; o resto e estimado
            bool exact = headNeeded && (int)idx < headN && c.pHead && ReadPos(c.pHead, c.head3);
            if (!exact) { c.head3 = c.root3; c.head3.y += 1.5f * c.scale; }

            Ent e;
            e.id = c.pl;
            e.head = c.head3; e.root = c.root3; e.vel = c.vel3;
            e.hp = c.hp; e.maxHp = c.maxHp; e.sameTeam = c.sameTeam;
            e.scale = c.scale; e.hasHp = c.hum != 0;
            e.tag = c.tag;
            memcpy(e.name, c.name, sizeof(e.name));

            // ---- silent hitbox
            Hb& hs = hb[c.pl];
            if (hs.ch != c.ch) { hs = Hb{}; hs.ch = c.ch; }   // personagem novo: descarta o estado sem escrever

            // silent aim: so quando dentro do FOV (e, se marcado, atirando)
            bool wantSilent = false;
            if (silentOn && M.hw && gameActive && (!a.silentClick || shooting) &&
                !(a.teamCheck && e.sameTeam) && e.hp > 0.f && c.dist <= a.maxDist) {
                float fx = std::max(a.silentFovX, 1.f), fy = std::max(a.silentFovY, 1.f);
                const Vec3 pts[2] = { e.head, e.root };
                for (const Vec3& p : pts) {
                    Vec2 s;
                    if (!W2S(mat, p, sw, sh, s)) continue;
                    float dx = (s.x - sw * 0.5f) / fx, dy = (s.y - sh * 0.5f) / fy;
                    if (dx * dx + dy * dy <= 1.f) { wantSilent = true; break; }
                }
            }
            // hitbox expander: persistente, sem FOV, so distancia e time
            bool wantExp = hbOn && M.hw && !(C.misc.hbTeamCheck && e.sameTeam) && e.hp > 0.f && c.dist <= C.misc.hbMaxDist;
            bool wantExpHead = wantExp && (C.misc.hbPart == 0 || C.misc.hbPart == 2);
            bool wantExpRoot = wantExp && (C.misc.hbPart == 1 || C.misc.hbPart == 2);
            bool wantHead = wantExpHead || (wantSilent && (a.silentPart == 0 || a.silentPart == 2));
            bool wantRoot = wantExpRoot || (wantSilent && (a.silentPart == 1 || a.silentPart == 2));
            float sizeHead = wantExpHead ? C.misc.hbSize : a.silentSize;
            float sizeRoot = wantExpRoot ? C.misc.hbSize : a.silentSize;
            bool hideHead = wantExpHead ? C.misc.hbHide : a.silentHide;
            bool hideRoot = wantExpRoot ? C.misc.hbHide : a.silentHide;
            HbSet(c.head, hs.head, wantHead, sizeHead, hideHead);
            HbSet(c.root, hs.root, wantRoot, sizeRoot, hideRoot);
            if (hs.head.on || hs.root.on) expanded++;

            list.push_back(e);
        }
        gStHb = expanded;
        gTracked = (int)list.size();

        if (now >= nextChars - 500) {   // junto com a leitura de personagens: nomes para a lista de tags
            std::vector<std::string> nm;
            for (auto* c : order) if (c->name[0]) nm.push_back(c->name);
            std::lock_guard<std::mutex> g(gNamesLock);
            gNames = std::move(nm);
        }
        auto snap = std::make_shared<const std::vector<Ent>>(std::move(list));
        {
            std::lock_guard<std::mutex> g(gLock);
            gSnap = std::move(snap);
            gMat = mat;
            gLocalPos = lpos;
        }
        if (now >= nextChars) {
            nextChars = now + 1000;
            std::lock_guard<std::mutex> g(gCharLock);
            gChars.clear();
            gChars.insert(chars.begin(), chars.end());
        }

        // ---- estatisticas + intervalo adaptativo
        LARGE_INTEGER t1;
        QueryPerformanceCounter(&t1);
        float ms = (float)((double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)qf.QuadPart);
        emaMs = emaMs * 0.9f + ms * 0.1f;
        loops++;
        if (now >= nextStat) {
            nextStat = now + 1000;
            uint64_t r = gReads.load(std::memory_order_relaxed);
            gReadsPerSec = r - lastReads;
            lastReads = r;
            gScanHz = loops;
            gScanMs = emaMs;
            loops = 0;
        }
        const int rate = (int)std::clamp(C.espRate, 20.f, 240.f);
        int sleepMs = std::max(1, 1000 / rate);
        sleepMs = std::max(sleepMs, (int)std::ceil(ms * 1.5f));     // nunca passa de ~40% de um nucleo
        Sleep((DWORD)std::min(sleepMs, 60));
    }

    // restaura hitboxes ao sair
    for (auto& kv : hb) {
        auto it = cache.find(kv.first);
        if (it == cache.end() || it->second.ch != kv.second.ch) continue;
        HbSet(it->second.head, kv.second.head, false, 0.f, false);
        HbSet(it->second.root, kv.second.root, false, 0.f, false);
    }
}

// ============================================================ magic bullet (tiros atravessam paredes)
// Desliga o bit CanQuery das partes do mapa perto de voce: raycasts (tiros) ignoram essas partes.
static bool IsPartClass(const std::string& s) {
    return s == "Part" || s == "MeshPart" || s == "UnionOperation" || s == "WedgePart" ||
           s == "CornerWedgePart" || s == "TrussPart";
}

static void MbLoop() {
    std::unordered_map<uintptr_t, std::string> classCache;
    struct Ap { uintptr_t part, prim; };
    std::vector<Ap> applied;
    std::unordered_set<uintptr_t> appliedSet;
    uintptr_t lastDm = 0, lastWs = 0;
    ULONGLONG nextScan = 0;
    bool wasActive = false;
    const uint8_t mask = (uint8_t)O.canQuery;

    auto restoreAll = [&]() {
        for (auto& p : applied) {
            if (M.Rd<uintptr_t>(p.part + O.primitive) != p.prim) continue;
            uint8_t f = 0;
            if (!M.Raw(p.prim + O.primFlags, &f, 1)) continue;
            if (!(f & mask)) { f |= mask; M.Wr(p.prim + O.primFlags, &f, 1); }
        }
        applied.clear();
        appliedSet.clear();
        gStMb = 0;
    };

    while (gRun && gWork) {
        bool want = gLicensedFlag.load() && C.mbEnabled && (!C.mbKey || (GetAsyncKeyState(C.mbKey) & 0x8000));
        if (!want) {
            if (wasActive) { restoreAll(); wasActive = false; }
            Sleep(60);
            continue;
        }
        if (!M.EnsureWrite()) { gStWrite = 2; Sleep(300); continue; }
        gStWrite = 1;
        wasActive = true;

        uintptr_t fdm = M.Rd<uintptr_t>(M.base + O.fdmPtr);
        uintptr_t dm = M.Rd<uintptr_t>(fdm + O.fdmReal);
        if (!dm) { Sleep(200); continue; }
        if (dm != lastDm) { applied.clear(); appliedSet.clear(); lastDm = dm; nextScan = 0; }

        ULONGLONG now = GetTickCount64();
        if (now >= nextScan) {
            nextScan = now + 10000;
            uintptr_t ws = M.Rd<uintptr_t>(dm + O.dmWorkspace);
            if (ws != lastWs) { applied.clear(); appliedSet.clear(); lastWs = ws; }   // outro mapa: nao toca nas partes antigas
            Vec3 lp;
            { std::lock_guard<std::mutex> g(gLock); lp = gLocalPos; }
            std::unordered_set<uintptr_t> chars;
            { std::lock_guard<std::mutex> g(gCharLock); chars = gChars; }
            const float range = std::max(C.mbRange, 10.f);
            const bool floors = C.mbFloors;

            std::vector<uintptr_t> st;
            if (ws) st.push_back(ws);
            std::unordered_set<uintptr_t> seen;
            size_t budget = 25000, n = 0;
            while (!st.empty() && budget > 0 && gRun && gWork) {
                budget--;
                if ((++n & 127) == 0) Sleep(1);          // cede CPU: varredura em segundo plano
                uintptr_t i = st.back();
                st.pop_back();
                if (!i || chars.count(i) || !seen.insert(i).second) continue;
                std::string cn = ClassOf(i, classCache);
                std::vector<uintptr_t> kids = Kids(i);
                if (cn == "Model") {   // NPCs / personagens: nao mexe (continuam acertaveis)
                    bool hum = false;
                    for (auto k : kids) if (InstName(k) == "Humanoid") { hum = true; break; }
                    if (hum) continue;
                } else if (IsPartClass(cn) && !appliedSet.count(i)) {
                    uintptr_t prim = M.Rd<uintptr_t>(i + O.primitive);
                    if (prim) {
                        Vec3 sz = M.Rd<Vec3>(prim + O.primSize), pos = M.Rd<Vec3>(prim + O.primPos);
                        if (Plausible(sz) && Plausible(pos) && sz.x > 0.f && sz.y > 0.f && sz.z > 0.f &&
                            sz.x < 4096.f && sz.y < 4096.f && sz.z < 4096.f) {
                            float mx = std::max(sz.x, std::max(sz.y, sz.z));
                            bool isFloor = pos.y < lp.y - 2.5f;
                            if (mx >= 3.f && (floors || !isFloor) && Dist(lp, pos) <= range) {
                                uint8_t f = 0;
                                if (M.Raw(prim + O.primFlags, &f, 1) && (f & mask)) {
                                    f &= (uint8_t)~mask;
                                    if (M.Wr(prim + O.primFlags, &f, 1)) {
                                        applied.push_back({ i, prim });
                                        appliedSet.insert(i);
                                    }
                                }
                            }
                        }
                    }
                }
                for (auto k : kids) st.push_back(k);
            }
            gStMb = (int)applied.size();
        }
        Sleep(60);
    }
    restoreAll();
}

// ============================================================ local loop (movimento, camera, mundo, ceu)
// So mexe no jogador local e em Lighting/Camera: muito mais leve que o Magic Bullet (nao varre o mapa).
static std::atomic<int> gStLocal{ 0 };   // 0 idle, 1 granted, 2 denied
static std::atomic<bool> gNoclipOn{ false }, gBlackSkyOn{ false };

struct LocalPatch { uintptr_t addr; uint8_t n; uint8_t orig[16]; };
using LocalMap = std::unordered_map<uintptr_t, LocalPatch>;

static bool LWrite(LocalMap& P, uintptr_t addr, const void* d, size_t n) {
    if (!addr || n == 0 || n > 16) return false;
    auto it = P.find(addr);
    if (it == P.end()) {
        LocalPatch p{};
        p.addr = addr;
        p.n = (uint8_t)n;
        if (!M.Raw(addr, p.orig, n)) return false;
        it = P.emplace(addr, p).first;
    }
    uint8_t cur[16]{};
    if (M.Raw(addr, cur, n) && !memcmp(cur, d, n)) return true;
    return M.Wr(addr, d, n);
}
static void LRestoreAll(LocalMap& P) {
    for (auto& kv : P) M.Wr(kv.second.addr, kv.second.orig, kv.second.n);
    P.clear();
}
static void LRestoreOne(LocalMap& P, uintptr_t addr) {
    auto it = P.find(addr);
    if (it == P.end()) return;
    M.Wr(it->second.addr, it->second.orig, it->second.n);
    P.erase(it);
}

// blanqueia o conteudo de uma propriedade Content (textura do skybox): mesma estrutura inline-string
// usada pelo motor para Decal/Texture/Sky, independente dos offsets de Instance.Name configurados.
static bool LBlankContent(LocalMap& P, uintptr_t addr) {
    uint64_t len = M.Rd<uint64_t>(addr + 0x10), cap = M.Rd<uint64_t>(addr + 0x18);
    if (len == 0) return true;             // ja vazio
    if (len > 400 || cap < len || cap > 4096) return false;
    uintptr_t data = cap >= 16 ? M.Rd<uintptr_t>(addr) : addr;
    uint8_t z8 = 0;
    uint64_t z64 = 0;
    bool ok = LWrite(P, data, &z8, 1);
    ok = LWrite(P, addr + 0x10, &z64, 8) && ok;
    return ok;
}

static void LocalLoop() {
    LocalMap patch;
    uintptr_t lastDm = 0, players = 0, local = 0, lch = 0, hum = 0, root = 0, cam = 0, lighting = 0, sky = 0;
    std::vector<uintptr_t> parts;
    ULONGLONG nextParts = 0, nextAfk = 0, nextLight = 0;
    bool prevNoclip = false, prevBright = false, prevClock = false, prevSky = false;

    while (gRun && gWork) {
        const MiscCfg& m = C.misc;
        bool anyWrite = m.speedEnabled || m.jumpEnabled || m.infJump || m.noclip ||
                        m.fovEnabled || m.fullbright || m.clockEnabled || m.blackSky;
        bool needRestore = prevNoclip && !m.noclip;
        if (!anyWrite && !needRestore) {
            if (!patch.empty()) LRestoreAll(patch);
            prevNoclip = prevBright = prevClock = prevSky = false;
            gNoclipOn = false;
            gBlackSkyOn = false;
            Sleep(120);
            continue;
        }
        if (!M.EnsureWrite()) { gStLocal = 2; Sleep(300); continue; }
        gStLocal = 1;
        gStWrite = 1;

        uintptr_t fdm = M.Rd<uintptr_t>(M.base + O.fdmPtr);
        uintptr_t dm = M.Rd<uintptr_t>(fdm + O.fdmReal);
        if (!dm) { Sleep(200); continue; }
        if (dm != lastDm) {
            LRestoreAll(patch);
            lastDm = dm; players = local = lch = hum = root = cam = lighting = sky = 0;
            parts.clear();
            prevNoclip = prevBright = prevClock = prevSky = false;
        }
        if (!players) players = Child(dm, "Players");
        local = players ? M.Rd<uintptr_t>(players + O.localPlayer) : 0;
        uintptr_t ch = local ? M.Rd<uintptr_t>(local + O.charModel) : 0;
        if (ch != lch) {
            lch = ch; hum = root = 0; parts.clear(); nextParts = 0;
            if (ch) {
                NameMapT nm = NameMap(ch);
                hum = Get(nm, "Humanoid");
                root = FindRootPart(nm, ch);
            }
            if (prevNoclip) { prevNoclip = false; gNoclipOn = false; }  // personagem trocou: nao mexe no antigo
        }
        uintptr_t ws = M.Rd<uintptr_t>(dm + O.dmWorkspace);
        uintptr_t nc = ws ? M.Rd<uintptr_t>(ws + O.wsCamera) : 0;
        if (nc != cam) cam = nc;
        if (!lighting) lighting = Child(dm, "Lighting");
        if (lighting && !sky) sky = Child(lighting, "Sky");

        ULONGLONG now = GetTickCount64();

        // ---- movimento (humanoid local)
        if (hum) {
            if (m.speedEnabled) { float v = std::max(m.speed, 0.f); LWrite(patch, hum + O.humWalkspeed, &v, 4); }
            else LRestoreOne(patch, hum + O.humWalkspeed);
            if (m.jumpEnabled) {
                float v = std::max(m.jumpPower, 0.f);
                LWrite(patch, hum + O.humJumpPower, &v, 4);
                uint8_t t = 1;
                LWrite(patch, hum + O.humUseJumpPower, &t, 1);
            } else {
                LRestoreOne(patch, hum + O.humJumpPower);
                LRestoreOne(patch, hum + O.humUseJumpPower);
            }
            bool infKeyOk = !m.infJumpKey || (GetAsyncKeyState(m.infJumpKey) & 0x8000);
            if (m.infJump && infKeyOk && (GetAsyncKeyState(VK_SPACE) & 0x8000)) {
                uint8_t j = 1;
                M.Wr(hum + O.humJump, &j, 1);     // consumido pelo jogo sozinho: nao precisa restaurar
            }
        }

        // ---- noclip: desliga CanCollide das partes do personagem local
        bool noclipKeyOk = !m.noclipKey || (GetAsyncKeyState(m.noclipKey) & 0x8000);
        bool wantNoclip = m.noclip && noclipKeyOk && root;
        if (wantNoclip && (parts.empty() || now >= nextParts)) {
            nextParts = now + 2000;
            parts.clear();
            std::vector<uintptr_t> st{ lch };
            std::unordered_set<uintptr_t> seen;
            size_t budget = 400;
            while (!st.empty() && budget > 0) {
                budget--;
                uintptr_t i = st.back(); st.pop_back();
                if (!i || !seen.insert(i).second) continue;
                if (M.Rd<uintptr_t>(i + O.primitive)) parts.push_back(i);
                for (auto k : Kids(i)) st.push_back(k);
            }
        }
        if (wantNoclip) {
            for (auto part : parts) {
                uintptr_t prim = M.Rd<uintptr_t>(part + O.primitive);
                if (!prim) continue;
                uint8_t f = 0;
                if (!M.Raw(prim + O.primFlags, &f, 1)) continue;
                if (f & O.canCollide) { f &= (uint8_t)~O.canCollide; LWrite(patch, prim + O.primFlags, &f, 1); }
            }
            prevNoclip = true;
            gNoclipOn = true;
        } else if (prevNoclip) {
            for (auto part : parts) {
                uintptr_t prim = M.Rd<uintptr_t>(part + O.primitive);
                if (prim) LRestoreOne(patch, prim + O.primFlags);
            }
            prevNoclip = false;
            gNoclipOn = false;
        }

        // ---- fov da camera
        if (cam) {
            if (m.fovEnabled) {
                float cur = M.Rd<float>(cam + O.camFov);
                if (std::isfinite(cur) && cur > 0.01f && cur < 180.f) {
                    float t = std::clamp(m.fov, 20.f, 150.f);
                    LWrite(patch, cam + O.camFov, &t, 4);
                }
            } else LRestoreOne(patch, cam + O.camFov);
        }

        // ---- lighting (menos frequente: nao precisa todo tick)
        if (lighting && now >= nextLight) {
            nextLight = now + 250;
            if (m.fullbright) {
                float b = std::max(m.brightness, 0.1f), amb[3] = { 0.8f, 0.8f, 0.8f };
                LWrite(patch, lighting + O.litBrightness, &b, 4);
                LWrite(patch, lighting + O.litAmbient, amb, 12);
                LWrite(patch, lighting + O.litOutAmbient, amb, 12);
                prevBright = true;
            } else if (prevBright) {
                LRestoreOne(patch, lighting + O.litBrightness);
                LRestoreOne(patch, lighting + O.litAmbient);
                LRestoreOne(patch, lighting + O.litOutAmbient);
                prevBright = false;
            }
            if (m.clockEnabled) {
                float t = std::clamp(m.clockTime, 0.f, 24.f);
                LWrite(patch, lighting + O.litClockTime, &t, 4);
                prevClock = true;
            } else if (prevClock) {
                LRestoreOne(patch, lighting + O.litClockTime);
                prevClock = false;
            }
            if (m.blackSky) {
                float amb[3] = { 0.f, 0.f, 0.f };
                LWrite(patch, lighting + O.litAmbient, amb, 12);
                LWrite(patch, lighting + O.litOutAmbient, amb, 12);
                bool ok = sky != 0;
                if (sky) {
                    const uintptr_t offs[6] = { O.skyUp, O.skyDn, O.skyLf, O.skyRt, O.skyBk, O.skyFt };
                    for (auto o : offs) ok = LBlankContent(patch, sky + o) && ok;
                }
                prevSky = true;
                gBlackSkyOn = ok;
            } else if (prevSky) {
                LRestoreOne(patch, lighting + O.litAmbient);
                LRestoreOne(patch, lighting + O.litOutAmbient);
                if (sky) {
                    const uintptr_t offs[6] = { O.skyUp, O.skyDn, O.skyLf, O.skyRt, O.skyBk, O.skyFt };
                    for (auto o : offs) { LRestoreOne(patch, sky + o); LRestoreOne(patch, sky + o + 0x10); }
                }
                prevSky = false;
                gBlackSkyOn = false;
            }
        }

        Sleep(30);
    }
    LRestoreAll(patch);
    gNoclipOn = false;
    gBlackSkyOn = false;
}

// anti-afk: so simula uma tecla inofensiva de vez em quando. Nao mexe em memoria (sempre confiavel).
static void AntiAfkLoop() {
    ULONGLONG next = 0;
    while (gRun && gWork) {
        if (C.misc.antiAfk && gGameActive.load()) {
            ULONGLONG now = GetTickCount64();
            if (now >= next) {
                next = now + 25000 + (GetTickCount64() % 5000);
                INPUT in[2]{};
                in[0].type = INPUT_KEYBOARD; in[0].ki.wVk = VK_SHIFT;
                in[1].type = INPUT_KEYBOARD; in[1].ki.wVk = VK_SHIFT; in[1].ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(2, in, sizeof(INPUT));
            }
        }
        Sleep(1000);
    }
}

// ============================================================ input (aimbot / triggerbot)
static void MoveMouse(int dx, int dy) {
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    in.mi.dx = dx;
    in.mi.dy = dy;
    SendInput(1, &in, sizeof(in));
}
static bool gTrigDown = false;
static void MouseButton(bool down) {
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    SendInput(1, &in, sizeof(in));
}

static void DrawEllipse(ImDrawList* dl, float cx, float cy, float rx, float ry, ImU32 col, float th) {
    const int N = 64;
    dl->PathClear();
    for (int i = 0; i < N; i++) {
        float a = (float)i / N * 6.2831853f;
        dl->PathLineTo(ImVec2(cx + std::cos(a) * rx, cy + std::sin(a) * ry));
    }
    dl->PathStroke(col, ImDrawFlags_Closed, th);
}

static bool EntBox(const Ent& e, const Mat4& mat, float w, float h, float& x0, float& y0, float& x1, float& y1) {
    Vec3 top = e.head; top.y += 0.8f * e.scale;
    Vec3 bot = e.root; bot.y -= 3.1f * e.scale;
    Vec2 st, sb;
    if (!W2S(mat, top, w, h, st) || !W2S(mat, bot, w, h, sb)) return false;
    float bh = sb.y - st.y;
    if (bh < 2.f) return false;
    float bw = bh * 0.55f, cx = (st.x + sb.x) * 0.5f;
    x0 = cx - bw / 2.f; x1 = cx + bw / 2.f; y0 = st.y; y1 = sb.y;
    return true;
}

static void RunAimbot(HWND rbx, float w, float h, const std::vector<Ent>& ents, const Mat4& mat, const Vec3& lpos, ImDrawList* dl) {
    static bool toggled = false, prev = false;
    static float ax = 0, ay = 0;
    static uintptr_t lockId = 0;
    const AimCfg& a = A();
    bool keyDown = a.key && (GetAsyncKeyState(a.key) & 0x8000) != 0;
    if (a.mode == 1) { if (keyDown && !prev) toggled = !toggled; prev = keyDown; }
    bool active = a.mode == 0 ? keyDown : toggled;
    if (!a.enabled || !active || GetForegroundWindow() != rbx) { ax = ay = 0; lockId = 0; return; }

    Vec2 c{ w / 2.f, h / 2.f }, tgt{};
    float fx = std::max(a.fovX, 1.f), fy = std::max(a.fovY, 1.f);
    auto score = [&](const Vec2& s) {
        float dx = (s.x - c.x) / fx, dy = (s.y - c.y) / fy;
        return std::sqrt(dx * dx + dy * dy);
    };

    bool found = false;
    float best = 1.0e9f;
    uintptr_t bestId = 0;
    for (auto& e : ents) {
        if (e.tag == 1) continue;                       // marcado como amigo: ignora
        if (a.teamCheck && e.sameTeam) continue;
        float d = Dist(lpos, e.root);
        if (d > a.maxDist) continue;
        Vec3 pts[2];
        int n = 0;
        if (a.part == 0 || a.part == 2) pts[n++] = e.head;
        if (a.part == 1 || a.part == 2) pts[n++] = e.root;
        float es = 2.f;
        Vec2 et{};
        for (int i = 0; i < n; i++) {
            Vec3 p = pts[i];
            if (a.predict) { p.x += e.vel.x * a.predictAmt; p.y += e.vel.y * a.predictAmt; p.z += e.vel.z * a.predictAmt; }
            Vec2 s;
            if (!W2S(mat, p, w, h, s)) continue;
            float sc = score(s);
            if (sc < es) { es = sc; et = s; }
        }
        if (es > 1.f) continue;
        if (a.sticky && e.id == lockId) { tgt = et; bestId = e.id; found = true; break; }
        // marcado como alvo prioritario: sempre vence
        float rank = e.tag == 2 ? -1000.f :
                     a.priority == 1 ? d * 0.01f :
                     a.priority == 2 ? e.hp :
                     es;
        if (rank < best) { best = rank; tgt = et; bestId = e.id; found = true; }
    }
    if (!found) { ax = ay = 0; lockId = 0; return; }
    lockId = a.sticky ? bestId : 0;

    float jx = 0.f, jy = 0.f;
    if (a.jitter > 0.f) {
        jx = ((float)rand() / RAND_MAX * 2.f - 1.f) * a.jitter;
        jy = ((float)rand() / RAND_MAX * 2.f - 1.f) * a.jitter;
    }
    tgt.x += jx;
    tgt.y += jy;

    float dx0 = tgt.x - c.x, dy0 = tgt.y - c.y;
    if (a.deadzone > 0.f && std::sqrt(dx0 * dx0 + dy0 * dy0) <= a.deadzone) { ax = ay = 0; return; }

    float sm = std::max(a.smooth, 1.f);
    float vx = dx0 / sm, vy = dy0 / sm;
    if (a.maxSpeed > 0.f) {
        float len = std::sqrt(vx * vx + vy * vy);
        if (len > a.maxSpeed) { vx *= a.maxSpeed / len; vy *= a.maxSpeed / len; }
    }
    ax += vx; ay += vy;
    int mx = (int)ax, my = (int)ay;
    ax -= mx; ay -= my;
    if (mx || my) MoveMouse(mx, my);
}

static void RunTrigger(HWND rbx, float w, float h, const std::vector<Ent>& ents, const Mat4& mat, const Vec3& lpos) {
    static ULONGLONG downAt = 0, lastClick = 0;
    const AimCfg& a = A();
    ULONGLONG now = GetTickCount64();
    if (gTrigDown && now - downAt >= 25) { MouseButton(false); gTrigDown = false; lastClick = now; }
    if (gTrigDown) return;
    if (!a.trigEnabled || GetForegroundWindow() != rbx) return;
    if (a.trigKey && !(GetAsyncKeyState(a.trigKey) & 0x8000)) return;
    if (now - lastClick < (ULONGLONG)std::max(a.trigDelay, 0.f)) return;
    float cx = w / 2.f, cy = h / 2.f, s = a.trigScale / 100.f;
    for (auto& e : ents) {
        if (e.tag == 1) continue;
        if (a.teamCheck && e.sameTeam) continue;
        if (Dist(lpos, e.root) > a.maxDist) continue;
        float x0, y0, x1, y1;
        if (!EntBox(e, mat, w, h, x0, y0, x1, y1)) continue;
        float hw = (x1 - x0) * 0.5f * s, hh = (y1 - y0) * 0.5f * s;
        float mx = (x0 + x1) * 0.5f, my = (y0 + y1) * 0.5f;
        if (std::fabs(cx - mx) <= hw && std::fabs(cy - my) <= hh) {
            MouseButton(true);
            gTrigDown = true;
            downAt = now;
            break;
        }
    }
}

// ============================================================ fonts / color helpers
static ImFont* fUI = nullptr;
static ImFont* fTitle = nullptr;
static ImFont* fLarge = nullptr;
static ImFont* fEsp = nullptr;

static ImFont* TryFont(const char* const* names, float size) {
    ImGuiIO& io = ImGui::GetIO();
    char win[MAX_PATH]{};
    GetWindowsDirectoryA(win, MAX_PATH);
    for (int i = 0; names[i]; i++) {
        std::string p = std::string(win) + "\\Fonts\\" + names[i];
        if (GetFileAttributesA(p.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
        ImFontConfig cfg;
        cfg.OversampleH = 2;
        cfg.OversampleV = 1;
        cfg.PixelSnapH = true;
        ImFont* f = io.Fonts->AddFontFromFileTTF(p.c_str(), size, &cfg);
        if (f) return f;
    }
    return nullptr;
}
static void InitFonts() {
    static const char* UI[]   = { "tahoma.ttf", "segoeui.ttf", "arial.ttf", nullptr };
    static const char* BOLD[] = { "tahomabd.ttf", "segoeuib.ttf", "arialbd.ttf", nullptr };
    static const char* SEMI[] = { "seguisb.ttf", "segoeui.ttf", "tahoma.ttf", nullptr };
    ImGuiIO& io = ImGui::GetIO();
    fUI = TryFont(UI, 13.f);
    if (!fUI) fUI = io.Fonts->AddFontDefault();
    fTitle = TryFont(BOLD, 13.f);
    if (!fTitle) fTitle = fUI;
    fLarge = TryFont(BOLD, 22.f);
    if (!fLarge) fLarge = fTitle;
    fEsp = TryFont(SEMI, 14.f);
    if (!fEsp) fEsp = fUI;
    io.FontDefault = fUI;
}

static ID3D11Device* gDev = nullptr;
static ID3D11DeviceContext* gCtx = nullptr;
static IDXGISwapChain* gSwap = nullptr;
static ID3D11RenderTargetView* gRtv = nullptr;

static ImVec4 G(int v, float a = 1.f) { float f = v / 255.f; return ImVec4(f, f, f, a); }
static ImU32 GU(int v, int a = 255) { return IM_COL32(v, v, v, a); }
static ImU32 U32(const float* c) { return ImGui::ColorConvertFloat4ToU32(ImVec4(c[0], c[1], c[2], c[3])); }
static ImU32 Acc(float aMul = 1.f) {
    return ImGui::ColorConvertFloat4ToU32(ImVec4(C.accent[0], C.accent[1], C.accent[2], C.accent[3] * aMul));
}

// ============================================================ esp (leve)
static void EspText(ImDrawList* dl, ImVec2 p, ImU32 col, const char* s, float size) {
    dl->AddText(fEsp, size, ImVec2(p.x + 1, p.y + 1), IM_COL32(0, 0, 0, 170), s);
    dl->AddText(fEsp, size, p, col, s);
}

static void BoxLines(ImDrawList* dl, float x0, float y0, float x1, float y1, ImU32 col, float th, int style) {
    if (style == 0) { dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), col, 0.f, 0, th); return; }
    float lw = (x1 - x0) * 0.28f, lh = (y1 - y0) * 0.18f;
    auto seg = [&](float ax, float ay, float bx, float by) { dl->AddLine(ImVec2(ax, ay), ImVec2(bx, by), col, th); };
    seg(x0, y0, x0 + lw, y0); seg(x0, y0, x0, y0 + lh);
    seg(x1, y0, x1 - lw, y0); seg(x1, y0, x1, y0 + lh);
    seg(x0, y1, x0 + lw, y1); seg(x0, y1, x0, y1 - lh);
    seg(x1, y1, x1 - lw, y1); seg(x1, y1, x1, y1 - lh);
}

static void DrawESP(ImDrawList* dl, float w, float h, const std::vector<Ent>& ents, const Mat4& mat, const Vec3& lpos) {
    const EspCfg& cf = C.esp;
    if (!cf.enabled) return;
    const ImU32 shade = IM_COL32(0, 0, 0, 190), distC = IM_COL32(175, 175, 175, 255);
    const float fs = cf.textSize;
    for (auto& e : ents) {
        if (e.sameTeam && !cf.showTeam && e.tag != 2) continue;
        float d = Dist(lpos, e.root);
        if (d > cf.maxDist) continue;
        float x0, y0, x1, y1;
        if (!EntBox(e, mat, w, h, x0, y0, x1, y1)) continue;
        float bh = y1 - y0;
        if (bh < 4.f) continue;                       // pequeno demais: nao vale desenhar
        if (x1 < -50.f || x0 > w + 50.f || y1 < -50.f || y0 > h + 50.f) continue;
        float cx = (x0 + x1) * 0.5f;
        const float* boxCol = e.tag == 2 ? cf.targetCol : e.tag == 1 ? cf.friendCol : e.sameTeam ? cf.teamCol : cf.boxCol;
        ImU32 boxC = U32(boxCol);
        if (cf.box) {
            BoxLines(dl, x0, y0, x1, y1, shade, 3.f, cf.boxStyle);
            BoxLines(dl, x0, y0, x1, y1, boxC, 1.3f, cf.boxStyle);
        }
        if (cf.headDot) {
            Vec2 hs;
            if (W2S(mat, e.head, w, h, hs)) {
                float r = std::max(2.f, bh * 0.06f);
                dl->AddCircle(ImVec2(hs.x, hs.y), r, shade, 16, 2.f);
                dl->AddCircle(ImVec2(hs.x, hs.y), r, boxC, 16, 1.2f);
            }
        }
        if (cf.health && e.hasHp && bh >= 10.f) {
            float pct = std::clamp(e.hp / std::max(e.maxHp, 1.f), 0.f, 1.f);
            float bx1 = x0 - 4.f, bx0 = bx1 - 3.f, top = y1 - bh * pct;
            dl->AddRectFilled(ImVec2(bx0 - 1, y0 - 1), ImVec2(bx1 + 1, y1 + 1), shade);
            dl->AddRectFilled(ImVec2(bx0, top), ImVec2(bx1, y1),
                              IM_COL32((int)(235 - 125 * pct), (int)(70 + 155 * pct), (int)(70 + 50 * pct), 255));
        }
        if (bh >= 16.f) {                              // texto so quando da para ler
            if (cf.name && e.name[0]) {
                ImVec2 sz = fEsp->CalcTextSizeA(fs, FLT_MAX, 0.f, e.name);
                EspText(dl, ImVec2(cx - sz.x * 0.5f, y0 - sz.y - 3.f), U32(cf.nameCol), e.name, fs);
            }
            if (cf.distance) {
                char b[24];
                snprintf(b, sizeof(b), "%.0fm", d);
                ImVec2 sz = fEsp->CalcTextSizeA(fs - 1.f, FLT_MAX, 0.f, b);
                EspText(dl, ImVec2(cx - sz.x * 0.5f, y1 + 3.f), distC, b, fs - 1.f);
            }
        }
        if (cf.tracer) {
            float oy = cf.tracerOrigin == 0 ? h : (cf.tracerOrigin == 1 ? h * 0.5f : 0.f);
            dl->AddLine(ImVec2(w * 0.5f, oy), ImVec2(cx, y1), U32(cf.tracerCol), 1.f);
        }
    }
}

static void DrawCrosshair(ImDrawList* dl, float w, float h) {
    if (!C.crossEnabled) return;
    ImU32 col = U32(C.crossCol);
    float cx = w * 0.5f, cy = h * 0.5f, g = C.crossGap, sz = C.crossSize, th = C.crossThick;
    if (C.crossStyle == 0 || C.crossStyle == 2) {
        dl->AddLine(ImVec2(cx - g - sz, cy), ImVec2(cx - g, cy), col, th);
        dl->AddLine(ImVec2(cx + g, cy), ImVec2(cx + g + sz, cy), col, th);
        dl->AddLine(ImVec2(cx, cy - g - sz), ImVec2(cx, cy - g), col, th);
        dl->AddLine(ImVec2(cx, cy + g), ImVec2(cx, cy + g + sz), col, th);
    }
    if (C.crossStyle == 1 || C.crossStyle == 2) dl->AddCircleFilled(ImVec2(cx, cy), std::max(th, 1.5f), col, 12);
}

static void DrawKeybindList(ImDrawList* dl, float w, float h) {
    if (!C.keybindList) return;
    const AimCfg& a = A();
    struct Row { const char* label; bool on; };
    Row rows[] = {
        { "Aimbot",    a.enabled && (GetAsyncKeyState(a.key) & 0x8000) != 0 },
        { "Silent",    a.silent },
        { "Trigger",   a.trigEnabled },
        { "MagicBullet", C.mbEnabled },
        { "ESP",       C.esp.enabled },
    };
    float y = h - 16.f - 5 * 16.f;
    float maxw = 0.f;
    for (auto& r : rows) maxw = std::max(maxw, ImGui::CalcTextSize(r.label).x);
    for (auto& r : rows) {
        char b[48];
        snprintf(b, sizeof(b), "[%s] %s", r.on ? "ON" : "OFF", r.label);
        ImU32 col = r.on ? IM_COL32(120, 235, 130, 255) : IM_COL32(150, 150, 150, 255);
        dl->AddText(ImVec2(w - maxw - 70.f, y), IM_COL32(0, 0, 0, 170), b);
        dl->AddText(ImVec2(w - maxw - 71.f, y - 1.f), col, b);
        y += 16.f;
    }
}

// ============================================================ theme / widgets (estilo "private")
static void ApplyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0; s.ChildRounding = 0; s.FrameRounding = 0; s.GrabRounding = 0;
    s.PopupRounding = 0; s.ScrollbarRounding = 0;
    s.WindowPadding = { 8, 8 }; s.FramePadding = { 6, 3 }; s.ItemSpacing = { 6, 4 };
    s.WindowBorderSize = 1; s.ChildBorderSize = 0; s.FrameBorderSize = 1; s.PopupBorderSize = 1; s.GrabMinSize = 8;
    s.ScrollbarSize = 5;
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = G(6);         c[ImGuiCol_ChildBg] = G(0, 0);
    c[ImGuiCol_PopupBg] = G(10);         c[ImGuiCol_Border] = G(95);
    c[ImGuiCol_Text] = G(215);           c[ImGuiCol_TextDisabled] = G(105);
    c[ImGuiCol_FrameBg] = G(10);         c[ImGuiCol_FrameBgHovered] = G(22);   c[ImGuiCol_FrameBgActive] = G(32);
    c[ImGuiCol_CheckMark] = G(235);      c[ImGuiCol_SliderGrab] = G(200);      c[ImGuiCol_SliderGrabActive] = G(235);
    c[ImGuiCol_Button] = G(12);          c[ImGuiCol_ButtonHovered] = G(30);    c[ImGuiCol_ButtonActive] = G(46);
    c[ImGuiCol_Header] = G(28);          c[ImGuiCol_HeaderHovered] = G(42);    c[ImGuiCol_HeaderActive] = G(58);
    c[ImGuiCol_Separator] = G(60);
    c[ImGuiCol_ScrollbarBg] = G(0, 0);   c[ImGuiCol_ScrollbarGrab] = G(210);
    c[ImGuiCol_ScrollbarGrabHovered] = G(235); c[ImGuiCol_ScrollbarGrabActive] = G(255);
}

static std::string KeyName(int vk) {
    if (vk == 0) return "none";
    switch (vk) {
        case VK_LBUTTON: return "lmb";  case VK_RBUTTON: return "rmb";
        case VK_MBUTTON: return "mmb";  case VK_XBUTTON1: return "m4";
        case VK_XBUTTON2: return "m5";
    }
    LONG lp = (LONG)MapVirtualKeyA(vk, MAPVK_VK_TO_VSC) << 16;
    switch (vk) {
        case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
        case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN: lp |= (1 << 24);
    }
    char b[32]{};
    if (GetKeyNameTextA(lp, b, sizeof(b)) > 0) {
        for (char* p = b; *p; ++p) *p = (char)std::tolower((unsigned char)*p);
        return b;
    }
    return "key" + std::to_string(vk);
}

static float RowH() { return ImGui::GetFrameHeight(); }

// texto da tecla a direita ("rmb", "none"...). Esc = none.
static void KeyTag(int* key, ImVec2 pos, float w, bool allowNone) {
    static int* waiting = nullptr;
    static double waitT = 0;
    float fh = RowH();
    ImGui::SetCursorScreenPos(pos);
    ImGui::PushID(key);
    bool clicked = ImGui::InvisibleButton("##k", ImVec2(w, fh));
    bool hov = ImGui::IsItemHovered();
    ImGui::PopID();
    if (clicked) { waiting = key; waitT = ImGui::GetTime(); }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    bool wt = waiting == key;
    std::string txt = wt ? "..." : KeyName(*key);
    ImVec2 ts = ImGui::CalcTextSize(txt.c_str());
    dl->AddText(ImVec2(pos.x + w - ts.x - 2.f, pos.y + (fh - ts.y) * 0.5f), wt ? GU(255) : (hov ? GU(235) : GU(170)), txt.c_str());
    if (wt && ImGui::GetTime() - waitT > 0.2) {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { if (allowNone) *key = 0; waiting = nullptr; }
        else for (int vk = 2; vk < 255; vk++) {
            if (vk == VK_ESCAPE) continue;
            if (GetAsyncKeyState(vk) & 0x8000) { *key = vk; waiting = nullptr; break; }
        }
    }
}

// titulo de secao: linha branca em cima + nome
static void Sec(const char* title) {
    ImGui::Dummy(ImVec2(0, 1));
    ImVec2 p = ImGui::GetCursorScreenPos();
    float W = ImGui::GetContentRegionAvail().x;
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y), ImVec2(p.x + W, p.y), GU(225), 1.f);
    ImGui::Dummy(ImVec2(W, 1));
    ImGui::TextColored(G(238), "%s", title);
}

// [ ] Label ........ [cor] rmb
static void Check(const char* label, bool* v, float* col = nullptr, int* key = nullptr, bool keyAllowNone = true) {
    ImGui::PushID((const void*)v);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float W = ImGui::GetContentRegionAvail().x, fh = RowH();
    const float kw = 44.f;
    float right = (key ? kw : 0.f) + (col ? fh + 4.f : 0.f);
    if (ImGui::InvisibleButton("##cb", ImVec2(std::max(W - right, 10.f), fh))) *v = !*v;
    bool hov = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float bs = 11.f;
    ImVec2 b0(p.x + 1.f, p.y + (fh - bs) * 0.5f), b1(b0.x + bs, b0.y + bs);
    dl->AddRectFilled(b0, b1, *v ? Acc() : GU(8));
    dl->AddRect(b0, b1, *v ? GU(255) : (hov ? GU(170) : GU(110)));
    dl->AddText(ImVec2(p.x + bs + 9.f, p.y + (fh - ImGui::GetTextLineHeight()) * 0.5f), *v ? GU(240) : GU(185), label);
    if (col) {
        ImGui::SetCursorScreenPos(ImVec2(p.x + W - (key ? kw : 0.f) - fh, p.y));
        ImGui::PushID("col");
        ImGui::ColorEdit4("##c", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
                                      ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
        ImGui::PopID();
    }
    if (key) KeyTag(key, ImVec2(p.x + W - kw, p.y), kw, keyAllowNone);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + fh));
    ImGui::Dummy(ImVec2(0, 0));
    ImGui::PopID();
}

// Label
// [=======  12.0  ======      ]
static void Slider(const char* label, float* v, float lo, float hi, const char* fmt, float step = 0.f) {
    ImGui::PushID((const void*)v);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float W = ImGui::GetContentRegionAvail().x, th = ImGui::GetTextLineHeight(), barH = 13.f;
    ImGui::InvisibleButton("##s", ImVec2(W, th + barH + 3.f));
    bool active = ImGui::IsItemActive();
    float x0 = p.x, x1 = p.x + W;
    if (active) {
        float t = std::clamp((ImGui::GetIO().MousePos.x - x0) / std::max(x1 - x0, 1.f), 0.f, 1.f);
        float nv = lo + t * (hi - lo);
        if (step > 0.f) nv = std::round(nv / step) * step;
        *v = std::clamp(nv, lo, hi);
    }
    float t = std::clamp((*v - lo) / (hi - lo), 0.f, 1.f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddText(p, GU(190), label);
    float by = p.y + th + 2.f;
    dl->AddRectFilled(ImVec2(x0, by), ImVec2(x1, by + barH), GU(10));
    if (t > 0.f) dl->AddRectFilled(ImVec2(x0 + 1.f, by + 1.f), ImVec2(x0 + 1.f + (W - 2.f) * t, by + barH - 1.f), Acc());
    dl->AddRect(ImVec2(x0, by), ImVec2(x1, by + barH), GU(120));
    char buf[48];
    snprintf(buf, sizeof(buf), fmt, *v);
    ImVec2 vs = ImGui::CalcTextSize(buf);
    ImVec2 tp(x0 + (W - vs.x) * 0.5f, by + (barH - vs.y) * 0.5f);
    dl->AddText(ImVec2(tp.x + 1, tp.y + 1), IM_COL32(0, 0, 0, 220), buf);
    dl->AddText(tp, GU(245), buf);
    ImGui::PopID();
}

// Label
// [ Head                    + ]
static void Combo(const char* label, int* v, const char* const* items, int n) {
    ImGui::PushID((const void*)v);
    ImGui::TextColored(G(190), "%s", label);
    ImVec2 a = ImGui::GetCursorScreenPos();
    float W = ImGui::GetContentRegionAvail().x, fh = RowH();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##c", items[std::clamp(*v, 0, n - 1)], ImGuiComboFlags_NoArrowButton)) {
        for (int i = 0; i < n; i++)
            if (ImGui::Selectable(items[i], *v == i)) *v = i;
        ImGui::EndCombo();
    }
    ImGui::GetWindowDrawList()->AddText(ImVec2(a.x + W - 14.f, a.y + (fh - ImGui::GetTextLineHeight()) * 0.5f), GU(225), "+");
    ImGui::PopID();
}

static void KeyRow(const char* label, int* key, bool allowNone) {
    ImGui::PushID((const void*)key);
    float fh = RowH(), W = ImGui::GetContentRegionAvail().x;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddText(ImVec2(p.x, p.y + (fh - ImGui::GetTextLineHeight()) * 0.5f), GU(190), label);
    KeyTag(key, ImVec2(p.x + W - 70.f, p.y), 70.f, allowNone);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + fh));
    ImGui::Dummy(ImVec2(0, 0));
    ImGui::PopID();
}

static void ColorRow(const char* label, float* col) {
    ImGui::PushID((const void*)col);
    float fh = RowH(), W = ImGui::GetContentRegionAvail().x;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddText(ImVec2(p.x, p.y + (fh - ImGui::GetTextLineHeight()) * 0.5f), GU(190), label);
    ImGui::SetCursorScreenPos(ImVec2(p.x + W - fh - 2.f, p.y));
    ImGui::ColorEdit4("##col", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
                                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
    ImGui::PopID();
}

static bool ButtonRow(const char* label) {
    return ImGui::Button(label, ImVec2(ImGui::GetContentRegionAvail().x, 22));
}

static void NoteText(const char* s) {
    ImGui::PushStyleColor(ImGuiCol_Text, G(120));
    ImGui::TextWrapped("%s", s);
    ImGui::PopStyleColor();
}

static void StatusRow(const char* label, const char* val, int kind) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float W = ImGui::GetContentRegionAvail().x, fh = RowH();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddText(ImVec2(p.x, p.y + (fh - ImGui::GetTextLineHeight()) * 0.5f), GU(190), label);
    ImVec2 vs = ImGui::CalcTextSize(val);
    ImU32 col = kind == 1 ? IM_COL32(120, 230, 130, 255) : (kind == 2 ? IM_COL32(240, 95, 95, 255) : GU(150));
    dl->AddText(ImVec2(p.x + W - vs.x, p.y + (fh - vs.y) * 0.5f), col, val);
    ImGui::Dummy(ImVec2(W, fh));
}

// coluna rolavel sem borda
static void ColBegin(const char* id, float w, float h) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild(id, ImVec2(w, h), ImGuiChildFlags_None);
    ImGui::PopStyleVar();
}
static void ColEnd() { ImGui::EndChild(); }

// ============================================================ licenca / inject (estado global)
static bool gLicensed = false;
static bool gInjected = false;
static std::string gHwid, gLicMsg, gInjMsg, gInjWarn;
static int gDaysLeft = -1;

static void DoInject(bool quiet) {
    gInjWarn.clear();
    if (!M.Attach(L"RobloxPlayerBeta.exe")) {
        gInjMsg = quiet ? "Waiting for Roblox..." : "Roblox not found. Open the game, run as administrator, then click Inject.";
        return;
    }
    gInjMsg.clear();
    wchar_t path[MAX_PATH]{};
    DWORD len = MAX_PATH;
    if (!O.version.empty() && QueryFullProcessImageNameW(M.h, 0, path, &len)) {
        std::wstring wp(path, len), wv(O.version.begin(), O.version.end());
        if (wp.find(wv) == std::wstring::npos) gInjWarn = "Roblox version differs from offsets.ini (ESP may be wrong).";
    }
    gInjected = true;
}

// ============================================================ live ESP preview (mannequin)
static void DrawEspPreview(float W, float H) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 e(p.x + W, p.y + H);
    dl->AddRectFilled(p, e, GU(10), 3.f);
    dl->PushClipRect(p, e, true);
    for (float gx = p.x + 16; gx < e.x; gx += 24) dl->AddLine(ImVec2(gx, p.y), ImVec2(gx, e.y), IM_COL32(255, 255, 255, 5));
    for (float gy = p.y + 16; gy < e.y; gy += 24) dl->AddLine(ImVec2(p.x, gy), ImVec2(e.x, gy), IM_COL32(255, 255, 255, 5));

    float bh = std::clamp(H - 70.f, 90.f, 240.f);
    float cx = p.x + W * 0.5f, y0 = p.y + (H - bh) * 0.5f, y1 = y0 + bh;
    const ImU32 body = GU(40), edge = GU(60);
    float hr = bh * 0.07f;
    dl->AddCircleFilled(ImVec2(cx, y0 + hr), hr, body, 20);
    dl->AddCircle(ImVec2(cx, y0 + hr), hr, edge, 20, 1.f);
    dl->AddRectFilled(ImVec2(cx - bh * 0.12f, y0 + hr * 1.6f), ImVec2(cx + bh * 0.12f, y1 - bh * 0.06f), body, bh * 0.04f);
    dl->AddRect(ImVec2(cx - bh * 0.12f, y0 + hr * 1.6f), ImVec2(cx + bh * 0.12f, y1 - bh * 0.06f), edge, bh * 0.04f);

    const EspCfg& cf = C.esp;
    if (cf.enabled) {
        float x0 = cx - bh * 0.27f, x1 = cx + bh * 0.27f;
        ImU32 boxC = U32(cf.boxCol);
        if (cf.box) {
            BoxLines(dl, x0, y0, x1, y1, IM_COL32(0, 0, 0, 190), 3.f, cf.boxStyle);
            BoxLines(dl, x0, y0, x1, y1, boxC, 1.3f, cf.boxStyle);
        }
        if (cf.headDot) {
            dl->AddCircle(ImVec2(cx, y0 + hr), hr * 0.8f, IM_COL32(0, 0, 0, 190), 16, 2.f);
            dl->AddCircle(ImVec2(cx, y0 + hr), hr * 0.8f, boxC, 16, 1.2f);
        }
        if (cf.health) {
            float pct = 0.72f, bx1 = x0 - 4.f, bx0 = bx1 - 3.f, top = y1 - bh * pct;
            dl->AddRectFilled(ImVec2(bx0 - 1, y0 - 1), ImVec2(bx1 + 1, y1 + 1), IM_COL32(0, 0, 0, 190));
            dl->AddRectFilled(ImVec2(bx0, top), ImVec2(bx1, y1), IM_COL32(100, 220, 110, 255));
        }
        if (cf.name) {
            ImVec2 sz = fEsp->CalcTextSizeA(cf.textSize, FLT_MAX, 0.f, "Player");
            EspText(dl, ImVec2(cx - sz.x * 0.5f, y0 - sz.y - 3.f), U32(cf.nameCol), "Player", cf.textSize);
        }
        if (cf.distance) {
            ImVec2 sz = fEsp->CalcTextSizeA(cf.textSize - 1.f, FLT_MAX, 0.f, "42m");
            EspText(dl, ImVec2(cx - sz.x * 0.5f, y1 + 3.f), IM_COL32(175, 175, 175, 255), "42m", cf.textSize - 1.f);
        }
    } else {
        ImVec2 sz = ImGui::CalcTextSize("ESP disabled");
        dl->AddText(ImVec2(cx - sz.x * 0.5f, p.y + 8), GU(110), "ESP disabled");
    }
    dl->PopClipRect();
    ImGui::Dummy(ImVec2(W, H));
}

// ============================================================ pages
enum { TAB_MAIN, TAB_VISUALS, TAB_MISC, TAB_SETTINGS };
static int gTab = TAB_MAIN;

static void PageMain() {
    const float gap = 10.f;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float cw = (avail.x - gap) * 0.5f;
    AimCfg& a = A();

    ColBegin("ml", cw, avail.y);
    Sec("Aimbot");
    static const char* profs[] = { "Legit", "Rage" };
    int p = C.profile & 1;
    Combo("Profile", &p, profs, 2);
    C.profile = p;
    Check("Enabled", &a.enabled, nullptr, &a.key, true);
    Check("Team Check", &a.teamCheck);
    Check("Sticky Target", &a.sticky);
    Check("Prediction", &a.predict);
    Check("Show FOV", &a.drawFov, a.fovCol);
    static const char* parts[] = { "Head", "Torso", "Nearest" };
    Combo("Target Part", &a.part, parts, 3);
    static const char* modes[] = { "Hold", "Toggle" };
    Combo("Aim Mode", &a.mode, modes, 2);
    static const char* prios[] = { "Closest to Crosshair", "Closest Distance", "Lowest Health" };
    Combo("Target Priority", &a.priority, prios, 3);
    Slider("Smoothing", &a.smooth, 1.f, 30.f, "%.2f", 0.05f);
    Slider("FOV X", &a.fovX, 10.f, 900.f, "%.0f", 1.f);
    Slider("FOV Y", &a.fovY, 10.f, 900.f, "%.0f", 1.f);
    Slider("Deadzone", &a.deadzone, 0.f, 30.f, "%.0f", 1.f);
    Slider("Max Speed (0 = off)", &a.maxSpeed, 0.f, 80.f, "%.0f", 1.f);
    Slider("Max Distance", &a.maxDist, 50.f, 3000.f, "%.0f", 10.f);
    Slider("Prediction Time", &a.predictAmt, 0.01f, 0.5f, "%.2f", 0.01f);
    Slider("Jitter", &a.jitter, 0.f, 10.f, "%.1f", 0.1f);
    if (ButtonRow("Reset profile")) A() = (C.profile & 1) == 0 ? PresetLegit() : PresetRage();
    Sec("Trigger Bot");
    Check("Enabled", &a.trigEnabled, nullptr, &a.trigKey, true);
    Slider("Delay (ms)", &a.trigDelay, 0.f, 300.f, "%.0f", 1.f);
    Slider("Hit Area", &a.trigScale, 30.f, 150.f, "%.0f", 1.f);
    ColEnd();

    ImGui::SetCursorPos(ImVec2(cw + gap, 0));
    ColBegin("mr", cw, avail.y);
    int w = gStWrite.load();
    Sec("Silent Aim");
    Check("Enabled", &a.silent, nullptr, &a.silentKey, true);
    Check("Only While Shooting", &a.silentClick);
    Check("Hide Hitbox Part", &a.silentHide);
    static const char* sp[] = { "Head", "Torso", "Head + Torso" };
    Combo("Hitbox Part", &a.silentPart, sp, 3);
    Slider("Hitbox Size", &a.silentSize, 2.f, 20.f, "%.1f", 0.5f);
    Slider("Silent FOV X", &a.silentFovX, 10.f, 900.f, "%.0f", 1.f);
    Slider("Silent FOV Y", &a.silentFovY, 10.f, 900.f, "%.0f", 1.f);
    {
        char b[48];
        snprintf(b, sizeof(b), "%d target(s)", gStHb.load());
        StatusRow("Expanded now", a.silent ? b : "off", (a.silent && gStHb.load() > 0) ? 1 : (w == 2 ? 2 : 0));
    }
    Sec("Magic Bullet");
    Check("Enabled", &C.mbEnabled, nullptr, &C.mbKey, true);
    Check("Include Floors", &C.mbFloors);
    Slider("Range", &C.mbRange, 30.f, 600.f, "%.0f", 5.f);
    {
        char b[48];
        snprintf(b, sizeof(b), "%d part(s)", gStMb.load());
        StatusRow("Walls ignored", C.mbEnabled ? b : "off", gStMb.load() > 0 ? 1 : (w == 2 ? 2 : 0));
    }
    NoteText("Silent Aim and Magic Bullet write to the game and are experimental: results depend on the game you're in.");
    Sec("Player Tags");
    {
        std::vector<std::string> names;
        { std::lock_guard<std::mutex> g(gNamesLock); names = gNames; }
        if (names.empty()) NoteText("No players nearby yet.");
        for (auto& n : names) {
            int t = TagOf(n.c_str());
            ImGui::PushID(n.c_str());
            ImGui::TextColored(t == 2 ? ImVec4(C.esp.targetCol[0], C.esp.targetCol[1], C.esp.targetCol[2], 1.f) :
                               t == 1 ? ImVec4(C.esp.friendCol[0], C.esp.friendCol[1], C.esp.friendCol[2], 1.f) : G(190),
                               "%s", n.c_str());
            ImGui::SameLine(ImGui::GetWindowWidth() - 150.f);
            if (ImGui::SmallButton(t == 1 ? "Unfriend" : "Friend")) SetTag(n, t == 1 ? 0 : 1);
            ImGui::SameLine();
            if (ImGui::SmallButton(t == 2 ? "Untarget" : "Target")) SetTag(n, t == 2 ? 0 : 2);
            ImGui::PopID();
        }
    }
    if (ButtonRow("Clear all tags")) ClearTags();
    ColEnd();
}

static void PageVisuals() {
    const float gap = 10.f;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float cw = (avail.x - gap) * 0.5f;
    EspCfg& cf = C.esp;

    ColBegin("vl", cw, avail.y);
    Sec("ESP Options");
    Check("Enabled", &cf.enabled, nullptr, &cf.key, true);
    Check("Show Teammates", &cf.showTeam, cf.teamCol);
    Check("Box ESP", &cf.box, cf.boxCol);
    Check("Head Dot", &cf.headDot);
    Check("Name ESP", &cf.name, cf.nameCol);
    Check("Health Bar", &cf.health);
    Check("Distance", &cf.distance);
    Check("Tracers", &cf.tracer, cf.tracerCol);
    Slider("Max Distance", &cf.maxDist, 100.f, 5000.f, "%.0f", 10.f);
    Slider("Text Size", &cf.textSize, 10.f, 20.f, "%.0f", 1.f);
    Sec("Crosshair");
    Check("Enabled", &C.crossEnabled, C.crossCol);
    static const char* cs[] = { "Cross", "Dot", "Cross + Dot" };
    Combo("Style", &C.crossStyle, cs, 3);
    Slider("Size", &C.crossSize, 3.f, 25.f, "%.0f", 1.f);
    Slider("Gap", &C.crossGap, 0.f, 15.f, "%.0f", 1.f);
    Slider("Thickness", &C.crossThick, 1.f, 4.f, "%.1f", 0.5f);
    ColEnd();

    ImGui::SetCursorPos(ImVec2(cw + gap, 0));
    ColBegin("vr", cw, avail.y);
    Sec("Preview");
    DrawEspPreview(cw, 185.f);
    Sec("ESP Configs");
    static const char* bs[] = { "Full", "Corners" };
    Combo("Box Type", &cf.boxStyle, bs, 2);
    static const char* to[] = { "Bottom", "Center", "Top" };
    Combo("Tracer Position", &cf.tracerOrigin, to, 3);
    ColorRow("Box Color", cf.boxCol);
    ColorRow("Teammate Color", cf.teamCol);
    ColorRow("Friend Color (tagged)", cf.friendCol);
    ColorRow("Target Color (tagged)", cf.targetCol);
    ColorRow("Name Color", cf.nameCol);
    ColorRow("Tracer Color", cf.tracerCol);
    Sec("Performance");
    Slider("Update Rate (Hz)", &C.espRate, 20.f, 240.f, "%.0f", 5.f);
    Slider("Max Players Tracked", &C.maxTrack, 4.f, 120.f, "%.0f", 1.f);
    Slider("Exact Head Reads", &C.headReads, 0.f, 40.f, "%.0f", 1.f);
    Slider("Overlay FPS Cap", &C.overlayFps, 30.f, 240.f, "%.0f", 5.f);
    NoteText("If the game lags in full servers: lower Update Rate, Max Players Tracked and Overlay FPS Cap.");
    ColEnd();
}

static void PageMisc() {
    const float gap = 10.f;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float cw = (avail.x - gap) * 0.5f;
    MiscCfg& m = C.misc;
    int w = gStLocal.load(), ww = gStWrite.load();

    ColBegin("ul", cw, avail.y);
    Sec("Utility");
    Check("Hitbox Expander", &m.hbEnabled, m.hbCol, &m.hbKey, true);
    Slider("Hitbox Size", &m.hbSize, 2.f, 25.f, "%.1f", 0.5f);
    Check("Hitbox Team Check", &m.hbTeamCheck);
    Check("Hide Expanded Part", &m.hbHide);
    static const char* hp[] = { "Head", "Torso", "Head + Torso" };
    Combo("Hitbox Part", &m.hbPart, hp, 3);
    Slider("Hitbox Max Distance", &m.hbMaxDist, 100.f, 5000.f, "%.0f", 10.f);
    {
        char b[48];
        snprintf(b, sizeof(b), "%d target(s)", gStHb.load());
        StatusRow("Expanded now", m.hbEnabled ? b : "off", (m.hbEnabled && gStHb.load() > 0) ? 1 : (ww == 2 ? 2 : 0));
    }
    ImGui::Dummy(ImVec2(0, 4));
    Check("Speed Modifier", &m.speedEnabled, nullptr, &m.speedKey, true);
    Slider("Speed Value", &m.speed, 16.f, 500.f, "%.0f", 1.f);
    Check("Jump Modifier", &m.jumpEnabled, nullptr, &m.jumpKey, true);
    Slider("Jump Power", &m.jumpPower, 50.f, 500.f, "%.0f", 1.f);
    Check("Infinite Jump", &m.infJump, nullptr, &m.infJumpKey, true);
    Check("No Clip", &m.noclip, nullptr, &m.noclipKey, true);
    StatusRow("Local writes", w == 1 ? "granted" : (w == 2 ? "denied" : "idle"), w == 1 ? 1 : (w == 2 ? 2 : 0));
    NoteText("Speed/Jump/Infinite Jump/No Clip and the Hitbox Expander write to the game. Server-authoritative games may reset or detect these.");
    ColEnd();

    ImGui::SetCursorPos(ImVec2(cw + gap, 0));
    ColBegin("ur", cw, avail.y);
    Sec("Other");
    Check("FOV Modifier", &m.fovEnabled);
    Slider("FOV Value", &m.fov, 30.f, 150.f, "%.0f", 1.f);
    Check("Fullbright", &m.fullbright);
    Slider("Brightness", &m.brightness, 1.f, 10.f, "%.1f", 0.1f);
    Check("Clock Time", &m.clockEnabled);
    Slider("Time", &m.clockTime, 0.f, 24.f, "%.1f", 0.25f);
    Check("Anti-AFK", &m.antiAfk);
    NoteText("Anti-AFK only taps a harmless key every ~25s while you're in-game; it never touches memory.");
    Sec("Sky");
    Check("Black Sky", &m.blackSky);
    Check("Sky Branding (\"AIMERS\")", &m.skyBranding);
    StatusRow("Black sky", m.blackSky ? (gBlackSkyOn.load() ? "active" : "trying...") : "off",
              m.blackSky ? (gBlackSkyOn.load() ? 1 : 0) : 0);
    NoteText("Black Sky darkens the skybox and lighting in-game (experimental, some skyboxes may resist it). "
             "Sky Branding adds a big \"AIMERS\" watermark across the top of your screen while it's on.");
    ColEnd();
}

static void PageSettings() {
    const float gap = 10.f;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float cw = (avail.x - gap) * 0.5f;

    ColBegin("sl", cw, avail.y);
    Sec("Game");
    static const char* tmodes[] = { "Auto (Team)", "Team Color", "None (free-for-all)" };
    Combo("Team Detection", &C.teamMode, tmodes, 3);
    Check("Auto Inject", &C.autoInject);
    NoteText("Free-for-all games: choose None. Games with custom teams: try Team Color.");
    Sec("Menu");
    KeyRow("Menu Key", &C.menuKey, false);
    KeyRow("Switch Profile Key", &C.profileKey, true);
    KeyRow("Panic Key (exit)", &C.panicKey, true);
    ColorRow("Accent Color", C.accent);
    Check("Watermark", &C.watermark);
    Check("Keybind List (in-game)", &C.keybindList);
    ColEnd();

    ImGui::SetCursorPos(ImVec2(cw + gap, 0));
    ColBegin("sr", cw, avail.y);
    Sec("License");
    {
        char b[40];
        if (gDaysLeft < 0) snprintf(b, sizeof(b), "lifetime");
        else snprintf(b, sizeof(b), "%d day(s) left", gDaysLeft);
        StatusRow("Status", b, 1);
        std::string hw = LicFmt(gHwid);
        StatusRow("HWID", hw.c_str(), 0);
    }
    Sec("Session");
    StatusRow("Offsets", O.version.empty() ? "n/a" : O.version.c_str(), 0);
    StatusRow("Roblox build", gInjWarn.empty() ? "matches offsets" : "differs", gInjWarn.empty() ? 1 : 2);
    {
        char b[48];
        snprintf(b, sizeof(b), "%d", gPlayers.load());
        StatusRow("Players in server", b, 0);
        snprintf(b, sizeof(b), "%d", gTracked.load());
        StatusRow("Tracked (near)", b, 1);
        snprintf(b, sizeof(b), "%d Hz", gScanHz.load());
        StatusRow("Scan rate", b, 0);
        snprintf(b, sizeof(b), "%.2f ms", gScanMs.load());
        StatusRow("Scan time", b, gScanMs.load() > 6.f ? 2 : 1);
        snprintf(b, sizeof(b), "%.0f fps", ImGui::GetIO().Framerate);
        StatusRow("Overlay", b, 0);
    }
    Sec("Config");
    Check("Auto Save On Exit", &C.autoSave);
    if (ButtonRow("Save config")) SaveCfg();
    if (ButtonRow("Load config")) LoadCfg();
    if (ButtonRow("Unload")) gRun = false;
    ColEnd();
}

// ============================================================ main menu (barra superior)
static void NavTab(const char* label, int id, float x, float y, float h) {
    ImVec2 ts = ImGui::CalcTextSize(label);
    float w = ts.x + 4.f;
    ImGui::SetCursorScreenPos(ImVec2(x, y));
    ImGui::PushID(id + 500);
    bool clicked = ImGui::InvisibleButton("##nt", ImVec2(w, h));
    ImGui::PopID();
    bool hov = ImGui::IsItemHovered(), sel = gTab == id;
    if (clicked) gTab = id;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 col = sel ? Acc() : (hov ? GU(215) : GU(135));
    dl->AddText(ImVec2(x + 2.f, y + (h - ts.y) * 0.5f), col, label);
    if (sel) dl->AddLine(ImVec2(x, y + h - 2.f), ImVec2(x + w, y + h - 2.f), Acc(), 2.f);
}

static void DrawMenu() {
    const float WW = 780.f, WH = 560.f, HEAD = 34.f, PAD = 14.f;
    ImGui::SetNextWindowSize(ImVec2(WW, WH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(60, 30), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, G(90));
    ImGui::Begin("##aimer", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos();

    // cabecalho: nome + tabs a direita, igual a referencia
    dl->AddRectFilled(wp, ImVec2(wp.x + WW, wp.y + HEAD), GU(9), 4.f, ImDrawFlags_RoundCornersTop);
    dl->AddLine(ImVec2(wp.x, wp.y + HEAD), ImVec2(wp.x + WW, wp.y + HEAD), GU(55));
    ImGui::PushFont(fTitle);
    dl->AddText(ImVec2(wp.x + PAD, wp.y + (HEAD - ImGui::GetTextLineHeight()) * 0.5f), Acc(), APP_NAME);
    ImVec2 ns = ImGui::CalcTextSize(APP_NAME);
    ImGui::PopFont();
    dl->AddText(ImVec2(wp.x + PAD + ns.x + 6.f, wp.y + (HEAD - ImGui::GetTextLineHeight()) * 0.5f), GU(110), "(Public Beta)");

    const char* names[4] = { "MAIN", "VISUALS", "MISC", "SETTINGS" };
    float tw[4];
    float total = 0.f;
    for (int i = 0; i < 4; i++) { tw[i] = ImGui::CalcTextSize(names[i]).x + 4.f + (i ? 24.f : 0.f); total += tw[i]; }
    float tx = wp.x + WW - PAD - total;
    for (int i = 0; i < 4; i++) { tx += (i ? 24.f : 0.f); NavTab(names[i], i, tx, wp.y, HEAD); tx += tw[i] - (i ? 24.f : 0.f); }

    {
        ImGui::SetCursorScreenPos(ImVec2(wp.x + WW - 22.f, wp.y + 3.f));
        bool clicked = ImGui::InvisibleButton("##close", ImVec2(16.f, 16.f));
        bool hov = ImGui::IsItemHovered();
        ImVec2 c(wp.x + WW - 14.f, wp.y + 11.f);
        ImU32 col = hov ? GU(255) : GU(190);
        dl->AddLine(ImVec2(c.x - 3.f, c.y - 3.f), ImVec2(c.x + 3.f, c.y + 3.f), col, 1.5f);
        dl->AddLine(ImVec2(c.x - 3.f, c.y + 3.f), ImVec2(c.x + 3.f, c.y - 3.f), col, 1.5f);
        if (clicked) gRun = false;
    }

    // conteudo
    ImGui::SetCursorPos(ImVec2(PAD, HEAD + PAD));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("content", ImVec2(WW - 2.f * PAD, WH - HEAD - 2.f * PAD), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    switch (gTab) {
        case TAB_MAIN:     PageMain(); break;
        case TAB_VISUALS:  PageVisuals(); break;
        case TAB_MISC:     PageMisc(); break;
        default:           PageSettings(); break;
    }
    ImGui::EndChild();

    ImGui::End();
}

// ============================================================ loader: login (key + HWID) e Inject
static void TryLogin(const std::string& key) {
    int days = -1;
    int r = LicCheck(key, gHwid, &days);
    if (r == LIC_OK) {
        gLicensed = true;
        gDaysLeft = days;
        gLicMsg.clear();
        std::ofstream f(KEY_FILE);
        f << LicClean(key);
    } else if (r == LIC_EXPIRED) gLicMsg = "This key has expired.";
    else if (r == LIC_WRONG_HWID) gLicMsg = "Invalid key (or key from another PC).";
    else gLicMsg = "Invalid key format.";
}

static void DrawLoader(float W, float H) {
    static char buf[64] = {};
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(W, H), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 12));
    ImGui::PushStyleColor(ImGuiCol_Border, G(170));
    ImGui::Begin("##loader", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushFont(fTitle);
    dl->AddText(ImVec2(8.f, 4.f), GU(240), APP_NAME);
    ImGui::PopFont();
    {
        ImGui::SetCursorScreenPos(ImVec2(W - 22.f, 2.f));
        bool clicked = ImGui::InvisibleButton("##x", ImVec2(18.f, 18.f));
        bool hov = ImGui::IsItemHovered();
        ImVec2 c(W - 13.f, 11.f);
        ImU32 col = hov ? GU(255) : GU(190);
        dl->AddLine(ImVec2(c.x - 3.5f, c.y - 3.5f), ImVec2(c.x + 3.5f, c.y + 3.5f), col, 1.5f);
        dl->AddLine(ImVec2(c.x - 3.5f, c.y + 3.5f), ImVec2(c.x + 3.5f, c.y - 3.5f), col, 1.5f);
        if (clicked) gRun = false;
    }
    dl->AddLine(ImVec2(0, 22.f), ImVec2(W, 22.f), GU(70));
    ImGui::SetCursorPos(ImVec2(20.f, 38.f));

    ImGui::PushFont(fLarge);
    const char* title = gLicensed ? "Inject" : "Sign in";
    ImVec2 t1 = ImGui::CalcTextSize(title);
    ImGui::SetCursorPosX((W - t1.x) * 0.5f);
    ImGui::TextColored(G(245), "%s", title);
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0, 6));

    if (!gLicensed) {
        ImGui::SetNextItemWidth(-1);
        bool enter = ImGui::InputTextWithHint("##key", "license key", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue);
        std::string hw = LicFmt(gHwid);
        ImGui::TextColored(G(120), "HWID  %s", hw.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("copy")) ImGui::SetClipboardText(hw.c_str());
        ImGui::Dummy(ImVec2(0, 2));
        bool go = ImGui::Button("Log In", ImVec2(-1, 28)) || enter;
        if (go) TryLogin(buf);
        if (!gLicMsg.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(240, 95, 95, 255));
            ImGui::TextWrapped("%s", gLicMsg.c_str());
            ImGui::PopStyleColor();
        }
    } else {
        Check("Auto inject", &C.autoInject);
        ImGui::PushFont(fTitle);
        bool go = ImGui::Button("Inject", ImVec2(-1, 30));
        ImGui::PopFont();
        if (go) DoInject(false);
        if (!gInjMsg.empty()) {
            bool wait = gInjMsg.rfind("Waiting", 0) == 0 || gInjMsg.rfind("Roblox closed", 0) == 0;
            ImGui::PushStyleColor(ImGuiCol_Text, wait ? G(140) : ImVec4(0.94f, 0.37f, 0.37f, 1.f));
            ImGui::TextWrapped("%s", gInjMsg.c_str());
            ImGui::PopStyleColor();
        }
    }
    ImGui::End();
}

// ============================================================ d3d / window
static void MakeRtv() {
    ID3D11Texture2D* bb = nullptr;
    if (SUCCEEDED(gSwap->GetBuffer(0, IID_PPV_ARGS(&bb))) && bb) {
        gDev->CreateRenderTargetView(bb, nullptr, &gRtv);
        bb->Release();
    }
}
static void ResizeSwap(UINT w, UINT h) {
    gCtx->OMSetRenderTargets(0, nullptr, nullptr);
    if (gRtv) { gRtv->Release(); gRtv = nullptr; }
    gSwap->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
    MakeRtv();
}
static bool InitD3D(HWND hw) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hw;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL fl;
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &gSwap, &gDev, &fl, &gCtx))) return false;
    MakeRtv();
    return gRtv != nullptr;
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) return true;
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, m, w, l);
}

struct FindCtx { DWORD pid; HWND hwnd; };
static BOOL CALLBACK EnumCb(HWND h, LPARAM lp) {
    auto* c = (FindCtx*)lp;
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid != c->pid || !IsWindowVisible(h) || GetWindow(h, GW_OWNER)) return TRUE;
    wchar_t t[64]{};
    GetWindowTextW(h, t, 64);
    if (!wcscmp(t, L"Roblox")) { c->hwnd = h; return FALSE; }
    return TRUE;
}
static HWND FindRoblox(DWORD pid) {
    FindCtx c{ pid, nullptr };
    EnumWindows(EnumCb, (LPARAM)&c);
    return c.hwnd;
}

static void SetClickThrough(HWND hw, bool on) {
    LONG_PTR ex = GetWindowLongPtrW(hw, GWL_EXSTYLE);
    ex = on ? (ex | WS_EX_TRANSPARENT) : (ex & ~(LONG_PTR)WS_EX_TRANSPARENT);
    SetWindowLongPtrW(hw, GWL_EXSTYLE, ex);
}

static void DrawSkyBranding(ImDrawList* dl, float w, float h) {
    if (!(C.misc.blackSky && C.misc.skyBranding)) return;
    ImGui::PushFont(fLarge);
    const char* txt = "AIMERS";
    ImVec2 sz = ImGui::CalcTextSize(txt);
    ImGui::PopFont();
    float scale = 2.4f;
    ImVec2 p(w * 0.5f - sz.x * scale * 0.5f, h * 0.10f);
    dl->AddRectFilled(ImVec2(0, p.y - 10.f), ImVec2(w, p.y + sz.y * scale + 10.f), IM_COL32(0, 0, 0, 120));
    ImU32 col = Acc(0.9f);
    dl->AddText(fLarge, fLarge->FontSize * scale, ImVec2(p.x + 2.f, p.y + 2.f), IM_COL32(0, 0, 0, 160), txt);
    dl->AddText(fLarge, fLarge->FontSize * scale, p, col, txt);
}

static void DrawWatermark(ImDrawList* dl) {
    char wm[64];
    snprintf(wm, sizeof(wm), "%s  |  %.0f fps", APP_NAME, ImGui::GetIO().Framerate);
    ImVec2 sz = ImGui::CalcTextSize(wm);
    ImVec2 a(10, 10), b(10 + sz.x + 16, 10 + sz.y + 8);
    dl->AddRectFilled(a, b, IM_COL32(6, 6, 6, 215));
    dl->AddRect(a, b, IM_COL32(150, 150, 150, 255));
    dl->AddLine(ImVec2(a.x, a.y), ImVec2(b.x, a.y), Acc(), 2.f);
    dl->AddText(ImVec2(a.x + 8, a.y + 4), IM_COL32(230, 230, 230, 255), wm);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int) {
    ImGui_ImplWin32_EnableDpiAwareness();
    timeBeginPeriod(1);

    char exe[MAX_PATH]{};
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    std::string dir = exe;
    dir = dir.substr(0, dir.find_last_of("\\/"));
    SetCurrentDirectoryA(dir.c_str());

    std::string missing;
    if (!LoadOffsets("offsets.ini", missing)) {
        std::string msg = "offsets.ini incompleto ou ausente:\n" + missing;
        MessageBoxA(nullptr, msg.c_str(), APP_NAME, MB_ICONERROR);
        return 1;
    }
    LoadCfg();
    LoadTags();

    // licenca: tenta a key salva
    gHwid = LicHwid();
    {
        std::ifstream kf(KEY_FILE);
        std::string saved;
        if (kf && std::getline(kf, saved) && !saved.empty()) {
            int days = -1;
            if (LicCheck(saved, gHwid, &days) == LIC_OK) { gLicensed = true; gDaysLeft = days; }
        }
    }
    gLicensedFlag = gLicensed;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.lpszClassName = L"AimerOverlay";
    RegisterClassExW(&wc);
    HWND hw = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName, APP_NAMEW, WS_POPUP, 0, 0, 320, 228, nullptr, nullptr, hi, nullptr);
    SetLayeredWindowAttributes(hw, 0, 255, LWA_ALPHA);
    MARGINS mg{ -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(hw, &mg);

    if (!InitD3D(hw)) { MessageBoxA(nullptr, "Falha ao iniciar o DirectX 11.", APP_NAME, MB_ICONERROR); return 1; }

    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigDebugHighlightIdConflicts = false;
    InitFonts();
    ImGui_ImplWin32_Init(hw);
    ImGui_ImplDX11_Init(gDev, gCtx);
    ApplyTheme();

    std::thread scan, mb, loc, afk;
    bool started = false;

    LARGE_INTEGER qf;
    QueryPerformanceFrequency(&qf);

    HWND rbx = nullptr;
    UINT lastW = 0, lastH = 0;
    bool menuOpen = true, keyPrev = false, profKeyPrev = false, espKeyPrev = false, shown = false, wasLoader = true;
    SetClickThrough(hw, false);

    MSG msg{};
    while (gRun) {
        LARGE_INTEGER f0;
        QueryPerformanceCounter(&f0);

        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) gRun = false;
        }

        const bool loader = !(gLicensed && gInjected);
        if (!loader && !M.Alive()) {                    // Roblox fechou / trocou de processo (outro jogo): volta ao Inject
            gWork = false;
            if (started) { scan.join(); mb.join(); loc.join(); afk.join(); started = false; }
            gWork = true;
            M.Detach();
            { std::lock_guard<std::mutex> g(gLock); gSnap.reset(); }
            gInjected = false;
            gInjMsg = "Roblox closed. Waiting for Roblox...";
            gInjWarn.clear();
            shown = false;
            rbx = nullptr;
            continue;
        }
        if (loader && gLicensed && C.autoInject) {      // injeta sozinho assim que o Roblox abrir
            static ULONGLONG nextAuto = 0;
            ULONGLONG tn = GetTickCount64();
            if (tn >= nextAuto) { nextAuto = tn + 1000; DoInject(true); }
        }
        int ox = 0, oy = 0, ow = 320, oh = 228;
        if (loader) {
            ox = (GetSystemMetrics(SM_CXSCREEN) - ow) / 2;
            oy = (GetSystemMetrics(SM_CYSCREEN) - oh) / 2;
            rbx = nullptr;
            gGameActive = false;
        } else {
            if (!rbx || !IsWindow(rbx)) rbx = FindRoblox(M.pid);
            RECT cr{};
            POINT tl{ 0, 0 };
            bool valid = rbx && !IsIconic(rbx) && GetClientRect(rbx, &cr) && ClientToScreen(rbx, &tl);
            ow = cr.right - cr.left;
            oh = cr.bottom - cr.top;
            if (!valid || ow < 64 || oh < 64) {
                gGameActive = false;
                if (shown) { ShowWindow(hw, SW_HIDE); shown = false; }
                Sleep(100);
                continue;
            }
            ox = tl.x;
            oy = tl.y;
        }

        if (!shown) {
            ShowWindow(hw, loader ? SW_SHOW : SW_SHOWNOACTIVATE);
            if (loader) SetForegroundWindow(hw);
            shown = true;
        }
        SetWindowPos(hw, HWND_TOPMOST, ox, oy, ow, oh, SWP_NOACTIVATE);
        if ((UINT)ow != lastW || (UINT)oh != lastH) { ResizeSwap((UINT)ow, (UINT)oh); lastW = (UINT)ow; lastH = (UINT)oh; }
        if (!gRtv) { Sleep(50); continue; }
        const float w = (float)ow, h = (float)oh;
        gSW = w;
        gSH = h;

        if (loader != wasLoader) {                      // acabou de injetar
            wasLoader = loader;
            if (!loader) { menuOpen = true; SetClickThrough(hw, false); }
        }

        // injetou: liga as threads uma unica vez
        if (!loader && !started) {
            gLicensedFlag = true;
            scan = std::thread(ScanLoop);
            mb = std::thread(MbLoop);
            loc = std::thread(LocalLoop);
            afk = std::thread(AntiAfkLoop);
            started = true;
        }

        HWND fg = GetForegroundWindow();
        bool active = loader || fg == rbx || fg == hw;
        gGameActive = !loader && (fg == rbx) && !menuOpen;

        if (!loader) {
            bool down = active && C.menuKey && (GetAsyncKeyState(C.menuKey) & 0x8000) != 0;
            if (down && !keyPrev) {
                menuOpen = !menuOpen;
                SetClickThrough(hw, !menuOpen);
                if (menuOpen) SetForegroundWindow(hw); else SetForegroundWindow(rbx);
            }
            keyPrev = down;

            bool espDown = active && C.esp.key && (GetAsyncKeyState(C.esp.key) & 0x8000) != 0;
            if (espDown && !espKeyPrev) C.esp.enabled = !C.esp.enabled;
            espKeyPrev = espDown;
            bool profDown = active && C.profileKey && (GetAsyncKeyState(C.profileKey) & 0x8000) != 0;
            if (profDown && !profKeyPrev) C.profile ^= 1;
            profKeyPrev = profDown;
            if (active && C.panicKey && (GetAsyncKeyState(C.panicKey) & 0x8000)) gRun = false;
        }

        std::shared_ptr<const std::vector<Ent>> snap;
        Mat4 mat;
        Vec3 lpos;
        if (!loader) { std::lock_guard<std::mutex> g(gLock); snap = gSnap; mat = gMat; lpos = gLocalPos; }
        static const std::vector<Ent> kEmpty;
        const std::vector<Ent>& ents = snap ? *snap : kEmpty;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        if (loader) {
            DrawLoader(w, h);
        } else if (active) {
            ImDrawList* dl = ImGui::GetBackgroundDrawList();
            const AimCfg& a = A();
            DrawESP(dl, w, h, ents, mat, lpos);
            if (a.enabled && a.drawFov)
                DrawEllipse(dl, w * 0.5f, h * 0.5f, std::max(a.fovX, 1.f), std::max(a.fovY, 1.f), U32(a.fovCol), 1.2f);
            if (a.silent && a.drawFov)
                DrawEllipse(dl, w * 0.5f, h * 0.5f, std::max(a.silentFovX, 1.f), std::max(a.silentFovY, 1.f),
                            ImGui::ColorConvertFloat4ToU32(ImVec4(a.fovCol[0], a.fovCol[1], a.fovCol[2], a.fovCol[3] * 0.45f)), 1.f);
            DrawCrosshair(dl, w, h);
            DrawKeybindList(dl, w, h);
            DrawSkyBranding(dl, w, h);
            if (C.watermark) DrawWatermark(dl);
            if (!menuOpen) {
                RunAimbot(rbx, w, h, ents, mat, lpos, dl);
                RunTrigger(rbx, w, h, ents, mat, lpos);
            }
            if (menuOpen) DrawMenu();
        }
        ImGui::Render();

        const float clear[4] = { 0, 0, 0, 0 };
        gCtx->OMSetRenderTargets(1, &gRtv, nullptr);
        gCtx->ClearRenderTargetView(gRtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        gSwap->Present(1, 0);

        // limitador de FPS do overlay (menos GPU/CPU)
        {
            LARGE_INTEGER f1;
            QueryPerformanceCounter(&f1);
            double ms = (double)(f1.QuadPart - f0.QuadPart) * 1000.0 / (double)qf.QuadPart;
            double target = 1000.0 / (double)std::clamp(C.overlayFps, 20.f, 240.f);
            if (ms < target) Sleep((DWORD)(target - ms));
        }
    }

    gRun = false;
    gGameActive = false;
    gWork = false;
    if (started) { scan.join(); mb.join(); loc.join(); afk.join(); }
    if (gTrigDown) MouseButton(false);
    if (gLicensed && C.autoSave) SaveCfg();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (gRtv) gRtv->Release();
    if (gSwap) gSwap->Release();
    if (gCtx) gCtx->Release();
    if (gDev) gDev->Release();
    DestroyWindow(hw);
    UnregisterClassW(wc.lpszClassName, hi);
    if (M.hw) CloseHandle(M.hw);
    if (M.h) CloseHandle(M.h);
    timeEndPeriod(1);
    return 0;
}
