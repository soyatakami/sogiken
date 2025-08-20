// ShowResultStatus������

// Coyomi_Client_Test_GUI_v22.c
// �\��: Main / CreateWindow.c / CollectData.c / InvalidValueHandling.c / ConnectToCoyomi.c / UIntRecive.c / ShowResultStatus.c
// �r���h��F
// gcc Coyomi_Client_Test_GUI_v22.c CreateWindow.c CollectData.c InvalidValueHandling.c ConnectToCoyomi.c UIntRecive.c ShowResultStatus.c \
//   -o Coyomi_Client_Test_v22.exe -lcomctl32 -lws2_32 -mwindows -municode -std=c11 -O2 \
//   -Wall -Wextra -Wpedantic -finput-charset=CP932 -fexec-charset=CP932 -fwide-exec-charset=UTF-16LE

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

// 1) winsock2 �� 2) ws2tcpip �� 3) windows
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

// ���ʃw�b�_�i1���j
#include "coyomi_all.h"

//==============================
// �ݒ�
//==============================
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 5000

//==============================
// �O���[�o��
//==============================
GUIHandles g_gui;
static HWND g_main_hwnd = NULL;

// wire ���� word �ɍڂ���u�Ō��1���v�G���[�i���݊��j
ErrorID g_error_id = ERR_NONE;

// 7��̔����t���O�i0:������, 1:�����j
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

// InvalidValueHandling / ConnectToCoyomi / UIntRecive ����ʒm����� 0��1 �X�V
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
// GUI�i�T�C�Y�Œ�j
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

static void set_status(LPCWSTR s){
    if(g_gui.hGUI_Message) SetWindowTextW(g_gui.hGUI_Message, s);
}

//==============================
// �X���b�h��GUI �A���p
//==============================
typedef struct {
    SentData    send;          // �iInvalidValueHandling �Ŋm��ς݁j
    ReceiveData recv;
    uint32_t start_y, start_m, start_d; // �\���p
    uint32_t ndays_ui;
    bool     dir_after;
} ClientResult;

//==============================
// ���s(����M)�X���b�h
//==============================
static unsigned __stdcall SendThread(void* arg){
    ClientResult* r = (ClientResult*)arg;

    // ���M���O�Ɂu�Ō��1���v���������i�A������M���s�̓X���b�h���ōX�V�j
    g_error_id = ERR_NONE;

    // �ڑ��i���s���� ConnectToCoyomi ���� ERR_ID1_CONNECT ���ʒm�����j
    COYOMI_SOCKET cs = ConnectToCoyomi_ConnectToCoyomi(SERVER_IP, SERVER_PORT, OnErrorRaised);
    if (cs == (COYOMI_SOCKET)INVALID_SOCKET) {
        // ���s: ���̂܂�GUI��
    } else {
        int fail_sr = 0;
        for (;;){
            if (UIntRecive_Sent(cs, &r->send, OnErrorRaised) > 0 &&
                UIntRecive_Recive(cs, &r->recv, OnErrorRaised) > 0){
                break; // OK
            }
            fail_sr++;
            if (fail_sr >= 5){
                OnErrorRaised(ERR_ID7_SEND_RECV_FAIL);
                break;
            }
            // �Đڑ�
            closesocket((SOCKET)cs);
            cs = ConnectToCoyomi_ConnectToCoyomi(SERVER_IP, SERVER_PORT, OnErrorRaised);
            if (cs == (COYOMI_SOCKET)INVALID_SOCKET){
                break;
            }
        }
        if (cs != (COYOMI_SOCKET)INVALID_SOCKET) closesocket((SOCKET)cs);
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
// ��s�錾
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

        // ---- ShowResultStatus �Ɉ�{�� ----
        ErrorFlags ef = {
            .id1_connect            = Error_ID_01,
            .id2_start_out_of_range = Error_ID_02,
            .id3_positive_only      = Error_ID_03,
            .id4_roundup            = Error_ID_04,
            .id5_cap_10000          = Error_ID_05,
            .id6_invalid_int        = Error_ID_06,
            .id7_send_recv_fail     = Error_ID_07,
        };
        bool has_comm_error = (ef.id1_connect || ef.id7_send_recv_fail) ? true : false;

        // ���ʗ��F�ʐM�G���[���������̂ݗj���t���ŏo��
        ShowResultStatus_Result(
            &g_gui,
            &res->recv,
            res->start_y, res->start_m, res->start_d,
            res->ndays_ui,
            res->dir_after,
            has_comm_error
        );

        // ���b�Z�[�W���F�G���[ID�Q�ɑΉ����镶������o��
        ShowResultStatus_Status(&g_gui, &ef);

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
// ���N�G�X�g�J�n�iMain ����͌��؂����A���W���[���ɈϏ��j
//==============================
static void start_request(HWND hwnd){
    (void)hwnd;

    if (g_gui.hGUI_Message) SetWindowTextW(g_gui.hGUI_Message, L"");
    if (g_gui.hGUI_Result)  SetWindowTextW(g_gui.hGUI_Result,  L"");

    reset_error_flags();

    // 1) GUI ���琶�f�[�^���W
    InputData iv;
    CollectData_CollectData(&g_gui, &iv);

    // 2) ���؁E�␳�� InvalidValueHandling �ɔC����i�o�͍͂ŏI���M SentData�j
    ClientResult* cr = (ClientResult*)calloc(1, sizeof(ClientResult));
    if(!cr){
        set_status(L"(�����G���[) �������m�ۂɎ��s���܂����B");
        return;
    }

    bool setDtpToday = false;
    InvalidValueHandling_InvalidValueHandling(
        &iv,                 // in
        &cr->send,           // out�i�ŏI���M�f�[�^�j
        &setDtpToday,        // DTP �������ɍ��킹��ׂ���
        NULL, 0,             // �����ł̓��b�Z�[�W�g�ݗ��Ă͕s�v
        OnErrorRaised        // 0��1 �X�V�p�R�[���o�b�N
    );

    // �\���p�̕⏕���i�J�n�� / ���� / �O��j
    cr->start_y   = cr->send.year;
    cr->start_m   = cr->send.month;
    cr->start_d   = cr->send.day;
    cr->ndays_ui  = cr->send.ndays;
    cr->dir_after = ((cr->send.flags & FLAG_DIR_MASK) != 0);

    // �K�v�Ȃ� DTP ��������
    if (setDtpToday){
        SYSTEMTIME now; InvalidValueHandling_GetTodayTime(&now);
        SendMessageW(g_gui.hGUI_Calendar, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&now);
    }

    // 3) ���s�X���b�h�N��
    EnableWindow(g_gui.hGUI_Execution, FALSE);

    HANDLE th = (HANDLE)_beginthreadex(NULL,0,SendThread,cr,0,NULL);
    if(th){
        g_hWorker = th;
    }else{
        set_status(L"(�����G���[) �X���b�h�N���Ɏ��s���܂����B");
        free(cr);
        EnableWindow(g_gui.hGUI_Execution, TRUE);
    }
}

//==============================
// �G���g���|�C���g
//==============================
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR lpCmdLine, int nShow){
    UNREFERENCED_PARAMETER(hPrev);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Winsock �������iConnectToCoyomi / UIntRecive �̑O��j
    WSADATA w;
    if (WSAStartup(MAKEWORD(2,2), &w) != 0){
        MessageBoxW(NULL,L"WSAStartup failed",L"Error",MB_ICONERROR);
        return 1;
    }

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_DATE_CLASSES }; // DTP �p
    InitCommonControlsEx(&icc);

    // �e�{�q�̈ꊇ�쐬
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

    // �\��
    CreateWindow_ShowWindow(&g_gui, nShow);

    // ���[�v
    MSG msg;
    while(GetMessageW(&msg,NULL,0,0)){
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    WSACleanup();
    return 0;
}
