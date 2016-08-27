/*
    MFCC：Mel频率倒谱系数

    Mel=2595*lg(1+f/700)
    1000Hz以下按线性刻度 1000Hz以上按对数刻度
    三角型滤波器中心频率 在Mel频率刻度上等间距排列
    预加重:6dB/倍频程 一阶高通滤波器  H(z)=1-uz^(-1) y(n)=x(n)-ux(n-1) u=0.94~0.97

    MFCC 步骤：
    1.对语音信号预加重、分帧、加汉明窗处理，然后进行短时傅里叶变换，得出频谱
    2.取频谱平方，得能量谱。并用24个Mel带通滤波器进行滤波，输出Mel功率谱
    3.对每个滤波器的输出值取对数，得到相应频带的对数功率谱。然后对24个对数功率进行
      反离散余弦变换得到12个MFCC系数
*/
#include "base.h"
//#include <math.h>
#include <arm_math.h>
#include "utils.h"
#include "voice.h"
#include "mfcc.h"
#include "mfcc_table.h"

// 实数FFT实例
arm_rfft_instance_q15 g_fft_inst;

// FFT输出
q15_t g_fft_out_buf[MFCC_FFT_POINT + 2];

// 三角滤波器输出对数功率谱
s32 *g_pow_spct = (s32*)g_fft_out_buf; //[MFCC_TRI_NUM];

/*-----------------------------------*/

// 计算FFT功率谱
// 参数：输入/输出buf(公用)，FFT点数(必须为256)
// 返回值：0成功/-1失败
s32 fft_pwr(s16 *buf, u16 len)
{
    //u32 i;
    //s32 real;
    //s32 imag;

    if (MFCC_FFT_POINT != len) {
        return -1;
    }

    arm_rfft_q15(&g_fft_inst, (q15_t*)buf, g_fft_out_buf);
    
    arm_cmplx_mag_squared_q15(g_fft_out_buf, (q15_t*)buf, MFCC_FFT_POINT / 2);

    /*for (i = 0; i < MFCC_FFT_POINT / 2; i++) {
        real = (s16)(pwr_out[i]);
        imag = (s16)(pwr_out[i] >> 16);
        pwr_out[i] = real * real + imag * imag;
    }*/

    return 0;
}

/*-----------------------------------*/

// MFCC初始化
// 参数：无
// 返回值：0成功/-1失败
s32 mfcc_init(void)
{
    if (ARM_MATH_SUCCESS != arm_rfft_init_q15(&g_fft_inst, MFCC_FFT_POINT, 0, 1)) {
        return -1;
    }
    
    return 0;
}

// 计算一帧语音MFCC
// 参数：语音数据buf(会被修改)，语音样本数，MFCC值(输出)
// 返回值：0成功/-1失败
s32 mfcc_frame(s16 *voice_ptr, u32 voice_len, mfcc_frame_t *mfcc_out)
{
    const s8 *p_dct;
    u16 *frq_spct;
    s32 ffe_val;
    s16 mfcc_val;
    u8 hamm_val;
    u32 i;
    u32 j;

    if ((NULL == voice_ptr) || (VOICE_FRAME_LEN != voice_len) || (NULL == mfcc_out)) {
        return -1;
    }

    for (i = VOICE_FRAME_LEN; i > 0; i--) {
        // 预加重，第一个点未处理
        ffe_val = (s32)voice_ptr[i] - (s32)voice_ptr[i - 1] * MFCC_HP_RATIO / MFCC_HP_SCALE;
        // 加汉明窗
        if (i < MFCC_HAMM_NUM / 2) {
            hamm_val = hamm_tab_256[i];
        } else {
            hamm_val = hamm_tab_256[MFCC_HAMM_NUM - 1 - i];
        }
        voice_ptr[i] = (s16)(ffe_val * hamm_val / MFCC_HAMM_SCALE);
    }

    // FFT能量谱
    if (0 != fft_pwr(voice_ptr, VOICE_FRAME_LEN)) {
        return -1;
    }
    frq_spct = (u16*)voice_ptr;

    // 加三角滤波器
    g_pow_spct[0] = 0;
    for (i = 0; i < tri_cen_tab_128[1]; i++) {
        g_pow_spct[0] += ((u32)frq_spct[i] * tri_even_tab_128[i] / MFCC_TRI_SCALE);
    }
    for (j = 2; j < MFCC_TRI_NUM; j += 2) {
        g_pow_spct[j] = 0;
        for (i = tri_cen_tab_128[j - 1]; i < tri_cen_tab_128[j + 1]; i++) {
            g_pow_spct[j] += ((u32)frq_spct[i] * tri_even_tab_128[i] / MFCC_TRI_SCALE);
        }
    }
    for (j = 1; j < (MFCC_TRI_NUM - 2); j += 2) {
        g_pow_spct[j] = 0;
        for (i = tri_cen_tab_128[j - 1]; i < tri_cen_tab_128[j + 1]; i++) {
            g_pow_spct[j] += ((u32)frq_spct[i] * tri_odd_tab_128[i] / MFCC_TRI_SCALE);
        }
    }
    g_pow_spct[MFCC_TRI_NUM - 1] = 0;
    for (i = tri_cen_tab_128[MFCC_TRI_NUM - 2]; i < (MFCC_FFT_POINT / 2); i++) {
        g_pow_spct[MFCC_TRI_NUM - 1] += ((u32)frq_spct[i] * tri_odd_tab_128[i] / MFCC_TRI_SCALE);
    }

    // 三角滤波器输出取对数
    for (j = 0; j < MFCC_TRI_NUM; j++) {
        //g_pow_spct[j] = (u32)(log(g_pow_spct[j]) * 128); // 取对数后 乘128 提升数据有效位数
        g_pow_spct[j] = logfix(g_pow_spct[j], 7);
    }

    // 反离散余弦变换
    p_dct = dct_tab_24x12;
    for (j = 0; j < MFCC_ORDER_NUM; j++) {
        mfcc_val = 0;
        for (i = 0; i < MFCC_TRI_NUM; i++) {
            mfcc_val += ((s32)g_pow_spct[i] * p_dct[i] / MFCC_DCT_SCALE);
        }
        mfcc_out->mfcc_dat[j] = mfcc_val;
        p_dct += MFCC_TRI_NUM;
    }

    return 0;
}
