#ifndef __AUDIO_IN_H__
#define __AUDIO_IN_H__

#include "base.h"

enum {
    AUDIO_IN_READY_NONE,
    AUDIO_IN_READY_HEAD,
    AUDIO_IN_READY_TAIL,
    AUDIO_IN_READY_MAX
};

s32 audio_in_init(u16 rate, s16 *buf, u16 len);
s32 audio_in_get_ready(void);

#endif // __AUDIO_IN_H__
