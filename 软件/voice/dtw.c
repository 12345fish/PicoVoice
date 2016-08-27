/*
    DTW�㷨 ͨ���ֲ��Ż��ķ���ʵ�ּ�Ȩ�����ܺ���С

    ʱ�����������
    C={c(1),c(2),��,c(N)}
    NΪ·�����ȣ�c(n)=(i(n),j(n))��ʾ��n��ƥ������вο�ģ���
    ��i(n)������ʸ�������ģ��ĵ�j(n)������ʸ�����ɵ�ƥ���ԣ���
    ��֮��ľ���d(x(i(n)),y(j(n)))��Ϊƥ����롣
    ʱ�������������һ��Լ����
    1.�����ԣ����������������ӡ�
    2.����յ�Լ����������㣬�յ���յ㡣
    3.�����ԣ������������κ�һ�㡣
    4.��������������ĳһ����ֵ��|i(n)-j(n)|<M,MΪ��������
    ��������������λ��ƽ���ı����ڡ��ֲ�·��Լ�����������Ƶ���n��
    Ϊ(i(n),j(n))ʱ��ǰ�������ڼ��ֿ��ܵ�·����

    DTW���裺
    1.��ʼ������i(0)=j(0)=0,i(N)=in_frm_num,j(N)=mdl_frm_num.
    ȷ��һ��ƽ���ı��Σ�������λ��(0,0)��(in_frm_num,mdl_frm_num)�Ķ��㣬����б��б
    �ʷֱ���2��1/2.�����������ɳ�����ƽ���ı��Ρ�
    2.�������ۼƾ��롣

    ����������������ģ���֡��������ֱ�ӽ�ƥ�������Ϊ���
    frm_in_num<(frm_mdl_num/2)||frm_in_num>(frm_mdl_num*2)
*/
#include "base.h"
//#include <math.h>
#include "utils.h"
#include "mfcc.h"
#include "dtw.h"

/*
    ��ȡ��������ʸ��֮��ľ���
    ����
    frm_ftr1    ����ʸ��1
    frm_ftr2    ����ʸ��2
    ����ֵ
    dis         ʸ������
*/
u32 get_dis(s16 *frm_ftr1, s16 *frm_ftr2)
{
    u32 i;
    u32 dis_sqr;
    u32 dis;
    s32 dif;    //��ʸ����ͬά���ϵĲ�ֵ

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

//ƽ���ı������������� X����ֵ
static u16 dtw_ix_up;       //�ϱ߽���
static u16 dtw_ix_down;     //�±߽���
static u16 dtw_x_limit;     //X���ֵ
static u16 dtw_y_limit;     //Y���ֵ

/*
    ��Χ����
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
    DTW ��̬ʱ�����
    ����
    vct_in  :��������ֵ
    vct_mdl :����ģ��
    ����ֵ
    dis     :�ۼ�ƥ�����
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
    u16 in_frm_num;     //��������֡��
    u16 mdl_frm_num;    //����ģ��֡��
    
    if ((NULL == vct_in) || (NULL == vct_mdl)) {
        return (s32)DTW_DIS_ERR;
    }

    in_frm_num = vct_in->frm_num;
    mdl_frm_num = vct_mdl->frm_num;

    if ((in_frm_num > (mdl_frm_num * 2)) || ((2 * in_frm_num) < mdl_frm_num)) {
        return (s32)DTW_DIS_ERR;
    }
    
    // ����Լ��ƽ���ı��ζ���ֵ
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

    return (s32)(dis / step); //������һ��
}

/*
    ��������ʸ����ȡ����ģ��
    ����
    ftr_in1 :��������ֵ
    ftr_in2 :��������ֵ
    ftr_mdl :����ģ��
    ����ֵ
    dis     :�ۼ�ƥ�����
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

    // ����Լ��ƽ���ı��ζ���ֵ
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
    
    return (dis / step); //������һ��
}

