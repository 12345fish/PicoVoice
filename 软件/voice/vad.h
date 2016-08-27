#ifndef __VAD_H__
#define __VAD_H__

#include "base.h"
#include "voice.h"

#define ADAPT_THL_N_RATIO       (256)   // ��������ϵ�� n_thl=n_max_mean*ADAPT_THL_N_RATIO
#define ADAPT_THL_S_RATIO       (282)   // ��ʱ�����о�����ϵ�� s_thl=sum_mean*ADAPT_THL_S_RATIO ~1.1
#define ADAPT_THL_Z_RATIO       (3)     // ��ʱ������ �о�����ϵ�� ���� ~0.012
#define ADAPT_THL_SCALE         (256)   // ϵ�����ű���

#define ADAPT_TIME              (128)   // ������������Ӧ��ʱ�� ms
#define VAD_SND_TIME_MIN        (96)    // ��Ч�������ʱ������ ms
#define VAD_MUTE_TIME_MAX       (128)   // �������ʱ������ ms

#define ADAPT_LEN               (ADAPT_TIME * VOICE_SR / 1000)              // ������������Ӧ�ܳ���
#define VAD_SND_FRAME_MIN       (VAD_SND_TIME_MIN / VOICE_FRAME_MOVE_TIME)  // ��Ч�������֡��
#define VAD_MUTE_FRAME_MAX      (VAD_MUTE_TIME_MAX / VOICE_FRAME_MOVE_TIME) // �������֡��

#define VAD_FLAG_VALID          (0x1)   // ��������Ч
#define VAD_FLAG_BEGIN          (0x2)   // ȷ��������ʼ
#define VAD_FLAG_END            (0x4)   // ȷ����������

// VAD״̬ö��
typedef enum {
    VAD_STA_SILENT,     // ������
    VAD_STA_PRE,        // ǰ�˹��ɶ�
    VAD_STA_VALID,      // ������
    VAD_STA_POST,       // ��˹��ɶ�
    VAD_STA_MAX
} vad_sta_t;

// ����״̬ö��
typedef enum {
    VAD_THL_NONE,
    VAD_THL_BELOW,      // ���޴�����
    VAD_THL_UPPER,      // ���޴�����
    VAD_THL_MAX
} vad_thl_sig_t;

// ����Ӧ����
typedef struct {
    s16 mid_val;        // ��������ֵ�����ڶ�ʱ�����ʼ���
    u16 n_thl;          // ������ֵ�����ڶ�ʱ�����ʼ���
    u16 z_thl;          // ��ʱ��������ֵ
    u32 s_thl;          // ��ʱ�ۼӺ���ֵ
} adapt_arg_t;

// VAD�ṹ
typedef struct {
    vad_sta_t state;            // ��ǰVAD״̬
    u32 flag;                   // ��ǰ�����α�־
    vad_thl_sig_t last_sig;     // �ϴ�Ծ�����޴���״̬
    adapt_arg_t adapt_arg;      // ������Ӧ����
    u32 dura_cnt;               // ���ɶγ���֡��
    u32 total_cnt;              // ����Ч��������֡��
} vad_t;

s32 vad_init(vad_t *vad);
s32 vad_reset(vad_t *vad);
s32 vad_adapt(vad_t *vad, const s16 *voice_ptr, u32 voice_len);
s32 vad_frame(vad_t *vad, const s16 *voice_ptr, u32 voice_len);

#endif // __VAD_H__

