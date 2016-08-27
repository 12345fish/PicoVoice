/*
    VAD (Voice activity detection) 语音活动检测
    检测出一段声音中的有效语音 起始点和长度 最多3段语音

    短时幅度 短时过零率求取：
    短时幅度直接累加
    短时过零率改进为过门限率，设置正负两个绝对值相等的门限。
    构成门限带，穿过门限带计作过零

    端点判决：
    1.判断语音起始点，要求能够滤除突发性噪声
    突发性噪声可以引起短时能量或过零率的数值很高，但是往往不能维持足够长的时间，
    如门窗的开关，物体的碰撞等引起的噪声，这些都可以通过设定最短时间门限来判别。
    超过两门限之一或全部，并且持续时间超过有效语音最短时间门限，
    返回最开始超过门限的时间点，将其标记为有效语音起始点。

    2.判断语音结束点，要求不能丢弃连词中间短暂的有可能被噪声淹没的“寂静段”
    同时低于两门限，并且持续时间超过无声最长时间门限，
    返回最开始低于门限的时间点，将其标记为有效语音结束点。
*/
#include "base.h"
#include <string.h>
#include "voice.h"
#include "vad.h"

// 初始化VAD
s32 vad_init(vad_t *vad)
{
    memset(vad, 0, sizeof(vad_t));

    return 0;
}

// 复位VAD
s32 vad_reset(vad_t *vad)
{
    adapt_arg_t adapt_arg;

    memcpy(&adapt_arg, &vad->adapt_arg, sizeof(adapt_arg_t));
    memset(vad, 0, sizeof(vad_t));
    memcpy(&vad->adapt_arg, &adapt_arg, sizeof(adapt_arg_t));
    
    return 0;
}

// 环境自适应
s32 vad_adapt(vad_t *vad, const s16 *voice_ptr, u32 voice_len)
{
    s32 n_sum;      // 所有数值之和 求平均值 确定零(中)值
    s32 mid;        // 中值
    u32 abs;        // 绝对值
    u32 abs_sum;    // 绝对值和
    u32 n_max;      // 每一帧噪声最大值
    u32 max_sum;    // 每一帧噪声最大值累加，取平均求噪声阈值
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
            if (abs > n_max) {  //每帧最大绝对值
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

// 处理一帧语音
// 参数: vad结构，语音指针，语音长度(必须为VAD_FRAME_LEN)
// 返回值: -1错误/0成功
s32 vad_frame(vad_t *vad, const s16 *voice_ptr, u32 voice_len)
{
    s16 val;
    s16 n_thl_u;        // 上门限值
    s16 n_thl_b;        // 下门限值
    u32 frm_sum;        // 短时绝对值和
    u32 frm_zero;       // 短时过零(门限)率
    u32 is_valid;       // 帧是否有效
    u32 i;

    if (NULL == vad || NULL == voice_ptr || voice_len != VOICE_FRAME_LEN) {
        return -1;
    }

    n_thl_u = vad->adapt_arg.mid_val + vad->adapt_arg.n_thl;
    n_thl_b = vad->adapt_arg.mid_val - vad->adapt_arg.n_thl;
    frm_sum = 0;
    frm_zero = 0;
    
    for (i = 0; i < VOICE_FRAME_LEN; i++) {
        // 短时过门限率
        val = *(voice_ptr + i);
        if (val >= n_thl_u) {         // 大于上门限值
            if (vad->last_sig == VAD_THL_BELOW) {
                frm_zero++;
            }
            vad->last_sig = VAD_THL_UPPER;
        } else if (val < n_thl_b) {   // 小于下门限值
            if (vad->last_sig == VAD_THL_UPPER) {
                frm_zero++;
            }
            vad->last_sig = VAD_THL_BELOW;
        }
        
        // 短时绝对值和
        val -= vad->adapt_arg.mid_val;
        frm_sum += ABS(val);
    }

    // 至少有一个参数超过其门限值
    is_valid = (frm_sum > vad->adapt_arg.s_thl) || (frm_zero > vad->adapt_arg.z_thl);

    vad->flag = 0;
    if (is_valid) {
        vad->flag |= VAD_FLAG_VALID;
    }

    // 状态转换
    switch (vad->state) {
    case VAD_STA_SILENT:    // 当前是无声段
        if (is_valid) {
            vad->state = VAD_STA_PRE;
            vad->dura_cnt = 1;
            vad->total_cnt = 1;
        } else {
            vad->dura_cnt = 0;
            vad->total_cnt = 0;
        }
        break;
    case VAD_STA_PRE:       // 当前是前端过渡段
        if (is_valid) {
            vad->dura_cnt++;
            vad->total_cnt++;
            if (vad->dura_cnt >= VAD_SND_FRAME_MIN) {
                // 前端过渡段帧数超过最短有效语音帧数，进入语音段
                vad->state = VAD_STA_VALID;
                vad->flag |= VAD_FLAG_BEGIN;
                vad->dura_cnt = 0;
            }
        } else {
            // 持续时间低于语音最短时间门限 视为短时噪声
            vad->state = VAD_STA_SILENT;
            vad->total_cnt = 0;
            vad->dura_cnt = 0;
        }
        break;
    case VAD_STA_VALID:     //当前是语音段
        vad->total_cnt++;
        if (!is_valid) {
            vad->state = VAD_STA_POST;
            vad->dura_cnt = 1;
        } else {
            if (vad->total_cnt > VIOCE_VALID_FRAME_MAX) {
                // 超出最大长度
                vad->state = VAD_STA_SILENT;
                vad->flag |= VAD_FLAG_END;
                vad->dura_cnt = 0;
            }
        }
        break;
    case VAD_STA_POST:      //当前是后端过渡段
        vad->total_cnt++;
        if (is_valid) {
            vad->state = VAD_STA_VALID;
            vad->dura_cnt = 0;
        } else {
            vad->dura_cnt++;
            if (vad->dura_cnt >= VAD_MUTE_FRAME_MAX) {
                //后端过渡段帧数超过最长无声帧数
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

