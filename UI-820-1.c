// gcc -o UI-820-1.exe UI-820-1.c -lws2_32 -lcomctl32   ./UI-820-1.exe
// �N���C�A���g�iUI ���j
// �u�J�n���v�̎���͗����폜���ADateTime Picker �݂̂��g�p
// UI-820-1�Ƃ̈Ⴂ�͓������͗��ɕςȓz�������邱��
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
#define SERVER_PORT 12345
#define MAX_DAYS 10000U
#define EPS 1e-12

// �G���[��ʃt���O�i�����j
#define F_ERR_DECIMAL_UP 0x0001u
#define F_ERR_DECIMAL_DOWN 0x0002u
#define F_ERR_EXCEED_MAX 0x0004u
#define F_ERR_INVALID_VALUE 0x0008u
#define F_ERR_DATE_REPLACED 0x0010u

// ���Ԓl�q���g
#define F_USE_DECIMAL_VALUE 0x0100u
#define F_USE_MAX_VALUE 0x0200u

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
} ErrorInfo;

// �O���[�o��
static HINSTANCE g_hInst;
static HWND g_hDatePicker, g_hEditDays, g_hStaticErr, g_hStaticRes;
static ErrorInfo g_LastDaysErr;
static int g_commErrorCount = 0;
static SOCKET g_sock = INVALID_SOCKET;

// �v���g�^�C�v
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CollectData(HWND, InputData *);
unsigned int InvalidValueHandling(InputData *, CorrectedData *);
static void BuildPacketFromCorrected(const CorrectedData *, uint8_t *);
static void ParsePacketToResult(const uint8_t *, ResultData *);
static int send_all(SOCKET, const char *, int);
static int recv_all(SOCKET, char *, int);
int ConnectToCoyomi(void);
int UISendReceive(const CorrectedData *, ResultData *);
void CloseConnection(void);
void ShowResultStatus(const CorrectedData *, const ResultData *, unsigned int);
SYSTEMTIME AddDays(SYSTEMTIME, unsigned int, bool, bool);
const char *GetWeekdayName(int);
static void CreateWindowModule(HWND hwnd, HINSTANCE hInst);

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

// CollectData: DatePicker �݂̂���N�����擾
void CollectData(HWND hwnd, InputData *in)
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
    in->beforeAfter = (IsDlgButtonChecked(hwnd, ID_RADIO_DIR_POS) == BST_CHECKED);
    // ���W�I: �J�n���܂ށ^�܂܂Ȃ�
    in->startDateSet = (IsDlgButtonChecked(hwnd, ID_RADIO_INC_YES) == BST_CHECKED);
    // �����e�L�X�g
    GetWindowTextA(g_hEditDays, in->daysText, sizeof(in->daysText));
}

// ShowResultStatus ����؂�o�����u���͒l�G���[�̂݁v�̐������W�b�N
static void BuildInputErrorString(unsigned int errFlags, char *errBuf, size_t bufSize)
{
    bool firstErr = true;
    errBuf[0] = '\0';

    // �����ȓ�������
    if (errFlags & F_ERR_INVALID_VALUE)
    {
        strncat(errBuf,
                "�����ȓ������͂̂��� �g1���h �Ƃ݂Ȃ��܂�",
                bufSize - strlen(errBuf) - 1);
        return;
    }

    // �����؂�グ
    if (errFlags & F_ERR_DECIMAL_UP)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp),
                 "�����_�ȉ��؂�グ�� �g%u���h �Ƃ݂Ȃ��܂�",
                 g_LastDaysErr.decimalValue);
        strncat(errBuf, tmp, bufSize - strlen(errBuf) - 1);
        firstErr = false;
    }

    // �����؂�̂�
    if (errFlags & F_ERR_DECIMAL_DOWN)
    {
        if (!firstErr)
            strncat(errBuf, "\r\n", bufSize - strlen(errBuf) - 1);
        char tmp[128];
        snprintf(tmp, sizeof(tmp),
                 "�����_�ȉ��؂�̂Ă� �g%u���h �Ƃ݂Ȃ��܂�",
                 g_LastDaysErr.decimalValue);
        strncat(errBuf, tmp, bufSize - strlen(errBuf) - 1);
        firstErr = false;
    }

    // �������
    if (errFlags & F_ERR_EXCEED_MAX)
    {
        if (!firstErr)
            strncat(errBuf, "\r\n", bufSize - strlen(errBuf) - 1);
        char tmp[128];
        snprintf(tmp, sizeof(tmp),
                 "%u���𒴂����̂� %u���Ƃ݂Ȃ��܂�",
                 MAX_DAYS,
                 g_LastDaysErr.maxValue);
        strncat(errBuf, tmp, bufSize - strlen(errBuf) - 1);
    }
}

// �������̓G���[�����i�]�����W�b�N���̂܂܁j
unsigned int InvalidValueHandling(InputData *in, CorrectedData *corr)
{
    const char *daysText = in->daysText;
    unsigned int errFlags = 0;
    ErrorInfo einfo = {0};

    // in��corr �R�s�[�iyear/month/day �͎g��Ȃ����\���̌݊����ێ��j
    corr->year = in->year;
    corr->month = in->month;
    corr->day = in->day;
    corr->startDateSet = in->startDateSet;
    corr->beforeAfter = in->beforeAfter;

    // ��������
    bool invalidChar = false;
    int dotCount = 0;
    int firstDec = -1;
    for (size_t i = 0; daysText[i]; i++)
    {
        char c = daysText[i];
        if (c == '.')
        {
            if (++dotCount > 1)
            {
                invalidChar = true;
                break;
            }
            if (isdigit((unsigned char)daysText[i + 1]))
                firstDec = daysText[i + 1] - '0';
        }
        else if (!isdigit((unsigned char)c))
        {
            invalidChar = true;
            break;
        }
    }
    if (invalidChar)
    {
        einfo.flags[F_ERR_INVALID_VALUE >> 1] = true;
        einfo.invalidValue = 1;
        corr->days = 1;
        g_LastDaysErr = einfo;
        return F_ERR_INVALID_VALUE;
    }

    double val = atof(daysText);
    double integer = floor(val);
    if (val <= 0.0)
    {
        einfo.flags[F_ERR_INVALID_VALUE >> 1] = true;
        corr->days = 1;
        g_LastDaysErr = einfo;
        return F_ERR_INVALID_VALUE;
    }

    if (dotCount == 1 && firstDec > 0)
    {
        einfo.flags[F_ERR_DECIMAL_UP >> 1] = true;
        einfo.decimalValue = (unsigned int)(integer + 1.0);
        integer = einfo.decimalValue;
    }
    else if (dotCount == 1 && firstDec == 0)
    {
        einfo.flags[F_ERR_DECIMAL_DOWN >> 1] = true;
        einfo.decimalValue = (unsigned int)integer;
    }

    if (integer > MAX_DAYS)
    {
        einfo.flags[F_ERR_EXCEED_MAX >> 1] = true;
        einfo.maxValue = MAX_DAYS;
        integer = MAX_DAYS;
    }

    corr->days = (unsigned int)integer;
    g_LastDaysErr = einfo;

    if (einfo.flags[F_ERR_DECIMAL_UP >> 1])
        errFlags |= F_ERR_DECIMAL_UP | F_USE_DECIMAL_VALUE;
    if (einfo.flags[F_ERR_DECIMAL_DOWN >> 1])
        errFlags |= F_ERR_DECIMAL_DOWN | F_USE_DECIMAL_VALUE;
    if (einfo.flags[F_ERR_EXCEED_MAX >> 1])
        errFlags |= F_ERR_EXCEED_MAX | F_USE_MAX_VALUE;

    return errFlags;
}

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

// ����ڑ�������M���ؒf
int ConnectToCoyomi(void)
{
    WSADATA wsa;
    struct sockaddr_in sa = {0};

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        g_commErrorCount++;
        return (g_commErrorCount >= 5) ? COMM_ERR_NOT_CONNECTED : COMM_ERR_GENERAL;
    }
    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sock == INVALID_SOCKET)
    {
        WSACleanup();
        g_commErrorCount++;
        return (g_commErrorCount >= 5) ? COMM_ERR_NOT_CONNECTED : COMM_ERR_GENERAL;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(SERVER_PORT);
    sa.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(g_sock, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        closesocket(g_sock);
        WSACleanup();
        g_commErrorCount++;
        return (g_commErrorCount >= 5) ? COMM_ERR_NOT_CONNECTED : COMM_ERR_GENERAL;
    }
    return COMM_ERR_NONE;
}

int UISendReceive(const CorrectedData *corr, ResultData *res)
{
    if (g_sock == INVALID_SOCKET)
    {
        g_commErrorCount++;
        return (g_commErrorCount >= 5) ? COMM_ERR_NOT_CONNECTED : COMM_ERR_GENERAL;
    }

    uint8_t pkt[PACKET_SIZE];
    BuildPacketFromCorrected(corr, pkt);
    if (send_all(g_sock, (char *)pkt, PACKET_SIZE) != PACKET_SIZE)
    {
        g_commErrorCount++;
        return (g_commErrorCount >= 5) ? COMM_ERR_NOT_CONNECTED : COMM_ERR_GENERAL;
    }
    if (recv_all(g_sock, (char *)pkt, PACKET_SIZE) != PACKET_SIZE)
    {
        g_commErrorCount++;
        return (g_commErrorCount >= 5) ? COMM_ERR_NOT_CONNECTED : COMM_ERR_GENERAL;
    }

    ParsePacketToResult(pkt, res);
    return COMM_ERR_NONE;
}

void CloseConnection(void)
{
    if (g_sock != INVALID_SOCKET)
    {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
    }
    WSACleanup();
}

// �G���[�^���ʕ\��
// --------------------------------------------------------
// �C����: �G���[�^���ʕ\��
// --------------------------------------------------------
void ShowResultStatus(
    const CorrectedData *corr,
    const ResultData *res,
    unsigned int errFlags)
{
    char resBuf[256];
    char errBuf[512];
    bool firstErr = true;

    // �������� ���ʂ���(resBuf) ��������
    snprintf(resBuf, sizeof(resBuf),
             "����: %04u�N%02u��%02u��(%s) [����=%u / %s / %s]",
             res->year, res->month, res->day,
             GetWeekdayName(res->weekday),
             corr->days,
             corr->beforeAfter ? "���Z" : "���Z",
             corr->startDateSet ? "�J�n���܂�" : "�J�n���܂܂Ȃ�");
    SetWindowTextA(g_hStaticRes, resBuf);

    // �������� �G���[����(errBuf) ��������
    errBuf[0] = '\0';

    // �����ȓ�������
    if (errFlags & F_ERR_INVALID_VALUE)
    {
        strncat(errBuf,
                "�����ȓ������͂̂��� �g1���h �Ƃ݂Ȃ��܂�",
                sizeof(errBuf) - strlen(errBuf) - 1);
        firstErr = false;
    }
    else
    {
        // �����؂�グ
        if (errFlags & F_ERR_DECIMAL_UP)
        {
            char tmp[128];
            snprintf(tmp, sizeof(tmp),
                     "�����_�ȉ��؂�グ�� �g%u���h �Ƃ݂Ȃ��܂�",
                     g_LastDaysErr.decimalValue);
            if (!firstErr)
                strncat(errBuf, "\r\n", sizeof(errBuf) - strlen(errBuf) - 1);
            strncat(errBuf, tmp, sizeof(errBuf) - strlen(errBuf) - 1);
            firstErr = false;
        }
        // �����؂�̂�
        if (errFlags & F_ERR_DECIMAL_DOWN)
        {
            char tmp[128];
            snprintf(tmp, sizeof(tmp),
                     "�����_�ȉ��؂�̂Ă� �g%u���h �Ƃ݂Ȃ��܂�",
                     g_LastDaysErr.decimalValue);
            if (!firstErr)
                strncat(errBuf, "\r\n", sizeof(errBuf) - strlen(errBuf) - 1);
            strncat(errBuf, tmp, sizeof(errBuf) - strlen(errBuf) - 1);
            firstErr = false;
        }
        // �������
        if (errFlags & F_ERR_EXCEED_MAX)
        {
            char tmp[128];
            snprintf(tmp, sizeof(tmp),
                     "%u���𒴂����̂� %u���Ƃ݂Ȃ��܂�",
                     MAX_DAYS,
                     g_LastDaysErr.maxValue);
            if (!firstErr)
                strncat(errBuf, "\r\n", sizeof(errBuf) - strlen(errBuf) - 1);
            strncat(errBuf, tmp, sizeof(errBuf) - strlen(errBuf) - 1);
            firstErr = false;
        }
    }

    SetWindowTextA(g_hStaticErr, errBuf);
}
static void CreateWindowModule(HWND hwnd, HINSTANCE hInst)

{
    SYSTEMTIME st;
    GetLocalTime(&st);

    // DateTime Picker
    g_hDatePicker = CreateWindowExA(
        0, DATETIMEPICK_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | DTS_SHORTDATEFORMAT,
        80, 40, 200, 24, hwnd,
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
    // �������Ȃ���Δ͈͊O�����͂ł��遪

    CreateWindowA("STATIC", "�J�n��", WS_CHILD | WS_VISIBLE,
                  20, 40, 50, 20, hwnd, NULL, hInst, NULL);
    CreateWindowA("STATIC", "�L���͈�\n2010\\01\\01-2099\\12\\31\n�����p�����E���̐����̂ݓ��͉�", WS_CHILD | WS_VISIBLE,
                  300, 30, 300, 60, hwnd, NULL, hInst, NULL);

    // �������͗�
    CreateWindowA("STATIC", "�����p�����E���̐���\n�̂ݓ��͉�", WS_CHILD | WS_VISIBLE,
                  80, 190, 170, 50, hwnd, NULL, hInst, NULL);
    CreateWindowA("STATIC", "����", WS_CHILD | WS_VISIBLE,
                  20, 170, 50, 20, hwnd, NULL, hInst, NULL);
    CreateWindowA("STATIC", "��", WS_CHILD | WS_VISIBLE,
                  220, 170, 20, 20, hwnd, NULL, hInst, NULL);
    g_hEditDays = CreateWindowA(
        "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        80, 168, 120, 24, hwnd, (HMENU)ID_EDIT_DAYS, hInst, NULL);


    // �����Z���W�I
    CreateWindowA("BUTTON", "��", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                  280, 185, 40, 22, hwnd, (HMENU)ID_RADIO_DIR_POS, hInst, NULL);
    CreateWindowA("BUTTON", "�O", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                  280, 155, 40, 22, hwnd, (HMENU)ID_RADIO_DIR_NEG, hInst, NULL);
    CheckDlgButton(hwnd, ID_RADIO_DIR_POS, BST_CHECKED);

    // �J�n���܂ރ��W�I
    CreateWindowA("STATIC", "�J�n����", WS_CHILD | WS_VISIBLE,
                  20, 100, 70, 20, hwnd, NULL, hInst, NULL);
    CreateWindowA("BUTTON", "1����", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                  220, 100, 80, 22, hwnd, (HMENU)ID_RADIO_INC_YES, hInst, NULL);
    CreateWindowA("BUTTON", "0����", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                  100, 100, 80, 22, hwnd, (HMENU)ID_RADIO_INC_NO, hInst, NULL);
    CheckDlgButton(hwnd, ID_RADIO_INC_YES, BST_CHECKED);
    CreateWindowA("STATIC", "�Ƃ���", WS_CHILD | WS_VISIBLE,
                  320, 100, 60, 20, hwnd, NULL, hInst, NULL);

    // �{�^��
    CreateWindowA("BUTTON", "�v�Z", WS_CHILD | WS_VISIBLE,
                  370, 160, 100, 40, hwnd, (HMENU)ID_BUTTON_CALC, hInst, NULL);
    CreateWindowA("BUTTON", "�I��", WS_CHILD | WS_VISIBLE,
                  570, 160, 100, 40, hwnd, (HMENU)ID_BUTTON_EXIT, hInst, NULL);

    // �G���[�E���ʕ\��
    CreateWindowA("STATIC", "�y���b�Z�[�W���z", WS_CHILD | WS_VISIBLE,
                  20, 270, 140, 20, hwnd, NULL, hInst, NULL);
    CreateWindowA("STATIC", "�y�v�Z���ʁz", WS_CHILD | WS_VISIBLE,
                  370, 270, 100, 20, hwnd, NULL, hInst, NULL);
    g_hStaticErr = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE,
                                 20, 300, 300, 160, hwnd,
                                 (HMENU)ID_STATIC_ERROR, hInst, NULL);
    g_hStaticRes = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE,
                                 370, 300, 300, 160, hwnd,
                                 (HMENU)ID_STATIC_RESULT, hInst, NULL);


}
// ���C���E�B���h�E�v���V�[�W��
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case ID_BUTTON_EXIT:
            DestroyWindow(hwnd);
            break;
        case ID_BUTTON_CALC:
        {
            InputData in = {0};
            CorrectedData corr = {0};
            ResultData res = {0};
            unsigned int errFlags;
            int commErr;

            // 1) ���[�U���͂����W
            CollectData(hwnd, &in);

            // 2) �J�n�����͈͓����`�F�b�N (2010�`2099)
            bool dateValid = (in.year >= 2010 && in.year <= 2099);

            // 3) �������̓G���[�����icorr.days �ɃZ�b�g�j
            errFlags = InvalidValueHandling(&in, &corr);

            // 4) �͈͊O�Ȃ瓖���Ńt�H�[���o�b�N���t���O
            if (!dateValid)
            {
                SYSTEMTIME today;
                GetLocalTime(&today);
                corr.year = today.wYear;
                corr.month = today.wMonth;
                corr.day = today.wDay;
                errFlags |= F_ERR_DATE_REPLACED;
            }

            // ���W�I�ݒ���R�s�[
            corr.startDateSet = in.startDateSet;
            corr.beforeAfter = in.beforeAfter;

            // 5) �ڑ�������M���ؒf
            commErr = ConnectToCoyomi();
            if (commErr == COMM_ERR_NONE)
            {
                commErr = UISendReceive(&corr, &res);
                CloseConnection();
            }

            // 6) ���b�Z�[�W�g�ݗ���
            char finalErr[1024] = {0};

            // 6-1) ���t�t�H�[���o�b�N�����b�Z�[�W
            if (errFlags & F_ERR_DATE_REPLACED)
            {
                snprintf(finalErr, sizeof(finalErr),
                         "�J�n�����͈͊O�̂��ߓ���(%04u/%02u/%02u)���J�n���Ƃ��Čv�Z���܂�",
                         corr.year, corr.month, corr.day);
            }

            // 6-2) �������̓G���[�����𐶐�
            char inputErr[512] = {0};
            BuildInputErrorString(errFlags, inputErr, sizeof(inputErr));
            if (inputErr[0])
            {
                if (finalErr[0])
                    strncat(finalErr, "\r\n", sizeof(finalErr) - strlen(finalErr) - 1);
                strncat(finalErr, inputErr, sizeof(finalErr) - strlen(finalErr) - 1);
            }

            // 6-3) �ʐM�G���[������ΒǋL
            const char *commMsgs[] = {
                "",
                "�ʐM�Ɏ��s���܂����B�ēx�N���b�N���Ă��������B",
                "�ڑ����m�����Ă��܂���B�ċN�����Ă��������B"};
            if (commErr != COMM_ERR_NONE)
            {
                if (commMsgs[commErr][0])
                {
                    if (finalErr[0])
                        strncat(finalErr, "\r\n", sizeof(finalErr) - strlen(finalErr) - 1);
                    strncat(finalErr, commMsgs[commErr], sizeof(finalErr) - strlen(finalErr) - 1);
                }
                // �G���[���ɏo�́A���ʗ��̓N���A
                SetWindowTextA(g_hStaticErr, finalErr);
                SetWindowTextA(g_hStaticRes, "");
            }
            else
            {
                // 7) �ʐM���� �� ���ʕ\��
                ShowResultStatus(&corr, &res, errFlags);

                // �t�H�[���o�b�N���͉��߂ă��b�Z�[�W�����㏑��
                if (errFlags & F_ERR_DATE_REPLACED)
                {
                    SetWindowTextA(g_hStaticErr, finalErr);
                }
            }

            break;
        }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

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

    // ���C���E�B���h�E����
    HWND hwnd = CreateWindowExA(
        0, CLASS_NAME, "���t�v�Z�A�v��",
        WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME),
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 500,
        NULL, NULL, hInst, NULL);
    if (!hwnd)
        return -1;

    // ������CreateWindow�Q���܂Ƃ߂ČĂяo��
    CreateWindowModule(hwnd, hInst);

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}