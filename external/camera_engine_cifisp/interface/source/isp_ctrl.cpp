#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <sys/ioctl.h>
#include <calib_xml/calibdb.h>
#include <utils/Log.h>

#include <isp_ctrl.h>
#include <HAL/CamIsp10CtrItf.h>

int getSensorModeData(int devFd,
    struct isp_supplemental_sensor_mode_data* data) {
	int ret = 0;

	ret = ioctl(devFd, RK_VIDIOC_SENSOR_MODE_DATA, data);

	if (ret < 0)
		LOGE("ERR(%s): RK_VIDIOC_SENSOR_MODE_DATA failed, err: %s \n",
		     __func__, strerror(errno));

	return ret;
}

int setExposure(int m_cam_fd_overlay, struct HAL_ISP_Set_Exp_s* exp) {
  int ret, idx = 0;
  struct v4l2_ext_control exp_gain[5];
  struct v4l2_ext_controls ctrls;

  if (exp->cls_exp_before) {
    exp_gain[idx].id = RK_V4L2_CID_CLS_EXP;
    exp_gain[idx++].value = 0;
  }

  exp_gain[idx].id = V4L2_CID_EXPOSURE;
  exp_gain[idx++].value = exp->exposure;
  exp_gain[idx].id = V4L2_CID_GAIN;
  exp_gain[idx++].value = exp->gain;
  exp_gain[idx].id = RK_V4L2_CID_GAIN_PERCENT;
  exp_gain[idx++].value = exp->gain_percent;
  exp_gain[idx].id = RK_V4L2_CID_VTS;
  exp_gain[idx++].value = exp->vts;

  ctrls.count = idx;
  ctrls.ctrl_class = V4L2_CTRL_CLASS_USER;
  ctrls.controls = exp_gain;
  ctrls.reserved[0] = 0;
  ctrls.reserved[1] = 0;
  ret = ioctl(m_cam_fd_overlay, VIDIOC_S_EXT_CTRLS, &ctrls);
  if (ret < 0) {
    LOGE("ERR(%s-%d):set of  AE seting to sensor config failed! err: %s\n",
         __func__,
         m_cam_fd_overlay,
         strerror(errno));
    return ret;
  }

  return ret;
}

int setAutoAdjustFps(int m_cam_fd_overlay, bool auto_adjust_fps) {
  int ret;
  struct v4l2_control ctrl;

  ctrl.id = RK_V4L2_CID_AUTO_FPS;
  ctrl.value = auto_adjust_fps;

  ret = ioctl(m_cam_fd_overlay, VIDIOC_S_CTRL, &ctrl);

  if (ret < 0)
  {
    LOGE("ERR(%s): oyyf set AE seting auto adjust fps enable(%d)to sensor config failed! ret:%d err: %s\n",
         __func__,
         auto_adjust_fps,ret,strerror(errno));
  }

  return ret;
}

int setFocusPos(int m_cam_fd_overlay, unsigned int position) {
  struct v4l2_control ctrl;
  int ret;

  ctrl.id = V4L2_CID_FOCUS_ABSOLUTE;
  ctrl.value = position;
  ret = ioctl(m_cam_fd_overlay, VIDIOC_S_CTRL, &ctrl);

  if (ret < 0) {
    LOGE("Could not set focus, error %d", ret);
  }

  return ret;
}
