//*CreateWindow.c
//�e�E�B���h�E�{�q�R���g���[�����ꊇ��������΂ɕK�v�ȕύX����
//�ύX�O��WinMain���W���[���̒��Őewindow�𐶐����Ă��܂��Ă����D
//���� 
//�Ԓl 
//*ShowWindoe���W���[�������ɓ����Ă��܂��Ă��邪��Ő؂藣���D�i�N�����肢�C�����āj

//* CreateWindow.c
// �e�E�B���h�E�{�q�R���g���[�����ꊇ�����i�t�H���g�T�C�Y�w��Ή��Łj

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
// #include "coyomi_gui_consts.h"   // ���W/�T�C�Y/GUI_SIZE, GUI_RATIO_X/Y, *_pt �Ȃ�
// #include "coyomi_messages_ja.h"  // GUI_Comment_* / �{�^��������Ȃ�

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>   // �� ���ꂪ�K�{�iDATETIMEPICK_CLASSW, DTM_*, GDT_* �Ȃǁj
#include "coyomi_all.h"

// ---------------- �t�H���g�K�p�ipt �w��j ----------------
// ���ȈՎ����F�s�x CreateFontIndirectW �� WM_SETFONT�B�K�v�Ȃ�L���b�V�����������B
static HFONT GetFontPt(HWND hwndRef, int pt, LPCWSTR face)
{
    if (pt <= 0) pt = 12; // �t�H�[���o�b�N
    HDC hdc = GetDC(hwndRef);
    int dpi = hdc ? GetDeviceCaps(hdc, LOGPIXELSY) : 96;
    if (hdc) ReleaseDC(hwndRef, hdc);

    LOGFONTW lf = {0};
    lf.lfHeight = -MulDiv(pt, dpi, 72);        // pt �� �_������
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

// ---------------- �q�R���g���[������ ----------------
static void create_children(HWND hwnd, GUIHandles* gui)
{
    // �u�J�n���v���x��
    gui->hGUI_Comment_1 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_1, WS_CHILD|WS_VISIBLE,
        GUI_Comment_1_Xp, GUI_Comment_1_Yp, GUI_Comment_1_Xs, GUI_Comment_1_Ys,
        hwnd, 0, 0, 0);
    APPLY_FONTPT(gui->hGUI_Comment_1, GUI_Comment_1_pt);

    // ���t(DTP)
    gui->hGUI_Calendar = CreateWindowExW(
        0, DATETIMEPICK_CLASSW, L"",
        WS_CHILD|WS_VISIBLE|DTS_SHORTDATEFORMAT,
        GUI_Calendar_Xp, GUI_Calendar_Yp, GUI_Calendar_Xs, GUI_Calendar_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_DTP,0,0);
    SYSTEMTIME st; GetLocalTime(&st);
    SendMessageW(gui->hGUI_Calendar, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    APPLY_FONTPT(gui->hGUI_Calendar, GUI_Calendar_pt);

    // �u�L���͈́v���x���i�����s�j
    gui->hGUI_Comment_2 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_2,
        WS_CHILD|WS_VISIBLE|SS_LEFT|SS_EDITCONTROL|SS_NOPREFIX,
        GUI_Comment_2_Xp, GUI_Comment_2_Yp, GUI_Comment_2_Xs, GUI_Comment_2_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(gui->hGUI_Comment_2, GUI_Comment_2_pt);

    // �u�J�n�����v
    gui->hGUI_Comment_3 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_3, WS_CHILD|WS_VISIBLE,
        GUI_Comment_3_Xp, GUI_Comment_3_Yp, GUI_Comment_3_Xs, GUI_Comment_3_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(gui->hGUI_Comment_3, GUI_Comment_3_pt);

    // 0����
    gui->hGUI_RadioBox_Zero = CreateWindowExW(
        0, L"BUTTON", GUI_RadioBox_Zero,
        WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON|WS_GROUP,
        GUI_RadioBox_Zero_Xp, GUI_RadioBox_Zero_Yp, GUI_RadioBox_Zero_Xs, GUI_RadioBox_Zero_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_FLAG0,0,0);
    APPLY_FONTPT(gui->hGUI_RadioBox_Zero, GUI_RadioBox_Zero_pt);

    // 1����
    gui->hGUI_RadioBox_One = CreateWindowExW(
        0, L"BUTTON", GUI_RadioBox_One,
        WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
        GUI_RadioBox_One_Xp, GUI_RadioBox_One_Yp, GUI_RadioBox_One_Xs, GUI_RadioBox_One_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_FLAG1,0,0);
    APPLY_FONTPT(gui->hGUI_RadioBox_One, GUI_RadioBox_One_pt);

    SendMessageW(gui->hGUI_RadioBox_Zero, BM_SETCHECK, BST_CHECKED, 0); // ����F0����

    // �u�Ƃ���v
    gui->hGUI_Comment_4 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_4, WS_CHILD|WS_VISIBLE,
        GUI_Comment_4_Xp, GUI_Comment_4_Yp, GUI_Comment_4_Xs, GUI_Comment_4_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(gui->hGUI_Comment_4, GUI_Comment_4_pt);

    // �u�J�n���́v
    gui->hGUI_Comment_5 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_5, WS_CHILD|WS_VISIBLE,
        GUI_Comment_5_Xp, GUI_Comment_5_Yp, GUI_Comment_5_Xs, GUI_Comment_5_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(gui->hGUI_Comment_5, GUI_Comment_5_pt);

    // �������́iES_NUMBER �͕t���Ȃ��j
    gui->hGUI_ShiftDays = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"7",
        WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
        GUI_ShiftDays_Xp, GUI_ShiftDays_Yp, GUI_ShiftDays_Xs, GUI_ShiftDays_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_NDAYS,0,0);
    APPLY_FONTPT(gui->hGUI_ShiftDays, GUI_ShiftDays_pt);

    // �u���v
    gui->hGUI_Comment_6 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_6, WS_CHILD|WS_VISIBLE,
        GUI_Comment_6_Xp, GUI_Comment_6_Yp, GUI_Comment_6_Xs, GUI_Comment_6_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(gui->hGUI_Comment_6, GUI_Comment_6_pt);

    // �O�i���W�I�j ����F�O
    gui->hGUI_RadioBox_Before = CreateWindowExW(
        0, L"BUTTON", GUI_RadioBox_Before,
        WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON|WS_GROUP,
        GUI_RadioBox_Before_Xp, GUI_RadioBox_Before_Yp, GUI_RadioBox_Before_Xs, GUI_RadioBox_Before_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_DIR_BACK,0,0);
    APPLY_FONTPT(gui->hGUI_RadioBox_Before, GUI_RadioBox_Before_pt);

    // ��i���W�I�j
    gui->hGUI_RadioBox_After  = CreateWindowExW(
        0, L"BUTTON", GUI_RadioBox_After,
        WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
        GUI_RadioBox_After_Xp, GUI_RadioBox_After_Yp, GUI_RadioBox_After_Xs, GUI_RadioBox_After_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_DIR_FWD,0,0);
    APPLY_FONTPT(gui->hGUI_RadioBox_After, GUI_RadioBox_After_pt);

    SendMessageW(gui->hGUI_RadioBox_Before, BM_SETCHECK, BST_CHECKED, 0);

    // ���ӏ���
    gui->hGUI_Comment_7 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_7, WS_CHILD|WS_VISIBLE,
        GUI_Comment_7_Xp, GUI_Comment_7_Yp, GUI_Comment_7_Xs, GUI_Comment_7_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(gui->hGUI_Comment_7, GUI_Comment_7_pt);

    // ���s�{�^��
    gui->hGUI_Execution = CreateWindowExW(
        0,L"BUTTON", GUI_Execution, WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
        GUI_Execution_Xp, GUI_Execution_Yp, GUI_Execution_Xs, GUI_Execution_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_SEND,0,0);
    APPLY_FONTPT(gui->hGUI_Execution, GUI_Execution_pt);

    // �I���{�^��
    gui->hGUI_End = CreateWindowExW(
        0,L"BUTTON", GUI_End, WS_CHILD|WS_VISIBLE,
        GUI_End_Xp, GUI_End_Yp, GUI_End_Xs, GUI_End_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_EXIT,0,0);
    APPLY_FONTPT(gui->hGUI_End, GUI_End_pt);

    // ���x���i���b�Z�[�W/���ʁj
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

    // ���b�Z�[�WEDIT�i�����͋�j
    gui->hGUI_Message = CreateWindowExW(
        WS_EX_CLIENTEDGE,L"EDIT", L"",
        WS_CHILD|WS_VISIBLE|ES_READONLY|ES_MULTILINE|ES_AUTOVSCROLL,
        GUI_Message_Xp, GUI_Message_Yp, GUI_Message_Xs, GUI_Message_Ys,
        hwnd,(HMENU)(INT_PTR)IDC_STATUS,0,0);
    APPLY_FONTPT(gui->hGUI_Message, GUI_Message_pt);

    // �܂Ƃߏo��EDIT
    gui->hGUI_Result = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_WANTRETURN,
        GUI_OutAll_Xp, GUI_OutAll_Yp, GUI_OutAll_Xs, GUI_OutAll_Ys,
        hwnd, (HMENU)(INT_PTR)IDC_OUTALL, 0, 0);
    APPLY_FONTPT(gui->hGUI_Result, GUI_OutAll_pt);
}

// ---------------- �e�E�B���h�E�����i�{�q�쐬�j ----------------
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

    // �N���X�o�^
    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = wndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = className;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    if(!RegisterClassW(&wc) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS){
        return NULL;
    }

    // �N���C�A���g� �� �O�`�T�C�Y��
    int clientW = (int)((long long)GUI_SIZE * GUI_RATIO_Y / GUI_RATIO_X);
    RECT rc = {0, 0, clientW, GUI_SIZE};
    AdjustWindowRectEx(&rc, style, FALSE, exStyle);

    // �e�E�B���h�E����
    HWND h = CreateWindowExW(
        exStyle, className, windowTitle, style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInst, NULL
    );
    if(!h) return NULL;
    gui->hMain = h;

    // �q�R���g���[������
    create_children(h, gui);

    return h;
}

// ---------------- �\�� ----------------
void CreateWindow_ShowWindow(GUIHandles* gui, int nShow)
{
    if(!gui || !gui->hMain) return;
    ShowWindow(gui->hMain, nShow);
    UpdateWindow(gui->hMain);
}
