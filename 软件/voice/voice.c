#include "base.h"
#include <stdio.h>
#include <string.h>
#include <arm_math.h>
#include "timebase.h"
#include "audio_in.h"
#include "vad.h"
#include "mfcc.h"
#include "dtw.h"

// 语音采样缓存
s16 g_voice_sample[VOICE_FRAME_LEN];

// 语音帧缓存
s16 g_voice_frame[VOICE_FRAME_LEN];

// 语音端点检测
vad_t g_vad;

// MFCC数据
mfcc_vct_t g_mfcc_vct;

/*-----------------------------------*/

// 语音初始化
// 参数：无
// 返回值：0成功/-1失败
s32 voice_init(void)
{
    s32 ret;
    
    ret = vad_init(&g_vad);
    if (0 != ret) {
        return -1;
    }
    ret = mfcc_init();
    if (0 != ret) {
        return -1;
    }
    ret = audio_in_init(VOICE_SR, g_voice_sample, VOICE_FRAME_LEN);
    if (0 != ret) {
        return -1;
    }

    return 0;
}

s32 voice_adapt(void)
{
    s32 ret;
    
    while (AUDIO_IN_READY_HEAD != audio_in_get_ready());
    memcpy(g_voice_frame, g_voice_sample, VOICE_FRAME_MOVE_SIZE);
    while (AUDIO_IN_READY_TAIL != audio_in_get_ready());
    memcpy((u8*)g_voice_frame + VOICE_FRAME_MOVE_SIZE, (u8*)g_voice_sample + VOICE_FRAME_MOVE_SIZE, VOICE_FRAME_MOVE_SIZE);
    
    ret = vad_adapt(&g_vad, g_voice_frame, VOICE_FRAME_LEN);
    if (0 != ret) {
        return -1;
    }
    
    return 0;
}

s32 voice_frame_shift(void)
{
    u32 i;
    
    for (i = 0; i < VOICE_FRAME_LEN; i++) {
        g_voice_frame[i] = (g_voice_frame[i] - g_vad.adapt_arg.mid_val) << 3;
    }
    
    return 0;
}

s32 voice_process(void)
{
    s32 ret;
    
    if (AUDIO_IN_READY_HEAD == audio_in_get_ready()) {
        memmove(g_voice_frame, (u8*)g_voice_frame + VOICE_FRAME_MOVE_SIZE, VOICE_FRAME_MOVE_SIZE);
        memcpy((u8*)g_voice_frame + VOICE_FRAME_MOVE_SIZE, g_voice_sample, VOICE_FRAME_MOVE_SIZE);
    } else if (AUDIO_IN_READY_TAIL == audio_in_get_ready()) {
        memmove(g_voice_frame, (u8*)g_voice_frame + VOICE_FRAME_MOVE_SIZE, VOICE_FRAME_MOVE_SIZE);
        memcpy((u8*)g_voice_frame + VOICE_FRAME_MOVE_SIZE, (u8*)g_voice_sample + VOICE_FRAME_MOVE_SIZE, VOICE_FRAME_MOVE_SIZE);
    } else {
        return 0;
    }
    
    vad_frame(&g_vad, g_voice_frame, VOICE_FRAME_LEN);
    if (g_vad.flag & VAD_FLAG_END) {
        if (g_mfcc_vct.frm_num > 0) {
            ret = dtw(&g_mfcc_vct, &g_mfcc_vct);
        }
        return ret;
    }
    
    if (VAD_STA_SILENT == g_vad.state) {
        g_mfcc_vct.frm_num = 0;
        return 0;
    }
    
    voice_frame_shift();
    
    ret = mfcc_frame(g_voice_frame, VOICE_FRAME_LEN, g_mfcc_vct.mfcc_frm + g_mfcc_vct.frm_num);
    g_mfcc_vct.frm_num++;
    if (g_mfcc_vct.frm_num > VIOCE_VALID_FRAME_MAX) {
        ret = dtw(&g_mfcc_vct, &g_mfcc_vct);
        g_mfcc_vct.frm_num = 0;
        vad_reset(&g_vad);
        return 0;
    }
    
    return ret;
}

/*-----------------------------------*/

void fill_square(s16 *buf, u16 n, s16 min, s16 max, u16 step)
{
    u16 i;
    u16 j;
    
    for (i = 0; i < n; i += step * 2) {
        for (j = 0; j < step; j++) {
            *buf++ = min;
        }
        for (j = 0; j < step; j++) {
            *buf++ = max;
        }
    }
}

volatile u32 t, t1, t2;

void voice_test(void)
{
    //fft_test();

    fill_square(g_voice_frame, VOICE_FRAME_LEN, -2048, 2048, 2);
    //voice_frame_shift();
    
    t = micros();
    vad_adapt(&g_vad, g_voice_frame, VOICE_FRAME_LEN);
    t = micros() - t;
    
    t = micros();
    vad_frame(&g_vad, g_voice_frame, VOICE_FRAME_LEN);
    t = micros() - t;
    
    t = micros();
    mfcc_frame(g_voice_frame, VOICE_FRAME_LEN, g_mfcc_vct.mfcc_frm);
    t = micros() - t;
    
    g_mfcc_vct.frm_num = 63;
    
    t = micros();
    dtw(&g_mfcc_vct, &g_mfcc_vct);
    t = micros() - t;
    t = t;
    
    while (1) {
        t = micros();
        while (AUDIO_IN_READY_NONE == audio_in_get_ready());
        t1 = micros() - t;
        while (AUDIO_IN_READY_NONE == audio_in_get_ready());
        t2 = micros() - t;
    }
}

/*-----------------------------------*/

#if 0
#define FFT_POINT       256

arm_rfft_instance_q15 g_fft_inst;
q15_t g_fft_in[FFT_POINT];
q15_t g_fft_out[FFT_POINT];

void fft_test(void)
{
    char buf[32] = "";
    u32 t;
    int i;
    
    for (i = 0; i < FFT_POINT; i = i + 8) {
        g_fft_in[i + 0] = 16384;
        g_fft_in[i + 1] = 16384;
        g_fft_in[i + 2] = 16384;
        g_fft_in[i + 3] = 16384;
        g_fft_in[i + 4] = -16384;
        g_fft_in[i + 5] = -16384;
        g_fft_in[i + 6] = -16384;
        g_fft_in[i + 7] = -16384;
    }

    if (ARM_MATH_SUCCESS != arm_rfft_init_q15(&g_fft_inst, FFT_POINT, 0, 1)) {
        return;
    }

    t = micros();
    arm_rfft_q15(&g_fft_inst, g_fft_in, g_fft_out);
    t = micros() - t;
    
    snprintf(buf, sizeof(buf), "FFT t=%uus", t);
}
#endif
