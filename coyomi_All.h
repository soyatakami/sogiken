//*coyomi_All.h
//�{���ɂ��݂܂���D�Փ˂�2�d�錾����̃G���[���������C�S���܂�܂����D
//�g���������̍Œ�ȃR�[�h�ł��D
//���W���[������2�̒i�K�ŁZ�Z�b�̃R���p�C������ CreateWindow(AndShowWindow)
//���W���[������3�̒i�K�ŁZ�Z�b�̃R���p�C������ 
//���W���[������4�̒i�K�ŁZ�Z�b�̃R���p�C������

//* coyomi_all.h ? 1���ɓ����������ʃw�b�_
#ifndef COYOMI_ALL_H
#define COYOMI_ALL_H

// ---- Windows�^�iHWND/HINSTANCE/WNDPROC ���j���g���̂� windows.h ��ǂ� ----
// ����: �ˑ����� .c �ł� winsock2.h �� windows.h ����� include ���Ă��������B
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// ---- C �����^�C�� ----
#include <stdint.h>
#include <stdbool.h>

/* ==== socket abstraction (no <winsock2.h> needed in headers) ==== */
#include <stdint.h>
#ifndef COYOMI_SOCKET_DEFINED
#define COYOMI_SOCKET_DEFINED
  /* winsock2.h ���ɓǂޖ|��P�ʂł� _WINSOCK2API_ ����`����� */
  #if defined(_WINSOCK2API_)
    typedef SOCKET COYOMI_SOCKET;
  #else
    typedef uintptr_t COYOMI_SOCKET;       /* SOCKET �ƌ݊��̕� */
  #endif
  #ifndef COYOMI_INVALID_SOCKET
  #define COYOMI_INVALID_SOCKET ((COYOMI_SOCKET)~(uintptr_t)0)
  #endif
#endif

/* ���M���� 0��1 �𗧂Ă邽�߂̃G���[�ʒm�R�[���o�b�N */
#ifndef COYOMI_ERRORCB_DEFINED
#define COYOMI_ERRORCB_DEFINED
  typedef void (*ErrorRaiseCb)(int id);
#endif

// ==============================
// �R���g���[�� ID
// ==============================
// IDC_DTP �c DateTimePicker �̎q�R���g���[��ID�iDTM_GETSYSTEMTIME ���Ɏg���j
// IDC_FLAG0 / IDC_FLAG1 �c �u0����/1���ځv�̃��W�I�{�^��
// IDC_DIR_BACK / IDC_DIR_FWD �c �u�O/��v�̃��W�I�{�^��
// IDC_NDAYS �c �������� EDIT ��ID
// IDC_SEND �c �u���s�v�{�^����ID�iWM_COMMAND�ŉ�������j
// IDC_EXIT �c �u�I���v�{�^����ID
// IDC_STATUS �c ���b�Z�[�W�\���p EDIT ��ID
// IDC_OUTALL �c ���ʕ\���p EDIT ��ID
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
// �E�B���h�E�i�N���C�A���g��j���@
// ==============================
#define GUI_SIZE     300
#define GUI_RATIO_X  10
#define GUI_RATIO_Y  18

// ==============================
// ���́��G���[����i���e�͈́j
// ==============================
#define MIN_Y 2010
#define MIN_M 1
#define MIN_D 1
#define MAX_Y 2099
#define MAX_M 12
#define MAX_D 31

// ==============================
// �ʐM�v���g�R���萔
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
// GUI �\���e�L�X�g�i���{��j
// ==============================
#define MSG_ERR_ID1 L"Connect���o���Ȃ��ꍇ \r\n\tCoyomiSever�A�v�����N�����Ă��邩�m�F���Ă��������D"
#define MSG_ERR_ID2 L"�J�n�����L���͈͊O�ł��D \r\n\t2010/01/01�`2099/12/31�ȊO�͈̔͂Ȃ̂�\r\n\t�����̓��t���J�n���Ɏw�肵�܂��D"
#define MSG_ERR_ID3 L"���p�����E���̐��̂ݓ��͉\�ł��D\r\n\t�����n�����͂��ꂽ�ׁC�����̓��t���J�n���Ɏw�肵�܂��D"
#define MSG_ERR_ID4 L"�����ł̓����̓��͂��L�����̂ŁD�����ꌅ�ڂŐ؂�グ�܂����D"
#define MSG_ERR_ID5 L"�����̏���i10000���j�𒴂��Ă���̂ŁC10000���ɕύX���܂����D"
#define MSG_ERR_ID6 L"���p�����E���̐����ȊO�̓��͂��������ׁC1���ɕύX���܂����D"
#define MSG_ERR_ID7 L"Sent�DReceive���A����5��ʐM���ł��Ȃ��ꍇ \r\n\t�ʐM���m���ł��܂���D"

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

// ==============================
// GUI �z�u�i�ʒu�E�T�C�Y�E�t�H���gpt�j
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
// �f�[�^�\���́i�ʐM / ���́j
// ==============================
// ���͒l�i�[�p�iGUI����擾�����g���̂܂܁h�j
typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    char     ndays_text[32]; // �����iUTF-8������j
    bool     flag01;         // 0/1���ځitrue=1���ځj
    bool     dir_after;      // �O��itrue=��, false=�O�j
} InputData;

// ���M�p�i���C���֍ڂ���f�[�^�j
typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t ndays;
    uint32_t flags;          // [weekday:bit8..10] [flag01:bit1] [dir:bit0]
} SentData;

// ��M�p�i�T�[�o����Ԃ�f�[�^�j
typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t ndays;
    uint32_t flags;          // ����
} ReceiveData;

// ==============================
// ��ʃn���h�����iCreateWindow ���W���[���Ƌ��L�j
// ==============================
typedef struct GUIHandles {
    HWND hMain;

    // ���x����
    HWND hGUI_Comment_1, hGUI_Comment_2, hGUI_Comment_3, hGUI_Comment_4;
    HWND hGUI_Comment_5, hGUI_Comment_6, hGUI_Comment_7, hGUI_Comment_8;
    HWND hGUI_Comment_9;

    // ���͌n
    HWND hGUI_Calendar;
    HWND hGUI_ShiftDays;

    // ���W�I
    HWND hGUI_RadioBox_Zero, hGUI_RadioBox_One;
    HWND hGUI_RadioBox_Before, hGUI_RadioBox_After;

    // �{�^��
    HWND hGUI_Execution, hGUI_End;

    // �o�͗�
    HWND hGUI_Message, hGUI_Result;
} GUIHandles;

// ==== Error IDs (���L�p) ====
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

// // ==== InvalidValueHandling ���W���[�� API ====
// // ������ SYSTEMTIME ���擾
// void InvalidValueHandling_GetTodayTime(SYSTEMTIME* out);

// // InputData �� Y/M/D ���͈͊O�Ȃ獡���ɍ����ւ��ASentData ���\��
// // - in:  ���� (���؂����擾�����l)
// // - out: �����ւ���̒l�ō\������ SentData�indays �� 0 ��ݒ�j
// // - outErr: �G���[ID��Ԃ� (NULL��)�B�͈͊O�Ȃ� ERR_ID2_START_OUT_OF_RANGE�A����Ȃ� ERR_NONE�B
// void InvalidValueHandling_InvalidValueHandling(
//     const InputData* in,
//     SentData* out,
//     ErrorID* outErr
// );

// ==== InvalidValueHandling API ====
// ������ SYSTEMTIME ���擾
// void InvalidValueHandling_GetTodayTime(SYSTEMTIME* out);

// ����(InputData)�����؂��A�K�v�Ȃ�J�n�����u�����v�ɍ����ւ��A�������␳���� SentData ���\��
// - in : CollectData �ł��̂܂܎擾�����l
// - out: �����ւ�/�␳��̑��M�p�f�[�^
// - out_setDtpToday: �J�n���������֍����ւ����ꍇ true�iDTP �̕\���X�V�p�j��NULL��
// - msgbuf/msgcap  : �G���[���b�Z�[�W��ǋL�������ꍇ�ɓn���o�b�t�@�iNULL�j
// - on_error       : �G���[ID�� 0��1 �X�V�������ꍇ�̃R�[���o�b�N�i��: record_error�j�BNULL��
// void InvalidValueHandling_InvalidValueHandling(
//     const InputData* in,
//     SentData* out,
//     bool* out_setDtpToday,
//     wchar_t* msgbuf,
//     size_t msgcap,
//     void (*on_error)(ErrorID)
// );


// ==============================
// CreateWindow ���W���[�� API
// ==============================
// �e�E�B���h�E�{�q�R���g���[�����܂Ƃ߂Đ���
HWND CreateWindow_CreateWindow(
    GUIHandles* gui,
    HINSTANCE   hInst,
    WNDPROC     wndProc,
    LPCWSTR     className,
    LPCWSTR     windowTitle,
    DWORD       style,
    DWORD       exStyle
);

// �\���iShowWindow/UpdateWindow�j
void CreateWindow_ShowWindow(GUIHandles* gui, int nShow);

// ==============================
// CollectData ���W���[�� API
// ==============================
void CollectData_CollectData(const GUIHandles* gui, InputData* out_iv);

// --- InvalidValueHandling ---
// ��֓��t�Ƃ��č������擾�iSYSTEMTIME�j
void InvalidValueHandling_GetTodayTime(SYSTEMTIME* out);

// ���͂̌��؁E�␳���s���A���M�p�� SentData ���ŏI�m��
// - in            : CollectData �ŏW�߂����̂܂܂̓���
// - out           : ���M�p�Ɋm�肳�����l�i���̃��W���[���̋K��ɓ���j
// - out_setDtpToday : true �̏ꍇ�A�J�n���������ɒu���������̂� DTP �������ɍ��킹��Ɨǂ�
// - msgbuf/msgcap : "IDx\t�{��" �`���Ń��b�Z�[�W�ǋL�iNULL �j/ �o�b�t�@�v�f��
// - on_error(id)  : �G���[ID�ʒm�R�[���o�b�N�i0��1 �X�V�p, NULL �j
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
// ConnectToCoyomi ���W���[�� API
// ==============================
// Winsock �̏�����(WSAStartup)�͌Ăяo�����ōs���Ă��������B
// ������: �ڑ��ς� SOCKET ��Ԃ��i�Ăяo������ closesocket ���K�v�j
// ���s��: INVALID_SOCKET ��Ԃ��Aon_error_raised ������� ERR_ID1_CONNECT ��ʒm
// SOCKET ConnectToCoyomi_ConnectToCoyomi(
//     const char* ip,             // "127.0.0.1" ���iNULL/��Ȃ� 127.0.0.1 �����j
//     uint16_t    port,           // ��: 5000
//     void (*on_error_raised)(int error_id) // ���s���� ERR_ID1_CONNECT ��ʒmf
// );

COYOMI_SOCKET ConnectToCoyomi_ConnectToCoyomi(
    const char* ip,
    uint16_t    port,
    ErrorRaiseCb on_error_raised
);

// �i�֗����b�p�jInvalidValueHandling �����̃��W���[���o�R�ŌĂт����ꍇ�p�B
// ���̂܂� InvalidValueHandling_InvalidValueHandling ���Ϗ����܂��B
void ConnectToCoyomi_InvalidValueHandling(
    const InputData* in,
    SentData*        out_sd,
    bool*            out_set_dtp_today,
    wchar_t*         opt_errbuf, size_t opt_errcap,
    void (*on_error_raised)(int error_id)
);

// --- UIntRecive: ����M���W���[�����JAPI -------------------------------
#ifndef COYOMI_SOCKET_DEFINED
// winsock2.h �Ɉˑ����Ȃ����߂̕ʖ��BSOCKET �Ɠ������� (UINT_PTR)�B
typedef UINT_PTR COYOMI_SOCKET;
#define COYOMI_SOCKET_DEFINED 1
#endif

#ifndef ERROR_RAISE_CB_DEFINED
typedef void (*ErrorRaiseCb)(int id);   // ���ɒ�`�ς݂Ȃ�폜OK
#define ERROR_RAISE_CB_DEFINED 1
#endif

// g_error_id �� wire �̖��� word �Ƃ��đ��M�^��M�ŋ��L�iMain �Œ�`�j
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

// ���M: SentData ���l�b�g���[�N���Ƀp�b�N���đ��M�B���s���� on_error(ERR_ID7_SEND_RECV_FAIL)
int UIntRecive_Sent(COYOMI_SOCKET s, const SentData* sd, ErrorRaiseCb on_error);

// ��M: ��M���� ReceiveData �ɃA���p�b�N�B���s���� on_error(ERR_ID7_SEND_RECV_FAIL)
int UIntRecive_Recive(COYOMI_SOCKET s, ReceiveData* rd, ErrorRaiseCb on_error);

// ����M�ł��Ȃ��ꍇ�� ErrorID ���X�V��������ʂŎg���⏕�֐�
void UIntRecive_InvalidValueHandling(ErrorRaiseCb on_error);

#ifdef __cplusplus
}
#endif
// -----------------------------------------------------------------------

/* ===== ShowResultStatus (result + status message) ===== */

#ifdef __cplusplus
extern "C" {
#endif

/* �e�G���[ID�̔����t���O���܂Ƃ߂ēn�����߂̍\���� */
typedef struct {
    /* 0:������ / 1:���� */
    uint32_t id1_connect;
    uint32_t id2_start_out_of_range;
    uint32_t id3_positive_only;
    uint32_t id4_roundup;
    uint32_t id5_cap_10000;
    uint32_t id6_invalid_int;
    uint32_t id7_send_recv_fail;
} ErrorFlags;

/* �ʐM�G���[�������ꍇ�̂݁A���ʗ��ɗj���t���̌��ʂ�\������ */
void ShowResultStatus_Result(
    const GUIHandles* gui,
    const ReceiveData* recv,
    uint32_t start_y, uint32_t start_m, uint32_t start_d,
    uint32_t ndays_ui,
    bool dir_after,
    bool has_comm_error /* true�Ȃ�\�����Ȃ� */
);

/* �G���[ID�Q�i�t���O�j�ɑΉ����郁�b�Z�[�W�����b�Z�[�W���֕\������ */
void ShowResultStatus_Status(
    const GUIHandles* gui,
    const ErrorFlags* flags
);

#ifdef __cplusplus
}
#endif



#endif // COYOMI_ALL_H


