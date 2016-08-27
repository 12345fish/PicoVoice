#ifndef __MFCC_H__
#define __MFCC_H__

#include "base.h"
#include "voice.h"

#define MFCC_HP_RATIO       (243)                   // 预加重系数分子 243/256=~0.95
#define MFCC_HP_SCALE       (256)                   // 预加重系数分母
#define MFCC_FFT_POINT      (256)                   // FFT点数
#define MFCC_ORDER_NUM      (12)                    // MFCC阶数

// MFCC帧参数
typedef struct {
    s16 mfcc_dat[MFCC_ORDER_NUM];   // MFCC参数
} mfcc_frame_t;

// 语音特征结构体
typedef struct {
    u16 flag;           // 标记
    u16 frm_num;        // 帧数
    mfcc_frame_t mfcc_frm[VIOCE_VALID_FRAME_MAX];  // MFCC
} mfcc_vct_t;

s32 mfcc_init(void);
s32 mfcc_frame(s16 *voice, u32 voice_len, mfcc_frame_t *mfcc_out);

#endif // __MFCC_H__
