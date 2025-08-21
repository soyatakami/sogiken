// gcc All.c -o All.exe -lcomctl32 -lws2_32 -mwindows -municode -std=c11 -O2 -Wall -Wextra -Wpedantic -finput-charset=CP932 -fexec-charset=CP932 -fwide-exec-charset=UTF-16LE

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef _WIN32_IE
#define _WIN32_IE    0x0500
#endif

/* include: Winsock �� windows.h ���� */
#include <winsock2.h>
#include <ws2tcpip.h>

#include "All.h"
#include <commctrl.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <math.h>

/* ====== �O���[�o�� ====== */
int ErrorID_01 = 0;
int ErrorID_02 = 0;
int ErrorID_03 = 0;
int ErrorID_04 = 0;
int ErrorID_05 = 0;
int ErrorID_06 = 0;
int ErrorID_07 = 0;
int ErrorID_08 = 0;
int ErrorID_09 = 0; /* �s��������1���ɕ␳ */

vsock UIClientSocket         = (vsock)INVALID_SOCKET;
Whwnd MainWindow             = NULL;
Whwnd StartData              = NULL;
Whwnd CalcDay                = NULL;
Whwnd RadioZero              = NULL;
Whwnd RadioOne               = NULL;
Whwnd RadioBefore            = NULL;
Whwnd RadioAfter             = NULL;
Whwnd OutputBoxResult        = NULL;
Whwnd OutputBoxMessage       = NULL;

const char*    IP_ADDR_SERVER = "127.0.0.1";
unsigned short PORT_NUMBER    = 50000;

vsock CoyomiClientSocket     = (vsock)INVALID_SOCKET;
vsock CoyomiListeningSocket  = (vsock)INVALID_SOCKET;

/* �p�P�b�g flags�i�ȗ��j */
enum {
    FLAG_DIR_MASK    = 0x00000001u, /* bit0: dir_after */
    FLAG_FLAG01_MASK = 0x00000002u  /* bit1: flag01    */
};

static void set_status(LPCWSTR s){
    if (OutputBoxMessage) SetWindowTextW(OutputBoxMessage, s);
}

/* ========== ���͎��W ========== */
void CollectData(HWND hwndOwner, InputData* outIv){
    (void)hwndOwner;
    if (!outIv) return;
    ZeroMemory(outIv, sizeof(*outIv));

    /* DTP �� SYSTEMTIME�i�����Ȃ猻�ݓ��j */
    SYSTEMTIME st;
    LRESULT r = SendMessageW(StartData, DTM_GETSYSTEMTIME, 0, (LPARAM)&st);
    if (r != GDT_VALID) GetLocalTime(&st);

    outIv->InputDataYear  = st.wYear;
    outIv->InputDataMonth = st.wMonth;
    outIv->InputDataDay   = st.wDay;

    /* �����iUTF-8������j */
    wchar_t wbuf[64] = L"";
    GetWindowTextW(CalcDay, wbuf, 64);
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1,
                        outIv->InputDataNdaysText, (int)sizeof(outIv->InputDataNdaysText),
                        NULL, NULL);

    /* ���W�I */
    outIv->InputDataFlag01   = (SendMessageW(RadioOne,   BM_GETCHECK, 0, 0)==BST_CHECKED);
    outIv->InputDataDirAfter = (SendMessageW(RadioAfter, BM_GETCHECK, 0, 0)==BST_CHECKED);
}

/* ========== �����p�[�X�iErrorID_06/07/08/09 ��K�v�ɉ����ăZ�b�g�j ========== */
static int parse_ndays_utf8(const char* u8, bool* outRounded, bool* outTrigPositive){
    if (outRounded)      *outRounded      = false;
    if (outTrigPositive) *outTrigPositive = false;

    if (!u8 || !*u8){ if(outTrigPositive)*outTrigPositive=true; ErrorID_09=1; return 1; }
    while (*u8==' '||*u8=='\t'||*u8=='\r'||*u8=='\n') ++u8;
    if (!*u8){ if(outTrigPositive)*outTrigPositive=true; ErrorID_09=1; return 1; }

    bool hasDot = false;
    for (const char* p=u8; *p; ++p){ if(*p=='.'){ hasDot=true; break; } }

    char* endp=NULL;
    double v = strtod(u8,&endp);
    bool extra=false;
    if (endp){
        while(*endp){
            if(!(*endp==' '||*endp=='\t'||*endp=='\r'||*endp=='\n')){ extra=true; break; }
            ++endp;
        }
    }else extra=true;

    if(!(v>0.0) || extra){
        if(outTrigPositive)*outTrigPositive=true;
        ErrorID_09 = 1; /* 0�A�����A�S�~�t���Ȃ� �� 1 �ɕ␳ */
        return 1;
    }

    int n;
    if (hasDot){
        ErrorID_07 = 1;              /* ���� �� �؂�グ */
        double c = ceil(v);

        double f = floor(v);
        bool hadFraction = (v > f);

        if (hadFraction && c >= 10000.0){
            ErrorID_08 = 1;          /* �؏グ���ʂ�10000�ȏ� */
        }

        if (c < 1.0) c = 1.0;
        n = (int)c;

        if (n > 10000){
            if (!hadFraction) ErrorID_06 = 1; /* ��: 20000.0 �� 06 */
            n = 10000;
        }
        if (outRounded) *outRounded = true;
    }else{
        n = (int)v;
        if (n < 1){
            if(outTrigPositive)*outTrigPositive=true;
            n = 1;
            ErrorID_09 = 1;          /* 0/�� �� 1 �␳ */
        }
        if (n > 10000){
            n = 10000;
            ErrorID_06 = 1;
        }
    }
    return n; /* �ŏ�1�ŕԂ�i����B�̂��ߌ�� 0 �ɂȂ�ꍇ����j */
}

/* ========== ���͐��K���i����B�F1���ڂȂ瑗�M������1���炷�j ========== */
// typedef enum {
//     ERR_NONE                     = 0,
//     ERR_ID1_CONNECT              = 1,
//     ERR_ID2_START_OUT_OF_RANGE   = 2,
//     ERR_ID3_POSITIVE_ONLY        = 3,
//     ERR_ID4_ROUNDUP              = 4,
//     ERR_ID5_CAP_10000            = 5,
//     ERR_ID6_INVALID_INT          = 6,
//     ERR_ID7_SEND_RECV_FAIL       = 7
// } ErrorId;

unsigned int InvalidValueHandling(InputData* in, SendData* outSend){
    if (!in || !outSend) return ERR_ID6_INVALID_INT;
    ZeroMemory(outSend, sizeof(*outSend));

    /* ���t�͈̓`�F�b�N�i2010/01/01�`2099/12/31�j */
    bool dateOk = true;
    if (in->InputDataYear < 2010 || in->InputDataYear > 2099) dateOk = false;
    if (dateOk && in->InputDataYear==2010){
        if (in->InputDataMonth < 1 || (in->InputDataMonth==1 && in->InputDataDay < 1)) dateOk=false;
    }
    if (dateOk && in->InputDataYear==2099){
        if (in->InputDataMonth > 12 || (in->InputDataMonth==12 && in->InputDataDay > 31)) dateOk=false;
    }
    if (!dateOk){
        ErrorID_05 = 1;
    }

    SYSTEMTIME today; GetLocalTime(&today);

    bool rounded=false, trigPos=false;
    int nd = parse_ndays_utf8(in->InputDataNdaysText, &rounded, &trigPos);

    /* ����B�F1���� �� 1 ���炷�i0���e�j */
    if (in->InputDataFlag01 && nd > 0) {
        nd -= 1;
    }

    bool replaceToToday = (!dateOk || trigPos);

    outSend->SendDataYear      = replaceToToday ? today.wYear  : in->InputDataYear;
    outSend->SendDataMonth     = replaceToToday ? today.wMonth : in->InputDataMonth;
    outSend->SendDataDay       = replaceToToday ? today.wDay   : in->InputDataDay;
    outSend->SendDataNdays     = (short)nd;
    outSend->SendDataFlag01    = in->InputDataFlag01;
    outSend->SendDataDirAfter  = in->InputDataDirAfter;

    if (!dateOk)                  return ERR_ID2_START_OUT_OF_RANGE;
    if (trigPos)                  return ERR_ID3_POSITIVE_ONLY;
    if (rounded)                  return ERR_ID4_ROUNDUP;
    if (ErrorID_06 || ErrorID_08) return ERR_ID5_CAP_10000;
    return ERR_NONE;
}

/* ========== �p�P�b�g pack/unpack ========== */
static void BuildPacketFromSendData(const SendData* s, uint32_t outWords[6]){
    uint32_t flags = 0;
    if (s->SendDataDirAfter) flags |= FLAG_DIR_MASK;
    if (s->SendDataFlag01)   flags |= FLAG_FLAG01_MASK;

    outWords[0] = htonl(s->SendDataYear);
    outWords[1] = htonl(s->SendDataMonth);
    outWords[2] = htonl(s->SendDataDay);
    outWords[3] = htonl((s->SendDataNdays < 0)? 0u : (uint32_t)s->SendDataNdays);
    outWords[4] = htonl(flags);
    outWords[5] = htonl(0);
}
static void ParsePacketToRecData(const uint32_t inWords[6], RecData* r){
    if (!r) return;
    uint32_t y = ntohl(inWords[0]);
    uint32_t m = ntohl(inWords[1]);
    uint32_t d = ntohl(inWords[2]);
    uint32_t n = ntohl(inWords[3]);
    uint32_t f = ntohl(inWords[4]);

    r->RecDataaYear    = y;
    r->RecDataMonth    = m;
    r->RecDataDay      = d;
    r->RecDataNdays    = (short)(n > 0x7fff ? 0x7fff : n);
    r->RecDataDirAfter = ((f & FLAG_DIR_MASK) != 0);
    r->RecDataFlag01   = ((f & FLAG_FLAG01_MASK) != 0);
}

/* ========== ����M���[�e�B���e�B ========== */
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

/* ========== �ڑ�/����M ========== */
int connectToCoyomi(void){
    ErrorID_01 = 0;
    ErrorID_02 = 0;

    static bool wsInited = false;
    if (!wsInited){
        WSADATA w;
        int rc = WSAStartup(MAKEWORD(2,2), &w);
        if (rc != 0){
            ErrorID_01 = 1;
            return -1;
        }
        wsInited = true;
    }

    if ((SOCKET)CoyomiClientSocket != INVALID_SOCKET){
        closesocket((SOCKET)CoyomiClientSocket);
        CoyomiClientSocket = (vsock)INVALID_SOCKET;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET){
        ErrorID_01 = 1;
        return -1;
    }

    struct sockaddr_in a; ZeroMemory(&a, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port   = htons(PORT_NUMBER);
    a.sin_addr.s_addr = inet_addr(IP_ADDR_SERVER);

    if (connect(s, (struct sockaddr*)&a, sizeof(a)) == SOCKET_ERROR){
        ErrorID_02 = 1;
        closesocket(s);
        return -1;
    }

    CoyomiClientSocket = (vsock)s;
    return 0;
}

int UISendReceive(const SendData* req, RecData* res){
    ErrorID_03 = 0;
    ErrorID_04 = 0;

    if (!req || !res) return -1;
    if ((SOCKET)CoyomiClientSocket == INVALID_SOCKET){
        return -1; /* ���ڑ� */
    }

    uint32_t words[6];
    BuildPacketFromSendData(req, words);

    if (send_all((SOCKET)CoyomiClientSocket, (const char*)words, (int)sizeof(words)) <= 0){
        ErrorID_03 = 1;
        return -1;
    }

    uint32_t inwords[6];
    if (recv_all((SOCKET)CoyomiClientSocket, (char*)inwords, (int)sizeof(inwords)) <= 0){
        ErrorID_04 = 1;
        return -1;
    }

    ParsePacketToRecData(inwords, res);
    return 0;
}

void CloseConnection(void){
    if ((SOCKET)CoyomiClientSocket != INVALID_SOCKET){
        closesocket((SOCKET)CoyomiClientSocket);
        CoyomiClientSocket = (vsock)INVALID_SOCKET;
    }
}

/* ========== �j�����i���C�h�����Œ��Ԃ��j ========== */
const wchar_t* GetWeekdayNameW(int w){
    static const wchar_t* tbl[7] = {
        L"���j��", L"���j��", L"�Ηj��", L"���j��", L"�ؗj��", L"���j��", L"�y�j��"
    };
    if (w < 0 || w > 6) return L"?";
    return tbl[w];
}

/* ========== ���ʕ\���i��E01..E04 �ŊY��������ɂ���j ========== */
void ShowResultStatus(const SendData* send, const RecData* recv,
                      int E01, int E02, int E03, int E04, int E05, int E06, int E07, int E08, int E09)
{
    /* ���ʗ� */
    if (OutputBoxResult && send){
        wchar_t buf[256];

        /* ���ʐM/����M�n�G���[��1�ł�����΁u�Y�����F�v�͋󗓂ŏo�� */
        if (E01 || E02 || E03 || E04){
            swprintf(buf, 256,
                L"�J�n���F%04u/%02u/%02u\r\n"
                L"�����F%d%s�i%s�j\r\n"
                L"�Y�����F",
                (unsigned)send->SendDataYear, (unsigned)send->SendDataMonth, (unsigned)send->SendDataDay,
                (int)send->SendDataNdays, (send->SendDataDirAfter?L"����":L"���O"),
                send->SendDataFlag01 ? L"1����" : L"0����"
            );
            SetWindowTextW(OutputBoxResult, buf);
        }else if (recv){
            /* ���펞�F�T�[�o����̔N�����{�j���\�� */
            SYSTEMTIME st = {0};
            st.wYear  = (WORD)recv->RecDataaYear;
            st.wMonth = (WORD)recv->RecDataMonth;
            st.wDay   = (WORD)recv->RecDataDay;

            SYSTEMTIME utc;
            TzSpecificLocalTimeToSystemTime(NULL, &st, &utc);
            FILETIME ft; SystemTimeToFileTime(&utc, &ft);
            SYSTEMTIME backUtc; FileTimeToSystemTime(&ft, &backUtc);
            int w = backUtc.wDayOfWeek % 7;

            const wchar_t* wname = GetWeekdayNameW(w);

            swprintf(buf,256,
                L"�J�n���F%04u/%02u/%02u\r\n"
                L"�����F%d%s�i%s�j\r\n"
                L"�Y�����F%04u/%02u/%02u�i%ls�j",
                (unsigned)send->SendDataYear, (unsigned)send->SendDataMonth, (unsigned)send->SendDataDay,
                (int)send->SendDataNdays, (send->SendDataDirAfter?L"����":L"���O"),
                send->SendDataFlag01 ? L"1����" : L"0����",
                (unsigned)recv->RecDataaYear, (unsigned)recv->RecDataMonth, (unsigned)recv->RecDataDay, wname
            );
            SetWindowTextW(OutputBoxResult, buf);
        }else{
            /* �O�̂��߂̃t�H�[���o�b�N�irecv�����j */
            swprintf(buf, 256,
                L"�J�n���F%04u/%02u/%02u\r\n"
                L"�����F%d%s�i%s�j\r\n"
                L"�Y�����F",
                (unsigned)send->SendDataYear, (unsigned)send->SendDataMonth, (unsigned)send->SendDataDay,
                (int)send->SendDataNdays, (send->SendDataDirAfter?L"����":L"���O"),
                send->SendDataFlag01 ? L"1����" : L"0����"
            );
            SetWindowTextW(OutputBoxResult, buf);
        }
    }

    /* ���b�Z�[�W���i�����s�\���j */
    if (OutputBoxMessage){
        wchar_t msg[1024];
        msg[0] = L'\0';
        size_t off = 0;
        #define APPEND_LINE(fmt, ...) do{ \
            int _n = swprintf(msg+off, (int)(sizeof(msg)/sizeof(msg[0]) - off), fmt L"\r\n", __VA_ARGS__); \
            if (_n > 0){ off += (size_t)_n; if (off >= (sizeof(msg)/sizeof(msg[0])) ) off = (sizeof(msg)/sizeof(msg[0]) - 1); } \
        }while(0)
        #define APPEND_CONSTLINE(wcs) do{ \
            int _n = swprintf(msg+off, (int)(sizeof(msg)/sizeof(msg[0]) - off), L"%ls\r\n", wcs); \
            if (_n > 0){ off += (size_t)_n; if (off >= (sizeof(msg)/sizeof(msg[0])) ) off = (sizeof(msg)/sizeof(msg[0]) - 1); } \
        }while(0)

        if (E01) APPEND_CONSTLINE(L"ErrorID_01---ConnectToCoyomi�ŃN���C�A���g�\�P�b�g���쐻���ɃG���[���������܂����D");
        if (E02) APPEND_CONSTLINE(L"ErrorID_02---ConnectToCoyomi��Coyomi�v���O�����ƒʐM�o���Ă��܂���D");
        if (E03) APPEND_CONSTLINE(L"ErrorID_03---UISentRecive�ő��M���o���܂���D");
        if (E04) APPEND_CONSTLINE(L"ErrorID_04---UISentRecive�Ŏ�M���o���܂���D");
        if (E05) APPEND_CONSTLINE(L"ErrorID_05---2010�N��1��1���`2099�N��12��31���ȊO�̊J�n�������͂��ꂽ�̂ō����ɓ��t�ɕύX���܂����D");
        if (E06) APPEND_CONSTLINE(L"ErrorID_06---10000�����傫�ȓ��������͂��ꂽ���߁C10000���Ƃ��܂����D");
        if (E07) APPEND_LINE    (L"ErrorID_07---�����_������̂ŁC�؂�グ%d���ɕύX���܂����D", (int)(send?send->SendDataNdays:0));
        if (E08) APPEND_CONSTLINE(L"ErrorID_08---�����_�̐؂�グ�̂��߁C10000���ȏ�ƂȂ����̂ŁC10000���ɕύX���܂����D");
        if (E09) APPEND_CONSTLINE(L"ErrorID_09---�����Ƃ��Ė����Ȓl�����͂��ꂽ���߁C1���ɕϊ����܂����i��F0��1�j�B");

        SetWindowTextW(OutputBoxMessage, msg);

        #undef APPEND_LINE
        #undef APPEND_CONSTLINE
    }
}

/* ========== ���t���Z�i�Q�l�j ========== */
SYSTEMTIME AddDays(SYSTEMTIME base, unsigned int ndays, bool dirAfter, bool flag01){
    (void)flag01;

    SYSTEMTIME utc;
    TzSpecificLocalTimeToSystemTime(NULL, &base, &utc);

    FILETIME ft;
    SystemTimeToFileTime(&utc, &ft);

    ULARGE_INTEGER u;
    u.LowPart  = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;

    const LONGLONG oneDay100ns = 864000000000LL;
    LONGLONG delta = (LONGLONG)ndays * oneDay100ns * (dirAfter ? +1 : -1);

    u.QuadPart += (ULONGLONG)delta;

    ft.dwLowDateTime  = u.LowPart;
    ft.dwHighDateTime = u.HighPart;

    SYSTEMTIME utc2, local2;
    FileTimeToSystemTime(&ft, &utc2);
    SystemTimeToTzSpecificLocalTime(NULL, &utc2, &local2);
    return local2;
}

/* ========== UI�i�q�R���g���[���쐬�j ========== */
static HFONT g_hFontNorm = NULL, g_hFontSmall = NULL;

static HFONT make_font(int pt, BOOL bold){
    HDC hdc = GetDC(NULL);
    int logpix = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);

    LOGFONTW lf = {0};
    lf.lfHeight = -MulDiv(pt, logpix, 72);
    lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
    wcscpy(lf.lfFaceName, L"Yu Gothic UI");
    return CreateFontIndirectW(&lf);
}

void createWindow(HWND hwnd, HINSTANCE hInst){
    if (!g_hFontNorm){  g_hFontNorm  = make_font(12, FALSE); }
    if (!g_hFontSmall){ g_hFontSmall = make_font(12, FALSE); } /* ���ʗ���12pt */

    /* �J�n���iDTP�j */
    StartData = CreateWindowExW(0, DATETIMEPICK_CLASS, L"",
                    WS_CHILD|WS_VISIBLE|WS_TABSTOP|DTS_SHORTDATEFORMAT,
                    GUI_DTP_XP, GUI_DTP_YP, GUI_DTP_XS, GUI_DTP_YS,
                    hwnd, (HMENU)IDC_DTP, hInst, NULL);
    SendMessageW(StartData, WM_SETFONT, (WPARAM)g_hFontNorm, TRUE);

    /* ���� EDIT */
    CalcDay = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                    WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,
                    GUI_EDIT_NDAYS_XP, GUI_EDIT_NDAYS_YP, GUI_EDIT_NDAYS_XS, GUI_EDIT_NDAYS_YS,
                    hwnd, (HMENU)IDC_EDIT_NDAYS, hInst, NULL);
    SendMessageW(CalcDay, WM_SETFONT, (WPARAM)g_hFontNorm, TRUE);

    /* ���W�I�F0/1���ځi��1���ڂ��f�t�H���g�j */
    RadioZero = CreateWindowExW(0, L"BUTTON", L"0����",
                    WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_GROUP|BS_AUTORADIOBUTTON,
                    GUI_RADIO_ZERO_XP, GUI_RADIO_ZERO_YP, GUI_RADIO_ZERO_XS, GUI_RADIO_ZERO_YS,
                    hwnd, (HMENU)IDC_RADIO_ZERO, hInst, NULL);
    RadioOne  = CreateWindowExW(0, L"BUTTON", L"1����",
                    WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
                    GUI_RADIO_ONE_XP, GUI_RADIO_ONE_YP, GUI_RADIO_ONE_XS, GUI_RADIO_ONE_YS,
                    hwnd, (HMENU)IDC_RADIO_ONE,  hInst, NULL);
    SendMessageW(RadioZero, WM_SETFONT, (WPARAM)g_hFontNorm, TRUE);
    SendMessageW(RadioOne,  WM_SETFONT, (WPARAM)g_hFontNorm, TRUE);
    SendMessageW(RadioOne,  BM_SETCHECK, BST_CHECKED, 0);   /* 1���ڂ������ */
    SendMessageW(RadioZero, BM_SETCHECK, BST_UNCHECKED, 0);

    /* ���W�I�F�O/��i�f�t�H���g�F��j */
    RadioBefore = CreateWindowExW(0, L"BUTTON", L"�O",
                    WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_GROUP|BS_AUTORADIOBUTTON,
                    GUI_RADIO_BEFORE_XP, GUI_RADIO_BEFORE_YP, GUI_RADIO_BEFORE_XS, GUI_RADIO_BEFORE_YS,
                    hwnd, (HMENU)IDC_RADIO_BEFORE, hInst, NULL);
    RadioAfter  = CreateWindowExW(0, L"BUTTON", L"��",
                    WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
                    GUI_RADIO_AFTER_XP, GUI_RADIO_AFTER_YP, GUI_RADIO_AFTER_XS, GUI_RADIO_AFTER_YS,
                    hwnd, (HMENU)IDC_RADIO_AFTER,  hInst, NULL);
    SendMessageW(RadioBefore, WM_SETFONT, (WPARAM)g_hFontNorm, TRUE);
    SendMessageW(RadioAfter,  WM_SETFONT, (WPARAM)g_hFontNorm, TRUE);
    SendMessageW(RadioAfter,  BM_SETCHECK, BST_CHECKED, 0);

    /* ���s/�I�� */
    HWND btnSend = CreateWindowExW(0, L"BUTTON", L"���s",
                    WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
                    GUI_BTN_SEND_XP, GUI_BTN_SEND_YP, GUI_BTN_SEND_XS, GUI_BTN_SEND_YS,
                    hwnd, (HMENU)IDC_BTN_SEND, hInst, NULL);
    HWND btnExit = CreateWindowExW(0, L"BUTTON", L"�I��",
                    WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
                    GUI_BTN_EXIT_XP, GUI_BTN_EXIT_YP, GUI_BTN_EXIT_XS, GUI_BTN_EXIT_YS,
                    hwnd, (HMENU)IDC_BTN_EXIT, hInst, NULL);
    SendMessageW(btnSend, WM_SETFONT, (WPARAM)g_hFontNorm, TRUE);
    SendMessageW(btnExit, WM_SETFONT, (WPARAM)g_hFontNorm, TRUE);

    /* ���ʁ^���b�Z�[�W�o�� */
    OutputBoxResult = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                    WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY|WS_VSCROLL,
                    GUI_EDIT_RESULT_XP, GUI_EDIT_RESULT_YP, GUI_EDIT_RESULT_XS, GUI_EDIT_RESULT_YS,
                    hwnd, (HMENU)IDC_EDIT_RESULT, hInst, NULL);
    OutputBoxMessage= CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                    WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY|WS_VSCROLL,
                    GUI_EDIT_STATUS_XP, GUI_EDIT_STATUS_YP, GUI_EDIT_STATUS_XS, GUI_EDIT_STATUS_YS,
                    hwnd, (HMENU)IDC_EDIT_STATUS, hInst, NULL);
    SendMessageW(OutputBoxResult,  WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);
    SendMessageW(OutputBoxMessage, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);
}

/* ========== ���[�J�[�X���b�h ========== */
typedef struct {
    SendData send;
    RecData  recv;
} ThreadCtx;

static unsigned __stdcall WorkerThread(void* p){
    ThreadCtx* ctx = (ThreadCtx*)p;

    if (connectToCoyomi() == 0){
        (void)UISendReceive(&ctx->send, &ctx->recv);
    }

    PostMessageW(MainWindow, WM_APP_RESULT, 0, (LPARAM)ctx);
    _endthreadex(0);
    return 0;
}

/* ========== ���N�G�X�g�J�n ========== */
static void UIProcessMain(void){
    if (OutputBoxMessage) SetWindowTextW(OutputBoxMessage, L"");
    if (OutputBoxResult)  SetWindowTextW(OutputBoxResult,  L"");

    InputData iv; CollectData(MainWindow, &iv);

    /* �V�K���͂��Ƃ� 05..09 ���N���A */
    ErrorID_05 = ErrorID_06 = ErrorID_07 = ErrorID_08 = 0;
    ErrorID_09 = 0;

    ThreadCtx* ctx = (ThreadCtx*)calloc(1, sizeof(ThreadCtx));
    if (!ctx){ set_status(L"(�����G���[) �������m�ێ��s"); return; }

    unsigned int normErr = InvalidValueHandling(&iv, &ctx->send);

    if (normErr==ERR_ID2_START_OUT_OF_RANGE || normErr==ERR_ID3_POSITIVE_ONLY){
        SYSTEMTIME now; GetLocalTime(&now);
        SendMessageW(StartData, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&now);
    }

    HANDLE h = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, ctx, 0, NULL);
    if (!h){
        set_status(L"(�����G���[) �X���b�h�N�����s");
        free(ctx);
        return;
    }
    CloseHandle(h);
}

/* ========== WndProc(��WinProc) ========== */
LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        createWindow(hwnd, hInst);
        return 0;
    }
    case WM_COMMAND:
        switch(LOWORD(wParam)){
            case IDC_BTN_SEND: UIProcessMain(); return 0;
            case IDC_BTN_EXIT: SendMessageW(hwnd, WM_CLOSE, 0, 0); return 0;
        }
        break;
    case WM_APP_RESULT:{
        ThreadCtx* ctx = (ThreadCtx*)lParam;
        ShowResultStatus(&ctx->send, &ctx->recv,
                         ErrorID_01, ErrorID_02, ErrorID_03, ErrorID_04,
                         ErrorID_05, ErrorID_06, ErrorID_07, ErrorID_08,
                         ErrorID_09);
        free(ctx);
        return 0;
    }
    case WM_DESTROY:
        CloseConnection();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* ========== �G���g���|�C���g ========== */
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR lpCmdLine, int nShow){
    (void)hPrev; (void)lpCmdLine;

    INITCOMMONCONTROLSEX icc = { sizeof(icc),
        ICC_DATE_CLASSES | ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WSADATA w;
    if (WSAStartup(MAKEWORD(2,2), &w) != 0){
        MessageBoxW(NULL, L"WSAStartup failed", L"Error", MB_ICONERROR);
        return 1;
    }

    WNDCLASSW wc = {0};
    wc.style         = CS_DBLCLKS;
    wc.lpfnWndProc   = WinProc;
    wc.hInstance     = hInst;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc.lpszClassName = APP_CLASS;
    if (!RegisterClassW(&wc)){
        MessageBoxW(NULL, L"RegisterClass failed", L"Error", MB_ICONERROR);
        WSACleanup();
        return 1;
    }

    MainWindow = CreateWindowExW(0, APP_CLASS, L"Coyomi Client (All-in-One)",
                    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                    CW_USEDEFAULT, CW_USEDEFAULT, 640, 280,
                    NULL, NULL, hInst, NULL);
    if (!MainWindow){
        MessageBoxW(NULL, L"CreateWindow failed", L"Error", MB_ICONERROR);
        WSACleanup();
        return 1;
    }

    ShowWindow(MainWindow, nShow);
    UpdateWindow(MainWindow);

    MSG m;
    while (GetMessageW(&m, NULL, 0, 0)){
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }

    WSACleanup();
    return 0;
}
