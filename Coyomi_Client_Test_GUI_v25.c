//* UIProcesMain�̕���

// �@�\�Ƃ��Ă�InvalidValueHandling�𗘗p���ăG���[����
// ConnectToCoyomi�𗘗p���ăT�[�o�[�Ƃ̒ʐM���o���邩�m���߂�
// UIntRecive���g�p���ĒʐM���s���D
// WinProc�Ƃ������W���[����ReceveData��n���D
// ���W���[�����FUIProcesMain
// �֐����FUIProcesMain_ UIProcesMain

// Coyomi_Client_Test_GUI_v25.c
// �r���h��F
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

// include��: winsock2 �� ws2tcpip �� windows
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

// �v���W�F�N�g���ʁi1���w�b�_�j
#include "coyomi_all.h"

//==============================
// �ڑ���
//==============================
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 5000

//==============================
// �O���[�o��
//==============================
GUIHandles g_gui;
static HWND g_main_hwnd = NULL;

// wire�����ɍڂ��鋌�݊��u�Ō�̃G���[ID�v
ErrorID g_error_id = ERR_NONE;

// �G���[�t���O�i0:������, 1:�����j��ShowResultStatus_Status �ɓn��
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

// UIProcesMain / InvalidValueHandling / ConnectToCoyomi / UISentRecive ����̒ʒm��0��1���f
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
// GUI�i�Œ�T�C�Y�j
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
#define WM_APP_RESULT (WM_APP + 1)  // UIProcesMain �����ʃ|�C���^��Ԃ�

static volatile LONG g_alive = 1;

static void set_status(LPCWSTR s){
    if (g_gui.hGUI_Message) SetWindowTextW(g_gui.hGUI_Message, s);
}

//==============================
// ��s�錾
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
        // UIProcesMain ����̌��ʃ|�C���^�icoyomi_all.h �Ő錾����Ă���\���́j
        UIProcesMain_Result* res = (UIProcesMain_Result*)lParam;

        // �G���[�t���O�� ShowResultStatus �ɓn���ĕ\���X�V
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

        // ���ʗ��i�ʐM�G���[��������Ηj���t���ŏo�́j
        ShowResultStatus_Result(
            &g_gui,
            &res->recv,
            res->start_y, res->start_m, res->start_d,
            res->ndays_ui,
            res->dir_after,
            has_comm_error
        );

        // ���b�Z�[�W���i�����G���[ID��񋓂��ĕ\���j
        ShowResultStatus_Status(&g_gui, &ef);

        // ����������
        EnableWindow(g_gui.hGUI_Execution, TRUE);

        // UIProcesMain �� malloc �������ʂ����
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
// ���N�G�X�g�J�n�i������ UIProcesMain �֊ۓ����j
//==============================
static void start_request(HWND hwnd){
    (void)hwnd;

    if (g_gui.hGUI_Message) SetWindowTextW(g_gui.hGUI_Message, L"");
    if (g_gui.hGUI_Result)  SetWindowTextW(g_gui.hGUI_Result,  L"");

    reset_error_flags();

    // �{�^���𖳌����i���ʂ��Ԃ����� WndProc �ŗL�����j
    EnableWindow(g_gui.hGUI_Execution, FALSE);

    // ������ UIProcesMain �Ɏ������Ϗ��i������ InvalidValueHandling / ConnectToCoyomi / UISentRecive ���g�p�j
    // UIProcesMain �͕K�v�Ȃ� DTP �������ɍX�V���A�X���b�h�ő���M���A�������� WM_APP_RESULT �Ō��ʂ�Ԃ��܂��B
    UIProcesMain_UIProcesMain(
        &g_gui,
        g_main_hwnd,
        SERVER_IP,
        SERVER_PORT,
        WM_APP_RESULT,
        OnErrorRaised  // 0��1 �X�V�p�R�[���o�b�N
    );
}

//==============================
// �G���g���|�C���g
//==============================
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR lpCmdLine, int nShow){
    UNREFERENCED_PARAMETER(hPrev);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Winsock �������iConnectToCoyomi / UISentRecive �̑O��j
    WSADATA w;
    if (WSAStartup(MAKEWORD(2,2), &w) != 0){
        MessageBoxW(NULL, L"WSAStartup failed", L"Error", MB_ICONERROR);
        return 1;
    }

    // DTP �Ȃǋ��ʃR���g���[��
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_DATE_CLASSES };
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

    // ���b�Z�[�W���[�v
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)){
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    WSACleanup();
    return 0;
}
