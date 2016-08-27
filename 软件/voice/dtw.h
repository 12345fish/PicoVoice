#ifndef __DTW_H__
#define __DTW_H__

#include "base.h"
#include "mfcc.h"

#define DTW_DIS_ERR     0xFFFFFFFF
#define DTW_DIS_MAX     0xFFFFFFFF

#define DTW_INSIDE      0
#define DTW_OUTSIDE     1

s32 dtw(mfcc_vct_t *vct_in, mfcc_vct_t *vct_mdl);

#endif // __DTW_H__
