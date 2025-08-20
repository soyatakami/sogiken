// �@�\�Ƃ��Ă�InvalidValueHandling�𗘗p���ăG���[����
// ConnectToCoyomi�𗘗p���ăT�[�o�[�Ƃ̒ʐM���o���邩�m���߂�
// UIntRecive���g�p���ĒʐM���s���D
// WinProc�Ƃ������W���[����ReceveData��n���D
// ���W���[�����FUIProcesMain
// �֐����FUIProcesMain_ UIProcesMain

// UIProcesMain.c
// ���͎��W �� ���ؕ␳ �� �ڑ��m�F �� ����M ��S�����A�������� WndProc �֌��ʂ𓊂���B

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

/* winsock2.h �� windows.h ���O�� */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <stdlib.h>

#include "coyomi_all.h"

/* �ڑ���i�K�v�Ȃ�r���h���� -DSERVER_IP="..." -DSERVER_PORT=... �ŏ㏑���j */
#ifndef SERVER_IP
#define SERVER_IP   "127.0.0.1"
#endif
#ifndef SERVER_PORT
#define SERVER_PORT 5000
#endif

/* �X���b�h���� */
typedef struct {
    HWND          hwnd_main;
    ClientResult* cr;
    ErrorRaiseCb  on_error;
} UIM_ThreadArg;

/* ����M�X���b�h�{�� */
static unsigned __stdcall UIM_SendThread(void* p)
{
    UIM_ThreadArg* arg = (UIM_ThreadArg*)p;
    ClientResult*  cr  = arg->cr;
    ErrorRaiseCb   on_error = arg->on_error;
    HWND           hwnd = arg->hwnd_main;

    /* ���݊��Fwire �����ɍڂ���u�Ō��1���v���J�n���ɃN���A */
    g_error_id = ERR_NONE;

    /* �܂��ڑ��m�F�i���s���� ConnectToCoyomi ���� ERR_ID1_CONNECT ��ʒm�j */
    COYOMI_SOCKET cs = ConnectToCoyomi_ConnectToCoyomi(SERVER_IP, SERVER_PORT, on_error);
    if (cs != (COYOMI_SOCKET)INVALID_SOCKET) {
        int retry = 0;
        for (;;) {
            if (UIntRecive_Sent(cs, &cr->send, on_error) > 0 &&
                UIntRecive_Recive(cs, &cr->recv, on_error) > 0) {
                break; /* �ʐM���� */
            }
            if (++retry >= 5) {                 /* 5�A�����s */
                if (on_error) on_error(ERR_ID7_SEND_RECV_FAIL);
                break;
            }
            /* �Đڑ� */
            closesocket((SOCKET)cs);
            cs = ConnectToCoyomi_ConnectToCoyomi(SERVER_IP, SERVER_PORT, on_error);
            if (cs == (COYOMI_SOCKET)INVALID_SOCKET) {
                /* Connect �ł��Ȃ��ꍇ�AConnect ���� ERR_ID1_CONNECT �ʒm�ς� */
                break;
            }
        }
        if (cs != (COYOMI_SOCKET)INVALID_SOCKET) {
            closesocket((SOCKET)cs);
        }
    }
    /* cs �� INVALID �Ȃ�ڑ����s�Bon_error �� Connect ���ł��łɒʒm�ς� */

    /* WndProc �֌��ʃ|�X�g�i���s�����炱����� free�j */
    if (!PostMessageW(hwnd, WM_APP_RESULT, 0, (LPARAM)cr)) {
        free(cr);
    }

    free(arg);
    _endthreadex(0);
    return 0;
}

/* ���JAPI�F�񓯊��� ���W�����ؕ␳���ڑ��m�F������M �����s�J�n */
HANDLE UIProcesMain_UIProcesMain(
    const GUIHandles* gui,
    HWND              hwnd_main,
    ErrorRaiseCb      on_error
){
    if (!gui || !hwnd_main) return NULL;

    /* ���b�Z�[�W/���ʗ����N���A�i���݂���΁j */
    if (gui->hGUI_Message) SetWindowTextW(gui->hGUI_Message, L"");
    if (gui->hGUI_Result)  SetWindowTextW(gui->hGUI_Result,  L"");

    /* �܂� GUI ������͎��W */
    InputData iv = {0};
    CollectData_CollectData(gui, &iv);

    /* ���ʃy�C���[�h��p�ӁiWndProc �� free ����O��j */
    ClientResult* cr = (ClientResult*)calloc(1, sizeof(ClientResult));
    if (!cr) return NULL;

    /* InvalidValueHandling �Ō��؁E�␳�i���M�p SentData ���ŏI�m��j */
    bool setDtpToday = false;
    InvalidValueHandling_InvalidValueHandling(
        &iv,               /* in  */
        &cr->send,         /* out: �ŏI���M�f�[�^ */
        &setDtpToday,      /* DTP �������ɍ��킹��ׂ��� */
        NULL, 0,           /* ���b�Z�[�W������͂����ł͕s�v */
        on_error           /* 0��1 �X�V�p */
    );

    /* �\���p�̕⏕���i�J�n��/����/�O��j���l�߂Ă��� */
    cr->start_y   = cr->send.year;
    cr->start_m   = cr->send.month;
    cr->start_d   = cr->send.day;
    cr->ndays_ui  = cr->send.ndays;
    cr->dir_after = ((cr->send.flags & FLAG_DIR_MASK) != 0);

    /* �K�v�Ȃ� DTP �������Ɋ񂹂� */
    if (setDtpToday && gui->hGUI_Calendar) {
        SYSTEMTIME now;
        InvalidValueHandling_GetTodayTime(&now);
        SendMessageW(gui->hGUI_Calendar, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&now);
    }

    /* ���s�{�^��������Ζ������i���������h�~�B�L������ WndProc ���Łj */
    if (gui->hGUI_Execution) EnableWindow(gui->hGUI_Execution, FALSE);

    /* �X���b�h�N���i�n���h���͌Ăь��ŕێ����Ă��̂ĂĂ�OK�j */
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
    return hth;  /* �Ăь��� Wait/Close ��������Ύg���B�s�v�Ȃ疳����OK */
}
