//*coyomi_structs.h
//�\���̂����ɂ���C�ςȃv���g�R���錾�Ɏg�p���Ȃ�
//���W���[�����ɐ؂蕪���Ă��邽�߁C��d�Œ�`���Ă��܂����̂��o�Ă���

#ifndef COYOMI_STRUCTS_H
#define COYOMI_STRUCTS_H

#include <stdint.h>
#include <stdbool.h>

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

#endif // COYOMI_STRUCTS_H
