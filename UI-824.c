// gcc -o UI-824.exe UI-824.c -lws2_32 -lcomctl32   ./UI-824.exe
// �N���C�A���g�iUI ���j
// CreateWindow���W���[���̎������@���킩��Ȃ�
// 5�񎸔s������\�����ς�邪6��ڂɂȂ�ƁH�H�H
// ���ʂ̕\�����@
// UImain�Ȃ�
#include <winsock2.h> // �K�� windows.h ����
#include <windows.h>
#include <commctrl.h> // Date and Time Picker
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // uint8_t, uint16_t, uint32_t
#include <string.h>
#include <stdbool.h>
#include <ws2tcpip.h>
#include <math.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")

#define PACKET_SIZE 20
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 50000
#define MAX_DAYS 10000U
#define EPS 1e-12

// �ʐM�G���[���
enum
{
    COMM_ERR_NONE = 0,
    COMM_ERR_GENERAL,
    COMM_ERR_NOT_CONNECTED
};

// �R���g���[�� ID
enum
{
    ID_DATE_PICKER = 2001,
    ID_EDIT_DAYS = 2002,
    ID_RADIO_DIR_POS = 2003,
    ID_RADIO_DIR_NEG = 2004,
    ID_RADIO_INC_YES = 2005,
    ID_RADIO_INC_NO = 2006,
    ID_BUTTON_CALC = 2007,
    ID_BUTTON_EXIT = 2008,
    ID_STATIC_ERROR = 2009,
    ID_STATIC_RESULT = 2010
};

// ���ׂẴG���[�^�x�����r�b�g�ŕ\��
typedef enum
{
    SF_NONE = 0,

    // ���͊֘A�G���[
    SF_ERR_DATE_REPLACED = 0x0010u, // ���t�t�H�[���o�b�N
    SF_ERR_INVALID_VALUE = 0x0008u,
    SF_ERR_DECIMAL_UP = 0x0001u,
    SF_ERR_DECIMAL_DOWN = 0x0002u,
    SF_ERR_EXCEED_MAX = 0x0004u,

    // ���Ԓl�q���g
    SF_USE_DECIMAL_VALUE = 0x0100u,
    SF_USE_MAX_VALUE = 0x0200u,

    // �ʐM�֘A�͕ʃr�b�g
    SF_COMM_GENERAL = 0x00010000u,
    SF_COMM_NOT_CONNECTED = 0x00020000u,
} StatusFlags;

// ���t�E���͍\����
typedef struct
{
    unsigned int year, month, day;
    char daysText[64];
    unsigned short weekday;
    bool startDateSet; // �u�J�n�����܂ށv���W�I
    bool beforeAfter;  // true=���Z, false=���Z
} InputData;

typedef struct
{
    unsigned int year, month, day, days;
    unsigned short weekday;
    bool startDateSet;
    bool beforeAfter;
} CorrectedData;

typedef struct
{
    unsigned int year, month, day, days;
    unsigned short weekday;
    bool startDateSet;
    bool beforeAfter;
} ResultData;

// �G���[�ڍוێ�
typedef struct
{
    bool flags[5];
    unsigned int decimalValue;
    unsigned int maxValue;
    unsigned int invalidValue;

    // �ǉ�: �ʐM�G���[�̘A��������
    int commErrorCount;
} ErrorInfo;

// �O���[�o��
static HINSTANCE g_hInst;
static HWND g_hDatePicker, g_hEditDays, g_hStaticErr, g_hStaticRes;
static ErrorInfo g_LastDaysErr;
static SOCKET g_sock = INVALID_SOCKET;
static HWND MainWindow;

// �v���g�^�C�v
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CollectData(HWND, InputData *);
StatusFlags InvalidValueHandling(InputData *in, CorrectedData *corr);
static void BuildPacketFromCorrected(const CorrectedData *, uint8_t *);
static void ParsePacketToResult(const uint8_t *, ResultData *);
StatusFlags ConnectToCoyomi(void);
StatusFlags UISendReceive(const CorrectedData *, ResultData *);
void CloseConnection(void);
StatusFlags UIProcessMain(const InputData *in, CorrectedData *corr, ResultData *res);
static void ShowResultStatus(const CorrectedData *, const ResultData *, unsigned int errFlags);
SYSTEMTIME AddDays(SYSTEMTIME, unsigned int, bool, bool);
const char *GetWeekdayName(int);
void CreateWindowModule(HINSTANCE hInst);

// �ʐM�w���p: �S�o�C�g���M
static int send_all(SOCKET s, const char *buf, int len)
{
    int total = 0;
    while (total < len)
    {
        int n = send(s, buf + total, len - total, 0);
        if (n <= 0)
            return n;
        total += n;
    }
    return total;
}

// �ʐM�w���p: �S�o�C�g��M
static int recv_all(SOCKET s, char *buf, int len)
{
    int total = 0;
    while (total < len)
    {
        int n = recv(s, buf + total, len - total, 0);
        if (n <= 0)
            return n;
        total += n;
    }
    return total;
}

// ���t�v�Z�i�T�[�o�Ɠ��d�l�EUI�t�H�[���o�b�N�p�Ɏc���j
SYSTEMTIME AddDays(SYSTEMTIME start, unsigned int days,
                   bool addDirection, bool includeStart)
{
    FILETIME ft;
    if (!SystemTimeToFileTime(&start, &ft))
    {
        SYSTEMTIME today;
        GetLocalTime(&today);
        if (!SystemTimeToFileTime(&today, &ft))
            return start;
        start = today;
    }
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    const LONGLONG ONE_DAY_100NS = 24LL * 60LL * 60LL * 10000000LL;
    unsigned int move = days;
    if (includeStart && move > 0)
        move--;
    LONGLONG delta = (LONGLONG)move * ONE_DAY_100NS;
    if (!addDirection)
        delta = -delta;

    uli.QuadPart += delta;
    ft.dwLowDateTime = uli.LowPart;
    ft.dwHighDateTime = uli.HighPart;

    SYSTEMTIME out = {0};
    if (!FileTimeToSystemTime(&ft, &out))
        return start;
    return out;
}

// �j����
const char *GetWeekdayName(int wday)
{
    static const char *names[] = {"��", "��", "��", "��", "��", "��", "�y"};
    return (wday >= 0 && wday < 7) ? names[wday] : "?";
}

//================================================================
// CollectData: DatePicker �݂̂���N�����擾
void CollectData(HWND MainWindow, InputData *in)
{
    SYSTEMTIME st = {0};
    if (DateTime_GetSystemtime(g_hDatePicker, &st) != GDT_NONE)
    {
        in->year = st.wYear;
        in->month = st.wMonth;
        in->day = st.wDay;
    }
    else
    {
        // �t�H�[���o�b�N: ���ݎ������g�p
        GetLocalTime(&st);
        in->year = st.wYear;
        in->month = st.wMonth;
        in->day = st.wDay;
    }
    // ���W�I: �����Z
    in->beforeAfter = (IsDlgButtonChecked(MainWindow, ID_RADIO_DIR_POS) == BST_CHECKED);
    // ���W�I: �J�n���܂ށ^�܂܂Ȃ�
    in->startDateSet = (IsDlgButtonChecked(MainWindow, ID_RADIO_INC_YES) == BST_CHECKED);
    // �����e�L�X�g
    GetWindowTextA(g_hEditDays, in->daysText, sizeof(in->daysText));
}
//================================================================

// ShowResultStatus ����؂�o�����u���͒l�G���[�̂݁v�̐������W�b�N
// errBuf: �o�̓o�b�t�@, bufSize: ���̒���
static void BuildInputErrorString(StatusFlags sf, char *errBuf, size_t bufSize)
{
    bool first = true;
    errBuf[0] = '\0';

    // �������� �� 1���Ƃ��Ĉ���
    if (sf & SF_ERR_INVALID_VALUE)
    {
        strncpy(errBuf,
                "�����ȓ������͂̂��� �g1���h �Ƃ݂Ȃ��܂�",
                bufSize - 1);
        return;
    }

    // �����؂�グ
    if (sf & SF_ERR_DECIMAL_UP)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp),
                 "�����_�ȉ��؂�グ�� �g%u���h �Ƃ݂Ȃ��܂�",
                 g_LastDaysErr.decimalValue);
        strncat(errBuf, tmp, bufSize - strlen(errBuf) - 1);
        first = false;
    }

    // �����؂�̂�
    if (sf & SF_ERR_DECIMAL_DOWN)
    {
        if (!first)
        {
            strncat(errBuf, "\r\n", bufSize - strlen(errBuf) - 1);
        }
        char tmp[128];
        snprintf(tmp, sizeof(tmp),
                 "�����_�ȉ��؂�̂Ă� �g%u���h �Ƃ݂Ȃ��܂�",
                 g_LastDaysErr.decimalValue);
        strncat(errBuf, tmp, bufSize - strlen(errBuf) - 1);
        first = false;
    }

    // �������
    if (sf & SF_ERR_EXCEED_MAX)
    {
        if (!first)
        {
            strncat(errBuf, "\r\n", bufSize - strlen(errBuf) - 1);
        }
        char tmp[128];
        snprintf(tmp, sizeof(tmp),
                 "%u���𒴂����̂� %u���Ƃ݂Ȃ��܂�",
                 MAX_DAYS,
                 g_LastDaysErr.maxValue);
        strncat(errBuf, tmp, bufSize - strlen(errBuf) - 1);
    }
}

//================================================================
// �������̓G���[�����i�]�����W�b�N���̂܂܁j
// ���͒l�␳�{�G���[���i�[
StatusFlags InvalidValueHandling(InputData *in, CorrectedData *corr)
{
    StatusFlags flags = SF_NONE;
    ErrorInfo einfo = {0};

    // �O���[�o���G���[�������Z�b�g�i�ʐM�񐔂����Z�b�g�j
    ErrorInfo prev = g_LastDaysErr;
    g_LastDaysErr = (ErrorInfo){.commErrorCount = prev.commErrorCount};

    // 1) ���͓����R�s�[
    corr->year = in->year;
    corr->month = in->month;
    corr->day = in->day;
    corr->startDateSet = in->startDateSet;
    corr->beforeAfter = in->beforeAfter;

    // 2) �N�͈̔̓`�F�b�N
    if (in->year < 2010 || in->year > 2099)
    {
        SYSTEMTIME today;
        GetLocalTime(&today);
        corr->year = today.wYear;
        corr->month = today.wMonth;
        corr->day = today.wDay;
        flags |= SF_ERR_DATE_REPLACED;
    }

    // 3) �������́^��������
    const char *s = in->daysText;
    bool invalid = false;
    int dotCount = 0, firstDec = -1;
    for (size_t i = 0; s[i]; i++)
    {
        if (s[i] == '.')
        {
            if (++dotCount > 1)
            {
                invalid = true;
                break;
            }
            if (isdigit((unsigned char)s[i + 1]))
                firstDec = s[i + 1] - '0';
        }
        else if (!isdigit((unsigned char)s[i]))
        {
            invalid = true;
            break;
        }
    }

    // �������͂Ȃ� 1���Ƀt�H�[���o�b�N
    if (invalid)
    {
        corr->days = 1;
        flags |= SF_ERR_INVALID_VALUE;
        // commErrorCount �͂��̂܂܂ɁA���̓G���[��񂾂��X�V
        g_LastDaysErr.decimalValue = einfo.decimalValue;
        g_LastDaysErr.maxValue = einfo.maxValue;
        g_LastDaysErr.invalidValue = einfo.invalidValue;
        goto FINALIZE;
    }

    // ������𐔒l��
    double val = atof(s);
    double integer = floor(val);

    // 0 �ȉ��������Ƃ݂Ȃ�
    if (val <= 0.0)
    {
        corr->days = 1;
        flags |= SF_ERR_INVALID_VALUE;
        g_LastDaysErr.decimalValue = einfo.decimalValue;
        g_LastDaysErr.maxValue = einfo.maxValue;
        g_LastDaysErr.invalidValue = einfo.invalidValue;
        goto FINALIZE;
    }

    // 4) �����_�ȉ�����
    if (dotCount == 1 && firstDec > 0)
    {
        einfo.decimalValue = (unsigned int)(integer + 1.0);
        integer = einfo.decimalValue;
        flags |= SF_ERR_DECIMAL_UP | SF_USE_DECIMAL_VALUE;
    }
    else if (dotCount == 1 && firstDec == 0)
    {
        einfo.decimalValue = (unsigned int)integer;
        flags |= SF_ERR_DECIMAL_DOWN | SF_USE_DECIMAL_VALUE;
    }

    // 5) ����`�F�b�N
    if (integer > MAX_DAYS)
    {
        einfo.maxValue = MAX_DAYS;
        integer = MAX_DAYS;
        flags |= SF_ERR_EXCEED_MAX | SF_USE_MAX_VALUE;
    }

    // �������m��
    corr->days = (unsigned int)integer;

    // ���[�J���G���[�����O���[�o���ɃR�s�[
    g_LastDaysErr.decimalValue = einfo.decimalValue;
    g_LastDaysErr.maxValue = einfo.maxValue;
    g_LastDaysErr.invalidValue = einfo.invalidValue;

FINALIZE:
    // 6) �J�n�����̗j���v�Z
    {
        SYSTEMTIME st = {0};
        st.wYear = corr->year;
        st.wMonth = corr->month;
        st.wDay = corr->day;
        FILETIME ft;
        if (SystemTimeToFileTime(&st, &ft) &&
            FileTimeToSystemTime(&ft, &st))
        {
            corr->weekday = st.wDayOfWeek;
        }
        else
        {
            corr->weekday = 0; // �t�H�[���o�b�N
        }
    }

    return flags;
}
//================================================================

// �␳�f�[�^���p�P�b�g����
static void BuildPacketFromCorrected(const CorrectedData *corr, uint8_t *buf)
{
    uint32_t *p32 = (uint32_t *)buf;
    p32[0] = htonl(corr->year);
    p32[1] = htonl(corr->month);
    p32[2] = htonl(corr->day);
    p32[3] = htonl(corr->days);
    uint16_t *p16 = (uint16_t *)(buf + 16);
    p16[0] = htons(corr->weekday);
    buf[18] = corr->startDateSet ? 1 : 0;
    buf[19] = corr->beforeAfter ? 1 : 0;
}

// �ԐM�p�P�b�g�����ʃf�[�^
static void ParsePacketToResult(const uint8_t *buf, ResultData *res)
{
    const uint32_t *p32 = (const uint32_t *)buf;
    res->year = ntohl(p32[0]);
    res->month = ntohl(p32[1]);
    res->day = ntohl(p32[2]);
    res->days = ntohl(p32[3]);
    const uint16_t *p16 = (const uint16_t *)(buf + 16);
    res->weekday = ntohs(p16[0]);
    res->startDateSet = buf[18] ? true : false;
    res->beforeAfter = buf[19] ? true : false;
}

//================================================================
// ����ڑ�������M���ؒf
StatusFlags ConnectToCoyomi(void)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return ++g_LastDaysErr.commErrorCount >= 5
                   ? SF_COMM_NOT_CONNECTED
                   : SF_COMM_GENERAL;

    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sock == INVALID_SOCKET)
    {
        WSACleanup();
        return ++g_LastDaysErr.commErrorCount >= 5
                   ? SF_COMM_NOT_CONNECTED
                   : SF_COMM_GENERAL;
    }

    struct sockaddr_in sa = {.sin_family = AF_INET,
                             .sin_port = htons(SERVER_PORT),
                             .sin_addr.s_addr = inet_addr(SERVER_IP)};
    if (connect(g_sock, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        closesocket(g_sock);
        WSACleanup();
        return ++g_LastDaysErr.commErrorCount >= 5
                   ? SF_COMM_NOT_CONNECTED
                   : SF_COMM_GENERAL;
    }
    // ���������玸�s�J�E���^�����Z�b�g

    g_LastDaysErr.commErrorCount = 0;

    return SF_NONE;
}

//================================================================

//================================================================
StatusFlags UISendReceive(const CorrectedData *corr, ResultData *res)
{
    if (g_sock == INVALID_SOCKET)
        return ++g_LastDaysErr.commErrorCount >= 5
                   ? SF_COMM_NOT_CONNECTED
                   : SF_COMM_GENERAL;

    uint8_t buf[PACKET_SIZE];
    BuildPacketFromCorrected(corr, buf);

    // �������������������� ���M ��������������������
    int sent = send(g_sock, (const char *)buf, PACKET_SIZE, 0);
    if (sent == SOCKET_ERROR || sent != PACKET_SIZE)
    {
        // ���M���s or �ꕔ��������Ă��Ȃ�
        ++g_LastDaysErr.commErrorCount;
        return g_LastDaysErr.commErrorCount >= 5
                   ? SF_COMM_NOT_CONNECTED
                   : SF_COMM_GENERAL;
    }

    // �������������������� ��M ��������������������
    int recvd = recv(g_sock, (char *)buf, PACKET_SIZE, 0);
    if (recvd == SOCKET_ERROR || recvd != PACKET_SIZE)
    {
        // ��M���s or �ꕔ�����󂯎��Ă��Ȃ�
        ++g_LastDaysErr.commErrorCount;
        return g_LastDaysErr.commErrorCount >= 5
                   ? SF_COMM_NOT_CONNECTED
                   : SF_COMM_GENERAL;
    }

    // �󂯎�ꂽ��p�[�X
    ParsePacketToResult(buf, res);
    // ����������J�E���^�����Z�b�g
    g_LastDaysErr.commErrorCount = 0;

    return SF_NONE;
}

//================================================================

void CloseConnection(void)
{
    if (g_sock != INVALID_SOCKET)
    {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
    }
    WSACleanup();
}

//================================================================
// �G���[�^���ʕ\��
static void ShowResultStatus(
    const CorrectedData *corr,
    const ResultData *res,
    StatusFlags sf)
{
    char finalErr[1024] = {0};
    char inputErr[512] = {0};
    char resultBuf[512] = {0};

    // 1) ���t�t�H�[���o�b�N���b�Z�[�W
    if (sf & SF_ERR_DATE_REPLACED)
    {
        snprintf(finalErr, sizeof(finalErr),
                 "�J�n�����͈͊O�̂��߁A����%04u�N%02u��%02u�����J�n���Ƃ��Čv�Z���܂�",
                 corr->year, corr->month, corr->day);
    }

    // 2) ���̓G���[����
    BuildInputErrorString(sf, inputErr, sizeof(inputErr));
    if (inputErr[0])
    {
        if (finalErr[0])
        {
            strncat(finalErr, "\r\n",
                    sizeof(finalErr) - strlen(finalErr) - 1);
        }
        strncat(finalErr, inputErr,
                sizeof(finalErr) - strlen(finalErr) - 1);
    }

    // 3) �ʐM�G���[����
    if (sf & SF_COMM_NOT_CONNECTED)
    {
        const char *msg = "�ڑ����m�����Ă��܂���B�ċN�����Ă��������B";
        if (finalErr[0])
        {
            strncat(finalErr, "\r\n", sizeof(finalErr) - strlen(finalErr) - 1);
        }
        strncat(finalErr, msg, sizeof(finalErr) - strlen(finalErr) - 1);
    }
    else if (sf & SF_COMM_GENERAL)
    {
        const char *msg = "�ʐM�Ɏ��s���܂����B�ēx�N���b�N���Ă��������B";
        if (finalErr[0])
        {
            strncat(finalErr, "\r\n", sizeof(finalErr) - strlen(finalErr) - 1);
        }
        strncat(finalErr, msg, sizeof(finalErr) - strlen(finalErr) - 1);
    }

    // 4) ���b�Z�[�W���ɔ��f
    SetWindowTextA(g_hStaticErr, finalErr);

    // 5) ���ʕ\�� or �N���A
    if (sf & (SF_COMM_GENERAL | SF_COMM_NOT_CONNECTED))
    {
        SetWindowTextA(g_hStaticRes, "");
    }
    else
    {
        // ������ �ǉ����� ������
        const char *dirStr = corr->beforeAfter ? "��" : "�O";
        const char *includeStr = corr->startDateSet ? "1����" : "0����";
        unsigned days = corr->days;

        // 1�s�ځF�I�v�V����
        // 2�s�ځF���t�ϊ�����
        snprintf(resultBuf, sizeof(resultBuf),
                 "�����F%s  �����F%u��  �J�n�F%s\r\n"
                 "%04u/%02u/%02u(%s)  ��  %04u/%02u/%02u(%s)",
                 dirStr, days, includeStr,
                 corr->year, corr->month, corr->day,
                 GetWeekdayName(corr->weekday),
                 res->year, res->month, res->day,
                 GetWeekdayName(res->weekday));

        SetWindowTextA(g_hStaticRes, resultBuf);
    }
}
//================================================================

//================================================================
StatusFlags UIProcessMain(
    const InputData *in,
    CorrectedData *corr,
    ResultData *res)
{
    StatusFlags status;

    // 1) ���͒l�␳�^�G���[���o
    status = InvalidValueHandling((InputData *)in, corr);

    // 2) �T�[�o�ڑ�
    status |= ConnectToCoyomi();

    // 3) �ڑ������Ȃ瑗��M���ؒf
    if ((status & (SF_COMM_GENERAL | SF_COMM_NOT_CONNECTED)) == SF_NONE)
    {
        status |= UISendReceive(corr, res);
        CloseConnection();
    }

    return status;
}
//================================================================

//================================================================
void CreateWindowModule(HINSTANCE hInst)
{
    // ���C���E�B���h�E����
    MainWindow = CreateWindowExA(
        0, "CalendarApp", "���t�v�Z�A�v��",
        WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME),
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 500,
        NULL, NULL, hInst, NULL);

    SYSTEMTIME st;
    GetLocalTime(&st);

    // DateTime Picker
    g_hDatePicker = CreateWindowExA(
        0, DATETIMEPICK_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | DTS_SHORTDATEFORMAT,
        80, 40, 200, 24, MainWindow,
        (HMENU)ID_DATE_PICKER, hInst, NULL);
    DateTime_SetSystemtime(g_hDatePicker, GDT_VALID, &st);
    // �������Ȃ���Δ͈͊O�����͂ł��遫
    //  �J�n���̗L���͈͂� 2010/01/01 �` 2099/12/31 �ɐ���
    SYSTEMTIME rg[2];
    rg[0].wYear = 2010;
    rg[0].wMonth = 1;
    rg[0].wDay = 1;
    rg[1].wYear = 2099;
    rg[1].wMonth = 12;
    rg[1].wDay = 31;
    // GDTR_MIN | GDTR_MAX �ōŏ��E�ő�𓯎��ɃZ�b�g
    DateTime_SetRange(g_hDatePicker,
                      GDTR_MIN | GDTR_MAX,
                      rg);
    // �������Ȃ���Δ͈͊O�����͂ł���

    CreateWindowA("STATIC", "�J�n��", WS_CHILD | WS_VISIBLE,
                  20, 40, 50, 20, MainWindow, NULL, hInst, NULL);
    CreateWindowA("STATIC", "�L���͈�\n2010\\01\\01-2099\\12\\31\n�����p�����E���̐����̂ݓ��͉�", WS_CHILD | WS_VISIBLE,
                  300, 30, 300, 60, MainWindow, NULL, hInst, NULL);

    // �������͗�
    CreateWindowA("STATIC", "�����p�����E���̐���\n�̂ݓ��͉�", WS_CHILD | WS_VISIBLE,
                  80, 190, 170, 50, MainWindow, NULL, hInst, NULL);
    CreateWindowA("STATIC", "����", WS_CHILD | WS_VISIBLE,
                  20, 170, 50, 20, MainWindow, NULL, hInst, NULL);
    CreateWindowA("STATIC", "��", WS_CHILD | WS_VISIBLE,
                  220, 170, 20, 20, MainWindow, NULL, hInst, NULL);
    g_hEditDays = CreateWindowA(
        "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        80, 168, 120, 24, MainWindow, (HMENU)ID_EDIT_DAYS, hInst, NULL);

    // �����Z���W�I
    CreateWindowA("BUTTON", "��", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                  280, 185, 40, 22, MainWindow, (HMENU)ID_RADIO_DIR_POS, hInst, NULL);
    CreateWindowA("BUTTON", "�O", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                  280, 155, 40, 22, MainWindow, (HMENU)ID_RADIO_DIR_NEG, hInst, NULL);
    CheckDlgButton(MainWindow, ID_RADIO_DIR_POS, BST_CHECKED);

    // �J�n���܂ރ��W�I
    CreateWindowA("STATIC", "�J�n����", WS_CHILD | WS_VISIBLE,
                  20, 100, 70, 20, MainWindow, NULL, hInst, NULL);
    CreateWindowA("BUTTON", "1����", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                  220, 100, 80, 22, MainWindow, (HMENU)ID_RADIO_INC_YES, hInst, NULL);
    CreateWindowA("BUTTON", "0����", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                  100, 100, 80, 22, MainWindow, (HMENU)ID_RADIO_INC_NO, hInst, NULL);
    CheckDlgButton(MainWindow, ID_RADIO_INC_YES, BST_CHECKED);
    CreateWindowA("STATIC", "�Ƃ���", WS_CHILD | WS_VISIBLE,
                  320, 100, 60, 20, MainWindow, NULL, hInst, NULL);

    // �{�^��
    CreateWindowA("BUTTON", "�v�Z", WS_CHILD | WS_VISIBLE,
                  370, 160, 100, 40, MainWindow, (HMENU)ID_BUTTON_CALC, hInst, NULL);
    CreateWindowA("BUTTON", "�I��", WS_CHILD | WS_VISIBLE,
                  570, 160, 100, 40, MainWindow, (HMENU)ID_BUTTON_EXIT, hInst, NULL);

    // �G���[�E���ʕ\��
    CreateWindowA("STATIC", "�y���b�Z�[�W���z", WS_CHILD | WS_VISIBLE,
                  20, 270, 140, 20, MainWindow, NULL, hInst, NULL);
    CreateWindowA("STATIC", "�y�v�Z���ʁz", WS_CHILD | WS_VISIBLE,
                  370, 270, 100, 20, MainWindow, NULL, hInst, NULL);
    g_hStaticErr = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE,
                                 20, 300, 300, 160, MainWindow,
                                 (HMENU)ID_STATIC_ERROR, hInst, NULL);
    g_hStaticRes = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE,
                                 370, 300, 300, 160, MainWindow,
                                 (HMENU)ID_STATIC_RESULT, hInst, NULL);
}
//================================================================

//================================================================
LRESULT CALLBACK WndProc(HWND MainWindow, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case ID_BUTTON_EXIT:
            DestroyWindow(MainWindow);
            break;

        case ID_BUTTON_CALC:
        {
            InputData in = {0};
            CorrectedData corr = {0};
            ResultData res = {0};
            StatusFlags status;

            // 1) �R���g���[�����琶���͂����W
            CollectData(MainWindow, &in);

            // 2) �␳���ʐM�����ʎ�M���܂Ƃ߂Ď��s
            status = UIProcessMain(&in, &corr, &res);

            // 3) ���b�Z�[�W�����ʂ�\��
            ShowResultStatus(&corr, &res, status);
            break;
        }

        } // switch(LOWORD(wp))
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(MainWindow, msg, wp, lp);
}
//================================================================

//================================================================
// WinMain: �E�B���h�E�^�R���g���[������
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow)
{
    g_hInst = hInst;

    // DateTime Picker ������
    INITCOMMONCONTROLSEX ic = {sizeof(ic), ICC_DATE_CLASSES};
    InitCommonControlsEx(&ic);

    // �E�B���h�E�N���X�o�^
    const char *CLASS_NAME = "CalendarApp";
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA(&wc);

    // ������CreateWindow�Q���܂Ƃ߂ČĂяo��
    CreateWindowModule(hInst);

    ShowWindow(MainWindow, nShow);
    UpdateWindow(MainWindow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
//================================================================