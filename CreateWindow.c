//*CreateWindow.c
//親ウィンドウ＋子コントロールを一括生成が絶対に必要な変更事項
//変更前はWinMainモジュールの中で親windowを生成してしまっていた．
//引数 
//返値 
//*ShowWindoeモジュールが中に入ってしまっているが後で切り離す．（誰かお願い，助けて）

//* CreateWindow.c
// 親ウィンドウ＋子コントロールを一括生成（フォントサイズ指定対応版）

#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

// #include <windows.h>
// #include <commctrl.h>
// #include <string.h>

// #include "CreateWindow.h"
// #include "coyomi_gui_consts.h"   // 座標/サイズ/GUI_SIZE, GUI_RATIO_X/Y, *_pt など
// #include "coyomi_messages_ja.h"  // GUI_Comment_* / ボタン文字列など

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>   // ← これが必須（DATETIMEPICK_CLASSW, DTM_*, GDT_* など）
#include "coyomi_all.h"

// ---------------- フォント適用（pt 指定） ----------------
// ※簡易実装：都度 CreateFontIndirectW し WM_SETFONT。必要ならキャッシュ化を検討。
static HFONT GetFontPt(HWND hwndRef, int pt, LPCWSTR face)
{
    if (pt <= 0) pt = 12; // フォールバック
    HDC hdc = GetDC(hwndRef);
    int dpi = hdc ? GetDeviceCaps(hdc, LOGPIXELSY) : 96;
    if (hdc) ReleaseDC(hwndRef, hdc);

    LOGFONTW lf = {0};
    lf.lfHeight = -MulDiv(pt, dpi, 72);        // pt → 論理高さ
    if (face && *face) {
        wcsncpy(lf.lfFaceName, face, LF_FACESIZE - 1);
    } else {
        wcsncpy(lf.lfFaceName, L"Meiryo UI", LF_FACESIZE - 1);
    }
    return CreateFontIndirectW(&lf);
}
#define APPLY_FONTPT(_HWND_, _PT_) do{ \
    if ((_HWND_) != NULL) { \
        HFONT __f = GetFontPt((_HWND_), (_PT_), L"Meiryo UI"); \
        SendMessageW((_HWND_), WM_SETFONT, (WPARAM)__f, TRUE); \
    } \
}while(0)

// ---------------- 子コントロール生成 ----------------
static void create_children(HWND hwnd, GUIHandles* gui)
{
    // 「開始日」ラベル
    gui->hGUI_Comment_1 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_1, WS_CHILD|WS_VISIBLE,
        GUI_Comment_1_Xp, GUI_Comment_1_Yp, GUI_Comment_1_Xs, GUI_Comment_1_Ys,
        hwnd, 0, 0, 0);
    APPLY_FONTPT(gui->hGUI_Comment_1, GUI_Comment_1_pt);

    // 日付(DTP)
    gui->hGUI_Calendar = CreateWindowExW(
        0, DATETIMEPICK_CLASSW, L"",
        WS_CHILD|WS_VISIBLE|DTS_SHORTDATEFORMAT,
        GUI_Calendar_Xp, GUI_Calendar_Yp, GUI_Calendar_Xs, GUI_Calendar_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_DTP,0,0);
    SYSTEMTIME st; GetLocalTime(&st);
    SendMessageW(gui->hGUI_Calendar, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    APPLY_FONTPT(gui->hGUI_Calendar, GUI_Calendar_pt);

    // 「有効範囲」ラベル（複数行）
    gui->hGUI_Comment_2 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_2,
        WS_CHILD|WS_VISIBLE|SS_LEFT|SS_EDITCONTROL|SS_NOPREFIX,
        GUI_Comment_2_Xp, GUI_Comment_2_Yp, GUI_Comment_2_Xs, GUI_Comment_2_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(gui->hGUI_Comment_2, GUI_Comment_2_pt);

    // 「開始日を」
    gui->hGUI_Comment_3 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_3, WS_CHILD|WS_VISIBLE,
        GUI_Comment_3_Xp, GUI_Comment_3_Yp, GUI_Comment_3_Xs, GUI_Comment_3_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(gui->hGUI_Comment_3, GUI_Comment_3_pt);

    // 0日目
    gui->hGUI_RadioBox_Zero = CreateWindowExW(
        0, L"BUTTON", GUI_RadioBox_Zero,
        WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON|WS_GROUP,
        GUI_RadioBox_Zero_Xp, GUI_RadioBox_Zero_Yp, GUI_RadioBox_Zero_Xs, GUI_RadioBox_Zero_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_FLAG0,0,0);
    APPLY_FONTPT(gui->hGUI_RadioBox_Zero, GUI_RadioBox_Zero_pt);

    // 1日目
    gui->hGUI_RadioBox_One = CreateWindowExW(
        0, L"BUTTON", GUI_RadioBox_One,
        WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
        GUI_RadioBox_One_Xp, GUI_RadioBox_One_Yp, GUI_RadioBox_One_Xs, GUI_RadioBox_One_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_FLAG1,0,0);
    APPLY_FONTPT(gui->hGUI_RadioBox_One, GUI_RadioBox_One_pt);

    SendMessageW(gui->hGUI_RadioBox_Zero, BM_SETCHECK, BST_CHECKED, 0); // 既定：0日目

    // 「とする」
    gui->hGUI_Comment_4 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_4, WS_CHILD|WS_VISIBLE,
        GUI_Comment_4_Xp, GUI_Comment_4_Yp, GUI_Comment_4_Xs, GUI_Comment_4_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(gui->hGUI_Comment_4, GUI_Comment_4_pt);

    // 「開始日の」
    gui->hGUI_Comment_5 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_5, WS_CHILD|WS_VISIBLE,
        GUI_Comment_5_Xp, GUI_Comment_5_Yp, GUI_Comment_5_Xs, GUI_Comment_5_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(gui->hGUI_Comment_5, GUI_Comment_5_pt);

    // 日数入力（ES_NUMBER は付けない）
    gui->hGUI_ShiftDays = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"7",
        WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
        GUI_ShiftDays_Xp, GUI_ShiftDays_Yp, GUI_ShiftDays_Xs, GUI_ShiftDays_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_NDAYS,0,0);
    APPLY_FONTPT(gui->hGUI_ShiftDays, GUI_ShiftDays_pt);

    // 「日」
    gui->hGUI_Comment_6 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_6, WS_CHILD|WS_VISIBLE,
        GUI_Comment_6_Xp, GUI_Comment_6_Yp, GUI_Comment_6_Xs, GUI_Comment_6_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(gui->hGUI_Comment_6, GUI_Comment_6_pt);

    // 前（ラジオ） 既定：前
    gui->hGUI_RadioBox_Before = CreateWindowExW(
        0, L"BUTTON", GUI_RadioBox_Before,
        WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON|WS_GROUP,
        GUI_RadioBox_Before_Xp, GUI_RadioBox_Before_Yp, GUI_RadioBox_Before_Xs, GUI_RadioBox_Before_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_DIR_BACK,0,0);
    APPLY_FONTPT(gui->hGUI_RadioBox_Before, GUI_RadioBox_Before_pt);

    // 後（ラジオ）
    gui->hGUI_RadioBox_After  = CreateWindowExW(
        0, L"BUTTON", GUI_RadioBox_After,
        WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
        GUI_RadioBox_After_Xp, GUI_RadioBox_After_Yp, GUI_RadioBox_After_Xs, GUI_RadioBox_After_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_DIR_FWD,0,0);
    APPLY_FONTPT(gui->hGUI_RadioBox_After, GUI_RadioBox_After_pt);

    SendMessageW(gui->hGUI_RadioBox_Before, BM_SETCHECK, BST_CHECKED, 0);

    // 注意書き
    gui->hGUI_Comment_7 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_7, WS_CHILD|WS_VISIBLE,
        GUI_Comment_7_Xp, GUI_Comment_7_Yp, GUI_Comment_7_Xs, GUI_Comment_7_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(gui->hGUI_Comment_7, GUI_Comment_7_pt);

    // 実行ボタン
    gui->hGUI_Execution = CreateWindowExW(
        0,L"BUTTON", GUI_Execution, WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
        GUI_Execution_Xp, GUI_Execution_Yp, GUI_Execution_Xs, GUI_Execution_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_SEND,0,0);
    APPLY_FONTPT(gui->hGUI_Execution, GUI_Execution_pt);

    // 終了ボタン
    gui->hGUI_End = CreateWindowExW(
        0,L"BUTTON", GUI_End, WS_CHILD|WS_VISIBLE,
        GUI_End_Xp, GUI_End_Yp, GUI_End_Xs, GUI_End_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_EXIT,0,0);
    APPLY_FONTPT(gui->hGUI_End, GUI_End_pt);

    // ラベル（メッセージ/結果）
    gui->hGUI_Comment_8 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_8, WS_CHILD|WS_VISIBLE,
        GUI_Comment_8_Xp, GUI_Comment_8_Yp, GUI_Comment_8_Xs, GUI_Comment_8_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(gui->hGUI_Comment_8, GUI_Comment_8_pt);

    gui->hGUI_Comment_9 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_9, WS_CHILD|WS_VISIBLE,
        GUI_Comment_9_Xp, GUI_Comment_9_Yp, GUI_Comment_9_Xs, GUI_Comment_9_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(gui->hGUI_Comment_9, GUI_Comment_9_pt);

    // メッセージEDIT（初期は空）
    gui->hGUI_Message = CreateWindowExW(
        WS_EX_CLIENTEDGE,L"EDIT", L"",
        WS_CHILD|WS_VISIBLE|ES_READONLY|ES_MULTILINE|ES_AUTOVSCROLL,
        GUI_Message_Xp, GUI_Message_Yp, GUI_Message_Xs, GUI_Message_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_STATUS,0,0);
    APPLY_FONTPT(gui->hGUI_Message, GUI_Message_pt);

    // まとめ出力EDIT
    gui->hGUI_Result = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_WANTRETURN,
        GUI_OutAll_Xp, GUI_OutAll_Yp, GUI_OutAll_Xs, GUI_OutAll_Ys,
        hwnd, (HMENU)(INT_PTR)IDC_OUTALL, 0, 0);
    APPLY_FONTPT(gui->hGUI_Result, GUI_OutAll_pt);
}

// ---------------- 親ウィンドウ生成（＋子作成） ----------------
HWND CreateWindow_CreateWindow(
    GUIHandles* gui,
    HINSTANCE   hInst,
    WNDPROC     wndProc,
    LPCWSTR     className,
    LPCWSTR     windowTitle,
    DWORD       style,
    DWORD       exStyle
){
    if(!gui) return NULL;
    ZeroMemory(gui, sizeof(*gui));

    // クラス登録
    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = wndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = className;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    if(!RegisterClassW(&wc) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS){
        return NULL;
    }

    // クライアント基準 → 外形サイズへ
    int clientW = (int)((long long)GUI_SIZE * GUI_RATIO_Y / GUI_RATIO_X);
    RECT rc = {0, 0, clientW, GUI_SIZE};
    AdjustWindowRectEx(&rc, style, FALSE, exStyle);

    // 親ウィンドウ生成
    HWND h = CreateWindowExW(
        exStyle, className, windowTitle, style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInst, NULL
    );
    if(!h) return NULL;
    gui->hMain = h;

    // 子コントロール生成
    create_children(h, gui);

    return h;
}

// ---------------- 表示 ----------------
void CreateWindow_ShowWindow(GUIHandles* gui, int nShow)
{
    if(!gui || !gui->hMain) return;
    ShowWindow(gui->hMain, nShow);
    UpdateWindow(gui->hMain);
}
