#ifndef __VOICE_H__
#define __VOICE_H__

#include "base.h"

#define VOICE_SR                (8000)  // 采样率

#define VOICE_FRAME_TIME        (32)    // 帧时间 ms
#define VOICE_FRAME_MOVE_TIME   (16)    // 帧移时间 ms
#define VOICE_VALID_TIME_MAX    (1024)  // 单段有效语音最长时间 ms

#define VOICE_FRAME_LEN         (VOICE_FRAME_TIME * VOICE_SR / 1000)        // 帧长度
#define VOICE_FRAME_MOVE_LEN    (VOICE_FRAME_MOVE_TIME * VOICE_SR / 1000)   // 帧移长度
#define VOICE_FRAME_SIZE        (VOICE_FRAME_LEN * sizeof(s16))             // 帧字节数
#define VOICE_FRAME_MOVE_SIZE   (VOICE_FRAME_MOVE_LEN * sizeof(s16))        // 帧移字节数

// 单段有效语音最长帧数
#define VIOCE_VALID_FRAME_MAX   ((VOICE_VALID_TIME_MAX - VOICE_FRAME_TIME) / VOICE_FRAME_MOVE_TIME + 1)    

s32 voice_init(void);

s32 voice_adapt(void);

s32 voice_process(void);

void voice_test(void);

#endif // __VOICE_H__
