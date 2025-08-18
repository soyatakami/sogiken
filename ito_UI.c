//* �R���p�C����iMinGW-w64�j:
//* gcc Coyomi_Client_Test_AllInOne_DTP.c -o Coyomi_Client_Test_v7.exe -lcomctl32 -lws2_32 -mwindows -municode -std=c11 -Wall -Wextra -Wpedantic -O2 -finput-charset=CP932 -fexec-charset=CP932 -fwide-exec-charset=UTF-16LE

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

// --- Winsock�� windows.h ����� ---
#include <winsock2.h>
#include <ws2tcpip.h>

// --- Windows GUI ---
#include <windows.h>
#include <commctrl.h>   
// DateTime Picker / InitCommonControlsEx
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>
#include <wctype.h>
#include <math.h>

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof((a)[0]))
#endif

//==============================
// (����) Config ����
//==============================
#define SERVER_IP   "127.0.0.1"   // �ڑ���
#define SERVER_PORT 5000          // �ڑ���|�[�g

//==============================
// �G���[���b�Z�[�WID�i�擪�Ő錾�j
//==============================
typedef enum {
    ERR_NONE                     = 0,
    ERR_ID1_CONNECT              = 1, // Connect�ł��Ȃ�
    ERR_ID2_START_OUT_OF_RANGE   = 2, // �J�n�����L���͈͊O
    ERR_ID3_POSITIVE_ONLY        = 3, // ���p�����E���̐��̂� �� ������
    ERR_ID4_ROUNDUP              = 4, // �������؂�グ
    ERR_ID5_CAP_10000            = 5, // ��� 10000
    ERR_ID6_INVALID_INT          = 6, // ���p�����E���̐����ȊO �� 1��
    ERR_ID7_SEND_RECV_FAIL       = 7  // ����M5�A�����s
} ErrorID;

// �{���iID���x���̓R�[�h���ŕt����j
static const wchar_t* g_errmsg_ID1 =
    L"Connect���o���Ȃ��ꍇ \r\n\tCoyomiSever�A�v�����N�����Ă��邩�m�F���Ă��������D";

static const wchar_t* g_errmsg_ID2 =
    L"�J�n�����L���͈͊O�ł��D \r\n\t2010/01/01�`2099/12/31�ȊO�͈̔͂Ȃ̂�\r\n\t�����̓��t���J�n���Ɏw�肵�܂��D";

static const wchar_t* g_errmsg_ID3 =
    L"���p�����E���̐��̂ݓ��͉\�ł��D\r\n\t�����n�����͂��ꂽ�ׁC�����̓��t���J�n���Ɏw�肵�܂��D";

static const wchar_t* g_errmsg_ID4 =
    L"�����ł̓����̓��͂��L�����̂ŁD�����ꌅ�ڂŐ؂�グ�܂����D";

static const wchar_t* g_errmsg_ID5 =
    L"�����̏���i10000���j�𒴂��Ă���̂ŁC10000���ɕύX���܂����D";

static const wchar_t* g_errmsg_ID6 =
    L"���p�����E���̐����ȊO�̓��͂��������ׁC1���ɕύX���܂����D";

static const wchar_t* g_errmsg_ID7 =
    L"Sent�DReceive���A����5��ʐM���ł��Ȃ��ꍇ \r\n\t�ʐM���m���ł��܂���D";

// ID���{�� �擾
static inline const wchar_t* error_body_from_id(ErrorID id){
    switch(id){
        case ERR_ID1_CONNECT:            return g_errmsg_ID1;
        case ERR_ID2_START_OUT_OF_RANGE: return g_errmsg_ID2;
        case ERR_ID3_POSITIVE_ONLY:      return g_errmsg_ID3;
        case ERR_ID4_ROUNDUP:            return g_errmsg_ID4;
        case ERR_ID5_CAP_10000:          return g_errmsg_ID5;
        case ERR_ID6_INVALID_INT:        return g_errmsg_ID6;
        case ERR_ID7_SEND_RECV_FAIL:     return g_errmsg_ID7;
        default:                         return NULL;
    }
}

//==============================
// (����) Proto �����i�V�t�H�[�}�b�g�F6���[�h�j
//==============================
// ����M�Ɏg�p����\���́F�e32bit
// [0] �N, [1] ��, [2] ��, [3] �V�t�g����(uint32_t), [4] �t���O(����/0-1����/�j�����p�b�N), [5] �G���[ID
typedef struct {
    uint32_t year;      // 0
    uint32_t month;     // 1
    uint32_t day;       // 2
    uint32_t ndays;     // 3 (char�����񁨐��l�ɕϊ����Ċi�[)
    uint32_t flags;     // 4 (dir/flag01/weekday ���p�b�N)
    uint32_t error;     // 5
} CalendarPacket;

enum { PACKET_WORDS = 6 };

// flags(32bit)�̊��蓖��
// bit0 : direction (0=�O/�ߋ�, 1=��/����)
// bit1 : flag01    (0=0����,  1=1����)
// bit[10:8] : weekday (0..6)
// ����ȊO��bit��0
#define FLAG_DIR_MASK      0x00000001u
#define FLAG_FLAG01_MASK   0x00000002u
#define FLAG_WD_MASK       0x00000700u
#define FLAG_WD_SHIFT      8

static inline uint32_t pack_flags(bool dir, bool flag01, short weekday){
    uint32_t f = 0;
    if(dir)    f |= FLAG_DIR_MASK;
    if(flag01) f |= FLAG_FLAG01_MASK;
    if(weekday >= 0){ f |= ((uint32_t)(weekday & 0x7)) << FLAG_WD_SHIFT; }
    return f;
}
static inline short  flags_get_weekday(uint32_t f){ return (short)((f & FLAG_WD_MASK) >> FLAG_WD_SHIFT); }

//==============================
// (����) Wire �����i�P�������j
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
static void pack_to_wire(const CalendarPacket* p, uint32_t netbuf[PACKET_WORDS]){
    uint32_t t[PACKET_WORDS] = {
        p->year, p->month, p->day, p->ndays, p->flags, p->error
    };
    for (int i=0;i<PACKET_WORDS;++i) netbuf[i] = htonl(t[i]);
}
static void unpack_from_wire(CalendarPacket* p, const uint32_t netbuf[PACKET_WORDS]){
    uint32_t t[PACKET_WORDS];
    for (int i=0;i<PACKET_WORDS;++i) t[i] = ntohl(netbuf[i]);
    p->year  = t[0];
    p->month = t[1];
    p->day   = t[2];
    p->ndays = t[3];
    p->flags = t[4];
    p->error = t[5];
}

//==============================
// (����) Network ����
//==============================
static int net_init(void){
    WSADATA w;
    return (WSAStartup(MAKEWORD(2,2), &w)==0) ? 0 : -1;
}
static void net_cleanup(void){
    WSACleanup();
}
static SOCKET net_connect_server(void){
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if(s==INVALID_SOCKET) return INVALID_SOCKET;

    struct sockaddr_in a;
    a.sin_family = AF_INET;
    a.sin_port   = htons(SERVER_PORT);
    a.sin_addr.s_addr = inet_addr(SERVER_IP);

    if(connect(s,(struct sockaddr*)&a,sizeof(a))==SOCKET_ERROR){
        closesocket(s);
        return INVALID_SOCKET;
    }
    return s;
}
static void net_close(SOCKET s){
    if (s != INVALID_SOCKET) closesocket(s);
}
// 6���[�h(24B)�̑��M/��M�i�p�P�b�g�P�ʁj
static int net_send_packet(SOCKET s, const CalendarPacket* pkt){
    uint32_t netbuf[PACKET_WORDS];
    pack_to_wire(pkt, netbuf);
    return send_all(s, (const char*)netbuf, (int)sizeof(netbuf));
}
static int net_recv_packet(SOCKET s, CalendarPacket* pkt){
    uint32_t netbuf[PACKET_WORDS];
    int r = recv_all(s, (char*)netbuf, (int)sizeof(netbuf));
    if (r <= 0) return r;
    unpack_from_wire(pkt, netbuf);
    return r;
}

//==============================
// GUI �{��
//==============================

// ===== �t�H���g�w���p�[ =====
#define FONT_FACE_DEFAULT L"Meiryo UI"

typedef struct { int pt; HFONT h; } FontEntry;
static FontEntry g_fonts[16];

static HFONT GetFontPt(HWND hwndRef, int pt, LPCWSTR face){
    for (int i=0;i<(int)(sizeof(g_fonts)/sizeof(g_fonts[0]));++i){
        if (g_fonts[i].h && g_fonts[i].pt==pt) return g_fonts[i].h;
    }
    HDC hdc = GetDC(hwndRef);
    int dpi = hdc ? GetDeviceCaps(hdc, LOGPIXELSY) : 96;
    if (hdc) ReleaseDC(hwndRef, hdc);

    LOGFONTW lf = {0};
    lf.lfHeight = -MulDiv(pt, dpi, 72);
    if (face && *face) wcsncpy(lf.lfFaceName, face, LF_FACESIZE-1);

    HFONT h = CreateFontIndirectW(&lf);
    for (int i=0;i<(int)(sizeof(g_fonts)/sizeof(g_fonts[0]));++i){
        if (!g_fonts[i].h){ g_fonts[i].h=h; g_fonts[i].pt=pt; break; }
    }
    return h;
}
#define APPLY_FONTPT(hWnd, pt) do{ \
    if(hWnd){ HFONT _f = GetFontPt(hWnd, (pt), FONT_FACE_DEFAULT); \
              SendMessageW((hWnd), WM_SETFONT, (WPARAM)_f, TRUE);} \
}while(0)

static void CleanupFonts(void){
    for (int i=0;i<(int)(sizeof(g_fonts)/sizeof(g_fonts[0]));++i){
        if (g_fonts[i].h){ DeleteObject(g_fonts[i].h); g_fonts[i].h=NULL; g_fonts[i].pt=0; }
    }
}

// ���� ��ʃe�L�X�g ������������������������������������������������������������������������������������������
#define GUI_Comment_1    L"�J�n��"
#define GUI_Comment_2    L"�L���͈�\r\n2010/01/01�`2099/12/31\r\n*���p�����E���̐����̂ݓ��͉�"
#define GUI_Comment_3    L"�J�n����"
#define GUI_Comment_4    L"�Ƃ���D"
#define GUI_Comment_5    L"�J�n����"
#define GUI_Comment_6    L"��"
#define GUI_Comment_7    L"*���p�����E���̐����̂ݓ��͉�"
#define GUI_Comment_8    L"[���b�Z�[�W�\����]"
#define GUI_Comment_9    L"[�v�Z���ʕ\����]"

#define GUI_Execution    L"���s"
#define GUI_End          L"�I��"

#define GUI_RadioBox_Before L"�O"
#define GUI_RadioBox_After  L"��"
#define GUI_RadioBox_Zero   L"0����"
#define GUI_RadioBox_One    L"1����"

// ���� �E�B���h�E�T�C�Y�i�N���C�A���g��j ������������������������������������������
static int   GUI_Size    = 300; // ����(px)
static int   GUI_Ratio_X = 10;
static int   GUI_Ratio_Y = 18;
static int   g_winW      = 0;
static int   g_winH      = 0;
static DWORD g_fixedStyle= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

static void ComputeWindowSize(void){
    int clientW = (int)((long long)GUI_Size * GUI_Ratio_Y / GUI_Ratio_X);
    RECT rc = {0, 0, clientW, GUI_Size};
    AdjustWindowRectEx(&rc, g_fixedStyle, FALSE, 0);
    g_winW = rc.right - rc.left;
    g_winH = rc.bottom - rc.top;
}

// ���� �z�u�E�t�H���g�w�� ������������������������������������������������������������������������������
static int  GUI_Comment_1_Xp = 20;
static int  GUI_Comment_1_Yp = 25;
static int  GUI_Comment_1_Xs = 60;
static int  GUI_Comment_1_Ys = 20;
static int  GUI_Comment_1_pt = 12;

static int  GUI_Calendar_Xp = 100;
static int  GUI_Calendar_Yp = 20;
static int  GUI_Calendar_Xs = 200;
static int  GUI_Calendar_Ys = 30;
static int  GUI_Calendar_pt = 12;

static int  GUI_Comment_2_Xp = 320;
static int  GUI_Comment_2_Yp = 20;
static int  GUI_Comment_2_Xs = 200;
static int  GUI_Comment_2_Ys = 35;
static int  GUI_Comment_2_pt = 6;

static int  GUI_Comment_3_Xp = 20;
static int  GUI_Comment_3_Yp = 75;
static int  GUI_Comment_3_Xs = 75;
static int  GUI_Comment_3_Ys = 20;
static int  GUI_Comment_3_pt = 12;

static int  GUI_RadioBox_Zero_Xp = 100;
static int  GUI_RadioBox_Zero_Yp = 75;
static int  GUI_RadioBox_Zero_Xs = 75;
static int  GUI_RadioBox_Zero_Ys = 20;
static int  GUI_RadioBox_Zero_pt = 12;

static int  GUI_RadioBox_One_Xp = 180;
static int  GUI_RadioBox_One_Yp = 75;
static int  GUI_RadioBox_One_Xs = 75;
static int  GUI_RadioBox_One_Ys = 20;
static int  GUI_RadioBox_One_pt = 12;

static int  GUI_Comment_4_Xp = 260;
static int  GUI_Comment_4_Yp = 75;
static int  GUI_Comment_4_Xs = 75;
static int  GUI_Comment_4_Ys = 20;
static int  GUI_Comment_4_pt = 12;

static int  GUI_Comment_5_Xp = 20;
static int  GUI_Comment_5_Yp = 125;
static int  GUI_Comment_5_Xs = 75;
static int  GUI_Comment_5_Ys = 20;
static int  GUI_Comment_5_pt = 12;

static int  GUI_ShiftDays_Xp = 100;
static int  GUI_ShiftDays_Yp = 125;
static int  GUI_ShiftDays_Xs = 75;
static int  GUI_ShiftDays_Ys = 25;
static int  GUI_ShiftDays_pt = 12;

static int  GUI_Comment_6_Xp = 180;
static int  GUI_Comment_6_Yp = 125;
static int  GUI_Comment_6_Xs = 25;
static int  GUI_Comment_6_Ys = 20;
static int  GUI_Comment_6_pt = 12;

static int  GUI_RadioBox_Before_Xp = 210;
static int  GUI_RadioBox_Before_Yp = 110;
static int  GUI_RadioBox_Before_Xs = 50;
static int  GUI_RadioBox_Before_Ys = 20;
static int  GUI_RadioBox_Before_pt = 12;

static int  GUI_RadioBox_After_Xp = 210;
static int  GUI_RadioBox_After_Yp = 140;
static int  GUI_RadioBox_After_Xs = 50;
static int  GUI_RadioBox_After_Ys = 20;
static int  GUI_RadioBox_After_pt = 12;

static int  GUI_Comment_7_Xp = 20;
static int  GUI_Comment_7_Yp = 155;
static int  GUI_Comment_7_Xs = 200;
static int  GUI_Comment_7_Ys = 12;
static int  GUI_Comment_7_pt = 6;

static int  GUI_Execution_Xp = 270;
static int  GUI_Execution_Yp = 125;
static int  GUI_Execution_Xs = 100;
static int  GUI_Execution_Ys = 25;
static int  GUI_Execution_pt = 12;

static int  GUI_End_Xp = 400;
static int  GUI_End_Yp = 125;
static int  GUI_End_Xs = 100;
static int  GUI_End_Ys = 25;
static int  GUI_End_pt = 12;

static int  GUI_Comment_8_Xp = 20;
static int  GUI_Comment_8_Yp = 170;
static int  GUI_Comment_8_Xs = 150;
static int  GUI_Comment_8_Ys = 20;
static int  GUI_Comment_8_pt = 12;

static int  GUI_Comment_9_Xp = 270;
static int  GUI_Comment_9_Yp = 170;
static int  GUI_Comment_9_Xs = 150;
static int  GUI_Comment_9_Ys = 20;
static int  GUI_Comment_9_pt = 12;

static int  GUI_Message_Xp = 20;
static int  GUI_Message_Yp = 200;
static int  GUI_Message_Xs = 225;
static int  GUI_Message_Ys = 75;
static int  GUI_Message_pt = 6;

static int  GUI_OutAll_Xp  = 270;
static int  GUI_OutAll_Yp  = 200;
static int  GUI_OutAll_Xs  = 225;
static int  GUI_OutAll_Ys  = 75;
static int  GUI_OutAll_pt  = 10;

// ���� �E�B���h�E/���b�Z�[�W��` ������������������������������������������������������������������
#define APP_CLASS     L"CoyomiClientWnd"
#define WM_APP_RESULT (WM_APP + 1)

// ���� �R���g���[��ID ��������������������������������������������������������������������������������������
enum {
    IDC_NDAYS=1100,
    IDC_DIR_BACK, IDC_DIR_FWD,
    IDC_FLAG0, IDC_FLAG1,
    IDC_SEND,
    IDC_OUTALL,
    IDC_STATUS,
    IDC_EXIT,
    IDC_DTP,
    IDC_COMMENT = 2001
};

// ���� �n���h���� ����������������������������������������������������������������������������������������������
static HWND hGUI_RadioBox_Zero, hGUI_RadioBox_One;
static HWND hGUI_RadioBox_Before, hGUI_RadioBox_After;
static HWND hGUI_Calendar;   // �� DTP�i���ꂪ�J�����_�[�j
static HWND hGUI_ShiftDays;
static HWND hGUI_Execution, hGUI_End;
static HWND hGUI_Comment_1,hGUI_Comment_2,hGUI_Comment_3,hGUI_Comment_4;
static HWND hGUI_Comment_5,hGUI_Comment_6,hGUI_Comment_7,hGUI_Comment_8;
static HWND hGUI_Comment_9;
static HWND hGUI_Result,hGUI_Message;

// ���C���E�B���h�E
static HWND g_main_hwnd = NULL;

// ���� �I������ ��������������������������������������������������������������������������������������������������
static HANDLE g_hWorker = NULL;
static volatile LONG g_alive = 1;

// ���� �j���\�L�i���{��j ������������������������������������������������������������������������������
static const wchar_t* weekday_str_ja(int w){
    static const wchar_t* tbl[]={L"���j��",L"���j��",L"�Ηj��",L"���j��",L"�ؗj��",L"���j��",L"�y�j��"};
    return (w>=0 && w<7)? tbl[w] : L"?";
}

static void set_status(LPCWSTR s){ if(hGUI_Message) SetWindowTextW(hGUI_Message, s); }

//==============================
// ���́��G���[����
//==============================

// �L���͈́i�J�n���p�j
#define MIN_Y 2010
#define MIN_M 1
#define MIN_D 1
#define MAX_Y 2099
#define MAX_M 12
#define MAX_D 31

static bool in_allowed_range_yMd(int y,int m,int d){
    if (y < MIN_Y) return false;
    if (y > MAX_Y) return false;
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

// �����񖖔��ɒǋL
static void append_line(wchar_t* buf, size_t cap, const wchar_t* line){
    if (!buf || !line) return;
    size_t cur = wcslen(buf);
    if (cur >= cap-1) return;
    _snwprintf(buf+cur, cap-cur, L"%ls%ls", (cur?L"\r\n":L""), line);
}

// ID�t���ŒǋL
static void add_error_by_id(wchar_t* buf, size_t cap, ErrorID id){
    const wchar_t* body = error_body_from_id(id);
    if (!body) return;
    wchar_t line[768];
    _snwprintf(line, _countof(line), L"ID%d\t%ls", id, body);
    append_line(buf, cap, line);
}

// ndays ��́E�␳�iID4/5/6�A�K�v�ɉ���ID3���j
static uint32_t parse_and_fix_ndays(const wchar_t* s, wchar_t* msgbuf, size_t msgcap, bool* out_triggered_id3){
    if (out_triggered_id3) *out_triggered_id3 = false;

    // ���󔒂݂͖̂���
    if(!s || !*s){
        add_error_by_id(msgbuf, msgcap, ERR_ID6_INVALID_INT);
        if (out_triggered_id3){
            add_error_by_id(msgbuf, msgcap, ERR_ID3_POSITIVE_ONLY);
            *out_triggered_id3 = true;
        }
        return 1;
    }

    // �O��̋󔒂��X�L�b�v
    while(*s==L' '||*s==L'\t'||*s==L'\r'||*s==L'\n') ++s;
    if(!*s){
        add_error_by_id(msgbuf, msgcap, ERR_ID6_INVALID_INT);
        if (out_triggered_id3){
            add_error_by_id(msgbuf, msgcap, ERR_ID3_POSITIVE_ONLY);
            *out_triggered_id3 = true;
        }
        return 1;
    }

    // �����_�̗L��
    bool has_dot = (wcschr(s, L'.') != NULL);

    wchar_t* endp = NULL;
    double val = wcstod(s, &endp);

    // ��͕s�� or �]�蕶������ �� ����
    bool extra = false;
    if (endp){
        while(*endp){
            if (!iswspace(*endp)) { extra = true; break; }
            ++endp;
        }
    }else{
        extra = true;
    }

    if (!isfinite(val) || val <= 0.0 || extra){
        add_error_by_id(msgbuf, msgcap, ERR_ID6_INVALID_INT);
        if (out_triggered_id3){
            add_error_by_id(msgbuf, msgcap, ERR_ID3_POSITIVE_ONLY);
            *out_triggered_id3 = true;
        }
        return 1;
    }

    uint32_t n;
    if (has_dot){
        // ���� �� �؂�グ
        double c = ceil(val);
        if (c < 1.0) c = 1.0;
        if (c > 1e9) c = 1e9; // �I�[�o�[�t���[�ی�
        n = (uint32_t)c;
        add_error_by_id(msgbuf, msgcap, ERR_ID4_ROUNDUP);
    }else{
        // ����
        if (val < 1.0){
            add_error_by_id(msgbuf, msgcap, ERR_ID6_INVALID_INT);
            return 1;
        }
        if (val > 1e9) val = 1e9;
        n = (uint32_t)val;
    }

    // ��� 10000
    if (n > 10000){
        n = 10000;
        add_error_by_id(msgbuf, msgcap, ERR_ID5_CAP_10000);
    }
    return n;
}

//==============================
// ���͒l�擾�p�̍\���́iGUI���A�v�������ێ��j
//==============================
typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    char     shift_str_utf8[32]; // �Q�l�ۑ��i���g�p�ł��j
    bool     dir_after;          // �O/��itrue=��, false=�O�j
    bool     flag01;             // 0/1���ځitrue=1���ځj
} InputValues;

// �X���b�h��GUI�֓n�����˃f�[�^
typedef struct {
    CalendarPacket pkt;     // ����M�p�i6���[�h�j
    uint32_t start_y, start_m, start_d; // ���M���̊J�n���i�\���p�j
    uint32_t ndays_ui;      // ���͂̃V�t�g�����i�\���p�j
    bool dir_after;         // �\���p
    wchar_t errbuf[1024];   // ������ID1?ID7�̃��b�Z�[�W���
} ClientResult;

// ���� �܂Ƃߌ��ʕ������EDIT�ɔ��f ����������������������������������������������������������
static void show_result_to_edit(const ClientResult* r){
    if (!hGUI_Result) return;

    const wchar_t* dir = r->dir_after ? L"����" : L"���O";
    short wd = (short)flags_get_weekday(r->pkt.flags);

    wchar_t line1[64], line2[64], line3[96], msg[320];
    swprintf(line1, 64, L"�J�n���F%04u/%02u/%02u",
             r->start_y, r->start_m, r->start_d);
    swprintf(line2, 64, L"�����F%u%s",
             r->ndays_ui, dir);
    swprintf(line3, 96, L"�Y�����F %04u/%02u/%02u�i%s�j",
             r->pkt.year, r->pkt.month, r->pkt.day, weekday_str_ja((int)wd));

    swprintf(msg, 320, L"%s\r\n%s\r\n%s", line1, line2, line3);
    SetWindowTextW(hGUI_Result, msg);

    // ���b�Z�[�W���F�G���[������ꍇ�̂ݕ\���A������΋�
    if (r->errbuf[0]) set_status(r->errbuf);
    else              set_status(L"");
}

// ���� ���s(����M)�X���b�h ��������������������������������������������������������������������������
// 1) connect ���s �� ID1
// 2) connect ������ASend/Recv �A��5�񎸔s �� ID7
static unsigned __stdcall SendThread(void* arg){
    ClientResult* r = (ClientResult*)arg;

    r->pkt.error = 0;

    SOCKET s = net_connect_server();
    if (s == INVALID_SOCKET) {
        add_error_by_id(r->errbuf, _countof(r->errbuf), ERR_ID1_CONNECT);
    } else {
        int fail_sr = 0;
        for (;;){
            if (net_send_packet(s, &r->pkt) > 0 && net_recv_packet(s, &r->pkt) > 0){
                break; // OK
            }
            fail_sr++;
            if (fail_sr >= 5){
                add_error_by_id(r->errbuf, _countof(r->errbuf), ERR_ID7_SEND_RECV_FAIL);
                break;
            }
            // ���g���C�͈�U�\�P�b�g����Đڑ���蒼��
            net_close(s);
            s = net_connect_server();
            if (s == INVALID_SOCKET){
                add_error_by_id(r->errbuf, _countof(r->errbuf), ERR_ID1_CONNECT);
                break;
            }
        }
        net_close(s);
    }

    if (g_alive && IsWindow(g_main_hwnd)) {
        PostMessageW(g_main_hwnd, WM_APP_RESULT, 0, (LPARAM)r);
    } else {
        free(r);
    }
    _endthreadex(0);
    return 0;
}

// ���� �R���g���[�������i�z�u�E�t�H���g�w������ׂĎg�p�j ��������������
static void create_controls(HWND hwnd){
    // �u�J�n���v���x��
    hGUI_Comment_1 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_1, WS_CHILD|WS_VISIBLE,
        GUI_Comment_1_Xp, GUI_Comment_1_Yp, GUI_Comment_1_Xs, GUI_Comment_1_Ys,
        hwnd, 0, 0, 0);
    APPLY_FONTPT(hGUI_Comment_1, GUI_Comment_1_pt);

    // ���t(DTP) ����=����
    hGUI_Calendar = CreateWindowExW(
        0, DATETIMEPICK_CLASSW, L"",
        WS_CHILD|WS_VISIBLE|DTS_SHORTDATEFORMAT,
        GUI_Calendar_Xp, GUI_Calendar_Yp, GUI_Calendar_Xs, GUI_Calendar_Ys,
        hwnd,(HMENU)IDC_DTP,0,0);
    SYSTEMTIME st; GetLocalTime(&st);
    SendMessageW(hGUI_Calendar, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    APPLY_FONTPT(hGUI_Calendar, GUI_Calendar_pt);

    // �u�L���͈́v���x���i�����s�j
    hGUI_Comment_2 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_2,
        WS_CHILD|WS_VISIBLE|SS_LEFT|SS_EDITCONTROL|SS_NOPREFIX,
        GUI_Comment_2_Xp, GUI_Comment_2_Yp, GUI_Comment_2_Xs, GUI_Comment_2_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(hGUI_Comment_2, GUI_Comment_2_pt);

    // �u�J�n�����v
    hGUI_Comment_3 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_3, WS_CHILD|WS_VISIBLE,
        GUI_Comment_3_Xp, GUI_Comment_3_Yp, GUI_Comment_3_Xs, GUI_Comment_3_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(hGUI_Comment_3, GUI_Comment_3_pt);

    // 0/1���ځi���W�I�j
    hGUI_RadioBox_Zero = CreateWindowExW(
        0, L"BUTTON", GUI_RadioBox_Zero,
        WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON|WS_GROUP,
        GUI_RadioBox_Zero_Xp, GUI_RadioBox_Zero_Yp, GUI_RadioBox_Zero_Xs, GUI_RadioBox_Zero_Ys,
        hwnd,(HMENU)IDC_FLAG0,0,0);
    APPLY_FONTPT(hGUI_RadioBox_Zero, GUI_RadioBox_Zero_pt);

    hGUI_RadioBox_One = CreateWindowExW(
        0, L"BUTTON", GUI_RadioBox_One,
        WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
        GUI_RadioBox_One_Xp, GUI_RadioBox_One_Yp, GUI_RadioBox_One_Xs, GUI_RadioBox_One_Ys,
        hwnd,(HMENU)IDC_FLAG1,0,0);
    APPLY_FONTPT(hGUI_RadioBox_One, GUI_RadioBox_One_pt);

    SendMessageW(hGUI_RadioBox_Zero, BM_SETCHECK, BST_CHECKED, 0); // ����F0����

    // �u�Ƃ���v
    hGUI_Comment_4 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_4, WS_CHILD|WS_VISIBLE,
        GUI_Comment_4_Xp, GUI_Comment_4_Yp, GUI_Comment_4_Xs, GUI_Comment_4_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(hGUI_Comment_4, GUI_Comment_4_pt);

    // �u�J�n���́v
    hGUI_Comment_5 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_5, WS_CHILD|WS_VISIBLE,
        GUI_Comment_5_Xp, GUI_Comment_5_Yp, GUI_Comment_5_Xs, GUI_Comment_5_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(hGUI_Comment_5, GUI_Comment_5_pt);

    // ShiftDays ���́i�������󂯂���悤 ES_NUMBER �͕t���Ȃ��j
    hGUI_ShiftDays = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"7",
        WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
        GUI_ShiftDays_Xp, GUI_ShiftDays_Yp, GUI_ShiftDays_Xs, GUI_ShiftDays_Ys,
        hwnd,(HMENU)IDC_NDAYS,0,0);
    APPLY_FONTPT(hGUI_ShiftDays, GUI_ShiftDays_pt);

    // �u���v
    hGUI_Comment_6 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_6, WS_CHILD|WS_VISIBLE,
        GUI_Comment_6_Xp, GUI_Comment_6_Yp, GUI_Comment_6_Xs, GUI_Comment_6_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(hGUI_Comment_6, GUI_Comment_6_pt);

    // �O/��i���W�I�j ����F�O
    hGUI_RadioBox_Before = CreateWindowExW(
        0, L"BUTTON", GUI_RadioBox_Before,
        WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON|WS_GROUP,
        GUI_RadioBox_Before_Xp, GUI_RadioBox_Before_Yp, GUI_RadioBox_Before_Xs, GUI_RadioBox_Before_Ys,
        hwnd,(HMENU)IDC_DIR_BACK,0,0);
    APPLY_FONTPT(hGUI_RadioBox_Before, GUI_RadioBox_Before_pt);

    hGUI_RadioBox_After  = CreateWindowExW(
        0, L"BUTTON", GUI_RadioBox_After,
        WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
        GUI_RadioBox_After_Xp, GUI_RadioBox_After_Yp, GUI_RadioBox_After_Xs, GUI_RadioBox_After_Ys,
        hwnd,(HMENU)IDC_DIR_FWD,0,0);
    APPLY_FONTPT(hGUI_RadioBox_After, GUI_RadioBox_After_pt);

    SendMessageW(hGUI_RadioBox_Before, BM_SETCHECK, BST_CHECKED, 0);

    // ���ӏ���
    hGUI_Comment_7 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_7, WS_CHILD|WS_VISIBLE,
        GUI_Comment_7_Xp, GUI_Comment_7_Yp, GUI_Comment_7_Xs, GUI_Comment_7_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(hGUI_Comment_7, GUI_Comment_7_pt);

    // ���s/�I���{�^��
    hGUI_Execution = CreateWindowExW(
        0,L"BUTTON", GUI_Execution, WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
        GUI_Execution_Xp, GUI_Execution_Yp, GUI_Execution_Xs, GUI_Execution_Ys,
        hwnd,(HMENU)IDC_SEND,0,0);
    APPLY_FONTPT(hGUI_Execution, GUI_Execution_pt);

    hGUI_End = CreateWindowExW(
        0,L"BUTTON", GUI_End, WS_CHILD|WS_VISIBLE,
        GUI_End_Xp, GUI_End_Yp, GUI_End_Xs, GUI_End_Ys,
        hwnd,(HMENU)IDC_EXIT,0,0);
    APPLY_FONTPT(hGUI_End, GUI_End_pt);

    // ���x���i���b�Z�[�W/���ʁj
    hGUI_Comment_8 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_8, WS_CHILD|WS_VISIBLE,
        GUI_Comment_8_Xp, GUI_Comment_8_Yp, GUI_Comment_8_Xs, GUI_Comment_8_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(hGUI_Comment_8, 12);

    hGUI_Comment_9 = CreateWindowExW(
        0,L"STATIC", GUI_Comment_9, WS_CHILD|WS_VISIBLE,
        GUI_Comment_9_Xp, GUI_Comment_9_Yp, GUI_Comment_9_Xs, GUI_Comment_9_Ys,
        hwnd,0,0,0);
    APPLY_FONTPT(hGUI_Comment_9, 12);

    // ���b�Z�[�WEDIT�i�����͋�j
    hGUI_Message = CreateWindowExW(
        WS_EX_CLIENTEDGE,L"EDIT", L"",
        WS_CHILD|WS_VISIBLE|ES_READONLY|ES_MULTILINE|ES_AUTOVSCROLL,
        GUI_Message_Xp, GUI_Message_Yp, GUI_Message_Xs, GUI_Message_Ys,
        hwnd,(HMENU)IDC_STATUS,0,0);
    APPLY_FONTPT(hGUI_Message, GUI_Message_pt);

    // �܂Ƃߏo��EDIT
    hGUI_Result = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_WANTRETURN,
        GUI_OutAll_Xp, GUI_OutAll_Yp, GUI_OutAll_Xs, GUI_OutAll_Ys,
        hwnd, (HMENU)IDC_OUTALL, NULL, NULL);
    APPLY_FONTPT(hGUI_Result, GUI_OutAll_pt);
}

// ���� ���N�G�X�g�J�n ��������������������������������������������������������������������������������������
// ���s�{�^�� �� ���͒l���W��(InputValues) �� ����M�p�\���̂֕ϊ�(CalendarPacket)
// ������ ID2/ID3/ID4/ID5/ID6 �𔻒肵�A�K�v�Ȃ� DTP �������ɍ��킹��
static void start_request(HWND hwnd){
    // �O��\���N���A
    if (hGUI_Message) SetWindowTextW(hGUI_Message, L"");
    if (hGUI_Result)  SetWindowTextW(hGUI_Result,  L"");

    InputValues iv = {0};
    wchar_t errbuf[1024] = L"";

    // DTP����J�n���擾
    SYSTEMTIME st;
    LRESULT r = SendMessageW(hGUI_Calendar, DTM_GETSYSTEMTIME, 0, (LPARAM)&st);
    if (r != GDT_VALID) {
        GetLocalTime(&st);
    }
    iv.year  = st.wYear;
    iv.month = st.wMonth;
    iv.day   = st.wDay;

    // �͈͊O�Ȃ獡���ɕύX�iID2�j
    if (!in_allowed_range_yMd((int)iv.year,(int)iv.month,(int)iv.day)){
        SYSTEMTIME now; GetLocalTime(&now);
        iv.year=now.wYear; iv.month=now.wMonth; iv.day=now.wDay;
        SendMessageW(hGUI_Calendar, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&now);
        add_error_by_id(errbuf, _countof(errbuf), ERR_ID2_START_OUT_OF_RANGE);
    }

    // �V�t�g�����iEDIT�������񁨉�́j
    wchar_t buf[64]; GetWindowTextW(hGUI_ShiftDays, buf, 64);
    // UTF-8�ۑ��i�I�v�V�����j
    WideCharToMultiByte(CP_UTF8,0,buf,-1,iv.shift_str_utf8,(int)sizeof(iv.shift_str_utf8),NULL,NULL);

    bool trigger_id3=false;
    uint32_t ndays_fixed = parse_and_fix_ndays(buf, errbuf, _countof(errbuf), &trigger_id3);
    if (trigger_id3){
        // �u�������J�n���v�ɂ���iDTP���X�V�j
        SYSTEMTIME now; GetLocalTime(&now);
        iv.year=now.wYear; iv.month=now.wMonth; iv.day=now.wDay;
        SendMessageW(hGUI_Calendar, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&now);
    }

    // �O/��E0/1����
    iv.dir_after = (SendMessageW(hGUI_RadioBox_After, BM_GETCHECK, 0, 0)==BST_CHECKED);
    iv.flag01    = (SendMessageW(hGUI_RadioBox_One,   BM_GETCHECK, 0, 0)==BST_CHECKED);

    // �X���b�h�֓n�����˃f�[�^�i�\���p�����܂߂�j
    ClientResult* cr = (ClientResult*)calloc(1, sizeof(ClientResult));
    if(!cr){
        append_line(errbuf, _countof(errbuf), L"(�����G���[) �������m�ۂɎ��s���܂����B");
        set_status(errbuf);
        return;
    }

    cr->start_y = iv.year; cr->start_m = iv.month; cr->start_d = iv.day;
    cr->ndays_ui = ndays_fixed;
    cr->dir_after = iv.dir_after;

    // ����M�p�p�P�b�g���쐬�i6���[�h�j
    cr->pkt.year  = iv.year;
    cr->pkt.month = iv.month;
    cr->pkt.day   = iv.day;
    cr->pkt.ndays = ndays_fixed;
    cr->pkt.flags = pack_flags(iv.dir_after, iv.flag01, 0); // �j���̓T�[�o�Őݒ�
    cr->pkt.error = 0;

    // �����_�̓��̓G���[�����p��
    wcsncpy(cr->errbuf, errbuf, _countof(cr->errbuf)-1);

    EnableWindow(hGUI_Execution, FALSE);

    HANDLE th = (HANDLE)_beginthreadex(NULL,0,SendThread,cr,0,NULL);
    if(th){
        g_hWorker = th;
    }else{
        append_line(errbuf, _countof(errbuf), L"(�����G���[) �X���b�h�N���Ɏ��s���܂����B");
        set_status(errbuf);
        free(cr);
        EnableWindow(hGUI_Execution, TRUE);
    }
}

// ���� WndProc ����������������������������������������������������������������������������������������������������
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
    case WM_CREATE:
        create_controls(hwnd);
        return 0;

    case WM_COMMAND:
        if(LOWORD(wParam)==IDC_SEND){ start_request(hwnd); return 0; }
        if(LOWORD(wParam)==IDC_EXIT){ SendMessageW(hwnd, WM_CLOSE, 0, 0); return 0; }
        break;

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }

    // �Œ�T�C�Y�i�ŏ�=�ő�j
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

        show_result_to_edit(res);              // ���ʂ��܂Ƃ�EDIT��
        EnableWindow(hGUI_Execution, TRUE);

        if (g_hWorker){ CloseHandle(g_hWorker); g_hWorker = NULL; }

        free(res);
        return 0;
    }

    case WM_CLOSE:
        InterlockedExchange(&g_alive, 0);
        EnableWindow(hGUI_Execution, FALSE);

        if (g_hWorker){
            WaitForSingleObject(g_hWorker, 5000);
            CloseHandle(g_hWorker);
            g_hWorker = NULL;
        }
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        CleanupFonts();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd,msg,wParam,lParam);
}

// ���� �G���g���|�C���g ��������������������������������������������������������������������������������
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR lpCmdLine, int nShow){
    UNREFERENCED_PARAMETER(hPrev);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if(net_init()!=0){
        MessageBoxW(NULL,L"WSAStartup failed",L"Error",MB_ICONERROR);
        return 1;
    }

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_DATE_CLASSES }; // DTP�ɕK�v
    InitCommonControlsEx(&icc);

    ComputeWindowSize();

    WNDCLASSW wc={0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = APP_CLASS;
    wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, APP_CLASS, L"Coyomi Client (Windows API)",
        g_fixedStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, g_winW, g_winH,
        NULL, NULL, hInst, NULL
    );
    g_main_hwnd = hwnd;

    ShowWindow(hwnd,nShow); UpdateWindow(hwnd);

    MSG msg;
    while(GetMessageW(&msg,NULL,0,0)){
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    net_cleanup();
    return 0;
}