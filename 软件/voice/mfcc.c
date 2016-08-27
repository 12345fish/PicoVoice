/*
    MFCC��MelƵ�ʵ���ϵ��

    Mel=2595*lg(1+f/700)
    1000Hz���°����Կ̶� 1000Hz���ϰ������̶�
    �������˲�������Ƶ�� ��MelƵ�ʿ̶��ϵȼ������
    Ԥ����:6dB/��Ƶ�� һ�׸�ͨ�˲���  H(z)=1-uz^(-1) y(n)=x(n)-ux(n-1) u=0.94~0.97

    MFCC ���裺
    1.�������ź�Ԥ���ء���֡���Ӻ���������Ȼ����ж�ʱ����Ҷ�任���ó�Ƶ��
    2.ȡƵ��ƽ�����������ס�����24��Mel��ͨ�˲��������˲������Mel������
    3.��ÿ���˲��������ֵȡ�������õ���ӦƵ���Ķ��������ס�Ȼ���24���������ʽ���
      ����ɢ���ұ任�õ�12��MFCCϵ��
*/
#include "base.h"
//#include <math.h>
#include <arm_math.h>
#include "utils.h"
#include "voice.h"
#include "mfcc.h"
#include "mfcc_table.h"

// ʵ��FFTʵ��
arm_rfft_instance_q15 g_fft_inst;

// FFT���
q15_t g_fft_out_buf[MFCC_FFT_POINT + 2];

// �����˲����������������
s32 *g_pow_spct = (s32*)g_fft_out_buf; //[MFCC_TRI_NUM];

/*-----------------------------------*/

// ����FFT������
// ����������/���buf(����)��FFT����(����Ϊ256)
// ����ֵ��0�ɹ�/-1ʧ��
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

// MFCC��ʼ��
// ��������
// ����ֵ��0�ɹ�/-1ʧ��
s32 mfcc_init(void)
{
    if (ARM_MATH_SUCCESS != arm_rfft_init_q15(&g_fft_inst, MFCC_FFT_POINT, 0, 1)) {
        return -1;
    }
    
    return 0;
}

// ����һ֡����MFCC
// ��������������buf(�ᱻ�޸�)��������������MFCCֵ(���)
// ����ֵ��0�ɹ�/-1ʧ��
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
        // Ԥ���أ���һ����δ����
        ffe_val = (s32)voice_ptr[i] - (s32)voice_ptr[i - 1] * MFCC_HP_RATIO / MFCC_HP_SCALE;
        // �Ӻ�����
        if (i < MFCC_HAMM_NUM / 2) {
            hamm_val = hamm_tab_256[i];
        } else {
            hamm_val = hamm_tab_256[MFCC_HAMM_NUM - 1 - i];
        }
        voice_ptr[i] = (s16)(ffe_val * hamm_val / MFCC_HAMM_SCALE);
    }

    // FFT������
    if (0 != fft_pwr(voice_ptr, VOICE_FRAME_LEN)) {
        return -1;
    }
    frq_spct = (u16*)voice_ptr;

    // �������˲���
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

    // �����˲������ȡ����
    for (j = 0; j < MFCC_TRI_NUM; j++) {
        //g_pow_spct[j] = (u32)(log(g_pow_spct[j]) * 128); // ȡ������ ��128 ����������Чλ��
        g_pow_spct[j] = logfix(g_pow_spct[j], 7);
    }

    // ����ɢ���ұ任
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
