// license.h - sistema de key atrelada ao HWID (offline, HMAC-SHA256)
// IMPORTANTE: troque LIC_SECRET por um texto seu antes de distribuir. Quem tiver este arquivo
// (ou o AimerKeygen.exe) consegue gerar keys, entao NAO distribua o keygen nem este arquivo.
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <intrin.h>
#include <ctime>
#include <cstdint>
#include <cstring>
#include <cctype>
#include <string>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")

// o SDK do runner nao expoe estas constantes com o nosso _WIN32_WINNT: definimos aqui
#ifndef RRF_RT_REG_SZ
#define RRF_RT_REG_SZ 0x00000002
#endif
#ifndef RRF_SUBKEY_WOW64_64KEY
#define RRF_SUBKEY_WOW64_64KEY 0x00010000
#endif

static const char LIC_SECRET[] = "e2a2770ca0cb69ce535b2fc33279dd1d";

static bool LicSha256(const std::string& data, uint8_t out[32]) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return false;
    NTSTATUS st = BCryptHash(alg, nullptr, 0, (PUCHAR)data.data(), (ULONG)data.size(), out, 32);
    BCryptCloseAlgorithmProvider(alg, 0);
    return st == 0;
}
static bool LicHmac(const std::string& data, uint8_t out[32]) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0) return false;
    NTSTATUS st = BCryptHash(alg, (PUCHAR)LIC_SECRET, (ULONG)strlen(LIC_SECRET),
                             (PUCHAR)data.data(), (ULONG)data.size(), out, 32);
    BCryptCloseAlgorithmProvider(alg, 0);
    return st == 0;
}

static const char LIC_B32[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

static std::string LicB32(const uint8_t* d, size_t n) {
    std::string s;
    uint32_t buf = 0;
    int bits = 0;
    for (size_t i = 0; i < n; i++) {
        buf = (buf << 8) | d[i];
        bits += 8;
        while (bits >= 5) { s += LIC_B32[(buf >> (bits - 5)) & 31]; bits -= 5; }
    }
    if (bits > 0) s += LIC_B32[(buf << (5 - bits)) & 31];
    return s;
}
static bool LicUnB32(const std::string& s, uint8_t* out, size_t n) {
    uint32_t buf = 0;
    int bits = 0;
    size_t o = 0;
    for (char ch : s) {
        if (!ch) return false;
        const char* p = strchr(LIC_B32, (int)std::toupper((unsigned char)ch));
        if (!p) return false;
        buf = (buf << 5) | (uint32_t)(p - LIC_B32);
        bits += 5;
        if (bits >= 8) {
            if (o < n) out[o++] = (uint8_t)((buf >> (bits - 8)) & 255);
            bits -= 8;
        }
    }
    return o == n;
}
static std::string LicClean(const std::string& in) {
    std::string k;
    for (char c : in) if (std::isalnum((unsigned char)c)) k += (char)std::toupper((unsigned char)c);
    return k;
}
static std::string LicFmt(const std::string& s) {
    std::string o;
    for (size_t i = 0; i < s.size(); i++) { if (i && i % 4 == 0) o += '-'; o += s[i]; }
    return o;
}

// HWID: 16 caracteres (sem hifens), derivado do MachineGuid + serial do volume do Windows + CPU
static std::string LicHwid() {
    std::string raw;
    char guid[128]{};
    DWORD sz = sizeof(guid);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", "MachineGuid",
                     RRF_RT_REG_SZ | RRF_SUBKEY_WOW64_64KEY, nullptr, guid, &sz) == ERROR_SUCCESS) raw += guid;
    char win[MAX_PATH]{};
    GetWindowsDirectoryA(win, MAX_PATH);
    char root[4] = { win[0], ':', '\\', 0 };
    DWORD vs = 0;
    GetVolumeInformationA(root, nullptr, 0, &vs, nullptr, nullptr, nullptr, 0);
    raw += "|" + std::to_string(vs) + "|";
    int r[4]{};
    char brand[49]{};
    __cpuid(r, (int)0x80000000);
    if ((unsigned)r[0] >= 0x80000004u) {
        for (int i = 0; i < 3; i++) { __cpuid(r, (int)(0x80000002 + i)); memcpy(brand + 16 * i, r, 16); }
    }
    raw += brand;
    uint8_t h[32];
    if (!LicSha256(raw, h)) return std::string(16, 'X');
    return LicB32(h, 10);
}

static int LicToday() { return (int)((long long)time(nullptr) / 86400LL) - 19723; }   // dias desde 2024-01-01

// days = 0 -> vitalicia
static std::string LicMakeKey(const std::string& hwid16, int days) {
    int exp = days > 0 ? LicToday() + days : 0;
    if (exp > 65535) exp = 65535;
    uint8_t mac[32];
    std::string msg = "AIMER|" + hwid16 + "|" + std::to_string(exp);
    if (!LicHmac(msg, mac)) return "";
    uint8_t raw[10];
    raw[0] = (uint8_t)(exp >> 8);
    raw[1] = (uint8_t)(exp & 255);
    memcpy(raw + 2, mac, 8);
    return LicFmt(LicB32(raw, 10));
}

enum { LIC_OK = 0, LIC_BAD_FORMAT = 1, LIC_WRONG_HWID = 2, LIC_EXPIRED = 3 };

static int LicCheck(const std::string& keyIn, const std::string& hwid16, int* daysLeft = nullptr) {
    std::string k = LicClean(keyIn);
    uint8_t raw[10];
    if (k.size() != 16 || !LicUnB32(k, raw, 10)) return LIC_BAD_FORMAT;
    int exp = (raw[0] << 8) | raw[1];
    uint8_t mac[32];
    std::string msg = "AIMER|" + hwid16 + "|" + std::to_string(exp);
    if (!LicHmac(msg, mac)) return LIC_BAD_FORMAT;
    if (memcmp(raw + 2, mac, 8) != 0) return LIC_WRONG_HWID;      // key invalida ou de outro PC
    if (exp != 0 && LicToday() > exp) return LIC_EXPIRED;
    if (daysLeft) *daysLeft = exp ? exp - LicToday() : -1;
    return LIC_OK;
}
