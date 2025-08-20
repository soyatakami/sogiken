//*CreateWindow.h
//�e�E�B���h�E�{�q�R���g���[�����ꊇ��������΂ɕK�v�ȕύX����
//�ύX�O��WinMain���W���[���̒��Őewindow�𐶐����Ă��܂��Ă����D
//����
//�Ԓl
//*ShowWindoe���W���[�������ɓ����Ă��܂��Ă��邪��Ő؂藣���D�i�N�����肢�C�����āj

#pragma once
#include <windows.h>

// ��ʒ��̑S�R���g���[���̃n���h���𑩂˂�\����
typedef struct GUIHandles {
    HWND hMain;

    // ���x����
    HWND hGUI_Comment_1, hGUI_Comment_2, hGUI_Comment_3, hGUI_Comment_4;
    HWND hGUI_Comment_5, hGUI_Comment_6, hGUI_Comment_7, hGUI_Comment_8;
    HWND hGUI_Comment_9;

    // ���͌n
    HWND hGUI_Calendar;
    HWND hGUI_ShiftDays;

    // ���W�I
    HWND hGUI_RadioBox_Zero, hGUI_RadioBox_One;
    HWND hGUI_RadioBox_Before, hGUI_RadioBox_After;

    // �{�^��
    HWND hGUI_Execution, hGUI_End;

    // �o�͗�
    HWND hGUI_Message, hGUI_Result;
} GUIHandles;

// �R���g���[��ID�iMain �� WM_COMMAND �ł��g���̂� .h �ɒu���j
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

// �e�E�B���h�E�{�q�R���g���[�����܂Ƃ߂Đ���
HWND CreateWindow_CreateWindow(
    GUIHandles* gui,
    HINSTANCE   hInst,
    WNDPROC     wndProc,
    LPCWSTR     className,
    LPCWSTR     windowTitle,
    DWORD       style,
    DWORD       exStyle
);

// �\���iShowWindow/UpdateWindow�j
void CreateWindow_ShowWindow(GUIHandles* gui, int nShow);
