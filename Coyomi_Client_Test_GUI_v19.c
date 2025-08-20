//* Coyomi_Client_Test�i���S�����ŁF���͎��W/���ؕ␳/�\���𕪗��j
//* �R���p�C����iMinGW-w64�j:
//* gcc Coyomi_Client_Test_GUI_v19.c CreateWindow.c -o Coyomi_Client_Test_v19.exe -lcomctl32 -lws2_32 -mwindows -municode -std=c11 -Wall -Wextra -Wpedantic -O2 -finput-charset=CP932 -fexec-charset=CP932 -fwide-exec-charset=UTF-16LE

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

// ========= �܂� Winsock �� ���̌�� Windows =========
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>

// ========= C �����^�C���n =========
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>
#include <wctype.h>
#include <math.h>
#include <string.h>

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof((a)[0]))
#endif

// ========= �v���W�F�N�g�w�b�_ =========
// #include "coyomi_structs.h"     // InputData / SentData / ReceiveData / (��PACKET_WORDS�������ɒ�`���Ă�������)
// #include "coyomi_gui_consts.h"  // GUI���C�A�E�g�萔�EGUI_SIZE/GUI_RATIO_X/Y�EMIN/MAX
// #include "coyomi_messages_ja.h" // ���{�ꃁ�b�Z�[�W�i�G���[/GUI�\�������j
// #include "CreateWindow.h"       // �e+�q�쐬�^�\�� & GUIHandles
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "coyomi_all.h"

// ���� coyomi_structs.h �� PACKET_WORDS ������`�Ȃ炱���Œ�`�i�萔�Ȃ̂Ō݊��}�N���ł͂���܂���j
#ifndef PACKET_WORDS
#define PACKET_WORDS 6  // [year,month,day,ndays,flags,error]
#endif

//==============================
// (����) Config ����
//==============================
#define SERVER_IP   "127.0.0.1"   // �ڑ���
#define SERVER_PORT 5000          // �ڑ���|�[�g

//==============================
// �G���[���b�Z�[�WID
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

// ID���{�� �擾�i�{���� coyomi_messages_ja.h �� MSG_ERR_ID* ���g�p�j
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

//==============================
// �O���[�o��
//==============================
GUIHandles g_gui;           // ��ʃn���h����
static HWND g_main_hwnd = NULL;

ErrorID g_error_id = ERR_NONE; // �u�Ō��1���v�G���[ID�i���v���g�R���݊��j

// ���v�]��7�̃t���O�i0:������, 1:�����j
static uint32_t Error_ID_01 = 0;
static uint32_t Error_ID_02 = 0;
static uint32_t Error_ID_03 = 0;
static uint32_t Error_ID_04 = 0;
static uint32_t Error_ID_05 = 0;
static uint32_t Error_ID_06 = 0;
static uint32_t Error_ID_07 = 0;

// �܂Ƃ߂ă��Z�b�g
static void reset_error_flags(void){
    Error_ID_01 = Error_ID_02 = Error_ID_03 = 0;
    Error_ID_04 = Error_ID_05 = Error_ID_06 = 0;
    Error_ID_07 = 0;
    g_error_id  = ERR_NONE;
}

// �����L�^�i0��1�j�Bg_error_id�����ݒ�Ȃ瓯���ɓ���Ă���
static void raise_error_flag(ErrorID id){
    switch(id){
        case ERR_ID1_CONNECT:            Error_ID_01 = 1; break;
        case ERR_ID2_START_OUT_OF_RANGE: Error_ID_02 = 1; break;
        case ERR_ID3_POSITIVE_ONLY:      Error_ID_03 = 1; break;
        case ERR_ID4_ROUNDUP:            Error_ID_04 = 1; break;
        case ERR_ID5_CAP_10000:          Error_ID_05 = 1; break;
        case ERR_ID6_INVALID_INT:        Error_ID_06 = 1; break;
        case ERR_ID7_SEND_RECV_FAIL:     Error_ID_07 = 1; break;
        default: break;
    }
    if (g_error_id == ERR_NONE) g_error_id = id;
}

//==============================
// flags �̃��[�e�B���e�B�iFLAG_* �� coyomi_structs.h or coyomi_gui_consts.h�j
//==============================
#ifndef FLAG_DIR_MASK
#define FLAG_DIR_MASK      0x00000001u
#define FLAG_FLAG01_MASK   0x00000002u
#define FLAG_WD_MASK       0x00000700u
#define FLAG_WD_SHIFT      8
#endif

static inline uint32_t pack_flags(bool dir, bool flag01, short weekday){
    uint32_t f = 0;
    if(dir)    f |= FLAG_DIR_MASK;
    if(flag01) f |= FLAG_FLAG01_MASK;
    if(weekday >= 0){ f |= ((uint32_t)(weekday & 0x7)) << FLAG_WD_SHIFT; }
    return f;
}
static inline short  flags_get_weekday(uint32_t f){ return (short)((f & FLAG_WD_MASK) >> FLAG_WD_SHIFT); }

//==============================
// Wire �����i�P�������j
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

// SentData �� wire
static void pack_to_wire_from_sent(const SentData* s, uint32_t netbuf[PACKET_WORDS]){
    uint32_t t[PACKET_WORDS] = {
        s->year, s->month, s->day, s->ndays, s->flags, (uint32_t)g_error_id
    };
    for (int i=0;i<PACKET_WORDS;++i) netbuf[i] = htonl(t[i]);
}

// wire �� ReceiveData�i���ł� g_error_id ���X�V�j
static void unpack_from_wire_to_recv(ReceiveData* r, const uint32_t netbuf[PACKET_WORDS]){
    uint32_t t[PACKET_WORDS];
    for (int i=0;i<PACKET_WORDS;++i) t[i] = ntohl(netbuf[i]);
    r->year  = t[0];
    r->month = t[1];
    r->day   = t[2];
    r->ndays = t[3];
    r->flags = t[4];
    g_error_id = (ErrorID)t[5];
}

//==============================
// Network ����
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

// SentData/ReceiveData �𒼐ڑ���M
static int net_send_data(SOCKET s, const SentData* sd){
    uint32_t netbuf[PACKET_WORDS];
    pack_to_wire_from_sent(sd, netbuf);
    return send_all(s, (const char*)netbuf, (int)sizeof(netbuf));
}
static int net_recv_data(SOCKET s, ReceiveData* rd){
    uint32_t netbuf[PACKET_WORDS];
    int r = recv_all(s, (char*)netbuf, (int)sizeof(netbuf));
    if (r <= 0) return r;
    unpack_from_wire_to_recv(rd, netbuf);
    return r;
}

//==============================
// GUI �֘A�iCreateWindow ���W���[�����g�p�j
//==============================

// ���� �E�B���h�E�T�C�Y�iWM_GETMINMAXINFO�p�̎Z�o�ێ��j ����
static int   g_winW      = 0;
static int   g_winH      = 0;
static DWORD g_fixedStyle= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

static void ComputeWindowSize(void){
    int clientW = (int)((long long)GUI_SIZE * GUI_RATIO_Y / GUI_RATIO_X);
    RECT rc = {0, 0, clientW, GUI_SIZE};
    AdjustWindowRectEx(&rc, g_fixedStyle, FALSE, 0);
    g_winW = rc.right - rc.left;
    g_winH = rc.bottom - rc.top;
}

// ���� �E�B���h�E/���b�Z�[�W��` ��������������������������������������������������������������
#define APP_CLASS     L"CoyomiClientWnd"
#define WM_APP_RESULT (WM_APP + 1)

// ���� �I������ ����������������������������������������������������������������������������������������������
static HANDLE g_hWorker = NULL;
static volatile LONG g_alive = 1;

// ���� �j���\�L�i���{��j ��������������������������������������������������������������������������
static const wchar_t* weekday_str_ja(int w){
    static const wchar_t* tbl[]={L"���j��",L"���j��",L"�Ηj��",L"���j��",L"�ؗj��",L"���j��",L"�y�j��"};
    return (w>=0 && w<7)? tbl[w] : L"?";
}

static void set_status(LPCWSTR s){
    if(g_gui.hGUI_Message) SetWindowTextW(g_gui.hGUI_Message, s);
}

//==============================
// ���́��G���[����
//==============================
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

// �����񖖔��ɒǋL�iwchar_t�Łj
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

// �L�^�{�i�K�v�Ȃ�jbuf�ɒǋL
static void record_error(ErrorID id, wchar_t* opt_buf, size_t opt_cap){
    raise_error_flag(id);
    if (opt_buf) add_error_by_id(opt_buf, opt_cap, id);
}

// ndays ��́E�␳
static uint32_t parse_and_fix_ndays(const wchar_t* s, wchar_t* msgbuf, size_t msgcap, bool* out_triggered_id3){
    if (out_triggered_id3) *out_triggered_id3 = false;

    // ���󔒂݂͖̂���
    if(!s || !*s){
        record_error(ERR_ID6_INVALID_INT, msgbuf, msgcap);
        if (out_triggered_id3){
            record_error(ERR_ID3_POSITIVE_ONLY, msgbuf, msgcap);
            *out_triggered_id3 = true;
        }
        return 1;
    }

    // �O��̋󔒂��X�L�b�v
    while(*s==L' '||*s==L'\t'||*s==L'\r'||*s==L'\n') ++s;
    if(!*s){
        record_error(ERR_ID6_INVALID_INT, msgbuf, msgcap);
        if (out_triggered_id3){
            record_error(ERR_ID3_POSITIVE_ONLY, msgbuf, msgcap);
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
        record_error(ERR_ID6_INVALID_INT, msgbuf, msgcap);
        if (out_triggered_id3){
            record_error(ERR_ID3_POSITIVE_ONLY, msgbuf, msgcap);
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
        record_error(ERR_ID4_ROUNDUP, msgbuf, msgcap);
    }else{
        // ����
        if (val < 1.0){
            record_error(ERR_ID6_INVALID_INT, msgbuf, msgcap);
            return 1;
        }
        if (val > 1e9) val = 1e9;
        n = (uint32_t)val;
    }

    // ��� 10000
    if (n > 10000){
        n = 10000;
        record_error(ERR_ID5_CAP_10000, msgbuf, msgcap);
    }
    return n;
}

//==============================
// �X���b�h��GUI�֓n�����˃f�[�^
//==============================
typedef struct {
    SentData    send;          // ���M�p
    ReceiveData recv;          // ��M�p
    uint32_t start_y, start_m, start_d; // �\���p�F�␳��̊J�n��
    uint32_t ndays_ui;      // �\���p�F�␳�����
    bool dir_after;         // �\���p
    wchar_t errbuf[1024];
} ClientResult;

//==============================
// �G���[�\���p�F���O�� Char[1024] ��g�ݗ��Ă�
//==============================
static void append_line_char(char* buf, size_t cap, const char* line){
    if(!buf || !line || cap==0) return;
    size_t cur = strlen(buf);
    if (cur >= cap-1) return;
    _snprintf(buf+cur, cap-cur, "%s%s", (cur? "\r\n":""), line);
}
static void add_error_to_charbuf(ErrorID id, char* out, size_t cap){
    const wchar_t* wmsg = error_body_from_id(id);
    if (!wmsg) return;

    char msg_utf8[800];
    int n = WideCharToMultiByte(CP_UTF8, 0, wmsg, -1, msg_utf8, (int)sizeof(msg_utf8), NULL, NULL);
    if (n <= 0) return;

    char line[840];
    _snprintf(line, sizeof(line), "ID%d\t%s", (int)id, msg_utf8);
    append_line_char(out, cap, line);
}
static void build_error_messages_char(char out[1024]){
    out[0] = '\0';
    if (Error_ID_01) add_error_to_charbuf(ERR_ID1_CONNECT, out, 1024);
    if (Error_ID_02) add_error_to_charbuf(ERR_ID2_START_OUT_OF_RANGE, out, 1024);
    if (Error_ID_03) add_error_to_charbuf(ERR_ID3_POSITIVE_ONLY, out, 1024);
    if (Error_ID_04) add_error_to_charbuf(ERR_ID4_ROUNDUP, out, 1024);
    if (Error_ID_05) add_error_to_charbuf(ERR_ID5_CAP_10000, out, 1024);
    if (Error_ID_06) add_error_to_charbuf(ERR_ID6_INVALID_INT, out, 1024);
    if (Error_ID_07) add_error_to_charbuf(ERR_ID7_SEND_RECV_FAIL, out, 1024);
}

//==============================
// ���͎��W�i���؂Ȃ��j
//==============================
static void collect_input_from_gui(InputData* iv){
    if (!iv) return;
    ZeroMemory(iv, sizeof(*iv));

    // DTP�i�����Ȃ猻�ݓ����j�������ł̓G���[���肵�Ȃ�
    SYSTEMTIME st;
    LRESULT r = SendMessageW(g_gui.hGUI_Calendar, DTM_GETSYSTEMTIME, 0, (LPARAM)&st);
    if (r != GDT_VALID) GetLocalTime(&st);
    iv->year  = st.wYear;
    iv->month = st.wMonth;
    iv->day   = st.wDay;

    // �����iUTF-8�ŕێ��j
    wchar_t buf_w[64];
    GetWindowTextW(g_gui.hGUI_ShiftDays, buf_w, _countof(buf_w));
    WideCharToMultiByte(CP_UTF8, 0, buf_w, -1, iv->ndays_text, (int)sizeof(iv->ndays_text), NULL, NULL);

    // ���W�I�i�O/��, 0/1���ځj
    iv->dir_after = (SendMessageW(g_gui.hGUI_RadioBox_After, BM_GETCHECK, 0, 0)==BST_CHECKED);
    iv->flag01    = (SendMessageW(g_gui.hGUI_RadioBox_One,   BM_GETCHECK, 0, 0)==BST_CHECKED);
}

//==============================
// ���؁E�␳�iInputData �� SentData/ClientResult�j
//==============================
typedef struct {
    bool set_dtp_today; // true�Ȃ�GUI��DTP�������ɍ��킹��
} ValidationHint;

static void validate_input_and_prepare(const InputData* in,
                                       SentData* out_send,
                                       ClientResult* out_cr,
                                       ValidationHint* hint,
                                       wchar_t* errbuf, size_t errcap)
{
    if(hint) hint->set_dtp_today = false;
    if(out_send) ZeroMemory(out_send, sizeof(*out_send));

    // 1) �J�n���̌���
    bool use_today = false;
    if (!in_allowed_range_yMd((int)in->year,(int)in->month,(int)in->day)){
        record_error(ERR_ID2_START_OUT_OF_RANGE, errbuf, errcap);
        use_today = true; // �J�n���͍�����
    }

    // 2) �����̌��؁E�␳
    wchar_t ndays_w[64];
    MultiByteToWideChar(CP_UTF8, 0, in->ndays_text, -1, ndays_w, _countof(ndays_w));
    bool trig_id3 = false;
    uint32_t ndays_fixed = parse_and_fix_ndays(ndays_w, errbuf, errcap, &trig_id3);
    if (trig_id3) use_today = true;

    // 3) �␳��̊J�n��������
    uint32_t y,m,d;
    if (use_today){
        SYSTEMTIME now; GetLocalTime(&now);
        y = now.wYear; m = now.wMonth; d = now.wDay;
    }else{
        y = in->year; m = in->month; d = in->day;
    }

    // 4) ���M�p�\���̂ɔ��f
    if (out_send){
        out_send->year  = y;
        out_send->month = m;
        out_send->day   = d;
        out_send->ndays = ndays_fixed;
        out_send->flags = pack_flags(in->dir_after, in->flag01, 0); // �j���̓T�[�o���Őݒ�
    }

    // 5) ��ʕ\���p���ɔ��f
    if (out_cr){
        out_cr->start_y = y;
        out_cr->start_m = m;
        out_cr->start_d = d;
        out_cr->ndays_ui = ndays_fixed;
        out_cr->dir_after = in->dir_after;
        if (errbuf && errcap>0){
            wcsncpy(out_cr->errbuf, errbuf, _countof(out_cr->errbuf)-1);
        }
    }

    // 6) GUI���f�w��
    if (hint) hint->set_dtp_today = use_today;
}

//==============================
// ���ʕ\��
//==============================
static void show_result_to_edit(const ClientResult* r){
    if (!g_gui.hGUI_Result) return;

    const wchar_t* dir = r->dir_after ? L"����" : L"���O";
    short wd = (short)flags_get_weekday(r->recv.flags);

    wchar_t line1[64], line2[64], line3[96], msg[320];
    swprintf(line1, 64, L"�J�n���F%04u/%02u/%02u", r->start_y, r->start_m, r->start_d);
    swprintf(line2, 64, L"�����F%u%s", r->ndays_ui, dir);
    swprintf(line3, 96, L"�Y�����F %04u/%02u/%02u�i%s�j",
             r->recv.year, r->recv.month, r->recv.day, weekday_str_ja((int)wd));
    swprintf(msg, 320, L"%s\r\n%s\r\n%s", line1, line2, line3);
    SetWindowTextW(g_gui.hGUI_Result, msg);

    char  errbuf8[1024];
    build_error_messages_char(errbuf8);

    if (errbuf8[0]){
        wchar_t errbufW[1024];
        int wn = MultiByteToWideChar(CP_UTF8, 0, errbuf8, -1, errbufW, _countof(errbufW));
        if (wn > 0) set_status(errbufW);
        else        set_status(L"(�G���[������̕ϊ��Ɏ��s���܂���)");
    }else{
        set_status(L"");
    }
}

//==============================
// ���s(����M)�X���b�h
//==============================
static unsigned __stdcall SendThread(void* arg){
    ClientResult* r = (ClientResult*)arg;

    g_error_id = ERR_NONE;

    SOCKET s = net_connect_server();
    if (s == INVALID_SOCKET) {
        record_error(ERR_ID1_CONNECT, r->errbuf, _countof(r->errbuf));
    } else {
        int fail_sr = 0;
        for (;;){
            if (net_send_data(s, &r->send) > 0 && net_recv_data(s, &r->recv) > 0){
                break; // OK
            }
            fail_sr++;
            if (fail_sr >= 5){
                record_error(ERR_ID7_SEND_RECV_FAIL, r->errbuf, _countof(r->errbuf));
                break;
            }
            net_close(s);
            s = net_connect_server();
            if (s == INVALID_SOCKET){
                record_error(ERR_ID1_CONNECT, r->errbuf, _countof(r->errbuf));
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

//==============================
// ���N�G�X�g�J�n�i���S�����j��s�錾
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

        show_result_to_edit(res);
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
// ���N�G�X�g�J�n�i���S�����j�{��
//==============================
static void start_request(HWND hwnd){
    (void)hwnd;

    if (g_gui.hGUI_Message) SetWindowTextW(g_gui.hGUI_Message, L"");
    if (g_gui.hGUI_Result)  SetWindowTextW(g_gui.hGUI_Result,  L"");

    reset_error_flags();

    InputData iv;
    collect_input_from_gui(&iv);

    wchar_t errbuf[1024] = L"";
    ClientResult* cr = (ClientResult*)calloc(1, sizeof(ClientResult));
    if(!cr){
        append_line(errbuf, _countof(errbuf), L"(�����G���[) �������m�ۂɎ��s���܂����B");
        set_status(errbuf);
        return;
    }

    ValidationHint hint = {0};
    validate_input_and_prepare(&iv, &cr->send, cr, &hint, errbuf, _countof(errbuf));

    if (hint.set_dtp_today){
        SYSTEMTIME now; GetLocalTime(&now);
        SendMessageW(g_gui.hGUI_Calendar, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&now);
    }

    EnableWindow(g_gui.hGUI_Execution, FALSE);

    HANDLE th = (HANDLE)_beginthreadex(NULL,0,SendThread,cr,0,NULL);
    if(th){
        g_hWorker = th;
    }else{
        append_line(errbuf, _countof(errbuf), L"(�����G���[) �X���b�h�N���Ɏ��s���܂����B");
        set_status(errbuf);
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

    if(net_init()!=0){
        MessageBoxW(NULL,L"WSAStartup failed",L"Error",MB_ICONERROR);
        return 1;
    }

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_DATE_CLASSES }; // DTP�ɕK�v
    InitCommonControlsEx(&icc);

    // �e�{�q�̈ꊇ�쐬
    HWND hwnd = CreateWindow_CreateWindow(
        &g_gui,
        hInst,
        WndProc,
        APP_CLASS,                       // �N���X��
        L"Coyomi Client (Windows API)",  // �E�B���h�E�^�C�g��
        g_fixedStyle,                    // �E�B���h�E�X�^�C��
        0                                // ExStyle
    );
    if (!hwnd) {
        MessageBoxW(NULL, L"CreateWindow failed", L"Error", MB_ICONERROR);
        net_cleanup();
        return 1;
    }
    g_main_hwnd = hwnd;

    // �\��
    CreateWindow_ShowWindow(&g_gui, nShow);

    MSG msg;
    while(GetMessageW(&msg,NULL,0,0)){
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    net_cleanup();
    return 0;
}
