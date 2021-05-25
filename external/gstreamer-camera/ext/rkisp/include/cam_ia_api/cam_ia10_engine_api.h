#ifndef _CAM_IA10_ENGINE_API_H_
#define _CAM_IA10_ENGINE_API_H_

using namespace std;

#include <memory>
#include "cameric.h"
#include <af/af.h>
#include <aec/aec.h>
#include <adpf/adpf.h>
#include <awb/awb.h>
#include <awdr/awdr.h>
#include <common/return_codes.h>
#include <common/cam_types.h>
#include <HAL/cam_types.h>


#ifdef ANDROID_SHARED_PTR
#include <shared_ptr.h>
# ifndef UTIL_GTL_USE_STD_SHARED_PTR
using google::protobuf::internal::shared_ptr;
using google::protobuf::internal::enable_shared_from_this;
using google::protobuf::internal::weak_ptr;
# endif
#endif

#define CAMIA10_BPC_MASK    (1 << 0)
#define CAMIA10_BLS_MASK    (1 << 1)
#define CAMIA10_SDG_MASK    (1 << 2)
#define CAMIA10_HST_MASK    (1 << 3)
#define CAMIA10_LSC_MASK    (1 << 4)
#define CAMIA10_AWB_GAIN_MASK (1 << 5)
#define CAMIA10_FLT_MASK  (1 << 6)
#define CAMIA10_BDM_MASK  (1 << 7)
#define CAMIA10_CTK_MASK  (1 << 8)
#define CAMIA10_GOC_MASK  (1 << 9)
#define CAMIA10_CPROC_MASK  (1 << 10)
#define CAMIA10_AFC_MASK  (1 << 11)
#define CAMIA10_AWB_MEAS_MASK (1 << 12)
#define CAMIA10_IE_MASK (1 << 13)
#define CAMIA10_AEC_MASK  (1 << 14)
#define CAMIA10_WDR_MASK  (1 << 15)
#define CAMIA10_DPF_MASK    (1 << 16)
#define CAMIA10_DPF_STRENGTH_MASK (1 << 17)
#define CAMIA10_DSP_3DNR_MASK (1 << 18)
#define CAMIA10_AEC_AFPS_MASK  (1<<19)

#define CAMIA10_ALL_MASK  (0xffffffff)

struct CamIA10_SensorModeData {
  unsigned int isp_input_width;
  unsigned int isp_input_height;

  unsigned int isp_output_width;
  unsigned int isp_output_height;

  unsigned int horizontal_crop_offset;
  unsigned int vertical_crop_offset;
  unsigned int cropped_image_width;
  unsigned int cropped_image_height;

  float pixel_clock_freq_mhz;
  unsigned int pixel_periods_per_line;
  unsigned int line_periods_per_field;
  unsigned int sensor_output_height;
  unsigned int fine_integration_time_min;
  unsigned int fine_integration_time_max_margin;
  unsigned int coarse_integration_time_min;
  unsigned int coarse_integration_time_max_margin;
  unsigned char exposure_valid_frame;
  int exp_time;
  unsigned short gain;
};


struct CamIA10_DyCfg {
  /*application can set */
  USE_CASE uc;
  struct HAL_AecCfg aec_cfg;
  struct HAL_AfcCfg afc_cfg;
  struct HAL_AwbCfg awb_cfg;
  HAL_FLASH_MODE flash_mode;
  HAL_IAMGE_EFFECT ie_mode;
  int aaa_locks;
  struct HAL_ColorProcCfg cproc;
  struct HAL_WdrCfg wdr_cfg;
  enum HAL_MODE_e dsp3dnr_mode;
  struct HAL_3DnrLevelCfg dsp3dnr_level;
  struct HAL_3DnrParamCfg dsp3dnr_param;
  enum HAL_MODE_e flt_mode;
  enum HAL_FLT_DENOISE_LEVEL_e flt_denoise;
  enum HAL_FLT_SHARPENING_LEVEL_e flt_sharp;
  /*sensor data*/
  struct CamIA10_SensorModeData sensor_mode;
  enum LIGHT_MODE LightMode;
  int len_pos;
};

struct CamIA10_Stats {
  unsigned int meas_type;
  AecStat_t aec;
  CamerIcAwbMeasuringResult_t awb;
  AfMeas_t af;
};

typedef struct CamIA10_AWB_Result_s {
  int actives;
  CamerIcGains_t awbGains;
  CamerIc3x3Matrix_t CcMatrix;
  CamerIcXTalkOffset_t CcOffset;
  CamLscMatrix_t                  LscMatrixTable;       /**< damped lsc matrix */
  CamerIcIspLscSectorConfig_t     SectorConfig;               /**< lsc grid */
  CamerIcIspAwbMeasuringMode_t    MeasMode;           /**< specifies the means measuring mode (YCbCr or RGB) */
  CamerIcAwbMeasuringConfig_t     MeasConfig;         /**< measuring config */
  Cam_Win_t           awbWin;
  uint8_t                 DoorType;
  char IllName[20];  //yamasaki
  int err_code;
} CamIA10_AWB_Result_t;

typedef struct CamIA10_AFC_Result_s {
  int32_t  LensePos;
  uint32_t Thres;
  uint32_t VarShift;

  uint32_t  Window_Num;
  struct Cam_Win WindowA;
  struct Cam_Win WindowB;
  struct Cam_Win WindowC;
} CamIA10_AFC_Result_t;

struct CamIA10_Results {
  unsigned int active;
  /*3A algorithm result*/
  AecResult_t aec;
  bool_t aec_enabled;
  AdpfResult_t adpf;
  AwdrResult_t awdr;
  bool_t adpf_enabled;
  bool_t adpf_strength_enabled;
  CamIA10_AWB_Result_t awb;
  bool_t awb_gains_enabled;
  bool_t awb_meas_enabled;
  bool_t lsc_enabled;
  bool_t ctk_enabled;
  CamIA10_AFC_Result_t af;
  bool_t afc_meas_enabled;
  /*ISP sub modules result*/
  CamerIcIspBlsConfig_t bls;
  CamerIcDpccConfig_t dpcc;
  CamerIcIspDegammaCurve_t sdg;
  CamerIcIspFltConfig_t flt;
  CamerIcIspGocConfig_t goc;
  CamerIcCprocConfig_t cproc;
  CamerIcIeConfig_t ie;
  //struct cifisp_wdr_config wdr;
  CamerIcIspHistConfig_t hst;
  CameraIcBdmConfig_t bdm;
  CameraIcWdrConfig_t wdr;
  /* following results are included in 3A*/
  //struct cifisp_lsc_config lsc;
  //struct cifisp_awb_gain_config awb_gain;
  //struct cifisp_ctk_config ctk;
  //struct cifisp_awb_meas_config awb_meas;
  //struct cifisp_aec_config aec;
  //struct cifisp_dpf_config dpf;
  //struct cifisp_afc_config afc;
};

class CamIA10EngineItf {
 public:
  CamIA10EngineItf() {};
  virtual ~CamIA10EngineItf() {};

  virtual RESULT initStatic(char* aiqb_data_file) = 0;
  virtual RESULT initDynamic(struct CamIA10_DyCfg* cfg) = 0;
  virtual RESULT setStatistics(struct CamIA10_Stats* stats) = 0;

  virtual RESULT runAEC() = 0;
  virtual RESULT getAECResults(AecResult_t* result) = 0;

  virtual RESULT runAWB() = 0;
  virtual RESULT getAWBResults(CamIA10_AWB_Result_t* result) = 0;

  virtual RESULT runADPF() = 0;
  virtual RESULT getADPFResults(AdpfResult_t* result) = 0;

  virtual RESULT runAF() = 0;
  virtual RESULT getAFResults(CamIA10_AFC_Result_t* result) = 0;

  virtual RESULT runAWDR() = 0;
  virtual RESULT getAWDRResults(AwdrResult_t* result) = 0;
  /* manual ISP configs*/
  virtual RESULT runManISP(
      struct HAL_ISP_cfg_s* manCfg,
      struct CamIA10_Results* result) = 0;
  /* map driver sensor exposure to HAL*/
  virtual void mapSensorExpToHal(
      int sensorGain,
      int sensorInttime,
      float& halGain,
      float& halInttime) = 0;
  virtual void mapHalExpToSensor(
      float hal_gain,
      float hal_time,
      int& sensor_gain,
      int& sensor_time) = 0;

  virtual void mapHalWinToIsp(
    uint16_t in_width, uint16_t in_height,
    uint16_t in_hOff, uint16_t in_vOff,
    uint16_t drvWidth, uint16_t drvHeight,
    uint16_t& out_width, uint16_t& out_height,
    uint16_t& out_hOff, uint16_t& out_vOff) = 0;

  virtual RESULT getWdrConfig(struct HAL_ISP_wdr_cfg_s* wdr_cfg, enum HAL_ISP_WDR_MODE_e wdr_mode) = 0;

};

shared_ptr<CamIA10EngineItf> getCamIA10EngineItf(void);
#endif
