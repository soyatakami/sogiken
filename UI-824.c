// gcc -o UI-824.exe UI-824.c -lws2_32 -lcomctl32   ./UI-824.exe
// クライアント（UI 側）
// CreateWindowモジュールの実装方法がわからない
// 5回失敗したら表示が変わるが6回目になると？？？
// 結果の表示方法
// UImainなし
#include <winsock2.h> // 必ず windows.h より先
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

// 通信エラー種別
enum
{
    COMM_ERR_NONE = 0,
    COMM_ERR_GENERAL,
    COMM_ERR_NOT_CONNECTED
};

// コントロール ID
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

// すべてのエラー／警告をビットで表現
typedef enum
{
    SF_NONE = 0,

    // 入力関連エラー
    SF_ERR_DATE_REPLACED = 0x0010u, // 日付フォールバック
    SF_ERR_INVALID_VALUE = 0x0008u,
    SF_ERR_DECIMAL_UP = 0x0001u,
    SF_ERR_DECIMAL_DOWN = 0x0002u,
    SF_ERR_EXCEED_MAX = 0x0004u,

    // 中間値ヒント
    SF_USE_DECIMAL_VALUE = 0x0100u,
    SF_USE_MAX_VALUE = 0x0200u,

    // 通信関連は別ビット
    SF_COMM_GENERAL = 0x00010000u,
    SF_COMM_NOT_CONNECTED = 0x00020000u,
} StatusFlags;

// 日付・入力構造体
typedef struct
{
    unsigned int year, month, day;
    char daysText[64];
    unsigned short weekday;
    bool startDateSet; // 「開始日を含む」ラジオ
    bool beforeAfter;  // true=加算, false=減算
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

// エラー詳細保持
typedef struct
{
    bool flags[5];
    unsigned int decimalValue;
    unsigned int maxValue;
    unsigned int invalidValue;

    // 追加: 通信エラーの連続発生回数
    int commErrorCount;
} ErrorInfo;

// グローバル
static HINSTANCE g_hInst;
static HWND g_hDatePicker, g_hEditDays, g_hStaticErr, g_hStaticRes;
static ErrorInfo g_LastDaysErr;
static SOCKET g_sock = INVALID_SOCKET;
static HWND MainWindow;

// プロトタイプ
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

// 通信ヘルパ: 全バイト送信
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

// 通信ヘルパ: 全バイト受信
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

// 日付計算（サーバと同仕様・UIフォールバック用に残す）
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

// 曜日名
const char *GetWeekdayName(int wday)
{
    static const char *names[] = {"日", "月", "火", "水", "木", "金", "土"};
    return (wday >= 0 && wday < 7) ? names[wday] : "?";
}

//================================================================
// CollectData: DatePicker のみから年月日取得
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
        // フォールバック: 現在時刻を使用
        GetLocalTime(&st);
        in->year = st.wYear;
        in->month = st.wMonth;
        in->day = st.wDay;
    }
    // ラジオ: 加減算
    in->beforeAfter = (IsDlgButtonChecked(MainWindow, ID_RADIO_DIR_POS) == BST_CHECKED);
    // ラジオ: 開始日含む／含まない
    in->startDateSet = (IsDlgButtonChecked(MainWindow, ID_RADIO_INC_YES) == BST_CHECKED);
    // 日数テキスト
    GetWindowTextA(g_hEditDays, in->daysText, sizeof(in->daysText));
}
//================================================================

// ShowResultStatus から切り出した「入力値エラーのみ」の生成ロジック
// errBuf: 出力バッファ, bufSize: その長さ
static void BuildInputErrorString(StatusFlags sf, char *errBuf, size_t bufSize)
{
    bool first = true;
    errBuf[0] = '\0';

    // 無効入力 → 1日として扱う
    if (sf & SF_ERR_INVALID_VALUE)
    {
        strncpy(errBuf,
                "無効な日数入力のため “1日” とみなします",
                bufSize - 1);
        return;
    }

    // 小数切り上げ
    if (sf & SF_ERR_DECIMAL_UP)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp),
                 "小数点以下切り上げて “%u日” とみなします",
                 g_LastDaysErr.decimalValue);
        strncat(errBuf, tmp, bufSize - strlen(errBuf) - 1);
        first = false;
    }

    // 小数切り捨て
    if (sf & SF_ERR_DECIMAL_DOWN)
    {
        if (!first)
        {
            strncat(errBuf, "\r\n", bufSize - strlen(errBuf) - 1);
        }
        char tmp[128];
        snprintf(tmp, sizeof(tmp),
                 "小数点以下切り捨てて “%u日” とみなします",
                 g_LastDaysErr.decimalValue);
        strncat(errBuf, tmp, bufSize - strlen(errBuf) - 1);
        first = false;
    }

    // 上限超過
    if (sf & SF_ERR_EXCEED_MAX)
    {
        if (!first)
        {
            strncat(errBuf, "\r\n", bufSize - strlen(errBuf) - 1);
        }
        char tmp[128];
        snprintf(tmp, sizeof(tmp),
                 "%u日を超えたので %u日とみなします",
                 MAX_DAYS,
                 g_LastDaysErr.maxValue);
        strncat(errBuf, tmp, bufSize - strlen(errBuf) - 1);
    }
}

//================================================================
// 日数入力エラー処理（従来ロジックそのまま）
// 入力値補正＋エラー情報格納
StatusFlags InvalidValueHandling(InputData *in, CorrectedData *corr)
{
    StatusFlags flags = SF_NONE;
    ErrorInfo einfo = {0};

    // グローバルエラー情報をリセット（通信回数もリセット）
    ErrorInfo prev = g_LastDaysErr;
    g_LastDaysErr = (ErrorInfo){.commErrorCount = prev.commErrorCount};

    // 1) 入力日をコピー
    corr->year = in->year;
    corr->month = in->month;
    corr->day = in->day;
    corr->startDateSet = in->startDateSet;
    corr->beforeAfter = in->beforeAfter;

    // 2) 年の範囲チェック
    if (in->year < 2010 || in->year > 2099)
    {
        SYSTEMTIME today;
        GetLocalTime(&today);
        corr->year = today.wYear;
        corr->month = today.wMonth;
        corr->day = today.wDay;
        flags |= SF_ERR_DATE_REPLACED;
    }

    // 3) 文字列解析／無効判定
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

    // 無効入力なら 1日にフォールバック
    if (invalid)
    {
        corr->days = 1;
        flags |= SF_ERR_INVALID_VALUE;
        // commErrorCount はそのままに、入力エラー情報だけ更新
        g_LastDaysErr.decimalValue = einfo.decimalValue;
        g_LastDaysErr.maxValue = einfo.maxValue;
        g_LastDaysErr.invalidValue = einfo.invalidValue;
        goto FINALIZE;
    }

    // 文字列を数値化
    double val = atof(s);
    double integer = floor(val);

    // 0 以下も無効とみなす
    if (val <= 0.0)
    {
        corr->days = 1;
        flags |= SF_ERR_INVALID_VALUE;
        g_LastDaysErr.decimalValue = einfo.decimalValue;
        g_LastDaysErr.maxValue = einfo.maxValue;
        g_LastDaysErr.invalidValue = einfo.invalidValue;
        goto FINALIZE;
    }

    // 4) 小数点以下処理
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

    // 5) 上限チェック
    if (integer > MAX_DAYS)
    {
        einfo.maxValue = MAX_DAYS;
        integer = MAX_DAYS;
        flags |= SF_ERR_EXCEED_MAX | SF_USE_MAX_VALUE;
    }

    // 日数を確定
    corr->days = (unsigned int)integer;

    // ローカルエラー情報をグローバルにコピー
    g_LastDaysErr.decimalValue = einfo.decimalValue;
    g_LastDaysErr.maxValue = einfo.maxValue;
    g_LastDaysErr.invalidValue = einfo.invalidValue;

FINALIZE:
    // 6) 開始日側の曜日計算
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
            corr->weekday = 0; // フォールバック
        }
    }

    return flags;
}
//================================================================

// 補正データ→パケット生成
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

// 返信パケット→結果データ
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
// 毎回接続→送受信→切断
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
    // 成功したら失敗カウンタをリセット

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

    // ────────── 送信 ──────────
    int sent = send(g_sock, (const char *)buf, PACKET_SIZE, 0);
    if (sent == SOCKET_ERROR || sent != PACKET_SIZE)
    {
        // 送信失敗 or 一部しか送れていない
        ++g_LastDaysErr.commErrorCount;
        return g_LastDaysErr.commErrorCount >= 5
                   ? SF_COMM_NOT_CONNECTED
                   : SF_COMM_GENERAL;
    }

    // ────────── 受信 ──────────
    int recvd = recv(g_sock, (char *)buf, PACKET_SIZE, 0);
    if (recvd == SOCKET_ERROR || recvd != PACKET_SIZE)
    {
        // 受信失敗 or 一部しか受け取れていない
        ++g_LastDaysErr.commErrorCount;
        return g_LastDaysErr.commErrorCount >= 5
                   ? SF_COMM_NOT_CONNECTED
                   : SF_COMM_GENERAL;
    }

    // 受け取れたらパース
    ParsePacketToResult(buf, res);
    // 成功したらカウンタをリセット
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
// エラー／結果表示
static void ShowResultStatus(
    const CorrectedData *corr,
    const ResultData *res,
    StatusFlags sf)
{
    char finalErr[1024] = {0};
    char inputErr[512] = {0};
    char resultBuf[512] = {0};

    // 1) 日付フォールバックメッセージ
    if (sf & SF_ERR_DATE_REPLACED)
    {
        snprintf(finalErr, sizeof(finalErr),
                 "開始日が範囲外のため、当日%04u年%02u月%02u日を開始日として計算します",
                 corr->year, corr->month, corr->day);
    }

    // 2) 入力エラー部分
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

    // 3) 通信エラー部分
    if (sf & SF_COMM_NOT_CONNECTED)
    {
        const char *msg = "接続が確立していません。再起動してください。";
        if (finalErr[0])
        {
            strncat(finalErr, "\r\n", sizeof(finalErr) - strlen(finalErr) - 1);
        }
        strncat(finalErr, msg, sizeof(finalErr) - strlen(finalErr) - 1);
    }
    else if (sf & SF_COMM_GENERAL)
    {
        const char *msg = "通信に失敗しました。再度クリックしてください。";
        if (finalErr[0])
        {
            strncat(finalErr, "\r\n", sizeof(finalErr) - strlen(finalErr) - 1);
        }
        strncat(finalErr, msg, sizeof(finalErr) - strlen(finalErr) - 1);
    }

    // 4) メッセージ欄に反映
    SetWindowTextA(g_hStaticErr, finalErr);

    // 5) 結果表示 or クリア
    if (sf & (SF_COMM_GENERAL | SF_COMM_NOT_CONNECTED))
    {
        SetWindowTextA(g_hStaticRes, "");
    }
    else
    {
        // ─── 追加部分 ───
        const char *dirStr = corr->beforeAfter ? "後" : "前";
        const char *includeStr = corr->startDateSet ? "1日目" : "0日目";
        unsigned days = corr->days;

        // 1行目：オプション
        // 2行目：日付変換結果
        snprintf(resultBuf, sizeof(resultBuf),
                 "方向：%s  日数：%u日  開始：%s\r\n"
                 "%04u/%02u/%02u(%s)  →  %04u/%02u/%02u(%s)",
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

    // 1) 入力値補正／エラー検出
    status = InvalidValueHandling((InputData *)in, corr);

    // 2) サーバ接続
    status |= ConnectToCoyomi();

    // 3) 接続成功なら送受信＆切断
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
    // メインウィンドウ生成
    MainWindow = CreateWindowExA(
        0, "CalendarApp", "日付計算アプリ",
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
    // ここがなければ範囲外も入力できる↓
    //  開始日の有効範囲を 2010/01/01 ～ 2099/12/31 に制限
    SYSTEMTIME rg[2];
    rg[0].wYear = 2010;
    rg[0].wMonth = 1;
    rg[0].wDay = 1;
    rg[1].wYear = 2099;
    rg[1].wMonth = 12;
    rg[1].wDay = 31;
    // GDTR_MIN | GDTR_MAX で最小・最大を同時にセット
    DateTime_SetRange(g_hDatePicker,
                      GDTR_MIN | GDTR_MAX,
                      rg);
    // ここがなければ範囲外も入力できる

    CreateWindowA("STATIC", "開始日", WS_CHILD | WS_VISIBLE,
                  20, 40, 50, 20, MainWindow, NULL, hInst, NULL);
    CreateWindowA("STATIC", "有効範囲\n2010\\01\\01-2099\\12\\31\n＊半角数字・正の整数のみ入力可", WS_CHILD | WS_VISIBLE,
                  300, 30, 300, 60, MainWindow, NULL, hInst, NULL);

    // 日数入力欄
    CreateWindowA("STATIC", "＊半角数字・正の整数\nのみ入力可", WS_CHILD | WS_VISIBLE,
                  80, 190, 170, 50, MainWindow, NULL, hInst, NULL);
    CreateWindowA("STATIC", "日数", WS_CHILD | WS_VISIBLE,
                  20, 170, 50, 20, MainWindow, NULL, hInst, NULL);
    CreateWindowA("STATIC", "日", WS_CHILD | WS_VISIBLE,
                  220, 170, 20, 20, MainWindow, NULL, hInst, NULL);
    g_hEditDays = CreateWindowA(
        "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        80, 168, 120, 24, MainWindow, (HMENU)ID_EDIT_DAYS, hInst, NULL);

    // 加減算ラジオ
    CreateWindowA("BUTTON", "後", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                  280, 185, 40, 22, MainWindow, (HMENU)ID_RADIO_DIR_POS, hInst, NULL);
    CreateWindowA("BUTTON", "前", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                  280, 155, 40, 22, MainWindow, (HMENU)ID_RADIO_DIR_NEG, hInst, NULL);
    CheckDlgButton(MainWindow, ID_RADIO_DIR_POS, BST_CHECKED);

    // 開始日含むラジオ
    CreateWindowA("STATIC", "開始日を", WS_CHILD | WS_VISIBLE,
                  20, 100, 70, 20, MainWindow, NULL, hInst, NULL);
    CreateWindowA("BUTTON", "1日目", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                  220, 100, 80, 22, MainWindow, (HMENU)ID_RADIO_INC_YES, hInst, NULL);
    CreateWindowA("BUTTON", "0日目", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                  100, 100, 80, 22, MainWindow, (HMENU)ID_RADIO_INC_NO, hInst, NULL);
    CheckDlgButton(MainWindow, ID_RADIO_INC_YES, BST_CHECKED);
    CreateWindowA("STATIC", "とする", WS_CHILD | WS_VISIBLE,
                  320, 100, 60, 20, MainWindow, NULL, hInst, NULL);

    // ボタン
    CreateWindowA("BUTTON", "計算", WS_CHILD | WS_VISIBLE,
                  370, 160, 100, 40, MainWindow, (HMENU)ID_BUTTON_CALC, hInst, NULL);
    CreateWindowA("BUTTON", "終了", WS_CHILD | WS_VISIBLE,
                  570, 160, 100, 40, MainWindow, (HMENU)ID_BUTTON_EXIT, hInst, NULL);

    // エラー・結果表示
    CreateWindowA("STATIC", "【メッセージ欄】", WS_CHILD | WS_VISIBLE,
                  20, 270, 140, 20, MainWindow, NULL, hInst, NULL);
    CreateWindowA("STATIC", "【計算結果】", WS_CHILD | WS_VISIBLE,
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

            // 1) コントロールから生入力を収集
            CollectData(MainWindow, &in);

            // 2) 補正→通信→結果受信をまとめて実行
            status = UIProcessMain(&in, &corr, &res);

            // 3) メッセージ＆結果を表示
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
// WinMain: ウィンドウ／コントロール生成
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow)
{
    g_hInst = hInst;

    // DateTime Picker 初期化
    INITCOMMONCONTROLSEX ic = {sizeof(ic), ICC_DATE_CLASSES};
    InitCommonControlsEx(&ic);

    // ウィンドウクラス登録
    const char *CLASS_NAME = "CalendarApp";
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA(&wc);

    // ここでCreateWindow群をまとめて呼び出す
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