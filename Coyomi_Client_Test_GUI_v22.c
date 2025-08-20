//* ConnectToCoyomi.c

//*今回は ConnectToCoyomiモジュールの切り離し
//*切り離し順は Main CreateWindow CollectData InvalidValueHandling ConnectToCoyomi

//使用関数名
//ConnectToCoyomi_ ConnectToCoyomi
//ConnectToCoyomi_ InvalidValueHandling

// Coyomi_Client_Test_GUI_v21.c
// 分離構成: Main / CreateWindow.c / CollectData.c / InvalidValueHandling.c / ConnectToCoyomi.c
// 送信データは InvalidValueHandling で確定した SentData をそのまま使用

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

// 1) winsock2 → 2) ws2tcpip → 3) windows の順
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

// プロジェクト共通ヘッダ（1枚構成）
#include "coyomi_all.h"

//==============================
// (統合) Config
//==============================
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 5000

//==============================
// グローバル
//==============================
GUIHandles g_gui;
static HWND g_main_hwnd = NULL;

// 旧プロトコル互換: 最後のエラーID（Wireに含める）
ErrorID g_error_id = ERR_NONE;

// ご要望の 7 フラグ（0:未発生, 1:発生）
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

// InvalidValueHandling / ConnectToCoyomi からの通知を受け取って 0→1 更新
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
// Wire 相当（単純実装）
//==============================
static int send_all(SOCKET s, const char* buf, int len){
    int sent = 0;
    while (sent < len){
        int r = send(s, buf + sent, len - sent, 0);
        if (r <= 0) return -1;
        sent += r;
    }
    return sent;
}
static int recv_all(SOCKET s, char* buf, int len){
    int recvd = 0;
    while (recvd < len){
        int r = recv(s, buf + recvd, len - recvd, 0);
        if (r <= 0) return -1;
        recvd += r;
    }
    return recvd;
}

// SentData → wire（g_error_id を末尾 word に載せる）
static void pack_to_wire_from_sent(const SentData* s, uint32_t netbuf[PACKET_WORDS]){
    uint32_t t[PACKET_WORDS] = {
        s->year, s->month, s->day, s->ndays, s->flags, (uint32_t)g_error_id
    };
    for (int i=0;i<PACKET_WORDS;++i) netbuf[i] = htonl(t[i]);
}

// wire → ReceiveData（ついでに g_error_id を更新）
static void unpack_from_wire_to_recv(ReceiveData* r, const uint32_t netbuf[PACKET_WORDS]){
    uint32_t t[PACKET_WORDS];
    for (int i=0;i<PACKET_WORDS;++i) t[i] = ntohl(netbuf[i]);
    r->year  = t[0];
    r->month = t[1];
    r->day   = t[2];
    r->ndays = t[3];
    r->flags = t[4];
    g_error_id = (ErrorID)t[5];
}

//==============================
// Network 相当（connect は ConnectToCoyomi に委譲）
//==============================
static int net_send_data(SOCKET s, const SentData* sd){
    uint32_t netbuf[PACKET_WORDS];
    pack_to_wire_from_sent(sd, netbuf);
    return send_all(s, (const char*)netbuf, (int)sizeof(netbuf));
}
static int net_recv_data(SOCKET s, ReceiveData* rd){
    uint32_t netbuf[PACKET_WORDS];
    int r = recv_all(s, (char*)netbuf, (int)sizeof(netbuf));
    if (r <= 0) return r;
    unpack_from_wire_to_recv(rd, netbuf);
    return r;
}

//==============================
// GUI（サイズ固定）
//==============================
static int   g_winW = 0;
static int   g_winH = 0;
static DWORD g_fixedStyle= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

static void ComputeWindowSize(void){
    int clientW = (int)((long long)GUI_SIZE * GUI_RATIO_Y / GUI_RATIO_X);
    RECT rc = {0, 0, clientW, GUI_SIZE};
    AdjustWindowRectEx(&rc, g_fixedStyle, FALSE, 0);
    g_winW = rc.right - rc.left;
    g_winH = rc.bottom - rc.top;
}

#define APP_CLASS     L"CoyomiClientWnd"
#define WM_APP_RESULT (WM_APP + 1)

static HANDLE g_hWorker = NULL;
static volatile LONG g_alive = 1;

static const wchar_t* weekday_str_ja(int w){
    static const wchar_t* tbl[]={L"日曜日",L"月曜日",L"火曜日",L"水曜日",L"木曜日",L"金曜日",L"土曜日"};
    return (w>=0 && w<7)? tbl[w] : L"?";
}
static inline short flags_get_weekday(uint32_t f){ return (short)((f & FLAG_WD_MASK) >> FLAG_WD_SHIFT); }

static void set_status(LPCWSTR s){
    if(g_gui.hGUI_Message) SetWindowTextW(g_gui.hGUI_Message, s);
}

//==============================
// メッセージ整形（表示）
//==============================
static inline const wchar_t* error_body_from_id(ErrorID id){
    switch(id){
        case ERR_ID1_CONNECT:            return MSG_ERR_ID1;
        case ERR_ID2_START_OUT_OF_RANGE: return MSG_ERR_ID2;
        case ERR_ID3_POSITIVE_ONLY:      return MSG_ERR_ID3;
        case ERR_ID4_ROUNDUP:            return MSG_ERR_ID4;
        case ERR_ID5_CAP_10000:          return MSG_ERR_ID5;
        case ERR_ID6_INVALID_INT:        return MSG_ERR_ID6;
        case ERR_ID7_SEND_RECV_FAIL:     return MSG_ERR_ID7;
        default:                         return NULL;
    }
}
static void append_line_char(char* buf, size_t cap, const char* line){
    if(!buf || !line || cap==0) return;
    size_t cur = strlen(buf);
    if (cur >= cap-1) return;
    _snprintf(buf+cur, cap-cur, "%s%s", (cur? "\r\n":""), line);
}
static void add_error_to_charbuf(ErrorID id, char* out, size_t cap){
    const wchar_t* wmsg = error_body_from_id(id);
    if (!wmsg) return;

    char msg_utf8[800];
    int n = WideCharToMultiByte(CP_UTF8, 0, wmsg, -1, msg_utf8, (int)sizeof(msg_utf8), NULL, NULL);
    if (n <= 0) return;

    char line[840];
    _snprintf(line, sizeof(line), "ID%d\t%s", (int)id, msg_utf8);
    append_line_char(out, cap, line);
}
static void build_error_messages_char(char out[1024]){
    out[0] = '\0';
    if (Error_ID_01) add_error_to_charbuf(ERR_ID1_CONNECT, out, 1024);
    if (Error_ID_02) add_error_to_charbuf(ERR_ID2_START_OUT_OF_RANGE, out, 1024);
    if (Error_ID_03) add_error_to_charbuf(ERR_ID3_POSITIVE_ONLY, out, 1024);
    if (Error_ID_04) add_error_to_charbuf(ERR_ID4_ROUNDUP, out, 1024);
    if (Error_ID_05) add_error_to_charbuf(ERR_ID5_CAP_10000, out, 1024);
    if (Error_ID_06) add_error_to_charbuf(ERR_ID6_INVALID_INT, out, 1024);
    if (Error_ID_07) add_error_to_charbuf(ERR_ID7_SEND_RECV_FAIL, out, 1024);
}

//==============================
// スレッド→GUI 連絡用
//==============================
typedef struct {
    SentData    send;          // 送信（InvalidValueHandling で確定済み）
    ReceiveData recv;          // 受信
    uint32_t start_y, start_m, start_d; // 表示用：開始日（確定後）
    uint32_t ndays_ui;                       // 表示用：日数
    bool     dir_after;                      // 表示用：方向（flagsから算出でもOK）
    wchar_t  errbuf[1024];                   // 予備（未使用でも可）
} ClientResult;

static void show_result_to_edit(const ClientResult* r){
    if (!g_gui.hGUI_Result) return;

    const wchar_t* dir = r->dir_after ? L"日後" : L"日前";
    short wd = (short)flags_get_weekday(r->recv.flags);

    wchar_t line1[64], line2[64], line3[96], msg[320];
    swprintf(line1, 64, L"開始日：%04u/%02u/%02u", r->start_y, r->start_m, r->start_d);
    swprintf(line2, 64, L"日数：%u%s", r->ndays_ui, dir);
    swprintf(line3, 96, L"該当日： %04u/%02u/%02u（%s）",
             r->recv.year, r->recv.month, r->recv.day, weekday_str_ja((int)wd));
    swprintf(msg, 320, L"%s\r\n%s\r\n%s", line1, line2, line3);
    SetWindowTextW(g_gui.hGUI_Result, msg);

    char  errbuf8[1024];
    build_error_messages_char(errbuf8);

    if (errbuf8[0]){
        wchar_t errbufW[1024];
        int wn = MultiByteToWideChar(CP_UTF8, 0, errbuf8, -1, errbufW, _countof(errbufW));
        if (wn > 0) set_status(errbufW);
        else        set_status(L"(エラー文字列の変換に失敗しました)");
    }else{
        set_status(L"");
    }
}

//==============================
// 実行(送受信)スレッド
//==============================
static unsigned __stdcall SendThread(void* arg){
    ClientResult* r = (ClientResult*)arg;

    // 送信直前に「最後の1件」を初期化（連続送受信失敗はスレッド内で更新）
    g_error_id = ERR_NONE;

    // まず接続確認：失敗時は ERR_ID1_CONNECT を OnErrorRaised が立てる
    SOCKET s = ConnectToCoyomi_ConnectToCoyomi(SERVER_IP, SERVER_PORT, OnErrorRaised);
    if (s == INVALID_SOCKET) {
        // 失敗: そのままGUIへ
    } else {
        int fail_sr = 0;
        for (;;){
            if (net_send_data(s, &r->send) > 0 && net_recv_data(s, &r->recv) > 0){
                break; // OK
            }
            fail_sr++;
            if (fail_sr >= 5){
                OnErrorRaised(ERR_ID7_SEND_RECV_FAIL);
                break;
            }
            closesocket(s);
            s = ConnectToCoyomi_ConnectToCoyomi(SERVER_IP, SERVER_PORT, OnErrorRaised);
            if (s == INVALID_SOCKET){
                break;
            }
        }
        if (s != INVALID_SOCKET) closesocket(s);
    }

    if (g_alive && IsWindow(g_main_hwnd)) {
        PostMessageW(g_main_hwnd, WM_APP_RESULT, 0, (LPARAM)r);
    } else {
        free(r);
    }
    _endthreadex(0);
    return 0;
}

//==============================
// 先行宣言
//==============================
static void start_request(HWND hwnd);

//==============================
// WndProc
//==============================
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
    case WM_CREATE:
        return 0;

    case WM_COMMAND:
        if(LOWORD(wParam)==IDC_SEND){
            start_request(hwnd);
            return 0;
        }
        if(LOWORD(wParam)==IDC_EXIT){
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

    case WM_APP_RESULT:{
        ClientResult* res = (ClientResult*)lParam;

        show_result_to_edit(res);
        EnableWindow(g_gui.hGUI_Execution, TRUE);

        if (g_hWorker){ CloseHandle(g_hWorker); g_hWorker = NULL; }

        free(res);
        return 0;
    }

    case WM_CLOSE:
        InterlockedExchange(&g_alive, 0);
        EnableWindow(g_gui.hGUI_Execution, FALSE);

        if (g_hWorker){
            WaitForSingleObject(g_hWorker, 5000);
            CloseHandle(g_hWorker);
            g_hWorker = NULL;
        }
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd,msg,wParam,lParam);
}

//==============================
// リクエスト開始（Main からは検証せず、モジュールに委譲）
//==============================
static void start_request(HWND hwnd){
    (void)hwnd;

    if (g_gui.hGUI_Message) SetWindowTextW(g_gui.hGUI_Message, L"");
    if (g_gui.hGUI_Result)  SetWindowTextW(g_gui.hGUI_Result,  L"");

    reset_error_flags();

    // 1) そのまま入力収集
    InputData iv;
    CollectData_CollectData(&g_gui, &iv);

    // 2) 検証・補正は InvalidValueHandling に任せる（出力は最終送信 SentData）
    ClientResult* cr = (ClientResult*)calloc(1, sizeof(ClientResult));
    if(!cr){
        set_status(L"(内部エラー) メモリ確保に失敗しました。");
        return;
    }

    bool setDtpToday = false;
    InvalidValueHandling_InvalidValueHandling(
        &iv,                 // in
        &cr->send,           // out（最終送信データ）
        &setDtpToday,        // DTP を今日に合わせるべきか
        NULL, 0,             // ここではメッセージ組み立ては不要
        OnErrorRaised        // 0→1 更新用コールバック
    );

    // 表示用の補助情報（開始日 / 日数 / 前後）
    cr->start_y   = cr->send.year;
    cr->start_m   = cr->send.month;
    cr->start_d   = cr->send.day;
    cr->ndays_ui  = cr->send.ndays;
    cr->dir_after = ((cr->send.flags & FLAG_DIR_MASK) != 0);

    // 必要なら DTP を今日に
    if (setDtpToday){
        SYSTEMTIME now; InvalidValueHandling_GetTodayTime(&now);
        SendMessageW(g_gui.hGUI_Calendar, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&now);
    }

    // 3) 実行スレッド起動
    EnableWindow(g_gui.hGUI_Execution, FALSE);

    HANDLE th = (HANDLE)_beginthreadex(NULL,0,SendThread,cr,0,NULL);
    if(th){
        g_hWorker = th;
    }else{
        set_status(L"(内部エラー) スレッド起動に失敗しました。");
        free(cr);
        EnableWindow(g_gui.hGUI_Execution, TRUE);
    }
}

//==============================
// エントリポイント
//==============================
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR lpCmdLine, int nShow){
    UNREFERENCED_PARAMETER(hPrev);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Winsock 初期化（ConnectToCoyomi は前提とします）
    WSADATA w;
    if (WSAStartup(MAKEWORD(2,2), &w) != 0){
        MessageBoxW(NULL,L"WSAStartup failed",L"Error",MB_ICONERROR);
        return 1;
    }

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_DATE_CLASSES }; // DTP 用
    InitCommonControlsEx(&icc);

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

    CreateWindow_ShowWindow(&g_gui, nShow);

    MSG msg;
    while(GetMessageW(&msg,NULL,0,0)){
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    WSACleanup();
    return 0;
}
