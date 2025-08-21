/* All.h - ����B: 1���ڂȂ瑗�M������ 1 ���炷�i0 �ɂȂ蓾��j */

#ifndef ALL_H_INCLUDED
#define ALL_H_INCLUDED

/* �����Z�b�g�E�^�[�Q�b�g�� L9.c ���Œ�` */
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

/* =========================
 *  �A�v���萔/ID
 * =========================*/
enum {
    IDC_DTP = 1100,
    IDC_EDIT_NDAYS,
    IDC_RADIO_ZERO,     /* 0���� */
    IDC_RADIO_ONE,      /* 1���� */
    IDC_RADIO_BEFORE,   /* �O */
    IDC_RADIO_AFTER,    /* �� */
    IDC_BTN_SEND,       /* ���s */
    IDC_BTN_EXIT,       /* �I�� */
    IDC_EDIT_STATUS,    /* ���b�Z�[�W�\���� */
    IDC_EDIT_RESULT     /* ���ʕ\���� */
};

#define APP_CLASS      L"CoyomiClientSingleWnd"
#define WM_APP_RESULT  (WM_APP + 1)

/* GUI ���C�A�E�g�萔�i��ŕҏW���₷���悤�Ɉ�ӏ��ցj */
/* �J�n�� DTP */
#define GUI_DTP_XP   20
#define GUI_DTP_YP   20
#define GUI_DTP_XS   180
#define GUI_DTP_YS   28
/* ���� EDIT */
#define GUI_EDIT_NDAYS_XP  220
#define GUI_EDIT_NDAYS_YP  20
#define GUI_EDIT_NDAYS_XS  80
#define GUI_EDIT_NDAYS_YS  28
/* ���W�I 0����/1���� */
#define GUI_RADIO_ZERO_XP  320
#define GUI_RADIO_ZERO_YP  20
#define GUI_RADIO_ZERO_XS  70
#define GUI_RADIO_ZERO_YS  24
#define GUI_RADIO_ONE_XP   400
#define GUI_RADIO_ONE_YP   20
#define GUI_RADIO_ONE_XS   70
#define GUI_RADIO_ONE_YS   24
/* ���W�I �O/�� */
#define GUI_RADIO_BEFORE_XP  320
#define GUI_RADIO_BEFORE_YP  50
#define GUI_RADIO_BEFORE_XS  40
#define GUI_RADIO_BEFORE_YS  24
#define GUI_RADIO_AFTER_XP   370
#define GUI_RADIO_AFTER_YP   50
#define GUI_RADIO_AFTER_XS   40
#define GUI_RADIO_AFTER_YS   24
/* �{�^�� */
#define GUI_BTN_SEND_XP  430
#define GUI_BTN_SEND_YP  50
#define GUI_BTN_SEND_XS  80
#define GUI_BTN_SEND_YS  28
#define GUI_BTN_EXIT_XP  520
#define GUI_BTN_EXIT_YP  50
#define GUI_BTN_EXIT_XS  80
#define GUI_BTN_EXIT_YS  28
/* �o�͗� */
#define GUI_EDIT_RESULT_XP  20
#define GUI_EDIT_RESULT_YP  100
#define GUI_EDIT_RESULT_XS  275
#define GUI_EDIT_RESULT_YS  120
#define GUI_EDIT_STATUS_XP  315
#define GUI_EDIT_STATUS_YP  100
#define GUI_EDIT_STATUS_XS  285
#define GUI_EDIT_STATUS_YS  120

/* =========================
 *  ���w��̖����K��
 * =========================*/
typedef HWND     Whwnd;
typedef UINT_PTR vsock;

/* =========================
 *  �f�[�^�\��
 * =========================*/
typedef struct {
    uint32_t InputDataYear;
    uint32_t InputDataMonth;
    uint32_t InputDataDay;
    char     InputDataNdaysText[20]; /* UTF-8 �������� */
    bool     InputDataFlag01;        /* true=1���� */
    bool     InputDataDirAfter;      /* true=��, false=�O */
} InputData;

#pragma pack(push, 1)
typedef struct {
    uint32_t SendDataYear;
    uint32_t SendDataMonth;
    uint32_t SendDataDay;
    short    SendDataNdays;    /* 0�`10000 ��z��i����B�� 0 ���蓾��j */
    bool     SendDataFlag01;   /* true=1���� */
    bool     SendDataDirAfter; /* true=��, false=�O */
} SendData;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint32_t RecDataaYear;     /* �Ԃ�͂��w��̂܂� */
    uint32_t RecDataMonth;
    uint32_t RecDataDay;
    short    RecDataNdays;
    bool     RecDataFlag01;
    bool     RecDataDirAfter;
} RecData;
#pragma pack(pop)

/* =========================
 *  �G���[ID�iUI�\���p�j
 * =========================*/
typedef enum {
    ERR_NONE                     = 0,
    ERR_ID1_CONNECT              = 1,
    ERR_ID2_START_OUT_OF_RANGE   = 2,
    ERR_ID3_POSITIVE_ONLY        = 3,
    ERR_ID4_ROUNDUP              = 4,
    ERR_ID5_CAP_10000            = 5,
    ERR_ID6_INVALID_INT          = 6,
    ERR_ID7_SEND_RECV_FAIL       = 7
} ErrorId;

/* =========================
 *  �O���[�o���i���t�@�C������Q�Ɓj
 * =========================*/
extern int ErrorID_01, ErrorID_02, ErrorID_03, ErrorID_04;
extern int ErrorID_05, ErrorID_06, ErrorID_07, ErrorID_08;
extern int ErrorID_09;

extern vsock UIClientSocket;
extern Whwnd MainWindow;
extern Whwnd StartData, CalcDay;
extern Whwnd RadioZero, RadioOne, RadioBefore, RadioAfter;
extern Whwnd OutputBoxResult, OutputBoxMessage;

extern const char*    IP_ADDR_SERVER;
extern unsigned short PORT_NUMBER;

extern vsock CoyomiClientSocket;
extern vsock CoyomiListeningSocket;

/* =========================
 *  �v���g�^�C�v
 * =========================*/
LRESULT CALLBACK WinProc(HWND, UINT, WPARAM, LPARAM);
void CollectData(HWND, InputData*);
unsigned int invalidValueHandling(InputData*, SendData*);
int connectToCoyomi(void);
int UISendReceive(const SendData*, RecData*);
void CloseConnection(void);
void ShowResultStatus(const SendData*, const RecData*,
                      int,int,int,int,int,int,int,int,int);
SYSTEMTIME AddDays(SYSTEMTIME, unsigned int, bool, bool);
const wchar_t* GetWeekdayNameW(int);
void createWindow(HWND hwnd, HINSTANCE hInst);

#endif /* ALL_H_INCLUDED */
