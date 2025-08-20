//*ShowResultStatus.c

// ShowResultStatus.c
// ���W���[�����FShowResultStatus
// �֐��FShowResultStatus_Result�i�ʐM�G���[���Ȃ猋�ʗ��ɗj���t���ŏo�́j
// �@�@�FShowResultStatus_Status�i�G���[ID�t���O�ɉ����ă��b�Z�[�W���ɏo�́j

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <wchar.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "coyomi_all.h"

/* ---- �����w���p ---- */

static inline short flags_get_weekday(uint32_t f){
    return (short)((f & FLAG_WD_MASK) >> FLAG_WD_SHIFT);
}

static const wchar_t* weekday_str_ja(int w){
    static const wchar_t* tbl[] = {
        L"���j��", L"���j��", L"�Ηj��", L"���j��", L"�ؗj��", L"���j��", L"�y�j��"
    };
    return (w>=0 && w<7)? tbl[w] : L"?";
}

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

static void append_line(wchar_t* buf, size_t cap, const wchar_t* line){
    if(!buf || !line || cap==0) return;
    size_t cur = wcslen(buf);
    if(cur >= cap-1) return;
    _snwprintf(buf + cur, cap - cur, L"%ls%ls", (cur?L"\r\n":L""), line);
}

/* ---- ���J�֐� ---- */

void ShowResultStatus_Result(
    const GUIHandles* gui,
    const ReceiveData* recv,
    uint32_t start_y, uint32_t start_m, uint32_t start_d,
    uint32_t ndays_ui,
    bool dir_after,
    bool has_comm_error
){
    if(!gui || !gui->hGUI_Result) return;

    // �ʐM�G���[������΁u���ʗ��v�͐G��Ȃ��i�d�l�F�ʐM�G���[�̖����ꍇ�ɏo�́j
    if (has_comm_error) return;

    const wchar_t* dir = dir_after ? L"����" : L"���O";
    short wd =  (recv ? flags_get_weekday(recv->flags) : -1);

    wchar_t line1[64], line2[64], line3[96], msg[320];
    _snwprintf(line1, _countof(line1), L"�J�n���F%04u/%02u/%02u", start_y, start_m, start_d);
    _snwprintf(line2, _countof(line2), L"�����F%u%ls", ndays_ui, dir);

    if (recv){
        _snwprintf(line3, _countof(line3), L"�Y�����F %04u/%02u/%02u�i%ls�j",
                   recv->year, recv->month, recv->day, weekday_str_ja((int)wd));
    }else{
        _snwprintf(line3, _countof(line3), L"�Y�����F -");
    }

    _snwprintf(msg, _countof(msg), L"%ls\r\n%ls\r\n%ls", line1, line2, line3);
    SetWindowTextW(gui->hGUI_Result, msg);
}

void ShowResultStatus_Status(
    const GUIHandles* gui,
    const ErrorFlags* flags
){
    if(!gui || !gui->hGUI_Message) return;

    // �����Ȃ���΃N���A
    if(!flags){
        SetWindowTextW(gui->hGUI_Message, L"");
        return;
    }

    wchar_t out[1024]; out[0] = L'\0';
    wchar_t line[768];

    // �������� ID �Ɩ{����ǋL
    if (flags->id1_connect){
        _snwprintf(line, _countof(line), L"ID1\t%ls", error_body_from_id(ERR_ID1_CONNECT));
        append_line(out, _countof(out), line);
    }
    if (flags->id2_start_out_of_range){
        _snwprintf(line, _countof(line), L"ID2\t%ls", error_body_from_id(ERR_ID2_START_OUT_OF_RANGE));
        append_line(out, _countof(out), line);
    }
    if (flags->id3_positive_only){
        _snwprintf(line, _countof(line), L"ID3\t%ls", error_body_from_id(ERR_ID3_POSITIVE_ONLY));
        append_line(out, _countof(out), line);
    }
    if (flags->id4_roundup){
        _snwprintf(line, _countof(line), L"ID4\t%ls", error_body_from_id(ERR_ID4_ROUNDUP));
        append_line(out, _countof(out), line);
    }
    if (flags->id5_cap_10000){
        _snwprintf(line, _countof(line), L"ID5\t%ls", error_body_from_id(ERR_ID5_CAP_10000));
        append_line(out, _countof(out), line);
    }
    if (flags->id6_invalid_int){
        _snwprintf(line, _countof(line), L"ID6\t%ls", error_body_from_id(ERR_ID6_INVALID_INT));
        append_line(out, _countof(out), line);
    }
    if (flags->id7_send_recv_fail){
        _snwprintf(line, _countof(line), L"ID7\t%ls", error_body_from_id(ERR_ID7_SEND_RECV_FAIL));
        append_line(out, _countof(out), line);
    }

    SetWindowTextW(gui->hGUI_Message, out);
}
