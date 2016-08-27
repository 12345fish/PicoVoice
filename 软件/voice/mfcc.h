#ifndef __MFCC_H__
#define __MFCC_H__

#include "base.h"
#include "voice.h"

#define MFCC_HP_RATIO       (243)                   // Ԥ����ϵ������ 243/256=~0.95
#define MFCC_HP_SCALE       (256)                   // Ԥ����ϵ����ĸ
#define MFCC_FFT_POINT      (256)                   // FFT����
#define MFCC_ORDER_NUM      (12)                    // MFCC����

// MFCC֡����
typedef struct {
    s16 mfcc_dat[MFCC_ORDER_NUM];   // MFCC����
} mfcc_frame_t;

// ���������ṹ��
typedef struct {
    u16 flag;           // ���
    u16 frm_num;        // ֡��
    mfcc_frame_t mfcc_frm[VIOCE_VALID_FRAME_MAX];  // MFCC
} mfcc_vct_t;

s32 mfcc_init(void);
s32 mfcc_frame(s16 *voice, u32 voice_len, mfcc_frame_t *mfcc_out);

#endif // __MFCC_H__
