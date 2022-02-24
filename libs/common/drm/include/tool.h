/*

 */

#ifndef TOOL_H_
#define TOOL_H_
//在此处包含其它头文件
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif
#define YUV420	0
#define YUV422 	YUV420 	+ 1
#define YUV444 	YUV422 	+ 1
/*************************************************************
Function:       ReadBmpFile
Description:    读取bmp图像到内存
Input:          pFilePath：bmp保存路径
                pData：rgb图像数据内存指针，rgb图像数据排练顺序为bgrbgr...bgr；rgb值必须为8bit
				width：图像宽
				height：图像高
Output:         无
*************************************************************/
extern void ReadBmpFile(char *pFilePath, unsigned char *pData, int & width, int & height);
/*************************************************************
Function:       SaveBmpFile
Description:    把rgb图像数据保存为bmp
Input:          pFilePath：bmp保存路径
                pData：rgb图像数据内存指针，rgb图像数据排练顺序为bgrbgr...bgr；rgb值必须为8bit
				width：图像宽
				height：图像高
Output:         无
*************************************************************/
extern void SaveBmpFile(char *pFilePath, unsigned char *pData, int width, int height);
/*************************************************************
Function:       SaveRaw
Description:    保存raw图
Input:          pSavePath：raw保存路径
                pData：raw数据
				width：raw图像宽
				height：raw图像高
Output:         无
*************************************************************/
extern void SaveRaw(char *pSavePath, short *pRawData, int width, int height);

extern void SaveRaw32bit(char *pSavePath, long *pRawData, int width, int height);

/*************************************************************
Function:       SaveBmpFile2
Description:    保存数据位宽大于8bit的bmp图像
Input:          pFilePath：bmp保存路径
				width：图像宽
				height：图像高
				bitValue：图像数据位宽
				pRGBData：rgb图像数据内存指针，rgb图像数据排练顺序为bgrbgr...bgr
Output:         无
*************************************************************/
extern void SaveBmpFile2(char *pFilePath, int width, int height, int bitValue, short *pRGBData);

/*************************************************************
Function:       SaveYUVData
Description:    保存8bit YUV图
Input:          pSavePath：保存路径
                pData：yuv数据，8bit，排列顺序yyy...yyyuuu...uuuvvv...vvv
				width：图像宽
				height：图像高
Output:         无
*************************************************************/
extern void SaveYUVData(char *pSavePath, unsigned char *pData, int width, int height);



/*************************************************************
Function:       SaveYUVData2
Description:    保存数据位宽大于8bit的YUV图
Input:          pSavePath：保存路径
                pData：yuv数据，数据位宽大于8bit，排列顺序yyy...yyyuuu...uuuvvv...vvv
				width：图像宽
				height：图像高
Output:         无
*************************************************************/
extern void SaveYUVData2(char *pSavePath, short *pData, int width, int height, int bitValue);
/*************************************************************
Function:       SaveYUVData1
Description:    保存8bit YUV420图
Input:          pSavePath：保存路径
                pData：yuv数据，8bit，排列顺序yyy...yyyuuu...uuuvvv...vvv
				width：图像宽
				height：图像高
Output:         无
*************************************************************/
extern void SaveYUVData1(char *pSavePath, unsigned char *pData, int width, int height, int fmt);
/*************************************************************
Function:       ReadYUVData1
Description:    读取8bit YUV420图
Input:          pReadPath：保存路径
                pData：yuv数据，8bit，排列顺序yyy...yyyuuu...uuuvvv...vvv
				width：图像宽
				height：图像高
Output:         无
*************************************************************/
extern void ReadYUVData1(char *pReadPath, unsigned char *pData, int width, int height, int fmt);
/*************************************************************
Function:     Yuvfmtconv
Description:    yuv fmt conversion.444 420 422 to 444 420 422
Input:   	pDatain 输入缓存
		pDataout 输出缓存
		width 宽
		height 高
		fmt_in 输入格式
		fmt_out 输出格式
Output:	      无
*************************************************************/
extern void Yuvfmtconv(void *pDatain, void *pDataout, int width, int height, int fmt_in, int fmt_out, int size);
/*************************************************************
Function:     Yuvbitstochar
Description:    save yuv to 8 bitdepth
Input:   	pDatain 输入缓存
		pDataout 输出缓存
		size yuv总数
		height 输入位宽
Output:	      无
*************************************************************/
extern void Yuvbitstochar(short *pDatain, unsigned char *pDataout, int size,  int bitdepth);

/*************************************************************
Function:       SaveCfaBmp
Description:    将raw保存成cfa图像
Input:          pRawData：输入的raw图；
                width：raw图宽；
				height：raw图高；
				bayerPattern：bayer pattern格式，取值范围[0，3]；
				bitValue：raw数据位宽；
Output:         无
*************************************************************/
extern void SaveCfaBmp(char *pFilePath, short *pRawData, int width, int height, int bayerPattern, int bitValue);

#ifdef __cplusplus
}
#endif

#endif  // TOOL_H_
