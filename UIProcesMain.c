// 機能としてはInvalidValueHandlingを利用してエラー処理
// ConnectToCoyomiを利用してサーバーとの通信が出来るか確かめる
// UIntReciveを使用して通信を行う．
// WinProcというモジュールにReceveDataを渡す．
// モジュール名：UIProcesMain
// 関数名：UIProcesMain_ UIProcesMain

// UIProcesMain.c
// 入力収集 → 検証補正 → 接続確認 → 送受信 を担当し、完了時に WndProc へ結果を投げる。

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

/* winsock2.h は windows.h より前に */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <stdlib.h>

#include "coyomi_all.h"

/* 接続先（必要ならビルド時に -DSERVER_IP="..." -DSERVER_PORT=... で上書き可） */
#ifndef SERVER_IP
#define SERVER_IP   "127.0.0.1"
#endif
#ifndef SERVER_PORT
#define SERVER_PORT 5000
#endif

/* スレッド引数 */
typedef struct {
    HWND          hwnd_main;
    ClientResult* cr;
    ErrorRaiseCb  on_error;
} UIM_ThreadArg;

/* 送受信スレッド本体 */
static unsigned __stdcall UIM_SendThread(void* p)
{
    UIM_ThreadArg* arg = (UIM_ThreadArg*)p;
    ClientResult*  cr  = arg->cr;
    ErrorRaiseCb   on_error = arg->on_error;
    HWND           hwnd = arg->hwnd_main;

    /* 旧互換：wire 末尾に載せる「最後の1件」を開始時にクリア */
    g_error_id = ERR_NONE;

    /* まず接続確認（失敗時は ConnectToCoyomi 内で ERR_ID1_CONNECT を通知） */
    COYOMI_SOCKET cs = ConnectToCoyomi_ConnectToCoyomi(SERVER_IP, SERVER_PORT, on_error);
    if (cs != (COYOMI_SOCKET)INVALID_SOCKET) {
        int retry = 0;
        for (;;) {
            if (UIntRecive_Sent(cs, &cr->send, on_error) > 0 &&
                UIntRecive_Recive(cs, &cr->recv, on_error) > 0) {
                break; /* 通信成功 */
            }
            if (++retry >= 5) {                 /* 5連続失敗 */
                if (on_error) on_error(ERR_ID7_SEND_RECV_FAIL);
                break;
            }
            /* 再接続 */
            closesocket((SOCKET)cs);
            cs = ConnectToCoyomi_ConnectToCoyomi(SERVER_IP, SERVER_PORT, on_error);
            if (cs == (COYOMI_SOCKET)INVALID_SOCKET) {
                /* Connect できない場合、Connect 側で ERR_ID1_CONNECT 通知済み */
                break;
            }
        }
        if (cs != (COYOMI_SOCKET)INVALID_SOCKET) {
            closesocket((SOCKET)cs);
        }
    }
    /* cs が INVALID なら接続失敗。on_error は Connect 側ですでに通知済み */

    /* WndProc へ結果ポスト（失敗したらこちらで free） */
    if (!PostMessageW(hwnd, WM_APP_RESULT, 0, (LPARAM)cr)) {
        free(cr);
    }

    free(arg);
    _endthreadex(0);
    return 0;
}

/* 公開API：非同期に 収集→検証補正→接続確認→送受信 を実行開始 */
HANDLE UIProcesMain_UIProcesMain(
    const GUIHandles* gui,
    HWND              hwnd_main,
    ErrorRaiseCb      on_error
){
    if (!gui || !hwnd_main) return NULL;

    /* メッセージ/結果欄をクリア（存在すれば） */
    if (gui->hGUI_Message) SetWindowTextW(gui->hGUI_Message, L"");
    if (gui->hGUI_Result)  SetWindowTextW(gui->hGUI_Result,  L"");

    /* まず GUI から入力収集 */
    InputData iv = {0};
    CollectData_CollectData(gui, &iv);

    /* 結果ペイロードを用意（WndProc で free する前提） */
    ClientResult* cr = (ClientResult*)calloc(1, sizeof(ClientResult));
    if (!cr) return NULL;

    /* InvalidValueHandling で検証・補正（送信用 SentData を最終確定） */
    bool setDtpToday = false;
    InvalidValueHandling_InvalidValueHandling(
        &iv,               /* in  */
        &cr->send,         /* out: 最終送信データ */
        &setDtpToday,      /* DTP を今日に合わせるべきか */
        NULL, 0,           /* メッセージ文字列はここでは不要 */
        on_error           /* 0→1 更新用 */
    );

    /* 表示用の補助情報（開始日/日数/前後）を詰めておく */
    cr->start_y   = cr->send.year;
    cr->start_m   = cr->send.month;
    cr->start_d   = cr->send.day;
    cr->ndays_ui  = cr->send.ndays;
    cr->dir_after = ((cr->send.flags & FLAG_DIR_MASK) != 0);

    /* 必要なら DTP を今日に寄せる */
    if (setDtpToday && gui->hGUI_Calendar) {
        SYSTEMTIME now;
        InvalidValueHandling_GetTodayTime(&now);
        SendMessageW(gui->hGUI_Calendar, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&now);
    }

    /* 実行ボタンがあれば無効化（押し直し防止。有効化は WndProc 側で） */
    if (gui->hGUI_Execution) EnableWindow(gui->hGUI_Execution, FALSE);

    /* スレッド起動（ハンドルは呼び元で保持しても捨ててもOK） */
    UIM_ThreadArg* arg = (UIM_ThreadArg*)calloc(1, sizeof(UIM_ThreadArg));
    if (!arg) { free(cr); return NULL; }

    arg->hwnd_main = hwnd_main;
    arg->cr        = cr;
    arg->on_error  = on_error;

    unsigned tid = 0;
    HANDLE hth = (HANDLE)_beginthreadex(NULL, 0, UIM_SendThread, arg, 0, &tid);
    if (!hth) {
        free(arg);
        free(cr);
        return NULL;
    }
    return hth;  /* 呼び元が Wait/Close したければ使う。不要なら無視でOK */
}
