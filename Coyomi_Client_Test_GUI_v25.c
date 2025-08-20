//* UIProcesMainの分離

// 機能としてはInvalidValueHandlingを利用してエラー処理
// ConnectToCoyomiを利用してサーバーとの通信が出来るか確かめる
// UIntReciveを使用して通信を行う．
// WinProcというモジュールにReceveDataを渡す．
// モジュール名：UIProcesMain
// 関数名：UIProcesMain_ UIProcesMain

// Coyomi_Client_Test_GUI_v25.c
// ビルド例：
// gcc Coyomi_Client_Test_GUI_v25.c CreateWindow.c CollectData.c InvalidValueHandling.c \
//     ConnectToCoyomi.c UISentRecive.c ShowResultStatus.c UIProcesMain.c \
//     -o Coyomi_Client_Test_v25.exe -lcomctl32 -lws2_32 -mwindows -municode \
//     -std=c11 -O2 -Wall -Wextra -Wpedantic -finput-charset=CP932 \
//     -fexec-charset=CP932 -fwide-exec-charset=UTF-16LE

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

// include順: winsock2 → ws2tcpip → windows
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>

#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>
#include <string.h>

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof((a)[0]))
#endif

// プロジェクト共通（1枚ヘッダ）
#include "coyomi_all.h"

//==============================
// 接続先
//==============================
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 5000

//==============================
// グローバル
//==============================
GUIHandles g_gui;
static HWND g_main_hwnd = NULL;

// wire末尾に載せる旧互換「最後のエラーID」
ErrorID g_error_id = ERR_NONE;

// エラーフラグ（0:未発生, 1:発生）※ShowResultStatus_Status に渡す
static uint32_t Error_ID_01 = 0;
static uint32_t Error_ID_02 = 0;
static uint32_t Error_ID_03 = 0;
static uint32_t Error_ID_04 = 0;
static uint32_t Error_ID_05 = 0;
static uint32_t Error_ID_06 = 0;
static uint32_t Error_ID_07 = 0;

static void reset_error_flags(void){
    Error_ID_01 = Error_ID_02 = Error_ID_03 = 0;
    Error_ID_04 = Error_ID_05 = Error_ID_06 = 0;
    Error_ID_07 = 0;
    g_error_id  = ERR_NONE;
}

// UIProcesMain / InvalidValueHandling / ConnectToCoyomi / UISentRecive からの通知を0→1反映
static void OnErrorRaised(int id){
    if (id <= 0) return;
    switch ((ErrorID)id){
        case ERR_ID1_CONNECT:            Error_ID_01 = 1; break;
        case ERR_ID2_START_OUT_OF_RANGE: Error_ID_02 = 1; break;
        case ERR_ID3_POSITIVE_ONLY:      Error_ID_03 = 1; break;
        case ERR_ID4_ROUNDUP:            Error_ID_04 = 1; break;
        case ERR_ID5_CAP_10000:          Error_ID_05 = 1; break;
        case ERR_ID6_INVALID_INT:        Error_ID_06 = 1; break;
        case ERR_ID7_SEND_RECV_FAIL:     Error_ID_07 = 1; break;
        default: break;
    }
    if (g_error_id == ERR_NONE) g_error_id = (ErrorID)id;
}

//==============================
// GUI（固定サイズ）
//==============================
static int   g_winW = 0;
static int   g_winH = 0;
static DWORD g_fixedStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

static void ComputeWindowSize(void){
    int clientW = (int)((long long)GUI_SIZE * GUI_RATIO_Y / GUI_RATIO_X);
    RECT rc = {0, 0, clientW, GUI_SIZE};
    AdjustWindowRectEx(&rc, g_fixedStyle, FALSE, 0);
    g_winW = rc.right - rc.left;
    g_winH = rc.bottom - rc.top;
}

#define APP_CLASS     L"CoyomiClientWnd"
#define WM_APP_RESULT (WM_APP + 1)  // UIProcesMain が結果ポインタを返す

static volatile LONG g_alive = 1;

static void set_status(LPCWSTR s){
    if (g_gui.hGUI_Message) SetWindowTextW(g_gui.hGUI_Message, s);
}

//==============================
// 先行宣言
//==============================
static void start_request(HWND hwnd);

//==============================
// WndProc
//==============================
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch (msg){
    case WM_CREATE:
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_SEND){
            start_request(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_EXIT){
            SendMessageW(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }

    case WM_GETMINMAXINFO: {
        LPMINMAXINFO mi = (LPMINMAXINFO)lParam;
        ComputeWindowSize();
        mi->ptMinTrackSize.x = g_winW;
        mi->ptMinTrackSize.y = g_winH;
        mi->ptMaxTrackSize.x = g_winW;
        mi->ptMaxTrackSize.y = g_winH;
        return 0;
    }

    case WM_APP_RESULT: {
        // UIProcesMain からの結果ポインタ（coyomi_all.h で宣言されている構造体）
        UIProcesMain_Result* res = (UIProcesMain_Result*)lParam;

        // エラーフラグを ShowResultStatus に渡して表示更新
        ErrorFlags ef = {
            .id1_connect            = Error_ID_01,
            .id2_start_out_of_range = Error_ID_02,
            .id3_positive_only      = Error_ID_03,
            .id4_roundup            = Error_ID_04,
            .id5_cap_10000          = Error_ID_05,
            .id6_invalid_int        = Error_ID_06,
            .id7_send_recv_fail     = Error_ID_07,
        };
        const bool has_comm_error = (ef.id1_connect || ef.id7_send_recv_fail) ? true : false;

        // 結果欄（通信エラーが無ければ曜日付きで出力）
        ShowResultStatus_Result(
            &g_gui,
            &res->recv,
            res->start_y, res->start_m, res->start_d,
            res->ndays_ui,
            res->dir_after,
            has_comm_error
        );

        // メッセージ欄（発生エラーIDを列挙して表示）
        ShowResultStatus_Status(&g_gui, &ef);

        // もう押せる
        EnableWindow(g_gui.hGUI_Execution, TRUE);

        // UIProcesMain が malloc した結果を解放
        free(res);
        return 0;
    }

    case WM_CLOSE:
        InterlockedExchange(&g_alive, 0);
        EnableWindow(g_gui.hGUI_Execution, FALSE);
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

//==============================
// リクエスト開始（処理は UIProcesMain へ丸投げ）
//==============================
static void start_request(HWND hwnd){
    (void)hwnd;

    if (g_gui.hGUI_Message) SetWindowTextW(g_gui.hGUI_Message, L"");
    if (g_gui.hGUI_Result)  SetWindowTextW(g_gui.hGUI_Result,  L"");

    reset_error_flags();

    // ボタンを無効化（結果が返ったら WndProc で有効化）
    EnableWindow(g_gui.hGUI_Execution, FALSE);

    // ここで UIProcesMain に実務を委譲（内部で InvalidValueHandling / ConnectToCoyomi / UISentRecive を使用）
    // UIProcesMain は必要なら DTP を今日に更新し、スレッドで送受信し、完了時に WM_APP_RESULT で結果を返します。
    UIProcesMain_UIProcesMain(
        &g_gui,
        g_main_hwnd,
        SERVER_IP,
        SERVER_PORT,
        WM_APP_RESULT,
        OnErrorRaised  // 0→1 更新用コールバック
    );
}

//==============================
// エントリポイント
//==============================
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR lpCmdLine, int nShow){
    UNREFERENCED_PARAMETER(hPrev);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Winsock 初期化（ConnectToCoyomi / UISentRecive の前提）
    WSADATA w;
    if (WSAStartup(MAKEWORD(2,2), &w) != 0){
        MessageBoxW(NULL, L"WSAStartup failed", L"Error", MB_ICONERROR);
        return 1;
    }

    // DTP など共通コントロール
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_DATE_CLASSES };
    InitCommonControlsEx(&icc);

    // 親＋子の一括作成
    HWND hwnd = CreateWindow_CreateWindow(
        &g_gui,
        hInst,
        WndProc,
        APP_CLASS,
        L"Coyomi Client (Windows API)",
        g_fixedStyle,
        0
    );
    if (!hwnd) {
        MessageBoxW(NULL, L"CreateWindow failed", L"Error", MB_ICONERROR);
        WSACleanup();
        return 1;
    }
    g_main_hwnd = hwnd;

    // 表示
    CreateWindow_ShowWindow(&g_gui, nShow);

    // メッセージループ
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)){
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    WSACleanup();
    return 0;
}
