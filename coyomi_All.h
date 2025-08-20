//*coyomi_All.h
//本当にすみません．衝突や2重宣言周りのエラーが多発し，心が折れました．
//拡張性無視の最低なコードです．
//モジュール分岐2個の段階で〇〇秒のコンパイル時間 CreateWindow(AndShowWindow)
//モジュール分岐3個の段階で〇〇秒のコンパイル時間 
//モジュール分岐4個の段階で〇〇秒のコンパイル時間

//* coyomi_all.h ? 1枚に統合した共通ヘッダ
#ifndef COYOMI_ALL_H
#define COYOMI_ALL_H

// ---- Windows型（HWND/HINSTANCE/WNDPROC 等）を使うので windows.h を読む ----
// 注意: 依存元の .c では winsock2.h を windows.h より先に include してください。
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// ---- C ランタイム ----
#include <stdint.h>
#include <stdbool.h>

/* ==== socket abstraction (no <winsock2.h> needed in headers) ==== */
#include <stdint.h>
#ifndef COYOMI_SOCKET_DEFINED
#define COYOMI_SOCKET_DEFINED
  /* winsock2.h を先に読む翻訳単位では _WINSOCK2API_ が定義される */
  #if defined(_WINSOCK2API_)
    typedef SOCKET COYOMI_SOCKET;
  #else
    typedef uintptr_t COYOMI_SOCKET;       /* SOCKET と互換の幅 */
  #endif
  #ifndef COYOMI_INVALID_SOCKET
  #define COYOMI_INVALID_SOCKET ((COYOMI_SOCKET)~(uintptr_t)0)
  #endif
#endif

/* 送信側で 0→1 を立てるためのエラー通知コールバック */
#ifndef COYOMI_ERRORCB_DEFINED
#define COYOMI_ERRORCB_DEFINED
  typedef void (*ErrorRaiseCb)(int id);
#endif

// ==============================
// コントロール ID
// ==============================
// IDC_DTP … DateTimePicker の子コントロールID（DTM_GETSYSTEMTIME 等に使う）
// IDC_FLAG0 / IDC_FLAG1 … 「0日目/1日目」のラジオボタン
// IDC_DIR_BACK / IDC_DIR_FWD … 「前/後」のラジオボタン
// IDC_NDAYS … 日数入力 EDIT のID
// IDC_SEND … 「実行」ボタンのID（WM_COMMANDで押下判定）
// IDC_EXIT … 「終了」ボタンのID
// IDC_STATUS … メッセージ表示用 EDIT のID
// IDC_OUTALL … 結果表示用 EDIT のID
enum {
    IDC_NDAYS = 1100,
    IDC_DIR_BACK,
    IDC_DIR_FWD,
    IDC_FLAG0,
    IDC_FLAG1,
    IDC_SEND,
    IDC_OUTALL,
    IDC_STATUS,
    IDC_EXIT,
    IDC_DTP,
};

// ==============================
// ウィンドウ（クライアント基準）寸法
// ==============================
#define GUI_SIZE     300
#define GUI_RATIO_X  10
#define GUI_RATIO_Y  18

// ==============================
// 入力＆エラー制御（許容範囲）
// ==============================
#define MIN_Y 2010
#define MIN_M 1
#define MIN_D 1
#define MAX_Y 2099
#define MAX_M 12
#define MAX_D 31

// ==============================
// 通信プロトコル定数
// ==============================
#ifndef PACKET_WORDS
#define PACKET_WORDS 6  // [year,month,day,ndays,flags,error]
#endif

#ifndef FLAG_DIR_MASK
#define FLAG_DIR_MASK     0x00000001u
#endif
#ifndef FLAG_FLAG01_MASK
#define FLAG_FLAG01_MASK  0x00000002u
#endif
#ifndef FLAG_WD_MASK
#define FLAG_WD_MASK      0x00000700u
#endif
#ifndef FLAG_WD_SHIFT
#define FLAG_WD_SHIFT     8
#endif

// ==============================
// GUI 表示テキスト（日本語）
// ==============================
#define MSG_ERR_ID1 L"Connectが出来ない場合 \r\n\tCoyomiSeverアプリが起動しているか確認してください．"
#define MSG_ERR_ID2 L"開始日が有効範囲外です． \r\n\t2010/01/01～2099/12/31以外の範囲なので\r\n\t今日の日付を開始日に指定します．"
#define MSG_ERR_ID3 L"半角数字・正の数のみ入力可能です．\r\n\t無効地が入力された為，今日の日付を開始日に指定します．"
#define MSG_ERR_ID4 L"少数での日数の入力が有ったので．整数一桁目で切り上げました．"
#define MSG_ERR_ID5 L"日数の上限（10000日）を超えているので，10000日に変更しました．"
#define MSG_ERR_ID6 L"半角数字・正の整数以外の入力があった為，1日に変更しました．"
#define MSG_ERR_ID7 L"Sent．Receiveが連続で5回通信ができない場合 \r\n\t通信が確立できません．"

#define GUI_Comment_1    L"開始日"
#define GUI_Comment_2    L"有効範囲\r\n2010/01/01～2099/12/31\r\n*半角数字・正の整数のみ入力可"
#define GUI_Comment_3    L"開始日を"
#define GUI_Comment_4    L"とする．"
#define GUI_Comment_5    L"開始日の"
#define GUI_Comment_6    L"日"
#define GUI_Comment_7    L"*半角数字・正の整数のみ入力可"
#define GUI_Comment_8    L"[メッセージ表示欄]"
#define GUI_Comment_9    L"[計算結果表示欄]"

#define GUI_Execution    L"実行"
#define GUI_End          L"終了"

#define GUI_RadioBox_Before L"前"
#define GUI_RadioBox_After  L"後"
#define GUI_RadioBox_Zero   L"0日目"
#define GUI_RadioBox_One    L"1日目"

// ==============================
// GUI 配置（位置・サイズ・フォントpt）
// ==============================
#define GUI_Comment_1_Xp 20
#define GUI_Comment_1_Yp 25
#define GUI_Comment_1_Xs 60
#define GUI_Comment_1_Ys 20
#define GUI_Comment_1_pt 12

#define GUI_Calendar_Xp 100
#define GUI_Calendar_Yp 20
#define GUI_Calendar_Xs 200
#define GUI_Calendar_Ys 30
#define GUI_Calendar_pt 12

#define GUI_Comment_2_Xp 320
#define GUI_Comment_2_Yp 20
#define GUI_Comment_2_Xs 200
#define GUI_Comment_2_Ys 35
#define GUI_Comment_2_pt 6

#define GUI_Comment_3_Xp 20
#define GUI_Comment_3_Yp 75
#define GUI_Comment_3_Xs 75
#define GUI_Comment_3_Ys 20
#define GUI_Comment_3_pt 12

#define GUI_RadioBox_Zero_Xp 100
#define GUI_RadioBox_Zero_Yp 75
#define GUI_RadioBox_Zero_Xs 75
#define GUI_RadioBox_Zero_Ys 20
#define GUI_RadioBox_Zero_pt 12

#define GUI_RadioBox_One_Xp 180
#define GUI_RadioBox_One_Yp 75
#define GUI_RadioBox_One_Xs 75
#define GUI_RadioBox_One_Ys 20
#define GUI_RadioBox_One_pt 12

#define GUI_Comment_4_Xp 260
#define GUI_Comment_4_Yp 75
#define GUI_Comment_4_Xs 75
#define GUI_Comment_4_Ys 20
#define GUI_Comment_4_pt 12

#define GUI_Comment_5_Xp 20
#define GUI_Comment_5_Yp 125
#define GUI_Comment_5_Xs 75
#define GUI_Comment_5_Ys 20
#define GUI_Comment_5_pt 12

#define GUI_ShiftDays_Xp 100
#define GUI_ShiftDays_Yp 125
#define GUI_ShiftDays_Xs 75
#define GUI_ShiftDays_Ys 25
#define GUI_ShiftDays_pt 12

#define GUI_Comment_6_Xp 180
#define GUI_Comment_6_Yp 125
#define GUI_Comment_6_Xs 25
#define GUI_Comment_6_Ys 20
#define GUI_Comment_6_pt 12

#define GUI_RadioBox_Before_Xp 210
#define GUI_RadioBox_Before_Yp 110
#define GUI_RadioBox_Before_Xs 50
#define GUI_RadioBox_Before_Ys 20
#define GUI_RadioBox_Before_pt 12

#define GUI_RadioBox_After_Xp 210
#define GUI_RadioBox_After_Yp 140
#define GUI_RadioBox_After_Xs 50
#define GUI_RadioBox_After_Ys 20
#define GUI_RadioBox_After_pt 12

#define GUI_Comment_7_Xp 20
#define GUI_Comment_7_Yp 155
#define GUI_Comment_7_Xs 200
#define GUI_Comment_7_Ys 12
#define GUI_Comment_7_pt 6

#define GUI_Execution_Xp 270
#define GUI_Execution_Yp 125
#define GUI_Execution_Xs 100
#define GUI_Execution_Ys 25
#define GUI_Execution_pt 12

#define GUI_End_Xp 400
#define GUI_End_Yp 125
#define GUI_End_Xs 100
#define GUI_End_Ys 25
#define GUI_End_pt 12

#define GUI_Comment_8_Xp 20
#define GUI_Comment_8_Yp 170
#define GUI_Comment_8_Xs 150
#define GUI_Comment_8_Ys 20
#define GUI_Comment_8_pt 12

#define GUI_Comment_9_Xp 270
#define GUI_Comment_9_Yp 170
#define GUI_Comment_9_Xs 150
#define GUI_Comment_9_Ys 20
#define GUI_Comment_9_pt 12

#define GUI_Message_Xp 20
#define GUI_Message_Yp 200
#define GUI_Message_Xs 225
#define GUI_Message_Ys 75
#define GUI_Message_pt 6

#define GUI_OutAll_Xp 270
#define GUI_OutAll_Yp 200
#define GUI_OutAll_Xs 225
#define GUI_OutAll_Ys 75
#define GUI_OutAll_pt 10

// ==============================
// データ構造体（通信 / 入力）
// ==============================
// 入力値格納用（GUIから取得した“そのまま”）
typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    char     ndays_text[32]; // 日数（UTF-8文字列）
    bool     flag01;         // 0/1日目（true=1日目）
    bool     dir_after;      // 前後（true=後, false=前）
} InputData;

// 送信用（ワイヤへ載せるデータ）
typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t ndays;
    uint32_t flags;          // [weekday:bit8..10] [flag01:bit1] [dir:bit0]
} SentData;

// 受信用（サーバから返るデータ）
typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t ndays;
    uint32_t flags;          // 同上
} ReceiveData;

// ==============================
// 画面ハンドル束（CreateWindow モジュールと共有）
// ==============================
typedef struct GUIHandles {
    HWND hMain;

    // ラベル類
    HWND hGUI_Comment_1, hGUI_Comment_2, hGUI_Comment_3, hGUI_Comment_4;
    HWND hGUI_Comment_5, hGUI_Comment_6, hGUI_Comment_7, hGUI_Comment_8;
    HWND hGUI_Comment_9;

    // 入力系
    HWND hGUI_Calendar;
    HWND hGUI_ShiftDays;

    // ラジオ
    HWND hGUI_RadioBox_Zero, hGUI_RadioBox_One;
    HWND hGUI_RadioBox_Before, hGUI_RadioBox_After;

    // ボタン
    HWND hGUI_Execution, hGUI_End;

    // 出力欄
    HWND hGUI_Message, hGUI_Result;
} GUIHandles;

// ==== Error IDs (共有用) ====
typedef enum {
    ERR_NONE                     = 0,
    ERR_ID1_CONNECT              = 1,
    ERR_ID2_START_OUT_OF_RANGE   = 2,
    ERR_ID3_POSITIVE_ONLY        = 3,
    ERR_ID4_ROUNDUP              = 4,
    ERR_ID5_CAP_10000            = 5,
    ERR_ID6_INVALID_INT          = 6,
    ERR_ID7_SEND_RECV_FAIL       = 7
} ErrorID;

// // ==== InvalidValueHandling モジュール API ====
// // 今日の SYSTEMTIME を取得
// void InvalidValueHandling_GetTodayTime(SYSTEMTIME* out);

// // InputData の Y/M/D が範囲外なら今日に差し替え、SentData を構成
// // - in:  入力 (検証せず取得した値)
// // - out: 差し替え後の値で構成した SentData（ndays は 0 を設定）
// // - outErr: エラーIDを返す (NULL可)。範囲外なら ERR_ID2_START_OUT_OF_RANGE、正常なら ERR_NONE。
// void InvalidValueHandling_InvalidValueHandling(
//     const InputData* in,
//     SentData* out,
//     ErrorID* outErr
// );

// ==== InvalidValueHandling API ====
// 今日の SYSTEMTIME を取得
// void InvalidValueHandling_GetTodayTime(SYSTEMTIME* out);

// 入力(InputData)を検証し、必要なら開始日を「今日」に差し替え、日数も補正して SentData を構成
// - in : CollectData でそのまま取得した値
// - out: 差し替え/補正後の送信用データ
// - out_setDtpToday: 開始日を今日へ差し替えた場合 true（DTP の表示更新用）※NULL可
// - msgbuf/msgcap  : エラーメッセージを追記したい場合に渡すバッファ（NULL可）
// - on_error       : エラーIDを 0→1 更新したい場合のコールバック（例: record_error）。NULL可
// void InvalidValueHandling_InvalidValueHandling(
//     const InputData* in,
//     SentData* out,
//     bool* out_setDtpToday,
//     wchar_t* msgbuf,
//     size_t msgcap,
//     void (*on_error)(ErrorID)
// );


// ==============================
// CreateWindow モジュール API
// ==============================
// 親ウィンドウ＋子コントロールをまとめて生成
HWND CreateWindow_CreateWindow(
    GUIHandles* gui,
    HINSTANCE   hInst,
    WNDPROC     wndProc,
    LPCWSTR     className,
    LPCWSTR     windowTitle,
    DWORD       style,
    DWORD       exStyle
);

// 表示（ShowWindow/UpdateWindow）
void CreateWindow_ShowWindow(GUIHandles* gui, int nShow);

// ==============================
// CollectData モジュール API
// ==============================
void CollectData_CollectData(const GUIHandles* gui, InputData* out_iv);

// --- InvalidValueHandling ---
// 代替日付として今日を取得（SYSTEMTIME）
void InvalidValueHandling_GetTodayTime(SYSTEMTIME* out);

// 入力の検証・補正を行い、送信用の SentData を最終確定
// - in            : CollectData で集めたそのままの入力
// - out           : 送信用に確定させた値（このモジュールの規約に統一）
// - out_setDtpToday : true の場合、開始日を今日に置き換えたので DTP を今日に合わせると良い
// - msgbuf/msgcap : "IDx\t本文" 形式でメッセージ追記（NULL 可）/ バッファ要素数
// - on_error(id)  : エラーID通知コールバック（0→1 更新用, NULL 可）
void InvalidValueHandling_InvalidValueHandling(
    const InputData* in,
    SentData* out,
    bool* out_setDtpToday,
    wchar_t* msgbuf,
    size_t msgcap,
    void (*on_error)(int id)
);

#ifdef __cplusplus
}
#endif

// ==============================
// ConnectToCoyomi モジュール API
// ==============================
// Winsock の初期化(WSAStartup)は呼び出し側で行ってください。
// 成功時: 接続済み SOCKET を返す（呼び出し側で closesocket が必要）
// 失敗時: INVALID_SOCKET を返し、on_error_raised があれば ERR_ID1_CONNECT を通知
// SOCKET ConnectToCoyomi_ConnectToCoyomi(
//     const char* ip,             // "127.0.0.1" 等（NULL/空なら 127.0.0.1 扱い）
//     uint16_t    port,           // 例: 5000
//     void (*on_error_raised)(int error_id) // 失敗時に ERR_ID1_CONNECT を通知f
// );

COYOMI_SOCKET ConnectToCoyomi_ConnectToCoyomi(
    const char* ip,
    uint16_t    port,
    ErrorRaiseCb on_error_raised
);

// （便利ラッパ）InvalidValueHandling をこのモジュール経由で呼びたい場合用。
// そのまま InvalidValueHandling_InvalidValueHandling を委譲します。
void ConnectToCoyomi_InvalidValueHandling(
    const InputData* in,
    SentData*        out_sd,
    bool*            out_set_dtp_today,
    wchar_t*         opt_errbuf, size_t opt_errcap,
    void (*on_error_raised)(int error_id)
);

// --- UIntRecive: 送受信モジュール公開API -------------------------------
#ifndef COYOMI_SOCKET_DEFINED
// winsock2.h に依存しないための別名。SOCKET と同じ実体 (UINT_PTR)。
typedef UINT_PTR COYOMI_SOCKET;
#define COYOMI_SOCKET_DEFINED 1
#endif

#ifndef ERROR_RAISE_CB_DEFINED
typedef void (*ErrorRaiseCb)(int id);   // 既に定義済みなら削除OK
#define ERROR_RAISE_CB_DEFINED 1
#endif

// g_error_id は wire の末尾 word として送信／受信で共有（Main で定義）
#ifndef EXTERN_G_ERROR_ID_DECLARED
extern ErrorID g_error_id;
#define EXTERN_G_ERROR_ID_DECLARED 1
#endif

#ifndef PACKET_WORDS
#define PACKET_WORDS 6  // [year, month, day, ndays, flags, error(last one)]
#endif

#ifdef __cplusplus
extern "C" {
#endif

// 送信: SentData をネットワーク順にパックして送信。失敗時は on_error(ERR_ID7_SEND_RECV_FAIL)
int UIntRecive_Sent(COYOMI_SOCKET s, const SentData* sd, ErrorRaiseCb on_error);

// 受信: 受信して ReceiveData にアンパック。失敗時は on_error(ERR_ID7_SEND_RECV_FAIL)
int UIntRecive_Recive(COYOMI_SOCKET s, ReceiveData* rd, ErrorRaiseCb on_error);

// 送受信できない場合に ErrorID を更新したい場面で使う補助関数
void UIntRecive_InvalidValueHandling(ErrorRaiseCb on_error);

#ifdef __cplusplus
}
#endif
// -----------------------------------------------------------------------

/* ===== ShowResultStatus (result + status message) ===== */

#ifdef __cplusplus
extern "C" {
#endif

/* 各エラーIDの発生フラグをまとめて渡すための構造体 */
typedef struct {
    /* 0:未発生 / 1:発生 */
    uint32_t id1_connect;
    uint32_t id2_start_out_of_range;
    uint32_t id3_positive_only;
    uint32_t id4_roundup;
    uint32_t id5_cap_10000;
    uint32_t id6_invalid_int;
    uint32_t id7_send_recv_fail;
} ErrorFlags;

/* 通信エラーが無い場合のみ、結果欄に曜日付きの結果を表示する */
void ShowResultStatus_Result(
    const GUIHandles* gui,
    const ReceiveData* recv,
    uint32_t start_y, uint32_t start_m, uint32_t start_d,
    uint32_t ndays_ui,
    bool dir_after,
    bool has_comm_error /* trueなら表示しない */
);

/* エラーID群（フラグ）に対応するメッセージをメッセージ欄へ表示する */
void ShowResultStatus_Status(
    const GUIHandles* gui,
    const ErrorFlags* flags
);

#ifdef __cplusplus
}
#endif



#endif // COYOMI_ALL_H


