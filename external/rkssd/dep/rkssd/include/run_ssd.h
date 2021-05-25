#ifndef _RUN_SSD_H
#define _RUN_SSD_H

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SSD 检测
 *
 * */

class CRunSSD {
public:
	/*
	*  descption:
	*		创建SSD运行实例。
    *  params:
    *       inputSize: 输入图像大小
    *       channel： 图像通道
    *       numResult： 结果数量
    *       numClasses: SSD分类数
	*       graphRuntimePath : 本so路径，对应Android而已只需要填写so名称。
    *       paramPath： rkl 参数路径
    * */
	static CRunSSD *create(int inputSize, int channel, int numResult, int numClasses,
			std::string graphRuntimePath, std::string paramPath);

	CRunSSD();

	virtual ~CRunSSD();

	/*
	*  descption:
	*		SSD检测
    *  params:
    *       inData:			输入图像, 目前支持RGB/BGR格式
	*		predictions:	用于保存预测框位置(xmin, ymin, xmax, ymax)(需要后处理，具体参考相应的demo)
	*		outputClasses:  用于保存confidence, confidence还需要做expit处理((float) (1. / (1. + Math.exp(-x)));)
    * */
	virtual void run_ssd(char *inData, float *predictions, float* outputClasses) = 0;

	/*
	*  descption:
	*		SSD检测, 只适用于Android平台
    *  params:
    *       textureId:		输入图像纹理Id
	*		predictions:	用于保存预测框位置(xmin, ymin, xmax, ymax)(需要后处理，具体参考相应的demo)
	*		outputClasses:  用于保存confidence, confidence还需要做expit处理((float) (1. / (1. + Math.exp(-x)));)
    * */
	virtual void run_ssd(int textureId, float *predictions, float* outputClasses) = 0;
};

#ifdef __cplusplus
}
#endif

#endif
