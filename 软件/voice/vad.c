/*
    VAD (Voice activity detection) ��������
    ����һ�������е���Ч���� ��ʼ��ͳ��� ���3������

    ��ʱ���� ��ʱ��������ȡ��
    ��ʱ����ֱ���ۼ�
    ��ʱ�����ʸĽ�Ϊ�������ʣ�����������������ֵ��ȵ����ޡ�
    �������޴����������޴���������

    �˵��о���
    1.�ж�������ʼ�㣬Ҫ���ܹ��˳�ͻ��������
    ͻ�����������������ʱ����������ʵ���ֵ�ܸߣ�������������ά���㹻����ʱ�䣬
    ���Ŵ��Ŀ��أ��������ײ���������������Щ������ͨ���趨���ʱ���������б�
    ����������֮һ��ȫ�������ҳ���ʱ�䳬����Ч�������ʱ�����ޣ�
    �����ʼ�������޵�ʱ��㣬������Ϊ��Ч������ʼ�㡣

    2.�ж����������㣬Ҫ���ܶ��������м���ݵ��п��ܱ�������û�ġ��ž��Ρ�
    ͬʱ���������ޣ����ҳ���ʱ�䳬�������ʱ�����ޣ�
    �����ʼ�������޵�ʱ��㣬������Ϊ��Ч���������㡣
*/
#include "base.h"
#include <string.h>
#include "voice.h"
#include "vad.h"

// ��ʼ��VAD
s32 vad_init(vad_t *vad)
{
    memset(vad, 0, sizeof(vad_t));

    return 0;
}

// ��λVAD
s32 vad_reset(vad_t *vad)
{
    adapt_arg_t adapt_arg;

    memcpy(&adapt_arg, &vad->adapt_arg, sizeof(adapt_arg_t));
    memset(vad, 0, sizeof(vad_t));
    memcpy(&vad->adapt_arg, &adapt_arg, sizeof(adapt_arg_t));
    
    return 0;
}

// ��������Ӧ
s32 vad_adapt(vad_t *vad, const s16 *voice_ptr, u32 voice_len)
{
    s32 n_sum;      // ������ֵ֮�� ��ƽ��ֵ ȷ����(��)ֵ
    s32 mid;        // ��ֵ
    u32 abs;        // ����ֵ
    u32 abs_sum;    // ����ֵ��
    u32 n_max;      // ÿһ֡�������ֵ
    u32 max_sum;    // ÿһ֡�������ֵ�ۼӣ�ȡƽ����������ֵ
    s32 val;
    u32 i;
    u32 j;

    if (NULL == vad || NULL == voice_ptr || voice_len < VOICE_FRAME_LEN) {
        return -1;
    }

    n_sum = 0;
    for (i = 0; i < voice_len; i++) {
        n_sum += *(voice_ptr + i);
    }
    mid = n_sum / (s32)voice_len;

    max_sum = 0;
    abs_sum = 0;
    for (i = 0; i <= (voice_len - VOICE_FRAME_LEN); i += VOICE_FRAME_LEN) {
        n_max = 0;
        for (j = 0; j < VOICE_FRAME_LEN; j++) {
            val = *(voice_ptr + i + j);
            abs = (val > mid) ? (val - mid) : (mid - val);
            if (abs > n_max) {  //ÿ֡������ֵ
                n_max = abs;
            }
            abs_sum += abs;
        }
        max_sum += n_max;
    }
    abs_sum /= (voice_len / VOICE_FRAME_LEN);
    max_sum /= (voice_len / VOICE_FRAME_LEN);
    
    vad->adapt_arg.mid_val = mid;
    vad->adapt_arg.n_thl = max_sum * ADAPT_THL_N_RATIO / ADAPT_THL_SCALE;
    vad->adapt_arg.s_thl = abs_sum * ADAPT_THL_S_RATIO / ADAPT_THL_SCALE;
    vad->adapt_arg.z_thl = VOICE_FRAME_LEN * ADAPT_THL_Z_RATIO / ADAPT_THL_N_RATIO;

    return 0;
}

// ����һ֡����
// ����: vad�ṹ������ָ�룬��������(����ΪVAD_FRAME_LEN)
// ����ֵ: -1����/0�ɹ�
s32 vad_frame(vad_t *vad, const s16 *voice_ptr, u32 voice_len)
{
    s16 val;
    s16 n_thl_u;        // ������ֵ
    s16 n_thl_b;        // ������ֵ
    u32 frm_sum;        // ��ʱ����ֵ��
    u32 frm_zero;       // ��ʱ����(����)��
    u32 is_valid;       // ֡�Ƿ���Ч
    u32 i;

    if (NULL == vad || NULL == voice_ptr || voice_len != VOICE_FRAME_LEN) {
        return -1;
    }

    n_thl_u = vad->adapt_arg.mid_val + vad->adapt_arg.n_thl;
    n_thl_b = vad->adapt_arg.mid_val - vad->adapt_arg.n_thl;
    frm_sum = 0;
    frm_zero = 0;
    
    for (i = 0; i < VOICE_FRAME_LEN; i++) {
        // ��ʱ��������
        val = *(voice_ptr + i);
        if (val >= n_thl_u) {         // ����������ֵ
            if (vad->last_sig == VAD_THL_BELOW) {
                frm_zero++;
            }
            vad->last_sig = VAD_THL_UPPER;
        } else if (val < n_thl_b) {   // С��������ֵ
            if (vad->last_sig == VAD_THL_UPPER) {
                frm_zero++;
            }
            vad->last_sig = VAD_THL_BELOW;
        }
        
        // ��ʱ����ֵ��
        val -= vad->adapt_arg.mid_val;
        frm_sum += ABS(val);
    }

    // ������һ����������������ֵ
    is_valid = (frm_sum > vad->adapt_arg.s_thl) || (frm_zero > vad->adapt_arg.z_thl);

    vad->flag = 0;
    if (is_valid) {
        vad->flag |= VAD_FLAG_VALID;
    }

    // ״̬ת��
    switch (vad->state) {
    case VAD_STA_SILENT:    // ��ǰ��������
        if (is_valid) {
            vad->state = VAD_STA_PRE;
            vad->dura_cnt = 1;
            vad->total_cnt = 1;
        } else {
            vad->dura_cnt = 0;
            vad->total_cnt = 0;
        }
        break;
    case VAD_STA_PRE:       // ��ǰ��ǰ�˹��ɶ�
        if (is_valid) {
            vad->dura_cnt++;
            vad->total_cnt++;
            if (vad->dura_cnt >= VAD_SND_FRAME_MIN) {
                // ǰ�˹��ɶ�֡�����������Ч����֡��������������
                vad->state = VAD_STA_VALID;
                vad->flag |= VAD_FLAG_BEGIN;
                vad->dura_cnt = 0;
            }
        } else {
            // ����ʱ������������ʱ������ ��Ϊ��ʱ����
            vad->state = VAD_STA_SILENT;
            vad->total_cnt = 0;
            vad->dura_cnt = 0;
        }
        break;
    case VAD_STA_VALID:     //��ǰ��������
        vad->total_cnt++;
        if (!is_valid) {
            vad->state = VAD_STA_POST;
            vad->dura_cnt = 1;
        } else {
            if (vad->total_cnt > VIOCE_VALID_FRAME_MAX) {
                // ������󳤶�
                vad->state = VAD_STA_SILENT;
                vad->flag |= VAD_FLAG_END;
                vad->dura_cnt = 0;
            }
        }
        break;
    case VAD_STA_POST:      //��ǰ�Ǻ�˹��ɶ�
        vad->total_cnt++;
        if (is_valid) {
            vad->state = VAD_STA_VALID;
            vad->dura_cnt = 0;
        } else {
            vad->dura_cnt++;
            if (vad->dura_cnt >= VAD_MUTE_FRAME_MAX) {
                //��˹��ɶ�֡�����������֡��
                vad->state = VAD_STA_SILENT;
                vad->flag |= VAD_FLAG_END;
                vad->dura_cnt = 0;
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

