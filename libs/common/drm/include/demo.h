/*

 */

#ifndef DEMO_H_
#define DEMO_H_
//在此处包含其它头文件
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "math.h"

#include "demo_define.h"
#include "tool.h"
#include "inital_alg_params_ynr.h"
#include "inital_alg_params_gic.h"
#include "inital_alg_params_lsc.h"
#include "inital_alg_params_lsc2.h"
#include "inital_alg_params_rk_shapren_HW.h"
#include "inital_alg_params_rk_edgefilter.h"

#include "initial_alg_params_bayernr.h"

#include "inital_alg_params_rkuvnr.h"
#include "inital_alg_params_rk_cnr.h"

#include "inital_alg_params_mfnr.h"
#include "rk_aiq_awb_algo_v200.h"
#define         FILE_RAW_EXT         ".raw"
#define         FILE_YUV_EXT         ".yuv"
#define         FILE_DAT_EXT         ".dat"

typedef enum YUV_FILE_FMT
{
    F_YUV_420SP        = 0x00,
    F_YUV_420P         = 0x01,
    F_YUV_422I         = 0x02,
    F_YUV_422SP        = 0x03,
    F_YUV_422P         = 0x04,
    F_YUV_444I         = 0x05,

    F_YUV_MAX          = 0x10,
}YUV_FILE_FMT_t;

typedef enum INPUT_FILE_FMT
{
    F_IN_FMT_RAW         = 0x00,
    F_IN_FMT_YUV,


    F_IN_FMT_MAX         = 0x10,
}INPUT_FILE_FMT_t;



//此处定义参数
typedef struct tag_config_com
{
    int exp_info_en    ;
    int framenum    ;
    int rawwid      ;
    int rawhgt      ;
    int rawbit      ;
    int bayerfmt    ;
    int yuvbit      ;
    int yuvfmt      ;
}tag_config_com;

typedef struct tag_config_txt
{
    tag_config_com config_com;

    int framecnt    ;
    int iso         ;
    int exptime[3]  ;
    int expgain[3]  ;
    int rgain       ;
    int bgain       ;
    int grgain      ;
    int gbgain      ;
    int dGain       ;
    int lux         ;
}tag_config_txt;

typedef struct tag_ST_DEMO_INPUT_PARAMS
{
	int width;        //raw图宽
	int height;       //raw图高
	int bayerPattern; //bayer pattern格式:0--BGGR,1--GBRG,2--GRBG,3--RGGB
	int yuvFmt;       //yuv file     格式: YUV_FILE_FMT_t
	int bitValue;     //raw数据位宽
	int hdr_framenum;
	float expGain[MAX_HDR_FRM_NUM];          //
	float expTime[MAX_HDR_FRM_NUM];      //曝光时间
	int rGain;        //wb rgain
	int bGain;        //wb bgain
	int grGain;        //wb grgain
	int gbGain;        //wb gbgain
	int dGain;        //wb gbgain
    int fileFmt;      //input file format:INPUT_FILE_FMT_t
	int width_full;        //raw图宽
	int height_full;       //raw图高
	int crop_width;
	int crop_height;
	int crop_xoffset;
	int crop_yoffset;

	char pathFileCfg[256];//config文件路径
	char pathRawData[256];//raw图路径
	char nameRawData[256];//raw图名称
	char pathExpInfo[256];//exp_info文件路径
	char pathReslut[256];//结果保存文件夹路径
	char suffix[256];       // 输出文件后缀字符
    char pathRtlin[256];    //rtl in path
	int skip_num;
	int frame_end;

	int hdr_proc_mode;
	int out_mode;


    char  dbgFlg[1024];        // must > ISP_CAP_MAX
    int  config_full;

    int exp_info_en;
    int file_info_en;
    FILE *fp_exp_info;
}ST_DEMO_INPUT_PARAMS;



//此处声明函数



#endif  // DEMO_H_
