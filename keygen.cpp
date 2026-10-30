// keygen.cpp - gerador de keys (SO PARA VOCE; nao distribua o AimerKeygen.exe)
// Uso:  AimerKeygen.exe <HWID> [dias]     (dias = 0 -> vitalicia)
//       AimerKeygen.exe me [dias]         (gera key para ESTE pc)
#include "license.h"
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    std::string hw;
    int days = 0;
    if (argc >= 2) hw = argv[1];
    else {
        printf("HWID do cliente (ou 'me' para este PC): ");
        char b[128]{};
        if (!fgets(b, sizeof(b), stdin)) return 1;
        hw = b;
        while (!hw.empty() && (hw.back() == '\n' || hw.back() == '\r')) hw.pop_back();
    }
    if (argc >= 3) days = atoi(argv[2]);
    else {
        printf("Dias de validade (0 = vitalicia): ");
        char b[32]{};
        if (fgets(b, sizeof(b), stdin)) days = atoi(b);
    }
    std::string clean = (hw == "me" || hw == "ME") ? LicHwid() : LicClean(hw);
    if (clean.size() != 16) {
        printf("\nHWID invalido (precisa ter 16 caracteres, ex: ABCD-EFGH-JKLM-NPQR).\n");
        system("pause");
        return 1;
    }
    std::string key = LicMakeKey(clean, days);
    printf("\nHWID : %s\n", LicFmt(clean).c_str());
    printf("Dias : %s\n", days > 0 ? std::to_string(days).c_str() : "vitalicia");
    printf("KEY  : %s\n\n", key.c_str());
    system("pause");
    return 0;
}
