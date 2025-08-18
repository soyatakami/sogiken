// 20000.1日としたときのエラー対応・エラー表示の仕方は？方式設計書との兼ね合い
// 一度サーバとつないだらつなぎっぱなしか，計算ボタン押すごとに切断するのか
// ラジオボタンとかほかにもエラー対応しとかないといけない？要求仕様書，ソ的との兼ね合い
// gcc -o UI-818.exe UI-818.c -lws2_32 -lcomctl32   ./UI-818
// クライアント（UI側）
#include <winsock2.h> // 必ず windows.h より先
#include <windows.h>
#include <commctrl.h> // Date and Time Picker 用
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // ← uint8_t, uint16_t, uint32_t
#include <string.h>
#include <stdbool.h>
#include <ws2tcpip.h>

#include <math.h>
#define PACKET_SIZE 20

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345

static SOCKET g_sock = INVALID_SOCKET;

#define MAX_DAYS 10000U
#define EPS 1e-12

// エラー種別フラグ（複合可）
#define F_ERR_DECIMAL_UP 0x0001u
#define F_ERR_DECIMAL_DOWN 0x0002u
#define F_ERR_EXCEED_MAX 0x0004u
#define F_ERR_INVALID_VALUE 0x0008u
#define F_ERR_DATE_REPLACED 0x0010u

// 中間値ヒント
#define F_USE_DECIMAL_VALUE 0x0100u
#define F_USE_MAX_VALUE 0x0200u

// ===== 構造体 =====
typedef struct
{
    unsigned int year;
    unsigned int month;
    unsigned int day;
    char days;
    unsigned short weekday;
    bool startDateSet; // UIでは「開始日を含む」ラジオの状態をここに載せてサーバへ送る
    bool beforeAfter;  // true=加算, false=減算
} InputData;

typedef struct
{
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int days;
    unsigned short weekday;
    bool startDateSet; // ここも「開始日を含む」状態を載せる（送信用）
    bool beforeAfter;
} CorrectedData;

typedef struct
{
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int days;
    unsigned short weekday;
    bool startDateSet; // サーバから戻る「開始日を含む」状態
    bool beforeAfter;
} ResultData;

// ===== コントロール ID =====
enum
{
    ID_EDIT_DATE = 2001,
    ID_EDIT_DAYS = 2002,
    ID_RADIO_DIR_POS = 2003,
    ID_RADIO_DIR_NEG = 2004,
    ID_RADIO_INC_YES = 2005,
    ID_RADIO_INC_NO = 2006,
    ID_BUTTON_CALC = 2007,
    ID_BUTTON_EXIT = 2008,
    ID_STATIC_ERROR = 2009,
    ID_STATIC_RESULT = 2010,
    ID_DATE_PICKER = 2011 // 追加: DateTime Picker
};

// ===== エラーID =====
enum
{
    ERR_DECIMAL_UP = 0,
    ERR_DECIMAL_DOWN,
    ERR_EXCEED_MAX,
    ERR_INVALID_VALUE,
    ERR_COUNT
};

typedef struct
{
    bool flags[ERR_COUNT];
    unsigned int decimalValue;
    unsigned int maxValue;
    unsigned int invalidValue;
} ErrorInfo;

enum
{
    COMM_ERR_NONE = 0,
    COMM_ERR_GENERAL = 1, // 「通信に失敗しました…」
    COMM_ERR_NOT_CONNECTED = 2  // 「接続が確立していません…」
};

// ===== メッセージ文字列 =====
static const char *ErrorMessageDate =
    "無効な開始日入力のため，本日 %04u年%02u月%02u日 を開始日とします";

static const char *ErrorMessagesDays[ERR_COUNT] = {
    "小数での日数入力のため，切り上げて \"%u日\" とみなします",
    "小数での日数入力かつ小数第一位が1未満のため，切り捨てて \"%u日\" とみなします",
    "日数上限(%u日)を超えているため，%u日とみなします",
    "無効な日数値入力なので \"1日\" とみなします"};

static const char *CommErrorMessages[] = {
    "",
    "通信に失敗しました．計算ボタンを再度クリックしてください．",
    "接続が確立していません．時間をおいて，再起動してください．"};

// ===== グローバル（UIハンドル類と検証中間値）=====
static HINSTANCE g_hInst;
static HWND g_hEditDate, g_hDatePicker, g_hEditDays, g_hStaticErr, g_hStaticRes;
static ErrorInfo g_LastDaysErr;
static SYSTEMTIME g_LastDateReplacement;
static int g_commErrorCount = 0;

// ===== 既存プロトタイプ =====
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
const char *GetWeekdayName(int);
SYSTEMTIME AddDays(SYSTEMTIME start, unsigned int days, bool addDirection, bool includeStart);

// ===== UIヘルパ =====
const char *GetWeekdayName(int wday)
{
    static const char *names[] = {"日", "月", "火", "水", "木", "金", "土"};
    return (wday >= 0 && wday <= 6) ? names[wday] : "?";
}

void CollectData(HWND hwnd, InputData *data, char *daysBuf, size_t daysBufSize)
{
    char dateBuf[64] = {0};
    unsigned int y = 0, m = 0, d = 0;

    GetWindowTextA(g_hEditDate, dateBuf, sizeof(dateBuf));
    sscanf_s(dateBuf, "%u %u %u", &y, &m, &d);

    data->year = y;
    data->month = m;
    data->day = d;
    data->weekday = 0;
    data->beforeAfter = (IsDlgButtonChecked(hwnd, ID_RADIO_DIR_POS) == BST_CHECKED);

    // ここで startDateSet はUIローカルでは使わない（後で送信用に設定）
    data->startDateSet = false;

    GetWindowTextA(g_hEditDays, daysBuf, (int)daysBufSize);
}

// ===== 無効値処理（これまで通り）=====
unsigned int InvalidValueHandling(InputData *in, CorrectedData *corr, const char *daysText)
{
    unsigned int errFlags = 0;
    SYSTEMTIME today;
    GetLocalTime(&today);

    // --- 開始日チェック（省略：現行コードのまま）---

    ErrorInfo einfo = {0};
    corr->year = in->year;
    corr->month = in->month;
    corr->day = in->day;
    corr->weekday = in->weekday;
    corr->startDateSet = in->startDateSet;
    corr->beforeAfter = in->beforeAfter;

    // --- 日数チェック（新方式）---
    bool dotFound = false;
    int dotCount = 0;
    int firstDecimalDigit = -1; // -1=未発見
    bool invalidChar = false;

    for (size_t i = 0; daysText[i] != '\0'; i++)
    {
        char c = daysText[i];
        if (c == '.')
        {
            dotCount++;
            if (dotCount > 1)
            { // 小数点が2つ以上
                invalidChar = true;
                break;
            }
            if (daysText[i + 1] != '\0' && isdigit((unsigned char)daysText[i + 1]))
            {
                firstDecimalDigit = daysText[i + 1] - '0';
            }
            else if (daysText[i + 1] != '\0')
            {
                invalidChar = true; // 小数点の直後が数字でない
                break;
            }
        }
        else if (!isdigit((unsigned char)c))
        {
            invalidChar = true;
            break;
        }
    }

    if (invalidChar)
    {
        einfo.flags[ERR_INVALID_VALUE] = true;
        einfo.invalidValue = 1;
        corr->days = 1;
        g_LastDaysErr = einfo;
        errFlags |= F_ERR_INVALID_VALUE;
        return errFlags;
    }

    // 数値化（整数部のみ or 小数部含めてもOK、atofで）
    double val = atof(daysText);
    if (val <= 0.0)
    {
        einfo.flags[ERR_INVALID_VALUE] = true;
        einfo.invalidValue = 1;
        corr->days = 1;
        g_LastDaysErr = einfo;
        errFlags |= F_ERR_INVALID_VALUE;
        return errFlags;
    }

    double intpart = floor(val);

    if (dotCount == 1 && firstDecimalDigit > 0)
    {
        // 小数第一位が 1?9 → 切り上げ
        einfo.flags[ERR_DECIMAL_UP] = true;
        einfo.decimalValue = (unsigned int)(intpart + 1.0);
        intpart = (double)einfo.decimalValue;
    }
    else if (dotCount == 1 && firstDecimalDigit == 0)
    {
        // 小数第一位が 0 → 切り下げ
        einfo.flags[ERR_DECIMAL_DOWN] = true;
        einfo.decimalValue = (unsigned int)intpart;
    }

    // 上限超過チェック
    if (intpart > MAX_DAYS)
    {
        einfo.flags[ERR_EXCEED_MAX] = true;
        einfo.maxValue = MAX_DAYS;
        intpart = (double)MAX_DAYS;
    }

    corr->days = (unsigned int)intpart;
    g_LastDaysErr = einfo;

    if (einfo.flags[ERR_DECIMAL_UP])
        errFlags |= F_ERR_DECIMAL_UP | F_USE_DECIMAL_VALUE;
    else if (einfo.flags[ERR_DECIMAL_DOWN])
        errFlags |= F_ERR_DECIMAL_DOWN | F_USE_DECIMAL_VALUE;
    if (einfo.flags[ERR_EXCEED_MAX])
        errFlags |= F_ERR_EXCEED_MAX | F_USE_MAX_VALUE;

    return errFlags;
}

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

// ===== 通信ヘルパ（部分送受信対応）=====
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

// ===== 通信（毎回接続して送受信して閉じる）=====
int ConnectToCoyomi(void)
{
    WSADATA wsa;
    struct sockaddr_in serverAddr;

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
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(g_sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
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

    if (send_all(g_sock, (const char *)pkt, PACKET_SIZE) != PACKET_SIZE)
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

// ===== 表示 =====
void ShowResultStatus(const CorrectedData *corr, const ResultData *res, unsigned int errFlags)
{
    char resBuf[256] = {0};
    _snprintf_s(resBuf, sizeof(resBuf), _TRUNCATE,
                "結果: %04u年%02u月%02u日(%s) [日数: %u / %s / %s]",
                res->year, res->month, res->day,
                GetWeekdayName(res->weekday),
                corr->days,
                corr->beforeAfter ? "加算" : "減算",
                corr->startDateSet ? "開始日含む" : "開始日含まない");
    SetWindowTextA(g_hStaticRes, resBuf);

    char errBuf[512] = {0};
    bool first = true;

    if (errFlags & F_ERR_DATE_REPLACED)
    {
        char tmp[160];
        _snprintf_s(tmp, sizeof(tmp), _TRUNCATE,
                    ErrorMessageDate,
                    g_LastDateReplacement.wYear,
                    g_LastDateReplacement.wMonth,
                    g_LastDateReplacement.wDay);
        strncat(errBuf, tmp, sizeof(errBuf) - strlen(errBuf) - 1);
        first = false;
    }
    if (errFlags & F_ERR_INVALID_VALUE)
    {
        if (!first)
            strncat(errBuf, "\r\n", sizeof(errBuf) - strlen(errBuf) - 1);
        strncat(errBuf, ErrorMessagesDays[ERR_INVALID_VALUE], sizeof(errBuf) - strlen(errBuf) - 1);
        first = false;
    }
    else
    {
        if (errFlags & F_ERR_DECIMAL_UP)
        {
            char tmp[200];
            unsigned int val = (errFlags & F_USE_DECIMAL_VALUE) ? g_LastDaysErr.decimalValue : corr->days;
            _snprintf_s(tmp, sizeof(tmp), _TRUNCATE, ErrorMessagesDays[ERR_DECIMAL_UP], val);
            if (!first)
                strncat(errBuf, "\r\n", sizeof(errBuf) - strlen(errBuf) - 1);
            strncat(errBuf, tmp, sizeof(errBuf) - strlen(errBuf) - 1);
            first = false;
        }
        else if (errFlags & F_ERR_DECIMAL_DOWN)
        {
            char tmp[220];
            unsigned int val = (errFlags & F_USE_DECIMAL_VALUE) ? g_LastDaysErr.decimalValue : corr->days;
            _snprintf_s(tmp, sizeof(tmp), _TRUNCATE, ErrorMessagesDays[ERR_DECIMAL_DOWN], val);
            if (!first)
                strncat(errBuf, "\r\n", sizeof(errBuf) - strlen(errBuf) - 1);
            strncat(errBuf, tmp, sizeof(errBuf) - strlen(errBuf) - 1);
            first = false;
        }
        if (errFlags & F_ERR_EXCEED_MAX)
        {
            char tmp[200];
            unsigned int maxv = (errFlags & F_USE_MAX_VALUE) ? g_LastDaysErr.maxValue : MAX_DAYS;
            _snprintf_s(tmp, sizeof(tmp), _TRUNCATE, ErrorMessagesDays[ERR_EXCEED_MAX], maxv, maxv);
            if (!first)
                strncat(errBuf, "\r\n", sizeof(errBuf) - strlen(errBuf) - 1);
            strncat(errBuf, tmp, sizeof(errBuf) - strlen(errBuf) - 1);
            first = false;
        }
    }
    SetWindowTextA(g_hStaticErr, errBuf);
}

// ===== AddDays（クライアント側では未使用でも残置可：ローカルフォールバック用）=====
SYSTEMTIME AddDays(SYSTEMTIME start, unsigned int days, bool addDirection, bool includeStart)
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
        move -= 1U;
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

// ===== UIメイン処理（毎回接続→送受信→切断）=====
unsigned int UIProcessMain(InputData *in, CorrectedData *corr, ResultData *res,
                           char *daysTextBuf, size_t daysTextBufSize, int *commErrId)
{
    unsigned int errFlags = 0;
    *commErrId = COMM_ERR_NONE;

    errFlags = InvalidValueHandling(in, corr, daysTextBuf);

    bool includeStart = (IsDlgButtonChecked(GetParent(g_hEditDays), ID_RADIO_INC_YES) == BST_CHECKED);
    corr->startDateSet = includeStart;
    corr->beforeAfter = in->beforeAfter;

    int cid = ConnectToCoyomi();
    if (cid != COMM_ERR_NONE)
    {
        *commErrId = cid;
        return errFlags;
    }

    cid = UISendReceive(corr, res);
    if (cid != COMM_ERR_NONE)
    {
        *commErrId = cid;
        CloseConnection();
        return errFlags;
    }

    CloseConnection();
    g_commErrorCount = 0; // 成功したらリセット
    return errFlags;
}

// ===== 新規: カレンダー→EDIT 同期 =====
static void SyncDatePickerToEdit(void)
{
    SYSTEMTIME st;
    if (DateTime_GetSystemtime(g_hDatePicker, &st) == GDT_VALID)
    {
        char buf[32];
        wsprintfA(buf, "%04u %02u %02u", st.wYear, st.wMonth, st.wDay);
        SetWindowTextA(g_hEditDate, buf);
    }
}

// ===== 新規: EDIT→カレンダー 同期 =====
static void SyncEditToDatePicker(void)
{
    char buf[64];
    GetWindowTextA(g_hEditDate, buf, sizeof(buf));
    unsigned int y, m, d;
    if (sscanf_s(buf, "%u %u %u", &y, &m, &d) == 3)
    {
        SYSTEMTIME st = {0};
        st.wYear = (WORD)y;
        st.wMonth = (WORD)m;
        st.wDay = (WORD)d;
        DateTime_SetSystemtime(g_hDatePicker, GDT_VALID, &st);
    }
}

// ===== WndProc（計算ボタン）=====
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        if (HIWORD(wParam) == EN_KILLFOCUS && LOWORD(wParam) == ID_EDIT_DATE)
        {
            // 日付手入力 → カレンダーへ反映
            SyncEditToDatePicker();
        }
        switch (LOWORD(wParam))
        {
        case ID_BUTTON_EXIT:
            DestroyWindow(hwnd);
            break;
        case ID_BUTTON_CALC:
        {
            InputData in = {0};
            CorrectedData corr = {0};
            ResultData res = {0};
            char daysBuf[64] = {0};
            int commErrId = COMM_ERR_NONE;

            // カレンダーの選択内容を EDIT に反映してから収集（念のため）
            SyncDatePickerToEdit();

            // 入力収集
            CollectData(hwnd, &in, daysBuf, sizeof(daysBuf));

            // メイン処理
            unsigned int errFlags = UIProcessMain(&in, &corr, &res, daysBuf, sizeof(daysBuf), &commErrId);

            // 結果表示 or 通信エラー表示
            if (commErrId != COMM_ERR_NONE)
            {
                SetWindowTextA(g_hStaticErr, CommErrorMessages[commErrId]);
            }
            else
            {
                ShowResultStatus(&corr, &res, errFlags);
            }
            break;
        }
        }
        break;

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->idFrom == ID_DATE_PICKER && pnmh->code == DTN_DATETIMECHANGE)
        {
            // カレンダー選択 → EDIT へ反映
            SyncDatePickerToEdit();
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ===== WinMain（UI生成）=====
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nCmdShow)
{
    g_hInst = hInst;

    // コモンコントロール初期化（DateTime Picker）
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icex);

    const char *kClass = "CalendarAppClass";
    WNDCLASSA wc = {0};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = g_hInst;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = kClass;

    if (!RegisterClassA(&wc))
        return 0;

    HWND hwnd = CreateWindowExA(
        0, kClass, "日付計算アプリ",
        WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME),
        CW_USEDEFAULT, CW_USEDEFAULT, 540, 320,
        NULL, NULL, g_hInst, NULL);
    if (!hwnd)
        return 0;

    // ラベル・入力欄
    CreateWindowA("STATIC", "開始日 (YYYY MM DD):", WS_CHILD | WS_VISIBLE,
                  20, 20, 170, 20, hwnd, NULL, g_hInst, NULL);
    g_hEditDate = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                200, 18, 140, 24, hwnd, (HMENU)ID_EDIT_DATE, g_hInst, NULL);

    // DateTime Picker（カレンダー）を EDIT の右に配置
    SYSTEMTIME st;
    GetLocalTime(&st);
    g_hDatePicker = CreateWindowExA(
        0, DATETIMEPICK_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | DTS_SHORTDATEFORMAT,
        350, 18, 140, 24, hwnd, (HMENU)ID_DATE_PICKER, g_hInst, NULL);
    DateTime_SetSystemtime(g_hDatePicker, GDT_VALID, &st);

    // 初期状態として EDIT へも反映
    {
        char buf[32];
        wsprintfA(buf, "%04u %02u %02u", st.wYear, st.wMonth, st.wDay);
        SetWindowTextA(g_hEditDate, buf);
    }

    CreateWindowA("STATIC", "日数:", WS_CHILD | WS_VISIBLE,
                  20, 56, 100, 20, hwnd, NULL, g_hInst, NULL);
    g_hEditDays = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                200, 54, 160, 24, hwnd, (HMENU)ID_EDIT_DAYS, g_hInst, NULL);

    CreateWindowA("BUTTON", "加算", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                  20, 90, 80, 22, hwnd, (HMENU)ID_RADIO_DIR_POS, g_hInst, NULL);
    CreateWindowA("BUTTON", "減算", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                  110, 90, 80, 22, hwnd, (HMENU)ID_RADIO_DIR_NEG, g_hInst, NULL);
    CheckDlgButton(hwnd, ID_RADIO_DIR_POS, BST_CHECKED);

    CreateWindowA("BUTTON", "開始日を含む", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                  210, 90, 110, 22, hwnd, (HMENU)ID_RADIO_INC_YES, g_hInst, NULL);
    CreateWindowA("BUTTON", "開始日を含まない", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                  330, 90, 140, 22, hwnd, (HMENU)ID_RADIO_INC_NO, g_hInst, NULL);
    CheckDlgButton(hwnd, ID_RADIO_INC_YES, BST_CHECKED);

    CreateWindowA("BUTTON", "計算", WS_CHILD | WS_VISIBLE,
                  20, 124, 100, 28, hwnd, (HMENU)ID_BUTTON_CALC, g_hInst, NULL);
    CreateWindowA("BUTTON", "終了", WS_CHILD | WS_VISIBLE,
                  130, 124, 100, 28, hwnd, (HMENU)ID_BUTTON_EXIT, g_hInst, NULL);

    g_hStaticErr = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE,
                                 20, 170, 490, 40, hwnd, (HMENU)ID_STATIC_ERROR, g_hInst, NULL);
    g_hStaticRes = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE,
                                 20, 220, 490, 24, hwnd, (HMENU)ID_STATIC_RESULT, g_hInst, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}