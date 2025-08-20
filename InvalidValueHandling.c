//* InvalidValueHandling.c

// InvalidValueHandling.c
// - InputData �̔N�����{����������
// - �N�������͈͊O or �������͂��u���p�����E���̐��v�ᔽ�Ȃ�A�J�n�����u�����v�ɍ����ւ�
// - ������ �������؂�グ�A��� 10000 �܂ŕ␳
// - �␳���ʂ� SentData ���\���iflags �� dir_after/flag01 ����j
// - �G���[ID�� 0��1 �X�V�� on_error(ErrorID) �R�[���o�b�N�Œʒm�i��: record_error�j

// InvalidValueHandling.c
// ����(InputData)�̑Ó����`�F�b�N�ƕ␳���s���A�ŏI���M�p SentData ���m�肷��B
// - �J�����_�[���t���͈͊O�Ȃ獡���ɍ����ւ��iERR_ID2�j
// - �������͂̌��؁F��/�s��/<=0 �� 1���ɕ␳�iERR_ID6�j�{�u���̐��̂݁v�iERR_ID3�j
// - �����͐؂�グ�iERR_ID4�j
// - ��� 10000 �Ɋۂ߁iERR_ID5�j
// - �K�v�ȃG���[�� on_error_raised �R�[���o�b�N�� 0��1 �֍X�V���˗�
// - DTP �������ɍ��킹��K�v������ꍇ�� out_set_dtp_today=true ��Ԃ�
// - �T�ԍ��̓T�[�o���ŕt�^����z��Ȃ̂� flags �� weekday �� 0 �Œ�

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <windows.h>
#include <wchar.h>
#include <stdlib.h>   // wcstod
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "coyomi_all.h"  // ������ ErrorID / �\���� / �萔 / �v���g�^�C�v���܂Ƃ܂��Ă���

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof((a)[0]))
#endif

// ---- ���[�e�B���e�B ----
static inline bool in_allowed_range_yMd(int y,int m,int d){
    if (y < MIN_Y || y > MAX_Y) return false;
    if (y == MIN_Y){
        if (m < MIN_M) return false;
        if (m == MIN_M && d < MIN_D) return false;
    }
    if (y == MAX_Y){
        if (m > MAX_M) return false;
        if (m == MAX_M && d > MAX_D) return false;
    }
    return true;
}

static inline uint32_t make_flags(bool dir_after, bool flag01){
    uint32_t f = 0u;
    if (dir_after) f |= FLAG_DIR_MASK;
    if (flag01)    f |= FLAG_FLAG01_MASK;
    // weekday �̓T�[�o���ݒ�̑z��Ȃ̂� 0 �Œ�
    return f;
}

// ndays �� UTF-8 �e�L�X�g�����؁E�␳���ĕԂ�
// - invalid �� <=0 �� 1�i���j�ɕ␳�� ERR_ID6, ����� ERR_ID3�i���̐��̂݁j�𔭉�
// - �����͐؂�グ �� ERR_ID4
// - ��� 10000 �� ERR_ID5
// �Ԃ�l: �␳�� ndays
static uint32_t parse_and_fix_ndays_utf8(const char* utf8,
                                         void (*on_error_raised)(int),
                                         bool* out_need_today)
{
    if (out_need_today) *out_need_today = false;

    // ��/NULL
    if (!utf8 || !*utf8){
        if (on_error_raised){ on_error_raised(ERR_ID6_INVALID_INT); on_error_raised(ERR_ID3_POSITIVE_ONLY); }
        if (out_need_today) *out_need_today = true;
        return 1u;
    }

    // UTF-8 �� wchar_t
    wchar_t wbuf[64];
    int wn = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wbuf, _countof(wbuf));
    if (wn <= 0){
        if (on_error_raised){ on_error_raised(ERR_ID6_INVALID_INT); on_error_raised(ERR_ID3_POSITIVE_ONLY); }
        if (out_need_today) *out_need_today = true;
        return 1u;
    }

    // �擪�̋󔒂��X�L�b�v
    const wchar_t* s = wbuf;
    while(*s==L' '||*s==L'\t'||*s==L'\r'||*s==L'\n') ++s;
    if (!*s){
        if (on_error_raised){ on_error_raised(ERR_ID6_INVALID_INT); on_error_raised(ERR_ID3_POSITIVE_ONLY); }
        if (out_need_today) *out_need_today = true;
        return 1u;
    }

    bool has_dot = (wcschr(s, L'.') != NULL);
    wchar_t* endp = NULL;
    double val = wcstod(s, &endp);

    // �]�蕶���̗L�����m�F�i�󔒈ȊO���c���Ă����� NG�j
    bool extra = false;
    if (endp){
        while(*endp){
            if (!iswspace(*endp)) { extra = true; break; }
            ++endp;
        }
    }else{
        extra = true;
    }

    if (!isfinite(val) || extra){
        if (on_error_raised){ on_error_raised(ERR_ID6_INVALID_INT); on_error_raised(ERR_ID3_POSITIVE_ONLY); }
        if (out_need_today) *out_need_today = true;
        return 1u;
    }

    uint32_t n;
    if (has_dot){
        // ���� �� �؂�グ
        double c = ceil(val);
        if (c < 1.0){
            if (on_error_raised){ on_error_raised(ERR_ID6_INVALID_INT); on_error_raised(ERR_ID3_POSITIVE_ONLY); }
            if (out_need_today) *out_need_today = true;
            return 1u;
        }
        if (c > 1e9) c = 1e9; // �ߏ�h�q
        n = (uint32_t)c;
        if (on_error_raised) on_error_raised(ERR_ID4_ROUNDUP);
    }else{
        // ��������
        if (val <= 0.0){
            if (on_error_raised){ on_error_raised(ERR_ID6_INVALID_INT); on_error_raised(ERR_ID3_POSITIVE_ONLY); }
            if (out_need_today) *out_need_today = true;
            return 1u;
        }
        if (val > 1e9) val = 1e9;
        n = (uint32_t)val;
    }

    if (n > 10000u){
        n = 10000u;
        if (on_error_raised) on_error_raised(ERR_ID5_CAP_10000);
    }
    return n;
}

// ---- ���J API �����icoyomi_all.h �ɐ錾�ς݁j ----

void InvalidValueHandling_GetTodayTime(SYSTEMTIME* out_now){
    if (!out_now) return;
    GetLocalTime(out_now);
}

void InvalidValueHandling_InvalidValueHandling(
    const InputData* in,
    SentData* out_sd,
    bool* out_set_dtp_today,
    wchar_t* opt_errbuf, size_t opt_errcap, // �g��Ȃ��ꍇ�� NULL,0 �� OK
    void (*on_error_raised)(int error_id)
){
    if (out_set_dtp_today) *out_set_dtp_today = false;
    if (out_sd) memset(out_sd, 0, sizeof(*out_sd));
    (void)opt_errbuf; (void)opt_errcap; // ����͕�����\�z�Ȃ�

    if (!in || !out_sd){
        // ���͂܂��͏o�͐悪�����ꍇ�́A����/ndays=1/�t���O0 �ŕԂ�
        SYSTEMTIME now; GetLocalTime(&now);
        out_sd->year  = (uint32_t)now.wYear;
        out_sd->month = (uint32_t)now.wMonth;
        out_sd->day   = (uint32_t)now.wDay;
        out_sd->ndays = 1u;
        out_sd->flags = 0u;
        if (out_set_dtp_today) *out_set_dtp_today = true;
        return;
    }

    // 1) �J�n���̌��؁i�͈͊O�Ȃ獡���ɍ����ւ��j
    bool need_today_by_date = false;
    uint32_t y = in->year, m = in->month, d = in->day;
    if (!in_allowed_range_yMd((int)y,(int)m,(int)d)){
        need_today_by_date = true;
        if (on_error_raised) on_error_raised(ERR_ID2_START_OUT_OF_RANGE);
    }

    SYSTEMTIME today;
    if (need_today_by_date){
        GetLocalTime(&today);
        y = (uint32_t)today.wYear;
        m = (uint32_t)today.wMonth;
        d = (uint32_t)today.wDay;
    }

    // 2) �����̌��؁E�␳
    bool need_today_by_ndays = false;
    uint32_t nd = parse_and_fix_ndays_utf8(in->ndays_text, on_error_raised, &need_today_by_ndays);

    // ndays ���� NG �̂Ƃ����J�n���͍����ɍ����ւ���
    if (need_today_by_ndays && !need_today_by_date){
        GetLocalTime(&today);
        y = (uint32_t)today.wYear;
        m = (uint32_t)today.wMonth;
        d = (uint32_t)today.wDay;
    }

    // 3) flags�iweekday �� 0 �Œ�j
    uint32_t flags = make_flags(in->dir_after, in->flag01);

    // 4) ���M�p�\���̂Ɋm��l���i�[
    out_sd->year  = y;
    out_sd->month = m;
    out_sd->day   = d;
    out_sd->ndays = nd;
    out_sd->flags = flags;

    // 5) DTP �������ɍ��킹��ׂ����̒ʒm
    if (out_set_dtp_today) *out_set_dtp_today = (need_today_by_date || need_today_by_ndays);
}
