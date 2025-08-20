//* CollectData.c
// ��ʂɓ��͂���Ă���l���A���̂܂� InputData �Ɋi�[���邾���B
// - DTP �̗L��/�����𔻒肵�Ȃ��i���ʃR�[�h���Ԃ��Ȃ��j
// - ���؂�␳�͍s��Ȃ��i�ʃ��W���[���Ŏ��{�j

//*InputData�\���̂̏����������Ă��܂��Ă���D��������WinProc�ōs���̂ňڂ��K�v���L

#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>   // DTM_GETSYSTEMTIME / BM_GETCHECK �Ȃ�
#include "coyomi_all.h"

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof((a)[0]))
#endif

void CollectData_CollectData(const GUIHandles* gui, InputData* out_iv)
{
    if (!out_iv) return;              // �l���l�߂�悪������Ή������Ȃ�
    ZeroMemory(out_iv, sizeof(*out_iv));

    if (!gui) return;                 // ��ʃn���h������������ΏI���i����͍ŏ����j

    // --- ���t�iDTP�j ---
    // DTP �̗L��/�����́u���肵�Ȃ��v�B�擾���āA���̒l�����̂܂܎g���B
    // �擾�������ł����Ă����򂹂��A�������ς݂� 0 �����邾���B////���̎����ŗǂ��̂��^��Ɋ�����
    SYSTEMTIME st = {0};
    if (gui->hGUI_Calendar) {
        SendMessageW(gui->hGUI_Calendar, DTM_GETSYSTEMTIME, 0, (LPARAM)&st);
        out_iv->year  = st.wYear;
        out_iv->month = st.wMonth;
        out_iv->day   = st.wDay;
    }

    // --- �����e�L�X�g�iEDIT �� UTF-8 �ցj ---
    if (gui->hGUI_ShiftDays) {
        wchar_t wbuf[64] = L"";
        GetWindowTextW(gui->hGUI_ShiftDays, wbuf, _countof(wbuf));
        int n = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1,
                                    out_iv->ndays_text, (int)sizeof(out_iv->ndays_text),
                                    NULL, NULL);
        if (n <= 0) {
            // �ϊ����s�������؂͂��Ȃ��B�󕶎��ɂ��Ă����B
            out_iv->ndays_text[0] = '\0';
        }
    }

    // --- ���W�I�{�^����ԁi�O/��, 0/1���ځj ---
    // ����Ƃ������P������F�`�F�b�N����Ă���� true�B
    if (gui->hGUI_RadioBox_After) {
        out_iv->dir_after = (SendMessageW(gui->hGUI_RadioBox_After, BM_GETCHECK, 0, 0) == BST_CHECKED);
    }
    if (gui->hGUI_RadioBox_One) {
        out_iv->flag01 = (SendMessageW(gui->hGUI_RadioBox_One, BM_GETCHECK, 0, 0) == BST_CHECKED);
    }
}
