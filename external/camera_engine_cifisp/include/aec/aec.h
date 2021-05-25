/******************************************************************************
 *
 * Copyright 2016, Fuzhou Rockchip Electronics Co.Ltd . All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Fuzhou Rockchip Electronics Co.Ltd .
 *
 *
 *****************************************************************************/
#ifndef __AEC_H__
#define __AEC_H__

/**
 * @file aec.h
 *
 * @brief
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup AECM Auto white Balance Module
 * @{
 *
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>
#include <common/return_codes.h>
#include <common/misc.h>
#include <common/cam_types.h>
#include "awb/awb.h"
/*
 ***************** AE LIB VERSION NOTE *****************
 * v0.1.0
 *  - support Linear-AE/Hdr-AE
 * v0.1.1
 *  - modify ExposureSeperate for different Hdr-Mode
 */

#define CONFIG_AE_LIB_VERSION "v0.1.1"
#ifdef __cplusplus
extern "C"
{
#endif

#define AEC_AFPS_MASK (1 << 0)
#define AEC_EFFECT_FNUM 5
//#define ZLJ_DEBUG
/*****************************************************************************/
/**
 * @brief   The number of mean and hist.
 */
/*****************************************************************************/
#define CIFISP_AE_MEAN_MAX 25
#define CIFISP_HIST_BIN_N_MAX 16
/*zlj add*/
#define LockAE_NO	(3)
#define CIFISP_HDRAE_LOGHISTBIN_NUM 100
#define CIFISP_HDRAE_HISTBIN_NUM (1<<8)
#define CIFISP_HDRAE_GRIDITEMS (15*15)
//#define CIFISP_HDRAE_SFREGION0_NUM (15*5)
//#define CIFISP_HDRAE_SFREGION1_NUM (10*3)
//#define CIFISP_HDRAE_SFREGION2_NUM (10*3)
//#define CIFISP_HDRAE_SFREGION3_NUM (10*9)

typedef struct AecContext_s* AeHandle_t;     /**< handle to AEC context */

typedef enum AecMeasuringMode_e {
  AEC_MEASURING_MODE_INVALID    = 0,    /**< invalid histogram measuring mode   */
  AEC_MEASURING_MODE_1          = 1,    /**< Y = (R + G + B) x (85/256)         */
  AEC_MEASURING_MODE_2          = 2,    /**< Y = 16 + 0.25R + 0.5G + 0.1094B    */
  AEC_MEASURING_MODE_MAX,
} AecMeasuringMode_t;

/*****************************************************************************/
/**
 *          AecDampingMode_t
 *
 * @brief   mode type of AEC Damping
 *
 */
/*****************************************************************************/
typedef enum AecDampingMode_e {
  AEC_DAMPING_MODE_INVALID        = 0,        /* invalid (only used for initialization) */
  AEC_DAMPING_MODE_STILL_IMAGE    = 1,        /* damping mode still image */
  AEC_DAMPING_MODE_VIDEO          = 2,        /* damping mode video */
  AEC_DAMPING_MODE_MAX
} AecDampingMode_t;


/*****************************************************************************/
/**
 *          AecEcmMode_t
 *
 * @brief   mode type of AEC Exposure Conversion
 *
 */
/*****************************************************************************/
typedef enum AecEcmMode_e {
  AEC_EXPOSURE_CONVERSION_INVALID = 0,        /* invalid (only used for initialization) */
  AEC_EXPOSURE_CONVERSION_LINEAR  = 1,         /* Exposure Conversion uses a linear function (eq. 38) */
  AEC_EXPOSURE_CONVERSION_MAX
} AecEcmMode_t;


/*****************************************************************************/
/**
 *          AecEcmFlickerPeriod_t
 *
 * @brief   flicker period types for the AEC algorithm
 *
 */
/*****************************************************************************/
typedef enum AecEcmFlickerPeriod_e {
  AEC_EXPOSURE_CONVERSION_FLICKER_OFF   = 0x00,
  AEC_EXPOSURE_CONVERSION_FLICKER_100HZ = 0x01,
  AEC_EXPOSURE_CONVERSION_FLICKER_120HZ = 0x02
} AecEcmFlickerPeriod_t;

typedef struct Aec_daynight_th_s {
  float         fac_th;
  uint8_t         holdon_times_th;
} Aec_daynight_th_t;

typedef struct AecInterAdjust_s{
	uint8_t enable;
	float	dluma_high_th;
	float	dluma_low_th;
	uint32_t  trigger_frame;
}AecInterAdjust_t;

/*****************************************************************************/
/**
 *          AeInstanceConfig_t
 *
 * @brief   AF Module instance configuration structure
 *
 *****************************************************************************/
typedef struct AeInstanceConfig_s {
  AeHandle_t              hAe;            /**< handle returned by AeInit() */
} AeInstanceConfig_t;

/*****************************************************************************/
/**
 *          AecConfig_t
 *
 * @brief   AEC Module configuration structure; used for re-configuration as well
 *
 *****************************************************************************/
typedef struct AecConfig_s {

  Cam5x5UCharMatrix_t         GridWeights;
  Cam5x5UCharMatrix_t         NightGridWeights;
  CamerIcIspHistMode_t        HistMode;
  AecMeasuringMode_t          meas_mode;

  float                       SetPoint;                   /**< set point to hit by the ae control system */
  float                       ClmTolerance;
  float                       DampOverStill;              /**< damping coefficient for still image mode */
  float                       DampUnderStill;             /**< damping coefficient for still image mode */
  float                       DampOverVideo;              /**< damping coefficient for video mode */
  float                       DampUnderVideo;             /**< damping coefficient for video mode */
  Cam6x1FloatMatrix_t         EcmTimeDot;
  Cam6x1FloatMatrix_t         EcmGainDot;
  Cam6x1FloatMatrix_t         FpsFixTimeDot;
  uint8_t                     isFpsFix;
  uint8_t                     FpsSetEnable;
  AecDampingMode_t            DampingMode;              /**< damping mode */
  AecSemMode_t                SemMode;                  /**< scene evaluation mode */
  AecEcmFlickerPeriod_t       EcmFlickerSelect;         /**< flicker period selection */
  uint8_t                     StepSize;
  float                       GainFactor;
  float                       GainBias;
  float                       LinePeriodsPerField;
  float                       PixelClockFreqMHZ;
  float                       PixelPeriodsPerLine;
  float                       ApiSetFps;
  /* gain range */

  uint32_t            GainRange_size;
  float               *pGainRange;
  float               TimeFactor[4];

  float         AOE_Enable;
  float         AOE_Max_point;
  float         AOE_Min_point;
  float         AOE_Y_Max_th;
  float         AOE_Y_Min_th;
  float         AOE_Step_Inc;
  float         AOE_Step_Dec;

  uint8_t       DON_Night_Trigger;
  uint8_t       DON_Night_Mode;
  float         DON_Day2Night_Fac_th; // yamasaki
  float         DON_Night2Day_Fac_th; // yamasaki
  uint8_t       DON_Bouncing_th;

  //zlj add for LockAE
  uint8_t             LockAE_enable;
  Cam3x1FloatMatrix_t GainValue;
  Cam3x1FloatMatrix_t TimeValue;

  AecInterAdjust_t IntervalAdjStgy;

  CamCalibAecExpSeparate_t *pExpSeparate[LIGHT_MODE_MAX];
  CamCalibAecDynamicSetpoint_t *pDySetpoint[LIGHT_MODE_MAX];
  CamCalibAecNLSC_t NLSC_config;
  CamCalibAecBacklight_t backLightConf;
  CamCalibAecHist2Hal_t hist2Hal;
  CamCalibAecHdrCtrl_t HdrCtrl;

  enum LIGHT_MODE LightMode;

  int ae_bias;
  float ApiSetMaxExpTime;
  float ApiSetMaxGain;

} AecConfig_t;

/*****************************************************************************/
/**
 *          Aec_stat_t
 *
 * @brief   AEC Module Hardware statistics structure
 *
 *****************************************************************************/

/*zlj add*/
typedef struct Hdrae_DRIndex_res
{
  uint32_t NormalIndex;
  uint32_t LongIndex;
} Hdrae_DRIndex_res_t;
typedef struct Hdrae_OE_meas_res
{
  uint32_t OE_Pixel;
  uint32_t SumHistPixel;
  uint32_t SframeMaxLuma;
  //struct Hdrae_sframe_regionmatrix_s SfRegionMatrix;
} Hdrae_OE_meas_res_t;

typedef struct Hdrae_stat_s
{
  unsigned int hdr_hist_bins[CIFISP_HDRAE_HISTBIN_NUM];
  unsigned short hdr_exp_mean[CIFISP_HDRAE_GRIDITEMS];
} Hdrae_stat_t;

typedef struct Hdr_sensor_metadata_s {
  unsigned int exp_time_l;
  unsigned int exp_time;
  unsigned int exp_time_s;
  unsigned int gain_l;
  unsigned int gain;
  unsigned int gain_s;
} Hdr_sensor_metadata_t;

typedef struct Hdr_sensor_expdata_s {
  float exp_time_l;
  float exp_time;
  float exp_time_s;
  float gain_l;
  float gain;
  float gain_s;
} Hdr_sensor_expdata_t;

typedef struct AecStat_s {
  unsigned char  exp_mean[CIFISP_AE_MEAN_MAX];
  unsigned int hist_bins[CIFISP_HIST_BIN_N_MAX];
  /*zlj add*/
  struct Hdrae_stat_s oneframe[3];
  struct Hdrae_DRIndex_res fDRIndex;
  struct Hdrae_OE_meas_res fOEMeasRes;
  struct Hdr_sensor_metadata_s sensor;
  struct Hdr_sensor_expdata_s curexp;
  unsigned int lgmean;
} AecStat_t;


struct AecDyCfg {
  AecEcmFlickerPeriod_t  flicker;
  struct Cam_Win win;
};

typedef struct ExpSet_s {
  int vts;
  int regTime;
  int regGain;
  float Exposure;
  float Gain;
  float Time;

  int RegHdrGains[3];
  int RegHdrTime[3];
  float HdrGains[3];
  float HdrIntTimes[3];
} ExpSet_t;

/*****************************************************************************/
/**
*     AecResult_t
 * @brief   Aec_Result.
 */
/*****************************************************************************/
typedef struct AecResult_s {
  float coarse_integration_time;
  float analog_gain_code_global;
  int regIntegrationTime;
  int regGain;
  float InputExposure;
  float Exposure;
  float PixelClockFreqMHZ;
  float PixelPeriodsPerLine;
  float LinePeriodsPerField;

  /*add for smoother HdrAE*/
  int regAssistTime[AEC_EFFECT_FNUM-1][3];
  int regAssistGain[AEC_EFFECT_FNUM-1][3];
  float AssistExposure[AEC_EFFECT_FNUM-1][3];
  float AssistGain[AEC_EFFECT_FNUM-1][3];
  float AssistIntegrationTime[AEC_EFFECT_FNUM-1][3];

  AecMeasuringMode_t meas_mode;
  struct Cam_Win meas_win;
  unsigned int actives;
  unsigned char GridWeights[CIFISP_HDRAE_GRIDITEMS];//zlj modify
  uint8_t stepSize;
  CamerIcIspHistMode_t HistMode;
  float gainFactor;
  float gainBias;
  bool_t aoe_enable;
  bool auto_adjust_fps;
  enum LIGHT_MODE DON_LightMode;
  float DON_Fac;
  float MeanLuma;
  float HdrMeanLuma[3];
  float MaxGainRange;
  uint8_t Night_Trigger;
  uint8_t Night_Mode;
  float overHistPercent;

  float MinGain;
  float MaxGain;
  float MinIntegrationTime;
  float MaxIntegrationTime;
  /*zlj add for hdr result*/
  bool IsHdrMode;
  float DCG_Ratio;
  float NormalExpRatio;
  float LongExpRatio;
  int RegHdrGains[3];
  int RegHdrTime[3];
  float HdrGains[3];
  float HdrIntTimes[3];
  float DarkROI;
  float OEROI;
  int   DarkNum;
  int   OENum;
  float TargetDarkLuma;
  float TargetOELuma;

  ExpSet_t exp_set[AEC_EFFECT_FNUM];
  int exp_set_cnt;
} AecResult_t;

/*****************************************************************************/
/**
 * @brief   This function init the AEC instance.
 *
 * @param   AecConfig
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_OUTOFRANGE
 *
 *****************************************************************************/
RESULT AecInit
(
    AeInstanceConfig_t* pInstConfig,
    AecConfig_t* AecConfig
);

RESULT AecUpdateConfig
(
    AeHandle_t           pAecCtx,
    AecConfig_t*         pConfig
);

RESULT AecStart
(
    AeHandle_t pAecCtx
);

RESULT AecStop
(
    AeHandle_t pAecCtx
);

/*****************************************************************************/
/**
 * @brief   This function single run the AEC instance.
 *
 * @param   Aec_stat_t
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_OUTOFRANGE
 *
 *****************************************************************************/
RESULT AecRun
(
    AeHandle_t pAecCtx,
    float Exposure,
    AecStat_t* ae_stat,
    AwbMeasuringResult_t *pAwbMesureResult,
    AecResult_t* AecResult
);

/*****************************************************************************/
/**
 * @brief   This function get the AEC result.
 *
 * @param
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_OUTOFRANGE
 *
 *****************************************************************************/
RESULT AecGetResults
(
    AeHandle_t pAecCtx,
    AecResult_t* AecResult
);


/*****************************************************************************/
/**
 * @brief   This function release the AEC instance.
 *
 * @param   AecConfig
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_OUTOFRANGE
 *
 *****************************************************************************/
RESULT AecRelease
(
    AeHandle_t pAecCtx
);

/*****************************************************************************/
/**
 * @brief   This function get the ratio of meanluma & exposure to check environmental luminance fluctuation
 *
 * @param
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_OUTOFRANGE
 *
 *****************************************************************************/
RESULT AecGetEnvLgt
(
    AeHandle_t pAecCtx,
    float expTime,
    float gain,
    AecStat_t* ae_stat,
    float* EnvLgt
);

/*****************************************************************************/
/**
 * @brief   This function get the ratio of meanluma & exposure to check environmental luminance fluctuation
 *
 * @param
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_OUTOFRANGE
 *
 *****************************************************************************/
RESULT AecHdrGetEnvLgt
(
    AeHandle_t pAecCtx,
    AecStat_t* ae_stat,
    float* EnvLgt1,
    float* EnvLgt2
);

#ifdef __cplusplus
}
#endif

/* @} AECM */


#endif /* __AEC_H__*/
