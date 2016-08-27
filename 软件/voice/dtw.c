/*
    DTW算法 通过局部优化的方法实现加权距离总和最小

    时间规整函数：
    C={c(1),c(2),…,c(N)}
    N为路径长度，c(n)=(i(n),j(n))表示第n个匹配点是有参考模板的
    第i(n)个特征矢量与待测模板的第j(n)个特征矢量构成的匹配点对，两
    者之间的距离d(x(i(n)),y(j(n)))称为匹配距离。
    时间规整函数满足一下约束：
    1.单调性，规整函数单调增加。
    2.起点终点约束，起点对起点，终点对终点。
    3.连续性，不允许跳过任何一点。
    4.最大规整量不超过某一极限值。|i(n)-j(n)|<M,M为窗宽。规整
    函数所处的区域位于平行四边形内。局部路径约束，用于限制当第n步
    为(i(n),j(n))时，前几步存在几种可能的路径。

    DTW步骤：
    1.初始化。令i(0)=j(0)=0,i(N)=in_frm_num,j(N)=mdl_frm_num.
    确定一个平行四边形，有两个位于(0,0)和(in_frm_num,mdl_frm_num)的顶点，相邻斜边斜
    率分别是2和1/2.规整函数不可超出此平行四边形。
    2.递推求累计距离。

    若输入特征与特征模板的帧数差别过大，直接将匹配距离设为最大
    frm_in_num<(frm_mdl_num/2)||frm_in_num>(frm_mdl_num*2)
*/
#include "base.h"
//#include <math.h>
#include "utils.h"
#include "mfcc.h"
#include "dtw.h"

/*
    获取两个特征矢量之间的距离
    参数
    frm_ftr1    特征矢量1
    frm_ftr2    特征矢量2
    返回值
    dis         矢量距离
*/
u32 get_dis(s16 *frm_ftr1, s16 *frm_ftr2)
{
    u32 i;
    u32 dis_sqr;
    u32 dis;
    s32 dif;    //两矢量相同维度上的差值

    dis_sqr = 0;
    for (i = 0; i < MFCC_ORDER_NUM; i++) {
        dif = frm_ftr1[i] - frm_ftr2[i];
        dis_sqr += (dif * dif);
    }
    dis = sqrt32(dis_sqr);
    
    return dis;
}

void get_mean(s16 *frm_ftr1, s16 *frm_ftr2, s16 *mean)
{
    u32 i;

    for (i = 0; i < MFCC_ORDER_NUM; i++) {
        mean[i] = (frm_ftr1[i] + frm_ftr2[i]) / 2;
    }
}

u32 min3(u32 a, u32 b, u32 c)
{
    u32 min;
    
    min = a;
    if (min > b) {
        min = b;
    }
    if (min > c) {
        min = c;
    }

    return min;
}

//平行四边形两外两顶点 X坐标值
static u16 dtw_ix_up;       //上边交点
static u16 dtw_ix_down;     //下边交点
static u16 dtw_x_limit;     //X最大值
static u16 dtw_y_limit;     //Y最大值

/*
    范围控制
*/
u32 dtw_limit(u16 x, u16 y)
{
    if (x < dtw_ix_up) {
        if (y >= (2 * x + 2)) {
            return DTW_OUTSIDE;
        }
    } else {
        if ((2 * (dtw_y_limit - y)) <= (dtw_x_limit - x - 4)) {
            return DTW_OUTSIDE;
        }
    }

    if (x < dtw_ix_down) {
        if ((2 * y + 2) <= x) {
            return DTW_OUTSIDE;
        }
    } else {
        if ((dtw_y_limit - y - 4) >= (2 * (dtw_x_limit - x))) {
            return DTW_OUTSIDE;
        }
    }

    return DTW_INSIDE;
}

/*
    DTW 动态时间规整
    参数
    vct_in  :输入特征值
    vct_mdl :特征模版
    返回值
    dis     :累计匹配距离
*/
s32 dtw(mfcc_vct_t *vct_in, mfcc_vct_t *vct_mdl)
{
    u32 dis;
    u16 x;
    u16 y;
    u16 step;
    s16 *in_ptr;
    s16 *mdl_ptr;
    u32 up;
    u32 right;
    u32 right_up;
    u32 min;
    u16 in_frm_num;     //输入特征帧数
    u16 mdl_frm_num;    //特征模板帧数
    
    if ((NULL == vct_in) || (NULL == vct_mdl)) {
        return (s32)DTW_DIS_ERR;
    }

    in_frm_num = vct_in->frm_num;
    mdl_frm_num = vct_mdl->frm_num;

    if ((in_frm_num > (mdl_frm_num * 2)) || ((2 * in_frm_num) < mdl_frm_num)) {
        return (s32)DTW_DIS_ERR;
    }
    
    // 计算约束平行四边形顶点值
    dtw_x_limit = in_frm_num;
    dtw_y_limit = mdl_frm_num;
    dtw_ix_up = (2 * dtw_y_limit - dtw_x_limit) / 3;
    dtw_ix_down = (4 * dtw_x_limit - 2 * dtw_y_limit) / 3;
    
    in_ptr = vct_in->mfcc_frm[0].mfcc_dat;
    mdl_ptr = vct_mdl->mfcc_frm[0].mfcc_dat;
    dis = get_dis(in_ptr, mdl_ptr);
    x = 1;
    y = 1;
    step = 1;
    
    do {
        up = DTW_DIS_MAX;
        right = DTW_DIS_MAX;
        right_up = DTW_DIS_MAX;
        if (dtw_limit(x, y + 1) == DTW_INSIDE) {
            up = get_dis(mdl_ptr + MFCC_ORDER_NUM, in_ptr);
        }
        if (dtw_limit(x + 1, y) == DTW_INSIDE) {
            right = get_dis(mdl_ptr, in_ptr + MFCC_ORDER_NUM);
        }
        if (dtw_limit(x + 1, y + 1) == DTW_INSIDE) {
            right_up = get_dis(mdl_ptr + MFCC_ORDER_NUM, in_ptr + MFCC_ORDER_NUM);
        }

        min = min3(right_up, right, up);
        dis += min;

        if (min == right_up) {
            in_ptr += MFCC_ORDER_NUM;
            mdl_ptr += MFCC_ORDER_NUM;
            x++;
            y++;
        } else if (min == up) {
            mdl_ptr += MFCC_ORDER_NUM;
            y++;
        } else {
            in_ptr += MFCC_ORDER_NUM;
            x++;
        }
        
        step++;
    } while ((x < in_frm_num) && (y < mdl_frm_num));

    return (s32)(dis / step); //步长归一化
}

/*
    从两特征矢量获取特征模板
    参数
    ftr_in1 :输入特征值
    ftr_in2 :输入特征值
    ftr_mdl :特征模版
    返回值
    dis     :累计匹配距离
*/
u32 get_mdl(mfcc_vct_t *vct_in1, mfcc_vct_t *vct_in2, mfcc_vct_t *vct_mdl)
{
    u16 x;
    u16 y;
    u16 step;
    s16 *in1;
    s16 *in2;
    s16 *mdl;
    u32 up;
    u32 right;
    u32 right_up;
    u32 min;
    u16 in1_frm_num;
    u16 in2_frm_num;
    u32 dis;

    in1_frm_num = vct_in1->frm_num;
    in2_frm_num = vct_in2->frm_num;

    if ((in1_frm_num > (in2_frm_num * 2)) || ((2 * in1_frm_num) < in2_frm_num)) {
        return DTW_DIS_ERR;
    }

    // 计算约束平行四边形顶点值
    dtw_x_limit = in1_frm_num;
    dtw_y_limit = in2_frm_num;
    dtw_ix_up = (2 * dtw_y_limit - dtw_x_limit) / 3;
    dtw_ix_down = (4 * dtw_x_limit - 2 * dtw_y_limit) / 3;

    in1 = vct_in1->mfcc_frm[0].mfcc_dat;
    in2 = vct_in2->mfcc_frm[0].mfcc_dat;
    mdl = vct_mdl->mfcc_frm[0].mfcc_dat;
    dis = get_dis(in1, in2);
    get_mean(in1, in2, mdl);
    x = 1;
    y = 1;
    step = 1;
    
    do {
        up = (dtw_limit(x, y + 1) == DTW_INSIDE) ? get_dis(in2 + MFCC_ORDER_NUM, in1) : DTW_DIS_MAX;
        right = (dtw_limit(x + 1, y) == DTW_INSIDE) ? get_dis(in2, in1 + MFCC_ORDER_NUM) : DTW_DIS_MAX;
        right_up = (dtw_limit(x + 1, y + 1) == DTW_INSIDE) ? get_dis(in2 + MFCC_ORDER_NUM, in1 + MFCC_ORDER_NUM) : DTW_DIS_MAX;

        min = right_up;
        if (min > right) {
            min = right;
        }
        if (min > up) {
            min = up;
        }

        dis += min;

        if (min == right_up) {
            in1 += MFCC_ORDER_NUM;
            x++;
            in2 += MFCC_ORDER_NUM;
            y++;
        } else if (min == up) {
            in2 += MFCC_ORDER_NUM;
            y++;
        } else {
            in1 += MFCC_ORDER_NUM;
            x++;
        }
        step++;

        mdl += MFCC_ORDER_NUM;
        get_mean(in1, in2, mdl);
    } while ((x < in1_frm_num) && (y < in2_frm_num));
    vct_mdl->frm_num = step;
    
    return (dis / step); //步长归一化
}

