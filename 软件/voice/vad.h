#ifndef __VAD_H__
#define __VAD_H__

#include "base.h"
#include "voice.h"

#define ADAPT_THL_N_RATIO       (256)   // 噪声门限系数 n_thl=n_max_mean*ADAPT_THL_N_RATIO
#define ADAPT_THL_S_RATIO       (282)   // 短时幅度判决门限系数 s_thl=sum_mean*ADAPT_THL_S_RATIO ~1.1
#define ADAPT_THL_Z_RATIO       (3)     // 短时过零率 判决门限系数 常数 ~0.012
#define ADAPT_THL_SCALE         (256)   // 系数缩放比例

#define ADAPT_TIME              (128)   // 背景噪音自适应总时间 ms
#define VAD_SND_TIME_MIN        (96)    // 有效语音最短时间门限 ms
#define VAD_MUTE_TIME_MAX       (128)   // 无声段最长时间门限 ms

#define ADAPT_LEN               (ADAPT_TIME * VOICE_SR / 1000)              // 背景噪音自适应总长度
#define VAD_SND_FRAME_MIN       (VAD_SND_TIME_MIN / VOICE_FRAME_MOVE_TIME)  // 有效语音最短帧数
#define VAD_MUTE_FRAME_MAX      (VAD_MUTE_TIME_MAX / VOICE_FRAME_MOVE_TIME) // 无声段最长帧数

#define VAD_FLAG_VALID          (0x1)   // 语音段有效
#define VAD_FLAG_BEGIN          (0x2)   // 确认语音开始
#define VAD_FLAG_END            (0x4)   // 确认语音结束

// VAD状态枚举
typedef enum {
    VAD_STA_SILENT,     // 无声段
    VAD_STA_PRE,        // 前端过渡段
    VAD_STA_VALID,      // 语音段
    VAD_STA_POST,       // 后端过渡段
    VAD_STA_MAX
} vad_sta_t;

// 门限状态枚举
typedef enum {
    VAD_THL_NONE,
    VAD_THL_BELOW,      // 门限带以下
    VAD_THL_UPPER,      // 门限带以上
    VAD_THL_MAX
} vad_thl_sig_t;

// 自适应参数
typedef struct {
    s16 mid_val;        // 语音段中值，用于短时过零率计算
    u16 n_thl;          // 噪声阈值，用于短时过零率计算
    u16 z_thl;          // 短时过零率阈值
    u32 s_thl;          // 短时累加和阈值
} adapt_arg_t;

// VAD结构
typedef struct {
    vad_sta_t state;            // 当前VAD状态
    u32 flag;                   // 当前语音段标志
    vad_thl_sig_t last_sig;     // 上次跃出门限带的状态
    adapt_arg_t adapt_arg;      // 噪声适应参数
    u32 dura_cnt;               // 过渡段持续帧数
    u32 total_cnt;              // 总有效语音持续帧数
} vad_t;

s32 vad_init(vad_t *vad);
s32 vad_reset(vad_t *vad);
s32 vad_adapt(vad_t *vad, const s16 *voice_ptr, u32 voice_len);
s32 vad_frame(vad_t *vad, const s16 *voice_ptr, u32 voice_len);

#endif // __VAD_H__

