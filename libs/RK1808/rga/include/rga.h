#ifndef _RGA_DRIVER_H_
#define _RGA_DRIVER_H_

#ifdef __cplusplus
extern "C"
{
#endif



#define RGA_BLIT_SYNC	0x5017
#define RGA_BLIT_ASYNC  0x5018
#define RGA_FLUSH       0x5019
#define RGA_GET_RESULT  0x501a
#define RGA_GET_VERSION 0x501b


#define RGA_REG_CTRL_LEN    0x8    /* 8  */
#define RGA_REG_CMD_LEN     0x1c   /* 28 */
#define RGA_CMD_BUF_SIZE    0x700  /* 16*28*4 */



#ifndef ENABLE
#define ENABLE 1
#endif


#ifndef DISABLE
#define DISABLE 0
#endif



/* RGA process mode enum */
enum
{    
    bitblt_mode               = 0x0,
    color_palette_mode        = 0x1,
    color_fill_mode           = 0x2,
    line_point_drawing_mode   = 0x3,
    blur_sharp_filter_mode    = 0x4,
    pre_scaling_mode          = 0x5,
    update_palette_table_mode = 0x6,
    update_patten_buff_mode   = 0x7,
};


enum
{
    rop_enable_mask          = 0x2,
    dither_enable_mask       = 0x8,
    fading_enable_mask       = 0x10,
    PD_enbale_mask           = 0x20,
};

enum
{
    yuv2rgb_mode0            = 0x0,     /* BT.601 MPEG */
    yuv2rgb_mode1            = 0x1,     /* BT.601 JPEG */
    yuv2rgb_mode2            = 0x2,     /* BT.709      */
};


/* RGA rotate mode */
enum 
{
    rotate_mode0             = 0x0,     /* no rotate */
    rotate_mode1             = 0x1,     /* rotate    */
    rotate_mode2             = 0x2,     /* x_mirror  */
    rotate_mode3             = 0x3,     /* y_mirror  */
};

enum
{
    color_palette_mode0      = 0x0,     /* 1K */
    color_palette_mode1      = 0x1,     /* 2K */
    color_palette_mode2      = 0x2,     /* 4K */
    color_palette_mode3      = 0x3,     /* 8K */
};

enum 
{
    BB_BYPASS   = 0x0,     /* no rotate */
    BB_ROTATE   = 0x1,     /* rotate    */
    BB_X_MIRROR = 0x2,     /* x_mirror  */
    BB_Y_MIRROR = 0x3      /* y_mirror  */
};

enum 
{
    nearby   = 0x0,     /* no rotate */
    bilinear = 0x1,     /* rotate    */
    bicubic  = 0x2,     /* x_mirror  */
};





/*
//          Alpha    Red     Green   Blue  
{  4, 32, {{32,24,   8, 0,  16, 8,  24,16 }}, GGL_RGBA },   // RK_FORMAT_RGBA_8888    
{  4, 24, {{ 0, 0,   8, 0,  16, 8,  24,16 }}, GGL_RGB  },   // RK_FORMAT_RGBX_8888    
{  3, 24, {{ 0, 0,   8, 0,  16, 8,  24,16 }}, GGL_RGB  },   // RK_FORMAT_RGB_888
{  4, 32, {{32,24,  24,16,  16, 8,   8, 0 }}, GGL_BGRA },   // RK_FORMAT_BGRA_8888
{  2, 16, {{ 0, 0,  16,11,  11, 5,   5, 0 }}, GGL_RGB  },   // RK_FORMAT_RGB_565        
{  2, 16, {{ 1, 0,  16,11,  11, 6,   6, 1 }}, GGL_RGBA },   // RK_FORMAT_RGBA_5551    
{  2, 16, {{ 4, 0,  16,12,  12, 8,   8, 4 }}, GGL_RGBA },   // RK_FORMAT_RGBA_4444
{  3, 24, {{ 0, 0,  24,16,  16, 8,   8, 0 }}, GGL_BGR  },   // RK_FORMAT_BGB_888

*/
    
typedef enum _Rga_SURF_FORMAT
{
	RK_FORMAT_RGBA_8888    = 0x0,
    RK_FORMAT_RGBX_8888    = 0x1,
    RK_FORMAT_RGB_888      = 0x2,
    RK_FORMAT_BGRA_8888    = 0x3,
    RK_FORMAT_BGRX_8888    = 0x4,
    RK_FORMAT_BGR_888      = 0x5,
    RK_FORMAT_RGB_565      = 0x6,
    RK_FORMAT_RGBA_5551    = 0x7,
    RK_FORMAT_RGBA_4444    = 0x8,
    RK_FORMAT_BGR_565      = 0x9,
    RK_FORMAT_BGRA_5551    = 0xa,
    RK_FORMAT_BGRA_4444    = 0xb,
    
    RK_FORMAT_Y4           = 0xe,
    RK_FORMAT_YCbCr_400    = 0xf,
    RK_FORMAT_YCbCr_422_SP = 0x10,
    RK_FORMAT_YCbCr_422_P  = 0x11,
    RK_FORMAT_YCbCr_420_SP = 0x12,
    RK_FORMAT_YCbCr_420_P  = 0x13,
    RK_FORMAT_YCrCb_422_SP = 0x14,
    RK_FORMAT_YCrCb_422_P  = 0x15,
    RK_FORMAT_YCrCb_420_SP = 0x16,
    RK_FORMAT_YCrCb_420_P  = 0x17,

    RK_FORMAT_YVYU_422 = 0x18,
    RK_FORMAT_YVYU_420 = 0x19,
    RK_FORMAT_VYUY_422 = 0x1a,
    RK_FORMAT_VYUY_420 = 0x1b,
    RK_FORMAT_YUYV_422 = 0x1c,
    RK_FORMAT_YUYV_420 = 0x1d,
    RK_FORMAT_UYVY_422 = 0x1e,
    RK_FORMAT_UYVY_420 = 0x1f,

    RK_FORMAT_YCbCr_422_10b_SP = 0x20,
    RK_FORMAT_YCbCr_420_10b_SP = 0x21,
    RK_FORMAT_YCrCb_422_10b_SP = 0x22,
    RK_FORMAT_YCrCb_420_10b_SP = 0x23,

	RK_FORMAT_UNKNOWN       = 0x100,
} RgaSURF_FORMAT;
    
typedef struct rga_img_info_t
{
#if defined(__arm64__) || defined(__aarch64__)
    unsigned long yrgb_addr;      /* yrgb    mem addr         */
    unsigned long uv_addr;        /* cb/cr   mem addr         */
    unsigned long v_addr;         /* cr      mem addr         */
#else
    unsigned int yrgb_addr;      /* yrgb    mem addr         */
    unsigned int uv_addr;        /* cb/cr   mem addr         */
    unsigned int v_addr;         /* cr      mem addr         */
#endif
    unsigned int format;         //definition by RK_FORMAT
    unsigned short act_w;
    unsigned short act_h;
    unsigned short x_offset;
    unsigned short y_offset;
    
    unsigned short vir_w;
    unsigned short vir_h;
    
    unsigned short endian_mode; //for BPP
    unsigned short alpha_swap;
}
rga_img_info_t;


typedef struct mdp_img_act
{
    unsigned short w;         // width
    unsigned short h;         // height
    short x_off;     // x offset for the vir
    short y_off;     // y offset for the vir
}
mdp_img_act;



typedef struct RANGE
{
    unsigned short min;
    unsigned short max;
}
RANGE;

typedef struct POINT_t
{
    unsigned short x;
    unsigned short y;
}
POINT_t;

typedef struct RECT_t
{
    unsigned short xmin;
    unsigned short xmax; // width - 1
    unsigned short ymin; 
    unsigned short ymax; // height - 1 
} RECT_t;

typedef struct RGT_t
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char res;
}RGB_t;


typedef struct MMU
{
    unsigned char mmu_en;
#if defined(__arm64__) || defined(__aarch64__)
    unsigned long base_addr;
#else
    unsigned int base_addr;
#endif
    unsigned int mmu_flag;     /* [0] mmu enable [1] src_flush [2] dst_flush [3] CMD_flush [4~5] page size*/
} MMU;




typedef struct COLOR_FILL
{
    short gr_x_a;
    short gr_y_a;
    short gr_x_b;
    short gr_y_b;
    short gr_x_g;
    short gr_y_g;
    short gr_x_r;
    short gr_y_r;

    //u8  cp_gr_saturation;
}
COLOR_FILL;

typedef struct FADING
{
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char res;
}
FADING;


typedef struct line_draw_t
{
    POINT_t start_point;              /* LineDraw_start_point                */
    POINT_t end_point;                /* LineDraw_end_point                  */
    unsigned int   color;               /* LineDraw_color                      */
    unsigned int   flag;                /* (enum) LineDrawing mode sel         */
    unsigned int   line_width;          /* range 1~16 */
}
line_draw_t;



 struct rga_req { 
    unsigned char render_mode;            /* (enum) process mode sel */
    
    rga_img_info_t src;                   /* src image info */
    rga_img_info_t dst;                   /* dst image info */
    rga_img_info_t pat;             /* patten image info */

#if defined(__arm64__) || defined(__aarch64__)
    unsigned long rop_mask_addr;         /* rop4 mask addr */
    unsigned long LUT_addr;              /* LUT addr */
#else
    unsigned int rop_mask_addr;         /* rop4 mask addr */
    unsigned int LUT_addr;              /* LUT addr */
#endif

    RECT_t clip;                      /* dst clip window default value is dst_vir */
                                    /* value from [0, w-1] / [0, h-1]*/
        
    int sina;                   /* dst angle  default value 0  16.16 scan from table */
    int cosa;                   /* dst angle  default value 0  16.16 scan from table */        

    unsigned short alpha_rop_flag;        /* alpha rop process flag           */
                                    /* ([0] = 1 alpha_rop_enable)       */
                                    /* ([1] = 1 rop enable)             */                                                                                                                
                                    /* ([2] = 1 fading_enable)          */
                                    /* ([3] = 1 PD_enable)              */
                                    /* ([4] = 1 alpha cal_mode_sel)     */
                                    /* ([5] = 1 dither_enable)          */
                                    /* ([6] = 1 gradient fill mode sel) */
                                    /* ([7] = 1 AA_enable)              */

    unsigned char  scale_mode;            /* 0 nearst / 1 bilnear / 2 bicubic */                             
                            
    unsigned int color_key_max;         /* color key max */
    unsigned int color_key_min;         /* color key min */     

    unsigned int fg_color;              /* foreground color */
    unsigned int bg_color;              /* background color */

    COLOR_FILL gr_color;            /* color fill use gradient */
    
    line_draw_t line_draw_info;
    
    FADING fading;
                              
    unsigned char PD_mode;                /* porter duff alpha mode sel */
    
    unsigned char alpha_global_value;     /* global alpha value */
     
    unsigned short rop_code;              /* rop2/3/4 code  scan from rop code table*/
    
    unsigned char bsfilter_flag;          /* [2] 0 blur 1 sharp / [1:0] filter_type*/
    
    unsigned char palette_mode;           /* (enum) color palatte  0/1bpp, 1/2bpp 2/4bpp 3/8bpp*/

    unsigned char yuv2rgb_mode;           /* (enum) BT.601 MPEG / BT.601 JPEG / BT.709  */ 
    
    unsigned char endian_mode;            /* 0/big endian 1/little endian*/

    unsigned char rotate_mode;            /* (enum) rotate mode  */
                                    /* 0x0,     no rotate  */
                                    /* 0x1,     rotate     */
                                    /* 0x2,     x_mirror   */
                                    /* 0x3,     y_mirror   */

    unsigned char color_fill_mode;        /* 0 solid color / 1 patten color */
                                    
    MMU mmu_info;                   /* mmu information */

    unsigned char  alpha_rop_mode;        /* ([0~1] alpha mode)       */
                                    /* ([2~3] rop   mode)       */
                                    /* ([4]   zero  mode en)    */
                                    /* ([5]   dst   alpha mode) */

    unsigned char  src_trans_mode;

	unsigned char  dither_mode;

    unsigned char CMD_fin_int_enable;                        

    /* completion is reported through a callback */
	void (*complete)(int retval);
};

#if 0    
typedef struct TILE_INFO
{
    int64_t matrix[4];
    
    uint16_t tile_x_num;     /* x axis tile num / tile size is 8x8 pixel */
    uint16_t tile_y_num;     /* y axis tile num */

    int16_t dst_x_tmp;      /* dst pos x = (xstart - xoff) default value 0 */
    int16_t dst_y_tmp;      /* dst pos y = (ystart - yoff) default value 0 */

    uint16_t tile_w;
    uint16_t tile_h;
    int16_t tile_start_x_coor;
    int16_t tile_start_y_coor;
    int32_t tile_xoff;
    int32_t tile_yoff;

    int32_t tile_temp_xstart;
    int32_t tile_temp_ystart;

    /* src tile incr */
    int32_t x_dx;
    int32_t x_dy;
    int32_t y_dx;
    int32_t y_dy;

    mdp_img_act dst_ctrl;
    
}
TILE_INFO;	
#endif

#if 0

#define RGA_BASE                 0x10114000

//General Registers
#define RGA_SYS_CTRL             0x000
#define RGA_CMD_CTRL             0x004
#define RGA_CMD_ADDR             0x008
#define RGA_STATUS               0x00c
#define RGA_INT                  0x010
#define RGA_AXI_ID               0x014
#define RGA_MMU_STA_CTRL         0x018
#define RGA_MMU_STA              0x01c

//Command code start
#define RGA_MODE_CTRL            0x100

//Source Image Registers
#define RGA_SRC_Y_MST            0x104
#define RGA_SRC_CB_MST           0x108
#define RGA_MASK_READ_MST        0x108  //repeat
#define RGA_SRC_CR_MST           0x10c
#define RGA_SRC_VIR_INFO         0x110
#define RGA_SRC_ACT_INFO         0x114
#define RGA_SRC_X_PARA           0x118
#define RGA_SRC_Y_PARA           0x11c
#define RGA_SRC_TILE_XINFO       0x120
#define RGA_SRC_TILE_YINFO       0x124
#define RGA_SRC_TILE_H_INCR      0x128
#define RGA_SRC_TILE_V_INCR      0x12c
#define RGA_SRC_TILE_OFFSETX     0x130
#define RGA_SRC_TILE_OFFSETY     0x134
#define RGA_SRC_BG_COLOR         0x138
#define RGA_SRC_FG_COLOR         0x13c
#define RGA_LINE_DRAWING_COLOR   0x13c  //repeat
#define RGA_SRC_TR_COLOR0        0x140
#define RGA_CP_GR_A              0x140  //repeat
#define RGA_SRC_TR_COLOR1        0x144
#define RGA_CP_GR_B              0x144  //repeat

#define RGA_LINE_DRAW            0x148
#define RGA_PAT_START_POINT      0x148  //repeat

//Destination Image Registers
#define RGA_DST_MST              0x14c
#define RGA_LUT_MST              0x14c  //repeat
#define RGA_PAT_MST              0x14c  //repeat
#define RGA_LINE_DRAWING_MST     0x14c  //repeat

#define RGA_DST_VIR_INFO         0x150

#define RGA_DST_CTR_INFO         0x154
#define RGA_LINE_DRAW_XY_INFO    0x154  //repeat 

//Alpha/ROP Registers
#define RGA_ALPHA_CON            0x158

#define RGA_PAT_CON              0x15c
#define RGA_DST_VIR_WIDTH_PIX    0x15c  //repeat

#define RGA_ROP_CON0             0x160
#define RGA_CP_GR_G              0x160  //repeat
#define RGA_PRESCL_CB_MST        0x160  //repeat

#define RGA_ROP_CON1             0x164
#define RGA_CP_GR_R              0x164  //repeat
#define RGA_PRESCL_CR_MST        0x164  //repeat

//MMU Register
#define RGA_FADING_CON           0x168
#define RGA_MMU_CTRL             0x168  //repeat

#define RGA_MMU_TBL              0x16c  //repeat


#define RGA_BLIT_COMPLETE_EVENT 1

#endif

int
RGA_set_src_act_info(
		struct rga_req *req,
		unsigned int   width,       /* act width  */
		unsigned int   height,      /* act height */
		unsigned int   x_off,       /* x_off      */
		unsigned int   y_off        /* y_off      */
		);

#if defined(__arm64__) || defined(__aarch64__)
int
RGA_set_src_vir_info(
		struct rga_req *req,
		unsigned long   yrgb_addr,       /* yrgb_addr  */
		unsigned long   uv_addr,         /* uv_addr    */
		unsigned long   v_addr,          /* v_addr     */
		unsigned int   vir_w,           /* vir width  */
		unsigned int   vir_h,           /* vir height */
		unsigned char   format,          /* format     */
		unsigned char  a_swap_en        /* only for 32bit RGB888 format */
		);
#else
int
RGA_set_src_vir_info(
		struct rga_req *req,
		unsigned int   yrgb_addr,       /* yrgb_addr  */
		unsigned int   uv_addr,         /* uv_addr    */
		unsigned int   v_addr,          /* v_addr     */
		unsigned int   vir_w,           /* vir width  */
		unsigned int   vir_h,           /* vir height */
		unsigned char  format,          /* format     */
		unsigned char  a_swap_en        /* only for 32bit RGB888 format */
		);
#endif

int
RGA_set_dst_act_info(
		struct rga_req *req,
		unsigned int   width,       /* act width  */
		unsigned int   height,      /* act height */
		unsigned int   x_off,       /* x_off      */
		unsigned int   y_off        /* y_off      */
		);

#if defined(__arm64__) || defined(__aarch64__)
int
RGA_set_dst_vir_info(
		struct rga_req *msg,
		unsigned long   yrgb_addr,   /* yrgb_addr   */
		unsigned long   uv_addr,     /* uv_addr     */
		unsigned long   v_addr,      /* v_addr      */
		unsigned int   vir_w,       /* vir width   */
		unsigned int   vir_h,       /* vir height  */
		RECT_t           *clip,        /* clip window */
		unsigned char  format,      /* format      */
		unsigned char  a_swap_en
		);
#else
int
RGA_set_dst_vir_info(
		struct rga_req *msg,
		unsigned int   yrgb_addr,   /* yrgb_addr   */
		unsigned int   uv_addr,     /* uv_addr     */
		unsigned int   v_addr,      /* v_addr      */
		unsigned int   vir_w,       /* vir width   */
		unsigned int   vir_h,       /* vir height  */
		RECT_t           *clip,        /* clip window */
		unsigned char  format,      /* format      */
		unsigned char  a_swap_en
		);
#endif

int 
RGA_set_pat_info(
    struct rga_req *msg,
    unsigned int width,
    unsigned int height,
    unsigned int x_off,
    unsigned int y_off,
    unsigned int pat_format    
    );

#if defined(__arm64__) || defined(__aarch64__)
int
RGA_set_rop_mask_info(
		struct rga_req *msg,
		unsigned long rop_mask_addr,
		unsigned int rop_mask_endian_mode
		);
#else
int
RGA_set_rop_mask_info(
		struct rga_req *msg,
		unsigned int rop_mask_addr,
		unsigned int rop_mask_endian_mode
		);
#endif
   
int RGA_set_alpha_en_info(
		struct rga_req *msg,
		unsigned int  alpha_cal_mode,    /* 0:alpha' = alpha + (alpha>>7) | alpha' = alpha */
		unsigned int  alpha_mode,        /* 0 global alpha / 1 per pixel alpha / 2 mix mode */
		unsigned int  global_a_value,
		unsigned int  PD_en,             /* porter duff alpha mode en */ 
		unsigned int  PD_mode, 
		unsigned int  dst_alpha_en );    /* use dst alpha  */

int
RGA_set_rop_en_info(
		struct rga_req *msg,
		unsigned int ROP_mode,
		unsigned int ROP_code,
		unsigned int color_mode,
		unsigned int solid_color
		);

 
int
RGA_set_fading_en_info(
		struct rga_req *msg,
		unsigned char r,
		unsigned char g,
		unsigned char b
		);

int
RGA_set_src_trans_mode_info(
		struct rga_req *msg,
		unsigned char trans_mode,
		unsigned char a_en,
		unsigned char b_en,
		unsigned char g_en,
		unsigned char r_en,
		unsigned char color_key_min,
		unsigned char color_key_max,
		unsigned char zero_mode_en
		);


int
RGA_set_bitblt_mode(
		struct rga_req *msg,
		unsigned char scale_mode,    // 0/near  1/bilnear  2/bicubic  
		unsigned char rotate_mode,   // 0/copy 1/rotate_scale 2/x_mirror 3/y_mirror 
		unsigned int  angle,         // rotate angle     
		unsigned int  dither_en,     // dither en flag   
		unsigned int  AA_en,         // AA flag          
		unsigned int  yuv2rgb_mode
		);


int
RGA_set_color_palette_mode(
		struct rga_req *msg,
		unsigned char  palette_mode,        /* 1bpp/2bpp/4bpp/8bpp */
		unsigned char  endian_mode,         /* src endian mode sel */
		unsigned int  bpp1_0_color,         /* BPP1 = 0 */
		unsigned int  bpp1_1_color          /* BPP1 = 1 */
		);


int
RGA_set_color_fill_mode(
		struct rga_req *msg,
		COLOR_FILL  *gr_color,                    /* gradient color part         */
		unsigned char  gr_satur_mode,            /* saturation mode             */
    	unsigned char  cf_mode,                  /* patten fill or solid fill   */
		unsigned int color,                      /* solid color                 */
		unsigned short pat_width,                /* pattern width               */
		unsigned short pat_height,               /* pattern height              */   
		unsigned char pat_x_off,                 /* pattern x offset            */
		unsigned char pat_y_off,                 /* pattern y offset            */
		unsigned char aa_en                      /* alpha en                    */
		);


int
RGA_set_line_point_drawing_mode(
		struct rga_req *msg,
		POINT_t sp,                     /* start point              */
		POINT_t ep,                     /* end   point              */
		unsigned int color,           /* line point drawing color */
		unsigned int line_width,      /* line width               */
		unsigned char AA_en,          /* AA en                    */
		unsigned char last_point_en   /* last point en            */
		);


int
RGA_set_blur_sharp_filter_mode(
		struct rga_req *msg,
		unsigned char filter_mode,   /* blur/sharpness   */
		unsigned char filter_type,   /* filter intensity */
		unsigned char dither_en      /* dither_en flag   */
		);

int
RGA_set_pre_scaling_mode(
		struct rga_req *msg,
		unsigned char dither_en
		);

#if defined(__arm64__) || defined(__aarch64__)
int
RGA_update_palette_table_mode(
		struct rga_req *msg,
		unsigned long LUT_addr,      /* LUT table addr      */
		unsigned int palette_mode   /* 1bpp/2bpp/4bpp/8bpp */
		);
#else
int
RGA_update_palette_table_mode(
		struct rga_req *msg,
		unsigned int LUT_addr,      /* LUT table addr      */
		unsigned int palette_mode   /* 1bpp/2bpp/4bpp/8bpp */
		);
#endif

int
RGA_set_update_patten_buff_mode(
		struct rga_req *msg,
		unsigned int pat_addr, /* patten addr    */
		unsigned int w,        /* patten width   */
		unsigned int h,        /* patten height  */
		unsigned int format    /* patten format  */
		);

#if defined(__arm64__) || defined(__aarch64__)
int
RGA_set_mmu_info(
		struct rga_req *msg,
		unsigned char  mmu_en,
		unsigned char  src_flush,
		unsigned char  dst_flush,
		unsigned char  cmd_flush,
		unsigned long base_addr,
		unsigned char  page_size
		);
#else
int
RGA_set_mmu_info(
		struct rga_req *msg,
		unsigned char  mmu_en,
		unsigned char  src_flush,
		unsigned char  dst_flush,
		unsigned char  cmd_flush,
		unsigned int base_addr,
		unsigned char  page_size
		);
#endif

void rga_set_fds_offsets(
        struct rga_req *rga_request,
        unsigned short src_fd,
        unsigned short dst_fd,
        unsigned int src_offset, 
        unsigned int dst_offset);

int
RGA_set_src_fence_flag(
    struct rga_req *msg,
    int acq_fence,
    int src_flag
);


int
RGA_set_dst_fence_flag(
    struct rga_req *msg,
    int dst_flag
);

int
RGA_get_dst_fence(
    struct rga_req *msg
);
#ifdef __cplusplus
}
#endif

#endif /*_RK29_IPP_DRIVER_H_*/
