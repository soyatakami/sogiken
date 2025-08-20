//* CollectData.c
// 画面に入力されている値を、そのまま InputData に格納するだけ。
// - DTP の有効/無効を判定しない（結果コードも返さない）
// - 検証や補正は行わない（別モジュールで実施）

//*InputData構造体の初期化をしてしまっている．初期化はWinProcで行うので移す必要性有

#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>   // DTM_GETSYSTEMTIME / BM_GETCHECK など
#include "coyomi_all.h"

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof((a)[0]))
#endif

void CollectData_CollectData(const GUIHandles* gui, InputData* out_iv)
{
    if (!out_iv) return;              // 値を詰める先が無ければ何もしない
    ZeroMemory(out_iv, sizeof(*out_iv));

    if (!gui) return;                 // 画面ハンドル束が無ければ終了（判定は最小限）

    // --- 日付（DTP） ---
    // DTP の有効/無効は「判定しない」。取得して、その値をそのまま使う。
    // 取得が無効であっても分岐せず、初期化済みの 0 が入るだけ。////この実装で良いのか疑問に感じる
    SYSTEMTIME st = {0};
    if (gui->hGUI_Calendar) {
        SendMessageW(gui->hGUI_Calendar, DTM_GETSYSTEMTIME, 0, (LPARAM)&st);
        out_iv->year  = st.wYear;
        out_iv->month = st.wMonth;
        out_iv->day   = st.wDay;
    }

    // --- 日数テキスト（EDIT → UTF-8 へ） ---
    if (gui->hGUI_ShiftDays) {
        wchar_t wbuf[64] = L"";
        GetWindowTextW(gui->hGUI_ShiftDays, wbuf, _countof(wbuf));
        int n = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1,
                                    out_iv->ndays_text, (int)sizeof(out_iv->ndays_text),
                                    NULL, NULL);
        if (n <= 0) {
            // 変換失敗時も検証はしない。空文字にしておく。
            out_iv->ndays_text[0] = '\0';
        }
    }

    // --- ラジオボタン状態（前/後, 0/1日目） ---
    // 判定というより単純代入：チェックされていれば true。
    if (gui->hGUI_RadioBox_After) {
        out_iv->dir_after = (SendMessageW(gui->hGUI_RadioBox_After, BM_GETCHECK, 0, 0) == BST_CHECKED);
    }
    if (gui->hGUI_RadioBox_One) {
        out_iv->flag01 = (SendMessageW(gui->hGUI_RadioBox_One, BM_GETCHECK, 0, 0) == BST_CHECKED);
    }
}
