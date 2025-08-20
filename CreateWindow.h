//*CreateWindow.h
//親ウィンドウ＋子コントロールを一括生成が絶対に必要な変更事項
//変更前はWinMainモジュールの中で親windowを生成してしまっていた．
//引数
//返値
//*ShowWindoeモジュールが中に入ってしまっているが後で切り離す．（誰かお願い，助けて）

#pragma once
#include <windows.h>

// 画面中の全コントロールのハンドルを束ねる構造体
typedef struct GUIHandles {
    HWND hMain;

    // ラベル類
    HWND hGUI_Comment_1, hGUI_Comment_2, hGUI_Comment_3, hGUI_Comment_4;
    HWND hGUI_Comment_5, hGUI_Comment_6, hGUI_Comment_7, hGUI_Comment_8;
    HWND hGUI_Comment_9;

    // 入力系
    HWND hGUI_Calendar;
    HWND hGUI_ShiftDays;

    // ラジオ
    HWND hGUI_RadioBox_Zero, hGUI_RadioBox_One;
    HWND hGUI_RadioBox_Before, hGUI_RadioBox_After;

    // ボタン
    HWND hGUI_Execution, hGUI_End;

    // 出力欄
    HWND hGUI_Message, hGUI_Result;
} GUIHandles;

// コントロールID（Main の WM_COMMAND でも使うので .h に置く）
// enum {
//     IDC_NDAYS   = 1100,
//     IDC_DIR_BACK= 1101,
//     IDC_DIR_FWD = 1102,
//     IDC_FLAG0   = 1103,
//     IDC_FLAG1   = 1104,
//     IDC_SEND    = 1105,
//     IDC_OUTALL  = 1106,
//     IDC_STATUS  = 1107,
//     IDC_EXIT    = 1108,
//     IDC_DTP     = 1109
// };

// 親ウィンドウ＋子コントロールをまとめて生成
HWND CreateWindow_CreateWindow(
    GUIHandles* gui,
    HINSTANCE   hInst,
    WNDPROC     wndProc,
    LPCWSTR     className,
    LPCWSTR     windowTitle,
    DWORD       style,
    DWORD       exStyle
);

// 表示（ShowWindow/UpdateWindow）
void CreateWindow_ShowWindow(GUIHandles* gui, int nShow);
