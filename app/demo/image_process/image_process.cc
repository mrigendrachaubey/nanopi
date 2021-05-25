/*
 * Copyright (C) 2018 hertz wangh@rock-chips.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

// #define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <termios.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#define STRIP_FLAG_HELP 1
#include <gflags/gflags.h>

#include <rga/RockchipRga.h>

#if RKNNCASCADE
#include <rknn_cascade/RknnCascade.h>
#include <rknn_cascade/ssd.h>
#endif

#include "version.h"

#define SLICE_NUM 4
#define JOIN_ROUND_BUFFER_NUM 4
#define WIDTH 4096
#define HEIGHT 2160
#define NPU_PIECE_WIDTH 300
#define NPU_PIECE_HEIGHT 300
#define NPU_MODEL_PATH "/userdata/ssd_inception_v2.rknn"
#define FONT_FILE_PATH "/usr/lib/fonts/DejaVuSansMono.ttf"
#define LOGO_FILE_PATH "/userdata/logo.png"

enum class Format { NV12, NV16, RGB888, RGBX, YUYV, YU12, RGB565 };

typedef std::underlying_type<Format>::type FormatType;
#define DEFAULT_JOIN_FORMAT static_cast<FormatType>(Format::RGB888)

static int request_exit = 0;

DEFINE_bool(tty, false, "enable accept tty charater");
DEFINE_bool(debug, false, "enable debug log");
DEFINE_bool(disp, true, "enable display");
DEFINE_uint32(disp_rotate, 0, "display rotate degree: 0, 90, 180, 270");
DEFINE_bool(loop, false, "enable loop playback, useful for non-realtime input");
DEFINE_string(disp_time, "post_processor",
              "where image display from. [pre_processor, post_processor]");
DEFINE_string(processor, "npu", "type of precessor. [npu, dsp, none]");
DEFINE_uint32(npu_piece_width, NPU_PIECE_WIDTH,
              "piece width for npu input, default 300");
DEFINE_uint32(npu_piece_height, NPU_PIECE_HEIGHT,
              "piece height for npu input, default 300");
DEFINE_string(npu_data_source, "drm",
              "npu input data source. [usb, drm, none]");
#if RKNNCASCADE
DEFINE_string(npu_model_path, NPU_MODEL_PATH, "the path of mpu model");
#endif
#if HAVE_ROCKX
DEFINE_string(npu_model_name, "", "model name list. [ssd, dms]");
#endif
DEFINE_string(drm_conn_type, "DSI", "drm connector type. [DSI, CSI]");
DEFINE_bool(drm_raw8_mode, false, "drm transfer with raw8");
DEFINE_string(input, "", "input paths. separate each path by space.");
DEFINE_uint32(input_width, 1280, "input width, such for v4l2, default 1280");
DEFINE_uint32(input_height, 720, "input height, such for v4l2, default 720");
DEFINE_int32(
    input_format, -1,
    "InputFormat: 0 nv12, 1 nv16, 2 rgb888, 3 rgbx, 4 yuyv, 5 yu12, 6 rgb565");
DEFINE_string(v4l2_memory_type, "mmap", "v4l2 memory type, mmap or dma");
DEFINE_uint32(width, WIDTH, "piece together width, default 4096");
DEFINE_uint32(height, HEIGHT, "piece together height, default 2160");
DEFINE_uint32(
    format, DEFAULT_JOIN_FORMAT,
    "OutFormat: 0 nv12, 1 nv16, 2 rgb888, 3 rgbx, 4 yuyv, 5 yu12, 6 rgb565");
DEFINE_int32(slice_num, SLICE_NUM, "piece num, default 4");
DEFINE_uint32(join_round_buffer_num, JOIN_ROUND_BUFFER_NUM,
              "round buffer num for join, default 4");
DEFINE_string(font, FONT_FILE_PATH, "font file path");
DEFINE_string(logo, LOGO_FILE_PATH, "png logo file path");

typedef enum { PRE_PROCESSOR = 1, POST_PROCESSOR } DISPLAY_TIMING;

#define PrintFunLine() printf("%s : %d\n", __FUNCTION__, __LINE__)

static int get_rga_format(Format f) {
  static std::map<Format, int> rga_format_map;
  if (rga_format_map.empty()) {
    rga_format_map[Format::NV12] = RK_FORMAT_YCrCb_420_SP;
    rga_format_map[Format::NV16] = RK_FORMAT_YCrCb_422_SP;
    rga_format_map[Format::RGB888] = RK_FORMAT_RGB_888;
    rga_format_map[Format::RGBX] = RK_FORMAT_RGBX_8888;
    rga_format_map[Format::YU12] = RK_FORMAT_YCrCb_420_P;
    rga_format_map[Format::RGB565] = RK_FORMAT_RGB_565;
  }
  auto it = rga_format_map.find(f);
  if (it != rga_format_map.end())
    return it->second;
  return -1;
}

static void msleep(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

class Handler {
public:
  Handler();
  virtual ~Handler() = 0;
  bool is_req_exit() { return req_exit_; }
  virtual bool Prepare();
  virtual bool Start();
  virtual void Run();
  virtual bool Stop();
  virtual void RequestStop();

protected:
  std::thread *thread_;
  bool start;
  volatile bool req_exit_;
};

static void routine(void *args) {
  Handler *h = (Handler *)args;
  h->Run();
}

Handler::Handler() : thread_(nullptr), start(false), req_exit_(false) {}

Handler::~Handler() {}

bool Handler::Prepare() {
  thread_ = new std::thread(routine, this);
  if (!thread_)
    return false;
  return true;
}

bool Handler::Start() {
  start = true;
  return true;
}

void Handler::Run() {
  while (!start) {
    if (req_exit_)
      break;
    msleep(1);
  }
}

void Handler::RequestStop() { req_exit_ = true; }

bool Handler::Stop() {
  RequestStop();
  if (thread_) {
    thread_->join();
    delete thread_;
    thread_ = nullptr;
  }
  return true;
}

extern "C" {
#include <libavcodec/avfft.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/avassert.h>
#include <libavutil/avstring.h>
#include <libavutil/dict.h>
#include <libavutil/eval.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/parseutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#include <libavutil/hwcontext_drm.h>
#include <libdrm/drm_fourcc.h>
}

static int get_drm_fmt(Format f) {
  switch (f) {
  case Format::NV12:
    return DRM_FORMAT_NV12;
  case Format::YU12:
    return DRM_FORMAT_YUV420;
  case Format::NV16:
    return DRM_FORMAT_NV16;
  case Format::YUYV:
    return DRM_FORMAT_YUYV;
  case Format::RGB565:
    return DRM_FORMAT_RGB565;
  case Format::RGB888:
    return DRM_FORMAT_RGB888;
  case Format::RGBX:
    return DRM_FORMAT_XRGB8888;
  }
  return -1;
}

static int get_rga_format_bydrmfmt(uint32_t drm_fmt) {
  switch (drm_fmt) {
  case DRM_FORMAT_NV12:
    return RK_FORMAT_YCrCb_420_SP;
  case DRM_FORMAT_YUV420:
    return RK_FORMAT_YCrCb_420_P;
  case DRM_FORMAT_NV16:
    return RK_FORMAT_YCrCb_422_SP;
  case DRM_FORMAT_RGB565:
    return RK_FORMAT_RGB_565;
  case DRM_FORMAT_YUYV:
    return -1;
  }
  return -1;
}

static Format get_format_bydrmfmt(uint32_t drm_fmt) {
  switch (drm_fmt) {
  case DRM_FORMAT_NV12:
    return Format::NV12;
  case DRM_FORMAT_YUV420:
    return Format::YU12;
  case DRM_FORMAT_NV16:
    return Format::NV16;
  case DRM_FORMAT_YUYV:
    return Format::YUYV;
  case DRM_FORMAT_RGB565:
    return Format::RGB565;
  default:
    break;
  }
  return static_cast<Format>(-1);
}

static int get_format_bits(Format f) {
  switch (f) {
  case Format::NV12:
  case Format::YU12:
    return 12;
  case Format::NV16:
  case Format::YUYV:
  case Format::RGB565:
    return 16;
  case Format::RGB888:
    return 24;
  case Format::RGBX:
    return 32;
  }
  return 0;
}

class Join;
static void push_frame(Join *jn, int index, std::shared_ptr<AVFrame> f);
static bool is_realtime(const char *name) {
  if (!strncmp(name, "rtp:", 4) || !strncmp(name, "rtsp:", 5) ||
      !strncmp(name, "sdp:", 4) || !strncmp(name, "/dev/video", 10))
    return true;

  return false;
}
class Input : public Handler {
public:
  Input(std::string path);
  virtual ~Input();
  void set_slice_index(int index) { slice_idx = index; }
  void set_join_handler(Join *j) { join = j; }
  virtual bool Prepare() override;
  virtual void Run() override;
  virtual bool Stop() override;

  void PushPacket(AVPacket *pkt);
  AVPacket *PopPacket();

  void PushFrame(AVFrame *frm);
  AVFrame *PopFrame();

  std::string path_;
  int slice_idx;
  Join *join;
  AVFormatContext *ic;
  int st_idx;
  AVStream *st;
  AVCodecContext *acc;
  double fps;
  AVRational tb;
  std::mutex pkt_mtx;
  // std::condition_variable cond;
  std::list<AVPacket *> vpkt_list;
  std::thread *dec_thread;
  std::mutex frame_mtx;
  std::list<AVFrame *> vfrm_list;
  std::thread *disp_thread;
  bool realtime;

  static const int kMaxPacketCacheNum = 60;
  static const int kMaxFrameCacheNum = 4;
};

Input::Input(std::string path)
    : path_(path), slice_idx(-1), join(nullptr), ic(NULL), st_idx(-1), st(NULL),
      acc(NULL), fps(0.0f), dec_thread(nullptr), disp_thread(nullptr),
      realtime(false) {
  realtime = is_realtime(path.c_str());
}
Input::~Input() {
  for (auto pkt : vpkt_list)
    av_packet_free(&pkt);
  for (auto frm : vfrm_list)
    av_frame_free(&frm);
  avcodec_free_context(&acc);
  avformat_close_input(&ic);
}

static void dec_loop(void *arg);
static void disp_loop(void *arg);

bool Input::Prepare() {
  int err;
  AVCodec *codec;
  AVRational frame_rate;
  AVDictionary *dict = NULL;

  ic = avformat_alloc_context();
  if (!ic) {
    av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
    goto fail;
  }

  av_dict_set(&dict, "rtsp_transport", "tcp", 0);
  av_dict_set(&dict, "stimeout", "30000000", 0);
  err = avformat_open_input(&ic, path_.c_str(), NULL, &dict);
  if (err < 0) {
    av_log(ic, AV_LOG_FATAL, "Could not open %s.\n", path_.c_str());
    goto fail;
  }

  ic->flags |= AVFMT_FLAG_GENPTS;
  av_format_inject_global_side_data(ic);

  err = avformat_find_stream_info(ic, NULL);
  if (err < 0) {
    av_log(ic, AV_LOG_FATAL, "Could not find stream info of %s.\n",
           path_.c_str());
    goto fail;
  }

  // av_dump_format(ic, 0, path_.c_str(), 0);

  st_idx = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  if (st_idx < 0) {
    av_log(ic, AV_LOG_FATAL, "Could not find video stream for %s.\n",
           path_.c_str());
    goto fail;
  }
  st = ic->streams[st_idx];

  acc = avcodec_alloc_context3(NULL);
  if (!acc) {
    av_log(ic, AV_LOG_FATAL, "Could not alloc avcodec ctx for %s.\n",
           path_.c_str());
    goto fail;
  }

  err = avcodec_parameters_to_context(acc, st->codecpar);
  if (err < 0) {
    av_log(ic, AV_LOG_FATAL, "Could not set avcodec ctx parameters for %s.\n",
           path_.c_str());
    goto fail;
  }
  acc->pkt_timebase = st->time_base;

  if (acc->codec_id != AV_CODEC_ID_H264) {
    av_log(ic, AV_LOG_FATAL, "Not h264 data in %s.\n", path_.c_str());
    goto fail;
  }

  codec = avcodec_find_decoder_by_name("h264_rkmpp");
  if (!codec) {
    av_log(ic, AV_LOG_FATAL, "Could not find codec h264_rkmpp.\n");
    codec = avcodec_find_decoder(acc->codec_id);
    if (!codec) {
      av_log(ic, AV_LOG_FATAL, "Could not find codec for AV_CODEC_ID_H264.\n");
      goto fail;
    }
  }

  if ((err = avcodec_open2(acc, codec, NULL)) < 0) {
    av_log(ic, AV_LOG_FATAL, "Could not open h264_rkmpp.\n");
    goto fail;
  }

  st->discard = AVDISCARD_DEFAULT;

  frame_rate = av_guess_frame_rate(ic, st, NULL);
  fps = (frame_rate.num && frame_rate.den ? av_q2d(frame_rate) : 30.0f);
  printf("fps: %f\n", fps);
  tb = st->time_base;

  if (dict != NULL)
    av_dict_free(&dict);

  dec_thread = new std::thread(dec_loop, this);
  if (!dec_thread)
    goto fail;

  disp_thread = new std::thread(disp_loop, this);
  if (!disp_thread)
    goto fail;

  if (!Handler::Prepare())
    goto fail;
  return true;

fail:
  if (dict != NULL)
    av_dict_free(&dict);
  Stop();
  return false;
}

void Input::PushPacket(AVPacket *pkt) {
  std::lock_guard<std::mutex> lkg(pkt_mtx);
  vpkt_list.push_back(pkt);
  // cond.notify_one();
}

AVPacket *Input::PopPacket() {
  AVPacket *pkt;
  std::lock_guard<std::mutex> lkg(pkt_mtx);
  if (vpkt_list.empty())
    return NULL;
  pkt = vpkt_list.front();
  vpkt_list.pop_front();
  // av_log(NULL, AV_LOG_ERROR, "pkt num decrease to %d\n", vpkt_list.size());
  return pkt;
}

void Input::PushFrame(AVFrame *frm) {
  std::lock_guard<std::mutex> lkg(frame_mtx);
  vfrm_list.push_back(frm);
}

AVFrame *Input::PopFrame() {
  AVFrame *frm;
  std::lock_guard<std::mutex> lkg(frame_mtx);
  if (vfrm_list.empty())
    return NULL;
  frm = vfrm_list.front();
  vfrm_list.pop_front();
  return frm;
}

void disp_loop(void *arg) {
  Input *input = (Input *)arg;
  static const double interval = 1000000.0 / input->fps;
  double q2d = av_q2d(input->tb) * 1000000.0;
  int64_t pre_pts = 0;
  int64_t frame_pre_pts = AV_NOPTS_VALUE;
  AVFrame *pending_frm = NULL;

  while (!input->is_req_exit()) {
    int64_t cur_pts;
    int remaining_time = 10000;
    double duration = interval;
    AVFrame *frm;

    if (pending_frm) {
      frm = pending_frm;
    } else {
      frm = input->PopFrame();
      if (!frm) {
        msleep(10);
        continue;
      }
      // printf("pop frame pts: %ld\n", frm->pts);
    }
    static auto delete_func = [](AVFrame *f) { av_frame_free(&f); };

    cur_pts = av_gettime_relative();
    if (frame_pre_pts != AV_NOPTS_VALUE)
      duration = (frm->pts - frame_pre_pts) * q2d;

    int countdown = (pre_pts == 0) ? 0 : (int)(duration - (cur_pts - pre_pts));
    remaining_time = std::min<int>(remaining_time, countdown);

    // printf("countdown: %d, remaining_time: %d us\n",
    //         countdown, remaining_time);
    if (input->realtime) {
      countdown = 0;
      remaining_time = 0;
    }
    if (countdown <= 0) {
      frame_pre_pts = frm->pts;
      pre_pts = cur_pts;
      if (frm == pending_frm)
        pending_frm = NULL;
      push_frame(input->join, input->slice_idx,
                 std::shared_ptr<AVFrame>(frm, delete_func));
    } else {
      pending_frm = frm;
    }

    if (remaining_time > 0)
      std::this_thread::sleep_for(std::chrono::microseconds(remaining_time));
  }
  if (pending_frm)
    av_frame_free(&pending_frm);
}

void dec_loop(void *arg) {
  AVPacket *pkt;
  AVPacket pending_pkt;
  bool eof = false;
  bool packet_eof = false;
  int ret;
  bool pkt_pending = false;
  Input *input = (Input *)arg;

  av_init_packet(&pending_pkt);
  while (!input->is_req_exit() && !eof) {
    int got_picture = 0;
    AVFrame *frame;

    if (input->vfrm_list.size() >= Input::kMaxFrameCacheNum) {
      if (input->realtime) {
        AVFrame *first_frame = input->PopFrame();
        // printf("drop frame pts: %ld\n", first_frame->pts);
        av_frame_free(&first_frame);
      } else {
        msleep(5);
      }
      continue;
    }

    if (!pkt_pending) {
      pkt = input->PopPacket();
      if (!pkt) {
        if (packet_eof)
          goto get_frame;
        msleep(10);
        continue;
      }
      if (pkt->size == 0 && !input->realtime)
        packet_eof = true;
    } else {
      pkt = &pending_pkt;
      pkt_pending = false;
    }

    ret = avcodec_send_packet(input->acc, pkt);
    if (ret == AVERROR(EAGAIN)) {
      // av_log(NULL, AV_LOG_ERROR, "pkt pending\n");
      pkt_pending = true;
      if (pkt != &pending_pkt)
        av_packet_move_ref(&pending_pkt, pkt);
    } else {
      if (pkt == &pending_pkt)
        av_packet_unref(pkt);
    }
    if (pkt != &pending_pkt)
      av_packet_free(&pkt);

  get_frame:
    frame = av_frame_alloc();
    av_assert0(frame);
    do {
      if (input->is_req_exit())
        break;
      ret = avcodec_receive_frame(input->acc, frame);
      if (ret == AVERROR_EOF) {
        avcodec_flush_buffers(input->acc);
        av_log(NULL, AV_LOG_ERROR, "avcodec_receive_frame eof\n");
        eof = true;
        break;
      }
      if (ret >= 0) {
        // frame->pts = frame->best_effort_timestamp;
        got_picture = 1;
        break;
      }
      if (ret == AVERROR(EAGAIN))
        msleep(5);
    } while (ret != AVERROR(EAGAIN));
    if (got_picture) {
      // printf("push frame pts: %ld\n", frame->pts);
      input->PushFrame(frame);
    } else {
      av_frame_free(&frame);
    }
  }
}

void Input::Run() {
  int ret;
  bool eof = false;
  AVPacket *pkt;

  Handler::Run();
  while (!req_exit_ && !eof) {
    if (vpkt_list.size() > kMaxPacketCacheNum) {
      msleep(10);
      continue;
    }
    pkt = av_packet_alloc();
    av_assert0(pkt);
    ret = av_read_frame(ic, pkt);
    if (ret < 0) {
      if (!realtime) {
        if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !eof) {
          av_log(NULL, AV_LOG_INFO, "%s reach eof\n", path_.c_str());
          // rewind
          if (FLAGS_loop) {
            ret = avformat_seek_file(ic, -1, INT64_MIN, 0, INT64_MAX, 0);
            if (ret < 0) {
              av_log(NULL, AV_LOG_ERROR, "%s: error while seeking\n",
                     path_.c_str());
            }
          } else {
            eof = true;
          }
        }
      }
      if (ic->pb && ic->pb->error) {
        av_log(NULL, AV_LOG_ERROR, "read error: %d\n", ic->pb->error);
        av_packet_free(&pkt);
        break;
      }
      av_packet_free(&pkt);
      continue;
    }
    if (pkt->stream_index == st_idx)
      PushPacket(pkt);
    else
      av_packet_free(&pkt);
  }
  // flush
  pkt = av_packet_alloc();
  av_assert0(pkt);
  PushPacket(pkt);
  printf("%s exit read packet\n", path_.c_str());
}

bool Input::Stop() {
  Handler::Stop();
  if (dec_thread) {
    dec_thread->join();
    delete dec_thread;
    dec_thread = nullptr;
  }
  if (disp_thread) {
    disp_thread->join();
    delete disp_thread;
    disp_thread = nullptr;
  }
  return true;
}

class spinlock_mutex {
  std::atomic_flag flag;

public:
  spinlock_mutex() : flag(ATOMIC_FLAG_INIT) {}
  spinlock_mutex(const spinlock_mutex &) {}
  void lock() {
    while (flag.test_and_set(std::memory_order_acquire))
      ;
  }
  void unlock() { flag.clear(std::memory_order_release); }
};

typedef struct {
  int32_t x, y;
  uint32_t w, h;
} Rect;

typedef struct {
  bo_t bo;
  int fd;
  int width;
  int height;
  std::vector<int64_t> slice_pts_vec;
  Format fmt;
} JoinBO;

static bool AllocJoinBO(int drmfd, RockchipRga &rga, JoinBO *jb, int w, int h) {
  bo_t bo;
  int fd = -1;
  memset(&bo, 0, sizeof(bo));
  memset(jb, 0, sizeof(*jb));
  jb->fd = -1;
  jb->bo.fd = -1;
  jb->bo.ptr = NULL;
  bo.fd = drmfd;
  if (rga.RkRgaAllocBuffer(drmfd, &bo, w, h, 32)) {
    av_log(NULL, AV_LOG_FATAL, "Fail to alloc large bo memory\n");
    return false;
  }
  if (rga.RkRgaGetBufferFd(&bo, &fd) || rga.RkRgaGetMmap(&bo)) {
    if (fd >= 0)
      close(fd);
    rga.RkRgaFreeBuffer(drmfd, &bo);
    av_log(NULL, AV_LOG_FATAL, "Fail to mmap bo memory\n");
    return false;
  }
  av_assert0(bo.handle > 0);
  jb->bo = bo;
  jb->fd = fd;
  jb->width = w;
  jb->height = h;
  av_log(NULL, AV_LOG_INFO,
         "alloc bo, fd<%d>, handle<%d>, ptr<%p>, w<%d>, h<%d>\n", fd, bo.handle,
         bo.ptr, w, h);
  return true;
}

static void FreeJoinBO(int drmfd, RockchipRga &rga, JoinBO *jb) {
  av_log(NULL, AV_LOG_INFO,
         "free bo, fd<%d>, handle<%d>, ptr<%p>, w<%d>, h<%d>\n", jb->fd,
         jb->bo.handle, jb->bo.ptr, jb->width, jb->height);
  if (jb->bo.ptr)
    rga.RkRgaUnmap(&jb->bo);
  rga.RkRgaFreeBuffer(drmfd, &jb->bo);
  jb->bo.fd = -1;
  if (jb->fd >= 0) {
    close(jb->fd);
    jb->fd = -1;
  }
}

static void DumpJoinBOToFile(JoinBO *jb, char *file_path, int length,
                             const int frame_start, const int frame_end,
                             int &dump_fd, int &frame_num) {
  if (dump_fd == -1) {
    dump_fd = open(file_path, O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    assert(dump_fd >= 0);
    printf("open %s\n", file_path);
  }
  frame_num++;
  if (frame_num >= frame_start && frame_num <= frame_end) {
    printf("frame_num : %d\n", frame_num);
    write(dump_fd, jb->bo.ptr, length);
  } else if (frame_num > frame_end) {
    if (dump_fd >= 0) {
      close(dump_fd);
      sync();
      printf("close %s\n", file_path);
    }
    dump_fd = -2;
  }
}

#define USE_LIBV4L2

#include <linux/videodev2.h>
#ifdef USE_LIBV4L2
#include <libv4l2.h>
#endif
#include <sys/ioctl.h>

static uint32_t get_v4l2_fmt(Format f) {
  switch (f) {
  case Format::NV12:
    return V4L2_PIX_FMT_NV12;
  case Format::YU12:
    return V4L2_PIX_FMT_YUV420;
  case Format::NV16:
    return V4L2_PIX_FMT_NV16;
  case Format::YUYV:
    return V4L2_PIX_FMT_YUYV;
  default:
    break;
  }
  return 0;
}

class CameraInput : public Input {
public:
  CameraInput(int drmfd, std::string path);
  virtual ~CameraInput();
  virtual bool Prepare() override;
  virtual void Run() override;
  virtual bool Stop() override;

  void enqueue_buffer(struct v4l2_buffer *buf);

  enum v4l2_buf_type capture_type;
  enum v4l2_memory memory_type;
  RockchipRga rga;
  int drm_fd;
  int fd; // camera fd
  std::atomic_int buffers_queued;
  std::vector<JoinBO *> buffer_list;
  int width, height;
  size_t buffer_length;
};

struct buff_data {
  CameraInput *s;
  int index;
};

#define DUMP_FOURCC(f)                                                         \
  f & 0xFF, (f >> 8) & 0xFF, (f >> 16) & 0xFF, (f >> 24) & 0xFF

#ifndef USE_LIBV4L2
#define v4l2_open open
#define v4l2_close close
#define v4l2_dup dup
#define v4l2_ioctl ioctl
#define v4l2_read read
#define v4l2_mmap mmap
#define v4l2_munmap munmap
#endif

static int xioctl(int fd, int request, void *argp) {
  int r;

  do
    r = v4l2_ioctl(fd, request, argp);
  while (-1 == r && EINTR == errno);

  return r;
}

CameraInput::CameraInput(int drmfd, std::string path)
    : Input(path), capture_type(V4L2_BUF_TYPE_VIDEO_CAPTURE),
      memory_type(V4L2_MEMORY_MMAP), drm_fd(drmfd), fd(-1), buffers_queued(0),
      width(FLAGS_input_width), height(FLAGS_input_height), buffer_length(0) {
  if (FLAGS_v4l2_memory_type == "dma")
    memory_type = V4L2_MEMORY_DMABUF;
}

CameraInput::~CameraInput() {
  for (JoinBO *jb : buffer_list) {
    if (memory_type == V4L2_MEMORY_DMABUF) {
      FreeJoinBO(drm_fd, rga, jb);
    }
    if (memory_type == V4L2_MEMORY_MMAP) {
      if (jb->fd >= 0)
        close(jb->fd);
      if (jb->bo.ptr)
        v4l2_munmap(jb->bo.ptr, buffer_length);
    }
    delete jb;
  }
  if (fd >= 0)
    close(fd);
}

bool CameraInput::Stop() {
  enum v4l2_buf_type type = capture_type;
  if (fd >= 0)
    xioctl(fd, VIDIOC_STREAMOFF, &type);
  bool ret = Input::Stop();
  for (auto frm : vfrm_list)
    av_frame_free(&frm);
  vfrm_list.clear();
  return ret;
}

static void release_av_frame(void *opaque, uint8_t *data) {
  struct v4l2_buffer buf;
  memset(&buf, 0, sizeof(buf));
  AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor *)data;
  struct buff_data *bd = (struct buff_data *)opaque;
  CameraInput *s = bd->s;
  buf.type = s->capture_type;
  buf.memory = s->memory_type;
  buf.index = bd->index;
  av_free(desc);
  av_free(bd);
  if (buf.memory == V4L2_MEMORY_DMABUF) {
    JoinBO *jb = s->buffer_list[buf.index];
    buf.m.fd = jb->fd;
    buf.length = s->buffer_length;
  }
  s->enqueue_buffer(&buf);
}

void CameraInput::enqueue_buffer(struct v4l2_buffer *buf) {
  if (xioctl(fd, VIDIOC_QBUF, buf) < 0) {
    av_log(NULL, AV_LOG_ERROR, "%s, ioctl(VIDIOC_QBUF): %m\n", path_.c_str());
  } else {
    buffers_queued.fetch_add(1);
  }
}

int buffer_export(int v4lfd, enum v4l2_buf_type bt, int index, int *dmafd) {
  struct v4l2_exportbuffer expbuf;

  memset(&expbuf, 0, sizeof(expbuf));
  expbuf.type = bt;
  expbuf.index = index;
  if (ioctl(v4lfd, VIDIOC_EXPBUF, &expbuf) == -1) {
    perror("VIDIOC_EXPBUF");
    return -1;
  }

  *dmafd = expbuf.fd;

  return 0;
}

bool CameraInput::Prepare() {
  int w = width;
  int h = height;
  const char *device = path_.c_str();
  struct v4l2_capability cap;
  Format f = static_cast<Format>(FLAGS_input_format);
  struct v4l2_format fmt;
  struct v4l2_requestbuffers req;
  struct v4l2_streamparm setfps;
  enum v4l2_buf_type type = capture_type;

  static char media_ctl_str[128] = {0};

  if (media_ctl_str[0] == 0) {
    snprintf(media_ctl_str, sizeof(media_ctl_str),
             "media-ctl -vvv -d /dev/media0 --set-v4l2 \'\"jaguar1 "
             "3-0030\":0[fmt:UYVY8_2X8/%dx%d]\'",
             w, h);
    system(media_ctl_str);
  }

  fd = v4l2_open(device, O_RDWR, 0);
  if (fd < 0) {
    av_log(NULL, AV_LOG_ERROR, "open %s failed %m\n", device);
    goto fail;
  }

  if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Failed to ioctl(VIDIOC_QUERYCAP): %m\n");
    goto fail;
  }
  av_log(NULL, AV_LOG_INFO, "cap.capabilities: %08x \n", cap.capabilities);
  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
#ifndef USE_LIBV4L2
      && !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
#endif
  ) {
    av_log(NULL, AV_LOG_ERROR, "%s, Not a video capture device.\n", device);
    goto fail;
  }
  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    av_log(NULL, AV_LOG_ERROR,
           "%s does not support the streaming I/O method.\n", device);
    goto fail;
  }

#ifndef USE_LIBV4L2
  if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
    capture_type = type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
#endif

  memset(&fmt, 0, sizeof(fmt));
  fmt.type = type;
  fmt.fmt.pix.width = w;
  fmt.fmt.pix.height = h;
  fmt.fmt.pix.pixelformat = get_v4l2_fmt(f);
  fmt.fmt.pix.field = V4L2_FIELD_ANY;

  static bool specail_for_n4_yuyv = (f == Format::YUYV);

  if (fmt.fmt.pix.pixelformat == 0) {
    av_log(NULL, AV_LOG_ERROR, "unsupport input format : %d\n",
           FLAGS_input_format);
    goto fail;
  }

  if (specail_for_n4_yuyv) {
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SRGGB8;
    fmt.fmt.pix.width = 2 * w;
  }

  if (xioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
    av_log(NULL, AV_LOG_ERROR, "%s, s fmt failed(type=%d, %c%c%c%c), %m\n",
           device, type, DUMP_FOURCC(fmt.fmt.pix.pixelformat));
    goto fail;
  }

  if (!specail_for_n4_yuyv) {
    if (w != (int)fmt.fmt.pix.width || h != (int)fmt.fmt.pix.height) {
      av_log(NULL, AV_LOG_WARNING, "%s change res from %dx%d to %dx%d\n",
             device, w, h, fmt.fmt.pix.width, fmt.fmt.pix.height);
      w = width = fmt.fmt.pix.width;
      h = height = fmt.fmt.pix.height;
    }
    if (get_v4l2_fmt(f) != fmt.fmt.pix.pixelformat) {
      av_log(NULL, AV_LOG_ERROR, "%s, expect %d, return %c%c%c%c\n", device,
             FLAGS_input_format, DUMP_FOURCC(fmt.fmt.pix.pixelformat));
      goto fail;
    }
  }

  av_log(NULL, AV_LOG_INFO, "fmt.fmt.pix_mp.num_planes: %d\n",
         fmt.fmt.pix_mp.num_planes);

  // FLAGS_input_format = 1;

  if (fmt.fmt.pix.field == V4L2_FIELD_INTERLACED)
    av_log(NULL, AV_LOG_INFO, "%s is using the interlaced mode\n", device);

  memset(&setfps, 0, sizeof(setfps));
  setfps.type = type;
  setfps.parm.capture.timeperframe.denominator = 30;
  setfps.parm.capture.timeperframe.numerator = 1;
  if (xioctl(fd, VIDIOC_S_PARM, &setfps) < 0) {
    av_log(NULL, AV_LOG_ERROR, "%s, s parm fps failed, %m\n", device);
    fps = 30.0;
  } else {
    fps = (double)setfps.parm.capture.timeperframe.denominator /
          setfps.parm.capture.timeperframe.numerator;
  }

  memset(&req, 0, sizeof(req));
  req.type = type;
  req.count = 4;
  req.memory = memory_type;

  if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
    av_log(NULL, AV_LOG_ERROR, "%s, ioctl(VIDIOC_REQBUFS): %m\n", device);
    goto fail;
  }
  assert(req.count == 4);
  // stream on
  if (memory_type == V4L2_MEMORY_DMABUF) {
    w = (w + 15) & (~15);
    h = (h + 15) & (~15);
    buffer_length = w * h * get_format_bits(f) / 8;
    for (size_t i = 0; i < req.count; i++) {
      struct v4l2_buffer buf;
      memset(&buf, 0, sizeof(buf));
      buf.type = type;
      buf.index = i;
      buf.memory = memory_type;

      JoinBO *jb = new JoinBO();
      assert(jb); // as only demo, do not set detail error log
      if (!AllocJoinBO(drm_fd, rga, jb, w, h))
        assert(0); // no memory
      buffer_list.push_back(jb);

      buf.m.fd = jb->fd;
      buf.length = buffer_length;
      if (xioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s ioctl(VIDIOC_QBUF): %m\n", device);
        goto fail;
      }
    }
  }

  if (memory_type == V4L2_MEMORY_MMAP) {
    for (size_t i = 0; i < req.count; i++) {
      struct v4l2_buffer buf;
      JoinBO *jb = nullptr;
      int dmafd = -1;
      void *ptr = MAP_FAILED;
      memset(&buf, 0, sizeof(buf));
      buf.type = type;
      buf.index = i;
      buf.memory = memory_type;
      if (xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s ioctl(VIDIOC_QUERYBUF): %m\n", device);
        goto fail;
      }
      jb = new JoinBO();
      assert(jb); // as only demo, do not set detail error log
      memset(jb, 0, sizeof(*jb));
      buffer_list.push_back(jb);

      if (true || buffer_export(fd, capture_type, i, &dmafd) < 0) {
        av_log(NULL, AV_LOG_INFO, "%s buffer_export (%d): %m\n", device,
               (int)i);
        ptr = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fd, buf.m.offset);
        if (ptr == MAP_FAILED) {
          av_log(NULL, AV_LOG_ERROR, "%s v4l2_mmap (%d): %m\n", device, (int)i);
          goto fail;
        }
      }
      jb->fd = dmafd;
      if (ptr)
        jb->bo.ptr = ptr;
      buffer_length = buf.length;
    }
    for (size_t i = 0; i < req.count; ++i) {
      struct v4l2_buffer buf;
      memset(&buf, 0, sizeof(buf));
      buf.type = capture_type;
      buf.memory = memory_type;
      buf.index = i;

      if (xioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s, ioctl(VIDIOC_QBUF): %m\n", device);
        goto fail;
      }
    }
  }

  buffers_queued = req.count;
  if (xioctl(fd, VIDIOC_STREAMON, &type) < 0) {
    av_log(NULL, AV_LOG_ERROR, "%s, ioctl(VIDIOC_STREAMON): %m\n", device);
    goto fail;
  }
  disp_thread = new std::thread(disp_loop, this);
  if (!disp_thread)
    goto fail;
  if (!Handler::Prepare())
    goto fail;

  return true;

fail:
  Stop();
  return false;
}

void CameraInput::Run() {
  const char *device = path_.c_str();
  Handler::Run();
  while (!req_exit_) {
    int res;
    struct timeval buf_ts;
    struct v4l2_buffer buf;

    memset(&buf, 0, sizeof(buf));
    buf.type = capture_type;
    buf.memory = memory_type;

    res = xioctl(fd, VIDIOC_DQBUF, &buf);
    if (res < 0) {
      if (errno == EAGAIN)
        continue;
      av_log(NULL, AV_LOG_ERROR, "%s, ioctl(VIDIOC_DQBUF): %m\n", device);
      break;
    }
    assert(buf.index < 4);
    buf_ts = buf.timestamp;
    int queued_num = buffers_queued.fetch_add(-1);
    if (queued_num < 0)
      av_log(NULL, AV_LOG_WARNING, "%s, dqbuf faster than qbuf\n", device);
    struct buff_data *buf_descriptor =
        (struct buff_data *)av_malloc(sizeof(struct buff_data));
    AVFrame *frame = av_frame_alloc();
    if (!buf_descriptor || !frame) {
      enqueue_buffer(&buf);
      av_frame_free(&frame);
      av_freep(&buf_descriptor);
      continue;
    }
    buf_descriptor->index = buf.index;
    buf_descriptor->s = this;

    JoinBO *jb = buffer_list[buf.index];
    static Format f = static_cast<Format>(FLAGS_input_format);
    frame->format = AV_PIX_FMT_DRM_PRIME;
    frame->width = width;
    frame->height = height;
    frame->pts = buf_ts.tv_sec * 1000000LL + buf_ts.tv_usec;
    AVDRMFrameDescriptor *desc =
        (AVDRMFrameDescriptor *)av_mallocz(sizeof(AVDRMFrameDescriptor));
    assert(desc);
    desc->nb_objects = 1;
    desc->objects[0].fd = jb->fd;
    desc->objects[0].ptr = jb->bo.ptr;
    desc->objects[0].size = buffer_length;
    desc->nb_layers = 1;
    AVDRMLayerDescriptor *layer = &desc->layers[0];
    layer->format = get_drm_fmt(f);
    layer->nb_planes = 2;

    layer->planes[0].object_index = 0;
    layer->planes[0].offset = 0;
    layer->planes[0].pitch = width * 1;
    layer->planes[1].object_index = 0;
    layer->planes[1].offset = layer->planes[0].pitch * height;
    layer->planes[1].pitch = layer->planes[0].pitch;
    frame->data[0] = (uint8_t *)desc;
    frame->buf[0] =
        av_buffer_create((uint8_t *)desc, sizeof(*desc), release_av_frame,
                         buf_descriptor, AV_BUFFER_FLAG_READONLY);
    assert(frame->buf[0]);

    PushFrame(frame);
#if 0
    static int dump_fd = -1;
    static int frame_num = 0;
    char *path = "/tmp/camera_dump.nv16";
    printf("width, height : %d, %d\n", width, height);
    DumpJoinBOToFile(jb, path, width * height * 2, 500, 650,
                     dump_fd, frame_num);
#endif
  }
}

class LeafHandler : public Handler {
public:
  LeafHandler() : max_bo_num(FLAGS_join_round_buffer_num - 1) {}
  virtual ~LeafHandler();
  void PushBO(std::shared_ptr<JoinBO> jb);
  std::shared_ptr<JoinBO> PopBO(bool wait = true);
  virtual void Run() override;
  virtual bool Stop() override;
  virtual void SubRun() = 0;

  uint32_t max_bo_num;
  std::mutex list_mutex;
  std::condition_variable list_cond;
  std::list<std::shared_ptr<JoinBO>> pending_buffer_list;
};

LeafHandler::~LeafHandler() { pending_buffer_list.clear(); }

void LeafHandler::PushBO(std::shared_ptr<JoinBO> jb) {
  std::lock_guard<std::mutex> _lg(list_mutex);
  if (req_exit_)
    return;
  pending_buffer_list.push_back(jb);
  if (pending_buffer_list.size() > max_bo_num)
    pending_buffer_list.pop_front();
  list_cond.notify_one();
}

std::shared_ptr<JoinBO> LeafHandler::PopBO(bool wait) {
  std::unique_lock<std::mutex> _lk(list_mutex);
  if (pending_buffer_list.empty()) {
    if (wait)
      list_cond.wait(_lk);

    if (req_exit_ || !wait)
      return nullptr;
  }
  std::shared_ptr<JoinBO> ret = pending_buffer_list.front();
  pending_buffer_list.pop_front();
  return ret;
}

void LeafHandler::Run() {
  Handler::Run();
  SubRun();
}

bool LeafHandler::Stop() {
  list_mutex.lock();
  RequestStop();
  list_cond.notify_one();
  list_mutex.unlock();
  return Handler::Stop();
}

class Join : public Handler {
public:
  Join(int drmfd);
  ~Join();
  void set_frame_rate(double frame_rate);
  bool Prepare(int w, int h, int slicenum, int buffernum);
  virtual void Run() override;
  virtual bool Stop() override;

  std::shared_ptr<JoinBO>
  get_available_buffer(std::vector<std::shared_ptr<AVFrame>> frames);
  void dispatch_buffer(std::shared_ptr<JoinBO> jb) {
    for (auto h : leaf_handler_list)
      h->PushBO(jb);
  }

  void PushFrame(int index, std::shared_ptr<AVFrame> frame);
  bool scale_frame_to(AVFrame *av_frame, JoinBO *jb, int index);

  void AddLeaf(LeafHandler *h) {
    std::lock_guard<std::mutex> _lg(leaf_mutex);
    leaf_handler_list.push_back(h);
  }

  void RemoveLeaf(LeafHandler *h) {
    std::lock_guard<std::mutex> _lg(leaf_mutex);
    leaf_handler_list.remove(h);
  }

  int GetLineSliceNum() { return (int)std::sqrt((float)slice_num); }

  double fps;
  int width, height;
  int slice_num;
  std::vector<Rect> slice_rect;
  std::vector<spinlock_mutex> slice_mutex;
  std::vector<std::shared_ptr<AVFrame>> slice_frame_vec;

  int drm_fd;
  RockchipRga rga;
  int buffer_num;
  std::mutex list_mutex;
  std::condition_variable list_cond;
  std::list<JoinBO *> available_buffer_list;

  std::mutex leaf_mutex;
  std::list<LeafHandler *> leaf_handler_list;
};

Join::Join(int drmfd)
    : fps(33.0f), width(0), height(0), slice_num(0), drm_fd(drmfd),
      buffer_num(0) {}
Join::~Join() {
  slice_frame_vec.clear();
  int i = 0;
  for (JoinBO *jb : available_buffer_list) {
    FreeJoinBO(drm_fd, rga, jb);
    delete jb;
    i++;
  }
  if (i < buffer_num)
    av_log(NULL, AV_LOG_WARNING,
           "%d bo memory leak when delete Join,"
           "make sure bo memory return back before\n",
           buffer_num - i);
}

void Join::set_frame_rate(double frame_rate) { fps = frame_rate; }

bool Join::Prepare(int w, int h, int slicenum, int buffernum) {
  if (!rga.RkRgaIsReady()) {
    av_log(NULL, AV_LOG_FATAL, "rga is not ready, check it!\n");
    return false;
  }
  if (w % 16 != 0)
    av_log(NULL, AV_LOG_WARNING, "warning: join width does not align to 16\n");
  if (h % 16 != 0)
    av_log(NULL, AV_LOG_WARNING, "warning: join height does not align to 16\n");
  w = width = (w + 15) & (~15);
  h = height = (h + 15) & (~15);
  int n = (int)std::sqrt((float)slicenum);
  if (n * n != slicenum) {
    n += 1;
    slice_num = n * n;
  } else {
    slice_num = slicenum;
  }

  // calculate slice rect
  int slice_w = (w / n) & (~15);
  int slice_h = (h / n) & (~15);
#if 0
    int slice_w_gap = (n == 1 ? 0 : (w - slice_w * n) / (n - 1));
    int slice_h_gap = (n == 1 ? 0 : (h - slice_h * n) / (n - 1));
    int slice_w_end_gap =
        (w - slice_w * n - slice_w_gap * (n - 1)) / 2;
    int slice_h_end_gap =
        (h - slice_h * n - slice_h_gap * (n - 1)) / 2;
#else
  int slice_w_gap = (n == 1 ? 0 : (w - slice_w * n) / (n + 1));
  int slice_h_gap = (n == 1 ? 0 : (h - slice_h * n) / (n + 1));
  int slice_w_end_gap = slice_w_gap;
  int slice_h_end_gap = slice_h_gap;
#endif
  for (int i = 0; i < slice_num; i++) {
    Rect r;
    r.x = slice_w_end_gap + (slice_w + slice_w_gap) * (i % n);
    if (FLAGS_format == static_cast<FormatType>(Format::NV12) ||
        FLAGS_format == static_cast<FormatType>(Format::YU12) ||
        FLAGS_format == static_cast<FormatType>(Format::NV16) ||
        FLAGS_format == static_cast<FormatType>(Format::YUYV))
      r.x = (r.x + 1) & (~1);
    r.y = slice_h_end_gap + (slice_h + slice_h_gap) * (i / n);
    if (FLAGS_format == static_cast<FormatType>(Format::NV12) ||
        FLAGS_format == static_cast<FormatType>(Format::YU12) ||
        FLAGS_format == static_cast<FormatType>(Format::NV16) ||
        FLAGS_format == static_cast<FormatType>(Format::YUYV))
      r.y = (r.y + 1) & (~1);
    r.w = slice_w;
    r.h = slice_h;
    slice_rect.push_back(r);
    printf("rect %d : (%d, %d, %d, %d)\n", i, r.x, r.y, r.x + r.w, r.y + r.h);
  }
  slice_mutex.resize(slice_num);
  slice_frame_vec.resize(slice_num);

  buffer_num = buffernum;
  for (int i = 0; i < buffernum; i++) {
    JoinBO *jb = new JoinBO();
    if (!jb) {
      av_log(NULL, AV_LOG_FATAL, "Fail to alloc JoinBO\n");
      buffer_num = i;
      return false;
    }

    if (!AllocJoinBO(drm_fd, rga, jb, w, h)) {
      delete jb;
      buffer_num = i;
      return false;
    }

    jb->slice_pts_vec.resize(slice_num, -1);
    jb->fmt = static_cast<Format>(FLAGS_format);
    available_buffer_list.push_back(jb);
  }
  return Handler::Prepare();
}

void Join::Run() {
  Handler::Run();
  // static const double remaining_time = 1000000 / fps;
  std::vector<std::shared_ptr<AVFrame>> frames;

  frames.resize(slice_num);
  while (!req_exit_) {
    double rt = 1000000 / fps;
    int64_t now = av_gettime_relative();
    for (int i = 0; i < slice_num; i++) {
      slice_mutex[i].lock();
      frames[i] = slice_frame_vec[i];
      slice_mutex[i].unlock();
    }

    std::shared_ptr<JoinBO> jb = get_available_buffer(frames);
    if (jb.get()) {
      std::vector<int64_t> &pts = jb->slice_pts_vec;
      for (int i = 0; i < slice_num; i++) {
        AVFrame *av_frame = frames[i].get();
        if (!av_frame)
          continue;
        if (pts[i] == av_frame->pts)
          continue;
        if (scale_frame_to(av_frame, jb.get(), i))
          pts[i] = av_frame->pts;
      }
      dispatch_buffer(jb);
      jb.reset();
      rt -= (av_gettime_relative() - now);
    }

    for (auto &f : frames)
      f.reset();
    if (rt > 0.0)
      av_usleep((int64_t)rt);
  }
}

bool Join::Stop() {
  list_mutex.lock();
  RequestStop();
  list_cond.notify_one();
  list_mutex.unlock();
  return Handler::Stop();
  ;
}

std::shared_ptr<JoinBO>
Join::get_available_buffer(std::vector<std::shared_ptr<AVFrame>> frames) {
  bool dirty = false;
  std::unique_lock<std::mutex> _lk(list_mutex);
  if (available_buffer_list.empty()) {
    if (req_exit_)
      return nullptr;
    list_cond.wait(_lk);
    if (req_exit_)
      return nullptr;
  }
  JoinBO *jb = available_buffer_list.front();
  for (int i = 0; i < slice_num; i++) {
    AVFrame *av_frame = frames[i].get();
    if (!av_frame)
      continue;
    if (jb->slice_pts_vec[i] != av_frame->pts) {
      dirty = true;
      break;
    }
  }

  if (dirty) {
    auto recyle_fun = [this](JoinBO *jj) {
      std::lock_guard<std::mutex> _lg(list_mutex);
      available_buffer_list.push_back(jj);
      list_cond.notify_one();
    };
    available_buffer_list.pop_front();
    return std::shared_ptr<JoinBO>(jb, recyle_fun);
  }
  return nullptr;
}

void Join::PushFrame(int index, std::shared_ptr<AVFrame> frame) {
  slice_mutex[index].lock();
  slice_frame_vec[index] = frame;
  slice_mutex[index].unlock();
}

void push_frame(Join *jn, int index, std::shared_ptr<AVFrame> f) {
  jn->PushFrame(index, f);
}

bool Join::scale_frame_to(AVFrame *av_frame, JoinBO *jb, int index) {
  static int dst_format = get_rga_format(static_cast<Format>(FLAGS_format));
  if (av_frame->format == AV_PIX_FMT_NONE) {
    av_log(NULL, AV_LOG_FATAL, "av_frame is pix_none!\n");
    return false;
  }
  if (av_frame->format != AV_PIX_FMT_DRM_PRIME) {
    av_log(NULL, AV_LOG_FATAL,
           "ffmpeg with mpp is broken ? "
           "index: %d, av_frame->format: %d, AV_PIX_FMT_DRM_PRIME: %d\n",
           index, av_frame->format, AV_PIX_FMT_DRM_PRIME);
    abort();
  }
  AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor *)av_frame->data[0];
  AVDRMLayerDescriptor *layer = &desc->layers[0];
  rga_info_t src, dst;
  int ret;

  Rect &dts_rect = slice_rect[index];

  int src_format = get_rga_format_bydrmfmt(layer->format);
  if (src_format < 0 && desc->nb_layers == 1 &&
      (get_format_bits(static_cast<Format>(FLAGS_format)) ==
       get_format_bits(get_format_bydrmfmt(layer->format))) &&
      (av_frame->width == (int)dts_rect.w) &&
      (av_frame->height == (int)dts_rect.h)) {
    // just do copy
    src_format = dst_format;
  }
  if (src_format < 0) {
    av_log(NULL, AV_LOG_FATAL, "rga do not support format : %c%c%c%c\n",
           DUMP_FOURCC(layer->format));
    abort();
  }
  memset(&src, 0, sizeof(src));
  src.fd = desc->objects[0].fd;
  if (src.fd < 0)
    src.virAddr = desc->objects[0].ptr;
  src.mmuFlag = 1;
  rga_set_rect(&src.rect, 0, 0, av_frame->width, av_frame->height,
               layer->planes[0].pitch,
               layer->planes[1].offset / layer->planes[0].pitch, src_format);

  memset(&dst, 0, sizeof(dst));
  dst.fd = jb->fd;
  dst.mmuFlag = 1;

  rga_set_rect(&dst.rect, dts_rect.x, dts_rect.y, dts_rect.w, dts_rect.h,
               jb->width, jb->height, dst_format);
  ret = rga.RkRgaBlit(&src, &dst, NULL);
  if (ret)
    av_log(NULL, AV_LOG_ERROR, "RkRgaBlit error : %s\n", strerror(ret));

  return ret == 0;
}

static void get_format_rational(Format f, int &num, int &den) {
  switch (f) {
  case Format::NV12:
  case Format::YU12:
    num = 3;
    den = 2;
    break;
  case Format::NV16:
  case Format::YUYV:
  case Format::RGB565:
    num = 2;
    den = 1;
    break;
  case Format::RGB888:
    num = 3;
    den = 1;
    break;
  case Format::RGBX:
    num = 4;
    den = 1;
    break;
  default:
    num = 0;
    den = 1;
    return;
  }
}

class DummyLeaf : public LeafHandler {
public:
  DummyLeaf() {
    get_format_rational(static_cast<Format>(FLAGS_format), bpp_num, bpp_den);
  }
  virtual void SubRun() override;
  int bpp_num;
  int bpp_den;
};

void DummyLeaf::SubRun() {
  static bool dump = true;
  static const int dump_max_num = 10;

  while (!req_exit_) {
    std::shared_ptr<JoinBO> sjb = PopBO();
    JoinBO *jb = sjb.get();
    if (jb) {
      printf("Buffer <%d>, image is ready\n", jb->fd);
      if (dump && bpp_num) {
        static int file_num = 0;
        static char file_path[128];
        snprintf(file_path, sizeof(file_path), "/userdata/%d.image",
                 ++file_num);
        int fd = open(file_path, O_RDWR | O_CREAT | O_CLOEXEC, 0600);
        if (fd >= 0) {
          write(fd, jb->bo.ptr, jb->width * jb->height * bpp_num / bpp_den);
          close(fd);
        }
        if (file_num >= dump_max_num)
          dump = false;
      }
    }
  }
}

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#ifndef DRAW_BY_SDL
#define DRAW_BY_SDL 0
#endif
#define SDL_RGA DRAW_BY_SDL

#ifndef DRAW_BY_OPENGLES
#define DRAW_BY_OPENGLES 0
#endif
// #define DRAW_BY_OPENGLES 1

#if DRAW_BY_OPENGLES
#include <libdrm/drm_fourcc.h>
#define MESA_EGL_NO_X11_HEADERS
#include <SDL2/SDL_egl.h>
#include <SDL2/SDL_opengles2.h>

#define _STR(s) #s
#define STR(s) _STR(s)
typedef struct GLES2_Context {
#define SDL_PROC(ret, func, params) ret(APIENTRY *func) params;
#include "SDL_gles2funcs.h"
  SDL_PROC(void, glLineWidth, (GLfloat width))
  SDL_PROC(void, glDeleteBuffers, (GLsizei n, const GLuint *buffers))
  SDL_PROC(void, glUniform1f, (GLint location, GLfloat x))
  SDL_PROC(void, glBlendFunc, (GLenum, GLenum))
  SDL_PROC(void, glDrawElements,
           (GLenum mode, GLsizei count, GLenum type, const void *indices))
  SDL_PROC(EGLDisplay, eglGetCurrentDisplay, (void))
  SDL_PROC(EGLDisplay, eglGetDisplay, (EGLNativeDisplayType display_id))
  SDL_PROC(EGLImageKHR, eglCreateImageKHR,
           (EGLDisplay dpy, EGLContext ctx, EGLenum target,
            EGLClientBuffer buffer, const EGLint *attrib_list))
  SDL_PROC(EGLBoolean, eglDestroyImageKHR, (EGLDisplay dpy, EGLImageKHR image))
  SDL_PROC(EGLint, eglGetError, (void))
  SDL_PROC(void, glEGLImageTargetTexture2DOES,
           (GLenum target, GLeglImageOES image))
#undef SDL_PROC
} GLES2_Context;

static int LoadContext(GLES2_Context *data) {
#if SDL_VIDEO_DRIVER_UIKIT
#define __SDL_NOGETPROCADDR__
#elif SDL_VIDEO_DRIVER_ANDROID
#define __SDL_NOGETPROCADDR__
#elif SDL_VIDEO_DRIVER_PANDORA
#define __SDL_NOGETPROCADDR__
#endif

#if defined __SDL_NOGETPROCADDR__
#define SDL_PROC(ret, func, params) data->func = func;
#else
#define SDL_PROC(ret, func, params)                                            \
  do {                                                                         \
    data->func = (ret(*) params)SDL_GL_GetProcAddress(#func);                  \
    if (!data->func) {                                                         \
      return SDL_SetError("Couldn't load GLES2 function %s: %s", #func,        \
                          SDL_GetError());                                     \
    }                                                                          \
  } while (0);
#endif /* __SDL_NOGETPROCADDR__ */

#include "SDL_gles2funcs.h" //STR(SDL_GLES2FUNCS_H)
  SDL_PROC(void, glLineWidth, (GLfloat width))
  SDL_PROC(void, glDeleteBuffers, (GLsizei n, const GLuint *buffers))
  SDL_PROC(void, glUniform1f, (GLint location, GLfloat x))
  SDL_PROC(void, glBlendFunc, (GLenum, GLenum))
  SDL_PROC(void, glDrawElements,
           (GLenum mode, GLsizei count, GLenum type, const void *indices))
  SDL_PROC(EGLDisplay, eglGetCurrentDisplay, (void))
  SDL_PROC(EGLDisplay, eglGetDisplay, (EGLNativeDisplayType display_id))
  SDL_PROC(EGLImageKHR, eglCreateImageKHR,
           (EGLDisplay dpy, EGLContext ctx, EGLenum target,
            EGLClientBuffer buffer, const EGLint *attrib_list))
  SDL_PROC(EGLBoolean, eglDestroyImageKHR, (EGLDisplay dpy, EGLImageKHR image))
  SDL_PROC(EGLint, eglGetError, (void))
  SDL_PROC(void, glEGLImageTargetTexture2DOES,
           (GLenum target, GLeglImageOES image))
#undef SDL_PROC
  return 0;
}

typedef struct OpenGLVertexInfo {
  float x, y;               ///< Position
  float s0, t0;             ///< Texture coords
  float angle;              ///< angle
  float center_x, center_y; ///< translate center point
} OpenGLVertexInfo;

/* defines 2 triangles to display */
static const GLushort g_index[6] = {
    0, 1, 2, 0, 3, 2,
};

typedef struct shader_base_data {
  GLuint shader_program, shader_frag, shader_vert;
  GLuint vertex_buffer;       ///< Vertex buffer
  OpenGLVertexInfo vertex[4]; ///< VBO
  GLint position_attrib;      ///< Attibutes' locations
  GLint texture_coords_attrib;
  GLint angle_attrib;
  GLint center_attrib;
  GLint projection_matrix_location; ///< Uniforms' locations
  GLint model_view_matrix_location;
  GLint color_location;
} shader_base_data;

typedef struct shader_picture_base_data {
  shader_base_data base_data;

  GLuint texture_name[4]; ///< Textures' IDs
  GLint color_map_location;
  GLint chroma_div_w_location;
  GLint chroma_div_h_location;

  GLint texture_location[4];
  GLfloat color_map[16]; ///< RGBA color map matrix
  GLfloat chroma_div_w;  ///< Chroma subsampling w ratio
  GLfloat chroma_div_h;  ///< Chroma subsampling h ratio

  GLenum format;
  GLenum type;
  int width, height; // buffer w/h
} shader_picture_base_data;

typedef struct shader_data {
  shader_picture_base_data pic_base_data;

  GLint max_texture_size;    ///< Maximum texture size
  GLint max_viewport_width;  ///< Maximum viewport size
  GLint max_viewport_height; ///< Maximum viewport size
  int non_pow_2_textures;    ///< 1 when non power of 2 textures are supported
  int unpack_subimage;       ///< 1 when GL_EXT_unpack_subimage is available

  GLuint index_buffer; ///< Index buffer

  GLfloat projection_matrix[16]; ///< Projection matrix
  GLfloat model_view_matrix[16]; ///< Modev view matrix

  int window_width, window_height; // sdl window w/h
  int picture_width, picture_height;
} shader_data;

static const char *const OPENGL_VERTEX_SHADER =
    "uniform mat4 u_projectionMatrix;"
    "uniform mat4 u_modelViewMatrix;"

    "attribute vec2 a_position;"
    "attribute vec2 a_textureCoords;"

    "attribute float a_angle;"
    "attribute vec2 a_center;"

    "varying vec2 texture_coordinate;"

    "void main()"
    "{"
    "float angle = radians(a_angle);"
    "float c = cos(angle);"
    "float s = sin(angle);"
    "mat2 rotationMatrix = mat2(c, -s, s, c);"
    "vec2 position = rotationMatrix * (a_position - a_center) + a_center;"
    "gl_Position = u_projectionMatrix * (vec4(position, 0.0, 1.0) * "
    "u_modelViewMatrix);"
    "texture_coordinate = a_textureCoords;"
    "}";

// "#extension GL_OES_EGL_image_external : require\n"
// "uniform samplerExternalOES u_texture0;"
static const char *const OPENGL_FRAGMENT_SHADER_RGB_PACKET =
    "precision mediump float;"
    "uniform sampler2D u_texture0;"
    "uniform mat4 u_colorMap;"

    "varying vec2 texture_coordinate;"

    "void main()"
    "{"
    "gl_FragColor = vec4((texture2D(u_texture0, texture_coordinate) * "
    "u_colorMap).rgb, 1.0);"
    "}";

static const char *const OPENGL_FRAGMENT_SHADER_RGBA_PACKET =
    "precision mediump float;"
    "uniform sampler2D u_texture0;"
    "uniform mat4 u_colorMap;"

    "varying vec2 texture_coordinate;"

    "void main()"
    "{"
    "gl_FragColor = texture2D(u_texture0, texture_coordinate) * u_colorMap;"
    "}";

static const char *const OPENGL_FRAGMENT_SHADER_SOLID_PACKET = " \
    precision mediump float; \
    uniform vec4 u_color; \
    \
    void main() \
    { \
        gl_FragColor = u_color; \
    } \
";

static const struct OpenGLFormatDesc {
  Format pixel_format;
  AVPixelFormat av_pix_fmt;
  const char *const *fragment_shader;
  GLenum format;
  GLenum type;
} opengl_format_desc[] = {
    {Format::RGB888, AV_PIX_FMT_RGB24, &OPENGL_FRAGMENT_SHADER_RGB_PACKET,
     GL_RGB, GL_UNSIGNED_BYTE},
    {Format::RGBX, AV_PIX_FMT_RGBA, &OPENGL_FRAGMENT_SHADER_RGBA_PACKET,
     GL_RGBA, GL_UNSIGNED_BYTE}
    // TODO: Format::NV12
};

#define OPENGL_ERROR_CHECK()                                                   \
  {                                                                            \
    GLenum err_code;                                                           \
    if ((err_code = ctx.glGetError()) != GL_NO_ERROR) {                        \
      av_log(NULL, AV_LOG_FATAL,                                               \
             "OpenGL error occurred in '%s', line %d. <0x%x>\n", __FUNCTION__, \
             __LINE__, err_code);                                              \
      goto fail;                                                               \
    }                                                                          \
  }

#define EGL_ERROR_CHECK()                                                      \
  {                                                                            \
    EGLint err_code;                                                           \
    if ((err_code = ctx.eglGetError()) != EGL_SUCCESS) {                       \
      av_log(NULL, AV_LOG_FATAL,                                               \
             "OpenGL error occurred in '%s', line %d. <0x%x>\n", __FUNCTION__, \
             __LINE__, err_code);                                              \
      goto fail;                                                               \
    }                                                                          \
  }

static bool process_shader(GLES2_Context &ctx, GLuint *shader,
                           const char *source, GLint shader_type) {
  GLint status = GL_FALSE;
  const char *shaders[1] = {NULL};
  char buffer[1024];
  GLsizei length = 0;

  /* Create shader and load into GL. */
  *shader = ctx.glCreateShader(shader_type);
  OPENGL_ERROR_CHECK();
  shaders[0] = source;

  ctx.glShaderSource(*shader, 1, shaders, NULL);
  OPENGL_ERROR_CHECK();
  /* Clean up shader source. */
  shaders[0] = NULL;

  /* Try compiling the shader. */
  ctx.glCompileShader(*shader);
  OPENGL_ERROR_CHECK();
  ctx.glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
  OPENGL_ERROR_CHECK();

  /* Dump debug info (source and log) if compilation failed. */
  if (status != GL_TRUE) {
    ctx.glGetProgramInfoLog(*shader, sizeof(buffer) - 1, &length, buffer);
    buffer[length] = '\0';
    av_log(NULL, AV_LOG_FATAL, "Shader compilation failed: %s\n", buffer);
    ctx.glDeleteShader(*shader);
    return false;
  }

  return true;

fail:
  return false;
}

static const char *get_shader_frag_src(Format f) {
  for (size_t i = 0;
       i < (sizeof(opengl_format_desc) / sizeof((opengl_format_desc)[0]));
       i++) {
    if (opengl_format_desc[i].pixel_format == f)
      return *opengl_format_desc[i].fragment_shader;
  }
  return NULL;
}

static bool set_texture_params(shader_picture_base_data *spbd,
                               AVPixelFormat f) {
  for (size_t i = 0;
       i < (sizeof(opengl_format_desc) / sizeof((opengl_format_desc)[0]));
       i++) {
    if (opengl_format_desc[i].av_pix_fmt == f) {
      spbd->format = opengl_format_desc[i].format;
      spbd->type = opengl_format_desc[i].type;
      return true;
    }
  }
  return false;
}

static AVPixelFormat get_av_pixel_fmt(Format f) {
  switch (f) {
  case Format::NV12:
    return AV_PIX_FMT_NV12;
  case Format::NV16:
    return AV_PIX_FMT_NV16;
  case Format::RGB888:
    return AV_PIX_FMT_RGB24;
  case Format::RGBX:
    return AV_PIX_FMT_RGBA;
  }
  return AV_PIX_FMT_NONE;
}

static bool opengl_read_limits(GLES2_Context &ctx, shader_data *data) {
  static const struct {
    const char *extension;
    int major;
    int minor;
  } required_extensions[] = {
      {"GL_ARB_multitexture", 1, 3},
      {"GL_ARB_vertex_buffer_object", 1, 5}, // GLX_ARB_vertex_buffer_object
      {"GL_ARB_vertex_shader", 2, 0},
      {"GL_ARB_fragment_shader", 2, 0},
      {"GL_ARB_shader_objects", 2, 0},
      {NULL, 0, 0}};
  int i, j, major, minor;
  const char *extensions, *version;
  char major_minor[32], ch;

  version = reinterpret_cast<const char *>(ctx.glGetString(GL_VERSION));
  extensions = reinterpret_cast<const char *>(ctx.glGetString(GL_EXTENSIONS));

  av_log(NULL, AV_LOG_DEBUG, "OpenGL version: %s\n", version);
  sprintf(major_minor, "1.0");
  i = j = 0;
  while (true) {
    ch = version[i];
    if (!ch || (ch >= '0' && ch <= '9'))
      break;
    while (version[i++] != ' ')
      ;
  }
  if (ch) {
    while (ch && ch != ' ') {
      major_minor[j++] = ch;
      ch = version[++i];
    }
  }
  sscanf(major_minor, "%d.%d", &major, &minor);
  printf("set version to %d.%d\n", major, minor);

  for (i = 0; required_extensions[i].extension; i++) {
    if (major < required_extensions[i].major &&
        (major == required_extensions[i].major &&
         minor < required_extensions[i].minor) &&
        !strstr(extensions, required_extensions[i].extension)) {
      av_log(NULL, AV_LOG_ERROR, "Required extension %s is not supported.\n",
             required_extensions[i].extension);
      av_log(NULL, AV_LOG_INFO, "Supported extensions are: %s\n", extensions);
      return AVERROR(ENOSYS);
    }
  }
  ctx.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &data->max_texture_size);
  ctx.glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &data->max_viewport_width);
  data->non_pow_2_textures =
      major >= 2 || strstr(extensions, "GL_ARB_texture_non_power_of_two");
  data->unpack_subimage = !!strstr(extensions, "GL_EXT_unpack_subimage");

  av_log(NULL, AV_LOG_INFO, "Non Power of 2 textures support: %s\n",
         data->non_pow_2_textures ? "Yes" : "No");
  av_log(NULL, AV_LOG_INFO, "Unpack Subimage extension support: %s\n",
         data->unpack_subimage ? "Yes" : "No");
  av_log(NULL, AV_LOG_INFO, "Max texture size: %dx%d\n", data->max_texture_size,
         data->max_texture_size);
  av_log(NULL, AV_LOG_INFO, "Max viewport size: %dx%d\n",
         data->max_viewport_width, data->max_viewport_height);

  if (FLAGS_width > (uint)data->max_viewport_width ||
      FLAGS_height > (uint)data->max_viewport_height) {
    av_log(NULL, AV_LOG_ERROR,
           "Too big picture %dx%d, max supported size is %dx%d\n", FLAGS_width,
           FLAGS_height, data->max_texture_size, data->max_texture_size);
    goto fail;
  }

  OPENGL_ERROR_CHECK();
  return true;
fail:
  return false;
}

#define GL_UNSIGNED_BYTE_3_3_2 0x8032
#define GL_UNSIGNED_BYTE_2_3_3_REV 0x8362

static int opengl_type_size(GLenum type) {
  switch (type) {
  case GL_UNSIGNED_SHORT:
  case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:
  case GL_UNSIGNED_SHORT_5_6_5:
    return 2;
  case GL_UNSIGNED_BYTE:
  case GL_UNSIGNED_BYTE_3_3_2:
  case GL_UNSIGNED_BYTE_2_3_3_REV:
  default:
    break;
  }
  return 1;
}

static void opengl_get_texture_size(shader_data *data, int in_width,
                                    int in_height, int *out_width,
                                    int *out_height) {
  if (data->non_pow_2_textures) {
    *out_width = in_width;
    *out_height = in_height;
  } else {
    int max = std::min<int>(std::max<int>(in_width, in_height),
                            data->max_texture_size);
    int power_of_2 = 1;
    while (power_of_2 < max)
      power_of_2 *= 2;
    *out_height = power_of_2;
    *out_width = power_of_2;
    av_log(NULL, AV_LOG_DEBUG,
           "Texture size calculated from %dx%d into %dx%d\n", in_width,
           in_height, *out_width, *out_height);
  }
}

static bool opengl_configure_texture(GLES2_Context &ctx, shader_data *data,
                                     GLuint texture, GLsizei width,
                                     GLsizei height) {
  if (texture) {
    int new_width, new_height;
    opengl_get_texture_size(data, width, height, &new_width, &new_height);
    ctx.glBindTexture(GL_TEXTURE_2D, texture);
    ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // printf("opengl_configure_texture, w,h: %d, %d\n", new_width, new_height);
    // ctx.glTexImage2D(GL_TEXTURE_2D, 0, data->format, new_width, new_height,
    // 0,
    //                  data->format, data->type, NULL);
    OPENGL_ERROR_CHECK();
  }
  return true;
fail:
  return false;
}

static void opengl_make_identity(float matrix[16]) {
  memset(matrix, 0, 16 * sizeof(float));
  matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
}

static void opengl_make_ortho(float matrix[16], float left, float right,
                              float bottom, float top, float nearZ,
                              float farZ) {
  float ral = right + left;
  float rsl = right - left;
  float tab = top + bottom;
  float tsb = top - bottom;
  float fan = farZ + nearZ;
  float fsn = farZ - nearZ;

  memset(matrix, 0, 16 * sizeof(float));
  matrix[0] = 2.0f / rsl;
  matrix[5] = 2.0f / tsb;
  matrix[10] = -2.0f / fsn;
  matrix[12] = -ral / rsl;
  matrix[13] = -tab / tsb;
  matrix[14] = -fan / fsn;
  matrix[15] = 1.0f;
}

static void opengl_compute_display_area(shader_data *data) {
  AVRational sar, dar; /* sample and display aspect ratios */
  int width = data->pic_base_data.width, height = data->pic_base_data.height;
  OpenGLVertexInfo *vertex = data->pic_base_data.base_data.vertex;
  bool swap_wh = (FLAGS_disp_rotate == 90 || FLAGS_disp_rotate == 270);

  if (swap_wh)
    std::swap<int>(width, height);

  /* compute overlay width and height */
  sar = (AVRational){1, 1};
  dar = av_mul_q(sar, (AVRational){width, height});

  /* we suppose the screen has a 1/1 sample aspect ratio */
  /* fit in the window */
  if (av_cmp_q(dar, (AVRational){data->window_width, data->window_height}) >
      0) {
    /* fit in width */
    data->picture_width = data->window_width;
    data->picture_height = av_rescale(data->picture_width, dar.den, dar.num);
  } else {
    /* fit in height */
    data->picture_height = data->window_height;
    data->picture_width = av_rescale(data->picture_height, dar.num, dar.den);
  }

  if (swap_wh)
    std::swap<int>(data->picture_width, data->picture_height);

  vertex[0].x = vertex[1].x = -(float)data->picture_width / 2.0f;
  vertex[2].x = vertex[3].x = (float)data->picture_width / 2.0f;
  vertex[1].y = vertex[2].y = -(float)data->picture_height / 2.0f;
  vertex[0].y = vertex[3].y = (float)data->picture_height / 2.0f;

  int tex_w, tex_h;
  opengl_get_texture_size(data, width, height, &tex_w, &tex_h);
  vertex[0].s0 = 0.0f;
  vertex[0].t0 = 0.0f;
  vertex[1].s0 = 0.0f;
  vertex[1].t0 = (float)height / (float)tex_h;
  vertex[2].s0 = (float)width / (float)tex_w;
  vertex[2].t0 = (float)height / (float)tex_h;
  vertex[3].s0 = (float)width / (float)tex_w;
  vertex[3].t0 = 0.0f;

  vertex[0].angle = vertex[1].angle = vertex[2].angle = vertex[3].angle =
      (float)FLAGS_disp_rotate;

  vertex[0].center_x = vertex[1].center_x = vertex[2].center_x =
      vertex[3].center_x = 0.0f;
  // (float)data->picture_width / 2.0f;

  vertex[0].center_y = vertex[1].center_y = vertex[2].center_y =
      vertex[3].center_y = 0.0f;
  // (float)data->picture_height / 2.0f;

  if (1) {
    printf("picture w,h: %d, %d; tex w, h: %d, %d\n", data->picture_width,
           data->picture_height, tex_w, tex_h);
    // dump vertex
    for (int i = 0; i < 4; i++) {
      printf("\tvertex[%d]: %f, %f; %f, %f; %f; %f, %f\n", i, vertex[i].x,
             vertex[i].y, vertex[i].s0, vertex[i].t0, vertex[i].angle,
             vertex[i].center_x, vertex[i].center_y);
    }
  }
}

#endif // #if DRAW_BY_OPENGLES

static SDL_Color white = {0xFF, 0xFF, 0xFF, 0x00};
static SDL_Color red = {0x00, 0x00, 0xFF, 0xFF};
static SDL_Color title_color = {0x06, 0xEB, 0xFF, 0xFF};
class SDLTTF {
public:
  SDLTTF() {
    if (TTF_Init() < 0)
      fprintf(stderr, "Couldn't initialize TTF: %s\n", SDL_GetError());
  }
  ~SDLTTF() { TTF_Quit(); }
};
static SDLTTF sdl_ttf;
class SDLFont {
public:
  SDLFont(SDL_Color forecol, int ptsize);
  ~SDLFont();
  SDL_Surface *DrawString(char *str, int str_length);
  SDL_Surface *GetFontPicture(char *str, int str_length, int bpp, int *w,
                              int *h);
#if DRAW_BY_OPENGLES
  GLuint GetFontTexture(const GLES2_Context &ctx, char *str, int str_length,
                        int bpp, int *w, int *h);
#endif
  SDL_Color fore_col;
  SDL_Color back_col;
  int renderstyle;
  enum { RENDER_LATIN1, RENDER_UTF8, RENDER_UNICODE } rendertype;
  int pt_size;
  TTF_Font *font;
};

SDLFont::SDLFont(SDL_Color forecol, int ptsize)
    : fore_col(forecol), back_col(white), renderstyle(TTF_STYLE_NORMAL),
      rendertype(RENDER_UTF8), pt_size(ptsize), font(NULL) {
  font = TTF_OpenFont(FLAGS_font.c_str(), ptsize);
  if (font == NULL) {
    fprintf(stderr, "hehe Couldn't load %d pt font from %s: %s\n", ptsize,
            FLAGS_font.c_str(), SDL_GetError());
    return;
  }
  TTF_SetFontStyle(font, renderstyle);
}

SDLFont::~SDLFont() {
  if (font) {
    TTF_CloseFont(font);
  }
}

SDL_Surface *SDLFont::DrawString(char *str, int str_length) {
  SDL_Surface *text = NULL;
  switch (rendertype) {
  case RENDER_LATIN1:
    text = TTF_RenderText_Blended(font, str, fore_col);
    break;

  case RENDER_UTF8:
    text = TTF_RenderUTF8_Blended(font, str, fore_col);
    break;

  case RENDER_UNICODE: {
    Uint16 *unicode_text = (Uint16 *)malloc(2 * str_length + 1);
    if (!unicode_text)
      break;
    int index;
    for (index = 0; (str[0] || str[1]); ++index) {
      unicode_text[index] = ((Uint8 *)str)[0];
      unicode_text[index] <<= 8;
      unicode_text[index] |= ((Uint8 *)str)[1];
      str += 2;
    }
    unicode_text[index] = 0;
    text = TTF_RenderUNICODE_Blended(font, unicode_text, fore_col);
    free(unicode_text);
  } break;
  default:
    /* This shouldn't happen */
    break;
  }
  return text;
}

// static int power_of_two(int input)
// {
//     int value = 1;

//     while ( value < input ) {
//         value <<= 1;
//     }
//     return value;
// }

SDL_Surface *SDLFont::GetFontPicture(char *str, int str_length, int bpp, int *w,
                                     int *h) {
  SDL_Surface *image;
  SDL_Rect area;
  // Uint8  saved_alpha;
  // SDL_BlendMode saved_mode;
  if (str_length <= 0)
    return NULL;
  SDL_Surface *text = DrawString(str, str_length);
  if (!text) {
    av_log(NULL, AV_LOG_FATAL, "draw %s to picture failed\n", str);
    return NULL;
  }
  *w = text->w; // power_of_two(text->w);
  *h = text->h; // power_of_two(text->h);
  if (bpp == 32)
    return text;
  image = SDL_CreateRGBSurfaceWithFormat(SDL_SWSURFACE, *w, *h, bpp,
                                         SDL_PIXELFORMAT_RGB24);
  if (image == NULL) {
    av_log(NULL, AV_LOG_FATAL, "SDL_CreateRGBSurface failed: %s\n",
           SDL_GetError());
    return NULL;
  }
  /* Save the alpha blending attributes */
  // SDL_GetSurfaceAlphaMod(text, &saved_alpha);
  // SDL_SetSurfaceAlphaMod(text, 0xFF);
  // SDL_GetSurfaceBlendMode(text, &saved_mode);
  // SDL_SetSurfaceBlendMode(text, SDL_BLENDMODE_NONE);
  /* Copy the text into the GL texture image */
  area.x = 0;
  area.y = 0;
  area.w = text->w;
  area.h = text->h;
  SDL_BlitSurface(text, &area, image, &area);
  /* Restore the alpha blending attributes */
  // SDL_SetSurfaceAlphaMod(text, saved_alpha);
  // SDL_SetSurfaceBlendMode(text, saved_mode);
  SDL_FreeSurface(text);
  return image;
}

#if DRAW_BY_OPENGLES
GLuint SDLFont::GetFontTexture(const GLES2_Context &ctx, char *str,
                               int str_length, int bpp, int *w, int *h) {
  GLuint texture;
  GLenum format;
  SDL_Surface *image = GetFontPicture(str, str_length, bpp, w, h);
  if (!image)
    return 0;
  switch (bpp) {
  case 24:
    format = GL_RGB;
    break;
  case 32:
    format = GL_RGBA;
    break;
  default:
    av_log(NULL, AV_LOG_FATAL, "Invalid bpp %d\n", bpp);
    return 0;
  }
  ctx.glGenTextures(1, &texture);
  ctx.glBindTexture(GL_TEXTURE_2D, texture);
  ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  ctx.glTexImage2D(GL_TEXTURE_2D, 0, format, image->w, image->h, 0, format,
                   GL_UNSIGNED_BYTE, image->pixels);
  OPENGL_ERROR_CHECK();
  SDL_FreeSurface(image);
  return texture;

fail:
  if (image)
    SDL_FreeSurface(image);
  return 0;
}
#endif

class NpuRect {
public:
  int left, top;
  int right, bottom;
  std::string word;
};
class NpuRectList {
public:
  int device_id;
  int64_t got_time;
  int64_t timeout;
  std::list<NpuRect> list;
};

class SDLDisplayLeaf : public LeafHandler {
public:
  SDLDisplayLeaf(int drmfd);
  virtual void SubRun() override;

  bool SDLPrepare();
  void SDLDestroy();
  void SetFaceSliceNum(int num);
  void SetFaceRectAt(int index, std::shared_ptr<NpuRectList> nrl);
  void get_face_rects(std::vector<std::shared_ptr<NpuRectList>> &nrl_vec);
#if DRAW_BY_SDL
  static int get_texture_fmt(Format f);
  int realloc_texture(SDL_Texture **texture, Uint32 new_format, int new_width,
                      int new_height, SDL_BlendMode blendmode);
#endif
#if DRAW_BY_OPENGLES
  int draw_rect(const Rect &irect);
  bool draw_texture(shader_picture_base_data &spdb, GLuint texture);
  void draw_texture_in_rect(shader_picture_base_data &spdb, GLuint texture,
                            Rect rect);
  void update_vector_array_buffer(GLuint buffer, GLsizeiptr size,
                                  const GLvoid *data, GLenum usage);
  bool init_shader_base_data(shader_base_data *sbd, const char *shader_vert_src,
                             const char *shader_frag_src);
  void deinit_shader_base_data(shader_base_data *sbd);

  bool init_shader_picture_base_data(shader_picture_base_data *spbd,
                                     const char *shader_vert_src,
                                     const char *shader_frag_src,
                                     enum AVPixelFormat pix_fmt,
                                     int buffer_width, int buffer_height);
  void deinit_shader_picture_base_data(shader_picture_base_data *spbd);
#endif

  int sdl_prepared; // hack for sdl hdmi
  SDL_Window *window;
  int drm_fd;

#if DRAW_BY_SDL
  SDL_Renderer *renderer;
  SDL_RendererInfo renderer_info;
  int texture_fmt;
  SDL_Texture *texture;

  RockchipRga rga;
  JoinBO jb;
#endif
#if DRAW_BY_OPENGLES
  SDL_GLContext gl_ctx;
  GLES2_Context ctx;
  shader_data sd;           // draw the video picture
  shader_base_data sd_line; // draw lines

  shader_picture_base_data rgba_sd; // draw rgba picture, include font and logo

  GLuint font_vertex_buffer;       ///< font Vertex buffer
  OpenGLVertexInfo font_vertex[4]; ///< font VBO
#endif

  SDLFont *sdl_font;
  std::vector<spinlock_mutex> face_mtxs;
  std::vector<std::shared_ptr<NpuRectList>> face_rects;
};

void SDLDisplayLeaf::SetFaceSliceNum(int num) {
  face_mtxs.resize(num);
  face_rects.resize(num);
}

void SDLDisplayLeaf::SetFaceRectAt(int index,
                                   std::shared_ptr<NpuRectList> nrl) {
  face_mtxs[index].lock();
  face_rects[index] = nrl;
  face_mtxs[index].unlock();
}

void SDLDisplayLeaf::get_face_rects(
    std::vector<std::shared_ptr<NpuRectList>> &nrl_vec) {
  int64_t now = av_gettime_relative() / 1000;
  for (size_t i = 0; i < face_mtxs.size(); i++) {
    spinlock_mutex &smtx = face_mtxs[i];
    smtx.lock();
    std::shared_ptr<NpuRectList> &nrl = face_rects[i];
    if (nrl) {
      if (now - nrl->got_time < nrl->timeout)
        nrl_vec[i] = nrl;
      else
        nrl_vec[i].reset();
    }
    smtx.unlock();
  }
}

SDLDisplayLeaf::SDLDisplayLeaf(int drmfd)
    : sdl_prepared(0), window(NULL), drm_fd(drmfd),
#if DRAW_BY_SDL
      renderer(NULL), texture_fmt(-1), texture(NULL) {
  memset(&renderer_info, 0, sizeof(renderer_info));
  memset(&jb, 0, sizeof(jb));
  sdl_font = NULL;
}
#elif DRAW_BY_OPENGLES
      gl_ctx(NULL) {
  memset(&ctx, 0, sizeof(ctx));
  memset(&sd, 0, sizeof(sd));
  memset(&sd_line, 0, sizeof(sd_line));
  memset(&rgba_sd, 0, sizeof(rgba_sd));
  font_vertex_buffer = 0;
  memset(font_vertex, 0, sizeof(font_vertex));
  sdl_font = NULL;
}
#endif

#if DRAW_BY_SDL
static int get_sdl_format(Format f) {
  static std::map<Format, int> sdl_format_map;
  if (sdl_format_map.empty()) {
    sdl_format_map[Format::NV12] = SDL_PIXELFORMAT_NV12;
    sdl_format_map[Format::RGB888] = SDL_PIXELFORMAT_RGB24;
    sdl_format_map[Format::RGBX] = SDL_PIXELFORMAT_RGBX8888;
  }
  auto it = sdl_format_map.find(f);
  if (it != sdl_format_map.end())
    return it->second;
  return SDL_PIXELFORMAT_UNKNOWN;
}

int SDLDisplayLeaf::get_texture_fmt(Format f) { return get_sdl_format(f); }

int SDLDisplayLeaf::realloc_texture(SDL_Texture **tex, Uint32 new_format,
                                    int new_width, int new_height,
                                    SDL_BlendMode blendmode) {
  Uint32 format;
  int access, w, h;
  if (!*tex || SDL_QueryTexture(*tex, &format, &access, &w, &h) < 0 ||
      new_width != w || new_height != h || new_format != format) {
    if (*tex)
      SDL_DestroyTexture(*tex);
    if (!(*tex = SDL_CreateTexture(renderer, new_format,
                                   SDL_TEXTUREACCESS_STREAMING, new_width,
                                   new_height)))
      return -1;
    if (SDL_SetTextureBlendMode(*tex, blendmode) < 0)
      return -1;
    av_log(NULL, AV_LOG_INFO, "Created %dx%d texture with %s.\n", new_width,
           new_height, SDL_GetPixelFormatName(new_format));
  }
  return 0;
}
#endif

#if DRAW_BY_OPENGLES
void SDLDisplayLeaf::update_vector_array_buffer(GLuint buffer, GLsizeiptr size,
                                                const GLvoid *data,
                                                GLenum usage) {
  ctx.glBindBuffer(GL_ARRAY_BUFFER, buffer);
  ctx.glBufferData(GL_ARRAY_BUFFER, size, data, usage);
  ctx.glBindBuffer(GL_ARRAY_BUFFER, 0);
}
bool SDLDisplayLeaf::init_shader_base_data(shader_base_data *sbd,
                                           const char *shader_vert_src,
                                           const char *shader_frag_src) {
  if (!process_shader(ctx, &sbd->shader_vert, shader_vert_src,
                      GL_VERTEX_SHADER))
    goto fail;
  if (!process_shader(ctx, &sbd->shader_frag, shader_frag_src,
                      GL_FRAGMENT_SHADER))
    goto fail;
  sbd->shader_program = ctx.glCreateProgram();
  OPENGL_ERROR_CHECK();
  ctx.glAttachShader(sbd->shader_program, sbd->shader_vert);
  OPENGL_ERROR_CHECK();
  ctx.glAttachShader(sbd->shader_program, sbd->shader_frag);
  OPENGL_ERROR_CHECK();
  ctx.glLinkProgram(sbd->shader_program);
  OPENGL_ERROR_CHECK();
  sbd->angle_attrib = ctx.glGetAttribLocation(sbd->shader_program, "a_angle");
  sbd->center_attrib = ctx.glGetAttribLocation(sbd->shader_program, "a_center");
  sbd->position_attrib =
      ctx.glGetAttribLocation(sbd->shader_program, "a_position");
  sbd->texture_coords_attrib =
      ctx.glGetAttribLocation(sbd->shader_program, "a_textureCoords");
  sbd->projection_matrix_location =
      ctx.glGetUniformLocation(sbd->shader_program, "u_projectionMatrix");
  sbd->model_view_matrix_location =
      ctx.glGetUniformLocation(sbd->shader_program, "u_modelViewMatrix");
  sbd->color_location =
      ctx.glGetUniformLocation(sbd->shader_program, "u_color");

  ctx.glGenBuffers(1, &sbd->vertex_buffer);
  if (!sbd->vertex_buffer) {
    av_log(NULL, AV_LOG_ERROR, "Vertex buffer generation failed.\n");
    goto fail;
  }
  OPENGL_ERROR_CHECK();
  return true;

fail:
  return false;
}

void SDLDisplayLeaf::deinit_shader_base_data(shader_base_data *sbd) {
  if (ctx.glDeleteProgram) {
    ctx.glDeleteProgram(sbd->shader_program);
    sbd->shader_program = 0;
  }
  if (ctx.glDeleteShader) {
    if (sbd->shader_vert) {
      ctx.glDeleteShader(sbd->shader_vert);
      sbd->shader_vert = 0;
    }
    if (sbd->shader_frag) {
      ctx.glDeleteShader(sbd->shader_frag);
      sbd->shader_frag = 0;
    }
  }
  if (ctx.glDeleteBuffers) {
    ctx.glDeleteBuffers(1, &sbd->vertex_buffer);
    sbd->vertex_buffer = 0;
  }
}

bool SDLDisplayLeaf::init_shader_picture_base_data(
    shader_picture_base_data *spbd, const char *shader_vert_src,
    const char *shader_frag_src, enum AVPixelFormat pix_fmt, int buffer_width,
    int buffer_height) {
  const AVPixFmtDescriptor *desc;
  int has_alpha;

  spbd->color_map_location = -1;
  spbd->chroma_div_w_location = -1;
  spbd->chroma_div_h_location = -1;
  bool ret =
      init_shader_base_data(&spbd->base_data, shader_vert_src, shader_frag_src);
  if (!ret)
    return ret;
  spbd->width = buffer_width;
  spbd->height = buffer_height;
  shader_base_data &base_data = spbd->base_data;
  spbd->texture_location[0] =
      ctx.glGetUniformLocation(base_data.shader_program, "u_texture0");
  spbd->texture_location[1] =
      ctx.glGetUniformLocation(base_data.shader_program, "u_texture1");
  spbd->texture_location[2] =
      ctx.glGetUniformLocation(base_data.shader_program, "u_texture2");
  spbd->texture_location[3] =
      ctx.glGetUniformLocation(base_data.shader_program, "u_texture3");
  spbd->color_map_location =
      ctx.glGetUniformLocation(base_data.shader_program, "u_colorMap");
  spbd->chroma_div_w_location =
      ctx.glGetUniformLocation(base_data.shader_program, "u_chroma_div_w");
  spbd->chroma_div_h_location =
      ctx.glGetUniformLocation(base_data.shader_program, "u_chroma_div_h");
  OPENGL_ERROR_CHECK();

  desc = av_pix_fmt_desc_get(pix_fmt);
  has_alpha = desc->flags & AV_PIX_FMT_FLAG_ALPHA;
  if (desc->flags & AV_PIX_FMT_FLAG_RGB) {
    int shift;

#define FILL_COMPONENT(i)                                                      \
  {                                                                            \
    shift = (desc->comp[i].depth - 1) >> 3;                                    \
    spbd->color_map[(i << 2) + (desc->comp[i].offset >> shift)] = 1.0;         \
  }

    memset(spbd->color_map, 0, sizeof(spbd->color_map));
    FILL_COMPONENT(0);
    FILL_COMPONENT(1);
    FILL_COMPONENT(2);
    if (has_alpha)
      FILL_COMPONENT(3);

#undef FILL_COMPONENT
  }
  if (!set_texture_params(spbd, pix_fmt))
    goto fail;
  av_assert0(desc->nb_components > 0 && desc->nb_components <= 4);
  if (spbd->width > 0 && spbd->height > 0)
    ctx.glGenTextures(desc->nb_components, spbd->texture_name);

  opengl_configure_texture(ctx, &sd, spbd->texture_name[0], spbd->width,
                           spbd->height);
  if (desc->nb_components > 1 && (desc->flags & AV_PIX_FMT_FLAG_PLANAR)) {
    int num_planes = desc->nb_components - (has_alpha ? 1 : 0);
    if (sd.non_pow_2_textures) {
      spbd->chroma_div_w = 1.0f;
      spbd->chroma_div_h = 1.0f;
    } else {
      spbd->chroma_div_w = 1 << desc->log2_chroma_w;
      spbd->chroma_div_h = 1 << desc->log2_chroma_h;
    }
    for (int i = 1; i < num_planes; i++)
      if (sd.non_pow_2_textures)
        opengl_configure_texture(
            ctx, &sd, spbd->texture_name[i],
            AV_CEIL_RSHIFT(spbd->width, desc->log2_chroma_w),
            AV_CEIL_RSHIFT(spbd->height, desc->log2_chroma_h));
      else
        opengl_configure_texture(ctx, &sd, spbd->texture_name[i], spbd->width,
                                 spbd->height);
    if (has_alpha)
      opengl_configure_texture(ctx, &sd, spbd->texture_name[3], spbd->width,
                               spbd->height);
  }
  if (has_alpha) {
    av_log(NULL, AV_LOG_INFO, "gl enable blend\n");
    ctx.glEnable(GL_BLEND);
    ctx.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  OPENGL_ERROR_CHECK();
  return true;

fail:
  return false;
}
void SDLDisplayLeaf::deinit_shader_picture_base_data(
    shader_picture_base_data *spbd) {
  if (spbd->texture_name[0]) {
    ctx.glDeleteTextures(4, spbd->texture_name);
    spbd->texture_name[0] = spbd->texture_name[1] = 0;
    spbd->texture_name[2] = spbd->texture_name[3] = 0;
  }
  deinit_shader_base_data(&spbd->base_data);
}

#endif

static const float inv255f = 1.0f / 255.0f;

bool SDLDisplayLeaf::SDLPrepare() {
#if SDL_RGA
  if (!rga.RkRgaIsReady())
    return false;
#endif

  SDL_LogSetPriority(SDL_LOG_CATEGORY_VIDEO, SDL_LOG_PRIORITY_VERBOSE);
  SDL_LogSetPriority(SDL_LOG_CATEGORY_RENDER, SDL_LOG_PRIORITY_VERBOSE);

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
    av_log(NULL, AV_LOG_FATAL, "Could not initialize SDL - %s\n",
           SDL_GetError());
    av_log(NULL, AV_LOG_FATAL, "(Did you set the DISPLAY variable?)\n");
    return false;
  }
  SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
  SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

  int flags = SDL_WINDOW_SHOWN;
  flags |= SDL_WINDOW_FULLSCREEN;
  flags |= SDL_WINDOW_OPENGL;
  // flags |= SDL_WINDOW_BORDERLESS;
  flags |= SDL_WINDOW_RESIZABLE;
  window = SDL_CreateWindow("NPU Demo", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, 1280, 720, flags);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  if (!window) {
    av_log(NULL, AV_LOG_FATAL, "Failed to create SDL window %s\n",
           SDL_GetError());
    goto fail;
  }

#if DRAW_BY_SDL
  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    av_log(NULL, AV_LOG_WARNING,
           "Failed to initialize a hardware accelerated renderer: %s\n",
           SDL_GetError());
    // renderer = SDL_CreateRenderer(window, -1, 0);
    goto fail;
  }

  if (!SDL_GetRendererInfo(renderer, &renderer_info))
    av_log(NULL, AV_LOG_INFO, "Initialized %s renderer\n", renderer_info.name);

  if (!renderer_info.num_texture_formats) {
    av_log(NULL, AV_LOG_FATAL, "Broken SDL renderer %s\n", renderer_info.name);
    goto fail;
  }
  texture_fmt = get_texture_fmt(static_cast<Format>(FLAGS_format));
  if (texture_fmt == SDL_PIXELFORMAT_UNKNOWN)
    return false;
#endif

#if DRAW_BY_OPENGLES
  AVPixelFormat pix_fmt;
  const char *shader_frag_src;
  shader_data *data;

  int value;
  int status;

  gl_ctx = SDL_GL_CreateContext(window);
  if (!gl_ctx) {
    av_log(NULL, AV_LOG_FATAL, "SDL_GL_CreateContext(): %s\n", SDL_GetError());
    goto fail;
  }
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  if (LoadContext(&ctx) < 0) {
    av_log(NULL, AV_LOG_FATAL, "Could not load GLES2 functions\n");
    goto fail;
  }
  SDL_GL_SetSwapInterval(1);
  SDL_DisplayMode mode;
  SDL_GetCurrentDisplayMode(0, &mode);
  printf("Screen bpp: %d\n", SDL_BITSPERPIXEL(mode.format));
  printf("\n");
  printf("Vendor     : %s\n", ctx.glGetString(GL_VENDOR));
  printf("Renderer   : %s\n", ctx.glGetString(GL_RENDERER));
  printf("Version    : %s\n", ctx.glGetString(GL_VERSION));
  printf("Extensions : %s\n", ctx.glGetString(GL_EXTENSIONS));
  printf("\n");

#define GetAttribute(attr)                                                     \
  status = SDL_GL_GetAttribute(attr, &value);                                  \
  if (!status) {                                                               \
    printf(#attr ": %d\n", value);                                             \
  } else {                                                                     \
    printf("Failed to get " #attr ": %s\n", SDL_GetError());                   \
    goto fail;                                                                 \
  }
  GetAttribute(SDL_GL_GREEN_SIZE) GetAttribute(SDL_GL_GREEN_SIZE)
      GetAttribute(SDL_GL_BLUE_SIZE) GetAttribute(SDL_GL_DEPTH_SIZE) status =
          SDL_GL_MakeCurrent(window, gl_ctx);
  if (status) {
    av_log(NULL, AV_LOG_FATAL, "SDL_GL_MakeCurrent(): %s\n", SDL_GetError());
    goto fail;
  }

  data = &sd;
  SDL_GL_GetDrawableSize(window, &data->window_width, &data->window_height);
  if (data->window_width < data->window_height) {
    FLAGS_disp_rotate = 90;
  } else {
    FLAGS_disp_rotate = 0;
  }
  ctx.glViewport(0, 0, data->window_width, data->window_height);
  av_log(NULL, AV_LOG_INFO, "window size: %d x %d\n", data->window_width,
         data->window_height);

  if (!opengl_read_limits(ctx, data)) {
    av_log(NULL, AV_LOG_FATAL, "opengl_read_limits fail\n");
    goto fail;
  }

  if (!init_shader_base_data(&sd_line, OPENGL_VERTEX_SHADER,
                             OPENGL_FRAGMENT_SHADER_SOLID_PACKET))
    goto fail;

  shader_frag_src = get_shader_frag_src(static_cast<Format>(FLAGS_format));
  if (!shader_frag_src) {
    av_log(NULL, AV_LOG_FATAL, "get_shader_frag_src fail\n");
    goto fail;
  }

  pix_fmt = get_av_pixel_fmt(static_cast<Format>(FLAGS_format));
  if (!init_shader_picture_base_data(&data->pic_base_data, OPENGL_VERTEX_SHADER,
                                     shader_frag_src, pix_fmt, FLAGS_width,
                                     FLAGS_height))
    goto fail;
  av_assert0(data->pic_base_data.texture_name[0] > 0);

  ctx.glGenBuffers(1, &data->index_buffer);
  if (!data->index_buffer) {
    av_log(NULL, AV_LOG_ERROR, "Index buffer generation failed.\n");
    goto fail;
  }
  ctx.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data->index_buffer);
  ctx.glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_index), g_index,
                   GL_STATIC_DRAW);
  ctx.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  OPENGL_ERROR_CHECK();

  opengl_make_ortho(data->projection_matrix, -(float)data->window_width / 2.0f,
                    (float)data->window_width / 2.0f,
                    -(float)data->window_height / 2.0f,
                    (float)data->window_height / 2.0f, 1.0f, -1.0f);
  opengl_make_identity(data->model_view_matrix);
  opengl_compute_display_area(data);

  update_vector_array_buffer(data->pic_base_data.base_data.vertex_buffer,
                             sizeof(data->pic_base_data.base_data.vertex),
                             data->pic_base_data.base_data.vertex,
                             GL_STATIC_DRAW);
  OPENGL_ERROR_CHECK();
  shader_frag_src = get_shader_frag_src(Format::RGBX);
  if (!shader_frag_src) {
    av_log(NULL, AV_LOG_FATAL, "get_shader_frag_src fail\n");
    goto fail;
  }
  if (!init_shader_picture_base_data(&rgba_sd, OPENGL_VERTEX_SHADER,
                                     shader_frag_src, AV_PIX_FMT_RGBA, 0, 0))
    goto fail;
#if 0
    ctx.glGenBuffers(1, &font_vertex_buffer);
    if (!font_vertex_buffer) {
        av_log(NULL, AV_LOG_ERROR, "Font Vertex buffer generation failed.\n");
        goto fail;
    }
#endif

  // ctx.glClearColor(0.0f, (GLfloat)0x16 * inv255f, (GLfloat)0x31 * inv255f,
  // 0.5f);
  ctx.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  ctx.glClear(GL_COLOR_BUFFER_BIT);
  OPENGL_ERROR_CHECK();
  SDL_GL_SwapWindow(window);

#endif // #if DRAW_BY_OPENGLES

  return true;

fail:
  SDLDestroy();
  return false;
}

void SDLDisplayLeaf::SDLDestroy() {
#if DRAW_BY_SDL
  if (texture) {
    SDL_DestroyTexture(texture);
    texture = NULL;
  }
  if (renderer) {
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
  }
#endif

#if DRAW_BY_OPENGLES
  if (ctx.glUseProgram)
    ctx.glUseProgram(0);
  if (ctx.glBindBuffer) {
    ctx.glBindBuffer(GL_ARRAY_BUFFER, 0);
    ctx.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
  deinit_shader_picture_base_data(&sd.pic_base_data);
  if (ctx.glDeleteBuffers) {
    ctx.glDeleteBuffers(1, &sd.index_buffer);
    sd.index_buffer = 0;
  }
  deinit_shader_base_data(&sd_line);
#if 0
    if (ctx.glDeleteBuffers) {
        ctx.glDeleteBuffers(1, &font_vertex_buffer);
        font_vertex_buffer = 0;
    }
#endif
  SDL_GL_DeleteContext(gl_ctx);
#endif

  if (window) {
    SDL_DestroyWindow(window);
    window = NULL;
  }

#if SDL_RGA
  FreeJoinBO(drm_fd, rga, &jb);
#endif

  SDL_Quit();
}

static int set_rga_ratation(const uint32_t rdegree, int *rotation) {
  // TODO: FLIP_H/FLIP_V
  switch (rdegree) {
  case 0:
    *rotation = 0;
    break;
  case 90:
    *rotation = HAL_TRANSFORM_ROT_90;
    break;
  case 180:
    *rotation = HAL_TRANSFORM_ROT_180;
    break;
  case 270:
    *rotation = HAL_TRANSFORM_ROT_270;
    break;
  default:
    av_log(NULL, AV_LOG_FATAL,
           "except 0/90/180/270 other rotate degree unsupported\n");
    return -1;
  }
  return 0;
}

#if DRAW_BY_SDL
void SDLDisplayLeaf::SubRun() {
  if (!SDLPrepare()) {
    sdl_prepared = -1;
    return;
  }
  int displayIndex = -1;
  SDL_Rect rect = {0, 0, 0, 0};
  SDL_Event event;
  rga_info_t src, dst;
  int rotation = 0;
  static int src_format = get_rga_format(static_cast<Format>(FLAGS_format));

  displayIndex = SDL_GetWindowDisplayIndex(window);
  SDL_GetDisplayBounds(displayIndex, &rect);
  av_log(NULL, AV_LOG_INFO, "window display w,h: %d, %d\n", rect.w, rect.h);

  if (!AllocJoinBO(drm_fd, rga, &jb, rect.w, rect.h))
    goto fail;

  memset(&dst, 0, sizeof(dst));
  dst.fd = jb.fd;
  dst.mmuFlag = 1;
  if (set_rga_ratation(FLAGS_disp_rotate, &rotation))
    goto fail;
  rga_set_rect(&dst.rect, 0, 0, jb.width, jb.height, jb.width, jb.height,
               RK_FORMAT_YCrCb_420_SP);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);
  sdl_prepared = 1;

  while (!req_exit_) {
    SDL_PumpEvents();
    while (!SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT,
                           SDL_LASTEVENT)) {
      std::shared_ptr<JoinBO> sjb = PopBO(false);
      JoinBO *pjb = sjb.get();
      if (!pjb) {
        if (req_exit_)
          break;
        SDL_Delay(10);
        SDL_PumpEvents();
        continue;
      }

      int64_t start_time = av_gettime_relative();
      int64_t now1 = start_time, now2;

      if (realloc_texture(&texture, texture_fmt, jb.width, jb.height,
                          SDL_BLENDMODE_NONE)) {
        av_log(NULL, AV_LOG_FATAL, "Failed to alloc render texture %s\n",
               SDL_GetError());
        break;
      }

      int ret;

      memset(&src, 0, sizeof(src));
      src.fd = pjb->fd;
      src.mmuFlag = 1;
      src.rotation = rotation;
      rga_set_rect(&src.rect, 0, 0, pjb->width, pjb->height, pjb->width,
                   pjb->height, src_format);

      ret = rga.RkRgaBlit(&src, &dst, NULL);
      if (ret) {
        av_log(NULL, AV_LOG_ERROR, "RkRgaBlit error : %s\n", strerror(ret));
        break;
      }
      now2 = av_gettime_relative();
      printf("RkRgaBlit consume: %d ms\n", (int)((now2 - now1) / 1000));
      now1 = now2;
      ret = SDL_UpdateTexture(texture, NULL, jb.bo.ptr, jb.width);
      if (ret) {
        av_log(NULL, AV_LOG_ERROR, "Failed to update render texture %s\n",
               SDL_GetError());
        break;
      }
      now2 = av_gettime_relative();
      printf("UpdateTexture consume: %d ms\n", (int)((now2 - now1) / 1000));
      now1 = now2;
      SDL_RenderCopyEx(renderer, texture, NULL, NULL, 0, NULL, SDL_FLIP_NONE);
      // SDL_RenderCopy(renderer, texture, NULL, NULL);
      now2 = av_gettime_relative();
      printf("RenderCopy consume: %d ms\n", (int)((now2 - now1) / 1000));
      now1 = now2;
      SDL_RenderPresent(renderer);
      now2 = av_gettime_relative();
      printf("RenderPresent consume: %d ms\n", (int)((now2 - now1) / 1000));
      printf("renderer consume: %d ms\n\n", (int)((now2 - start_time) / 1000));
      SDL_PumpEvents();
    }
    if (event.type == SDL_QUIT)
      break;
    else
      printf("event type: %d\n", event.type);
  }

fail:
  SDLDestroy();
}
#endif

#if DRAW_BY_OPENGLES
static bool configure_vertexinfo(const GLES2_Context &ctx,
                                 OpenGLVertexInfo *vertex, int vertex_size,
                                 GLuint g_vertex_buffer, const Rect &irect,
                                 bool tex_coords) {
  // static bool swap = (FLAGS_disp_rotate == 90 || FLAGS_disp_rotate == 270);
  Rect rect = irect;
  // if (swap) {
  //     std::swap<int32_t>(rect.x, rect.y);
  //     std::swap<uint32_t>(rect.w, rect.h);
  // }
  vertex[0].x = rect.x;
  vertex[0].y = rect.y;
  vertex[1].x = rect.x;
  vertex[1].y = (float)(rect.y - (int)rect.h);
  vertex[2].x = rect.x + (int)rect.w;
  vertex[2].y = rect.y - (int)rect.h;
  vertex[3].x = rect.x + (int)rect.w;
  vertex[3].y = rect.y;

  // printf("[%d, %d, %d, %d] vertex: (%f, %f) (%f, %f) (%f, %f) (%f, %f)\n",
  //         rect.x, rect.y, rect.w, rect.h,
  //         vertex[0].x, vertex[0].y,vertex[1].x, vertex[1].y
  //         ,vertex[2].x, vertex[2].y,vertex[3].x, vertex[3].y);

  if (tex_coords) {
    vertex[0].s0 = 0.0f;
    vertex[0].t0 = 0.0f;
    vertex[1].s0 = 0.0f;
    vertex[1].t0 = 1.0f;
    vertex[2].s0 = 1.0f;
    vertex[2].t0 = 1.0f;
    vertex[3].s0 = 1.0f;
    vertex[3].t0 = 0.0f;
  }

  vertex[0].angle = vertex[1].angle = vertex[2].angle = vertex[3].angle =
      (float)FLAGS_disp_rotate;
  vertex[0].center_x = vertex[1].center_x = vertex[2].center_x =
      vertex[3].center_x = 0.0f;
  vertex[0].center_y = vertex[1].center_y = vertex[2].center_y =
      vertex[3].center_y = 0.0f;

  ctx.glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
  ctx.glBufferData(GL_ARRAY_BUFFER, vertex_size, vertex, GL_STREAM_DRAW);
  ctx.glBindBuffer(GL_ARRAY_BUFFER, 0);
  OPENGL_ERROR_CHECK();

  return true;
fail:
  return false;
}

int SDLDisplayLeaf::draw_rect(const Rect &irect) {
  if (!configure_vertexinfo(ctx, sd_line.vertex, sizeof(sd_line.vertex),
                            sd_line.vertex_buffer, irect, false))
    goto fail;

  ctx.glUseProgram(sd_line.shader_program);
  ctx.glUniformMatrix4fv(sd_line.projection_matrix_location, 1, GL_FALSE,
                         sd.projection_matrix);
  ctx.glUniformMatrix4fv(sd_line.model_view_matrix_location, 1, GL_FALSE,
                         sd.model_view_matrix);
  // argb: 0xff06ebff
  ctx.glUniform4f(sd_line.color_location, (GLfloat)0x06 * inv255f,
                  (GLfloat)0xeb * inv255f, 1.0f, 1.0f);
  ctx.glBindBuffer(GL_ARRAY_BUFFER, sd_line.vertex_buffer);
  ctx.glVertexAttribPointer(sd_line.position_attrib, 2, GL_FLOAT, GL_FALSE,
                            sizeof(OpenGLVertexInfo), 0);
  ctx.glEnableVertexAttribArray(sd_line.position_attrib);
  if (FLAGS_disp_rotate != 0) {
    ctx.glVertexAttribPointer(sd_line.angle_attrib, 1, GL_FLOAT, GL_FALSE,
                              sizeof(OpenGLVertexInfo), (const void *)16);
    ctx.glEnableVertexAttribArray(sd_line.angle_attrib);
    ctx.glVertexAttribPointer(sd_line.center_attrib, 2, GL_FLOAT, GL_FALSE,
                              sizeof(OpenGLVertexInfo), (const void *)20);
    ctx.glEnableVertexAttribArray(sd_line.center_attrib);
  }
  OPENGL_ERROR_CHECK();
  ctx.glDrawArrays(GL_LINE_LOOP, 0, 4);
  ctx.glDisableVertexAttribArray(sd_line.position_attrib);
  if (FLAGS_disp_rotate != 0) {
    ctx.glDisableVertexAttribArray(sd_line.angle_attrib);
    ctx.glDisableVertexAttribArray(sd_line.center_attrib);
  }
  ctx.glBindBuffer(GL_ARRAY_BUFFER, 0);

  return 0;

fail:
  return -1;
}

bool SDLDisplayLeaf::draw_texture(shader_picture_base_data &spdb,
                                  GLuint texture) {
  shader_base_data &sbd = spdb.base_data;
  ctx.glUseProgram(sbd.shader_program);
  ctx.glUniformMatrix4fv(sbd.projection_matrix_location, 1, GL_FALSE,
                         sd.projection_matrix);
  ctx.glUniformMatrix4fv(sbd.model_view_matrix_location, 1, GL_FALSE,
                         sd.model_view_matrix);
  if (texture) {
    av_assert0(spdb.texture_location[0] != -1);
    ctx.glActiveTexture(GL_TEXTURE0 + 0);
    ctx.glBindTexture(GL_TEXTURE_2D, texture);
    ctx.glUniform1i(spdb.texture_location[0], 0);
  } else {
    for (int i = 0; i < 4; i++) {
      if (spdb.texture_location[i] != -1) {
        ctx.glActiveTexture(GL_TEXTURE0 + i);
        ctx.glBindTexture(GL_TEXTURE_2D, spdb.texture_name[i]);
        // ctx.glBindTexture(GL_TEXTURE_EXTERNAL_OES, sd.texture_name[i]);
        ctx.glUniform1i(spdb.texture_location[i], i);
      }
    }
  }
  OPENGL_ERROR_CHECK();
  if (spdb.color_map_location != -1)
    ctx.glUniformMatrix4fv(spdb.color_map_location, 1, GL_FALSE,
                           spdb.color_map);
  if (spdb.chroma_div_h_location != -1)
    ctx.glUniform1f(spdb.chroma_div_h_location, spdb.chroma_div_h);
  if (spdb.chroma_div_w_location != -1)
    ctx.glUniform1f(spdb.chroma_div_w_location, spdb.chroma_div_w);

  OPENGL_ERROR_CHECK();

  ctx.glBindBuffer(GL_ARRAY_BUFFER, sbd.vertex_buffer);
  ctx.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sd.index_buffer);
  ctx.glVertexAttribPointer(sbd.position_attrib, 2, GL_FLOAT, GL_FALSE,
                            sizeof(OpenGLVertexInfo), 0);
  ctx.glEnableVertexAttribArray(sbd.position_attrib);
  ctx.glVertexAttribPointer(sbd.texture_coords_attrib, 2, GL_FLOAT, GL_FALSE,
                            sizeof(OpenGLVertexInfo), (const void *)8);
  ctx.glEnableVertexAttribArray(sbd.texture_coords_attrib);
  OPENGL_ERROR_CHECK();
  if (FLAGS_disp_rotate != 0) {
    ctx.glVertexAttribPointer(sbd.angle_attrib, 1, GL_FLOAT, GL_FALSE,
                              sizeof(OpenGLVertexInfo), (const void *)16);
    ctx.glEnableVertexAttribArray(sbd.angle_attrib);
    ctx.glVertexAttribPointer(sbd.center_attrib, 2, GL_FLOAT, GL_FALSE,
                              sizeof(OpenGLVertexInfo), (const void *)20);
    ctx.glEnableVertexAttribArray(sbd.center_attrib);
    OPENGL_ERROR_CHECK();
  }

  ctx.glDrawElements(GL_TRIANGLES, FF_ARRAY_ELEMS(g_index), GL_UNSIGNED_SHORT,
                     0);
  ctx.glDisableVertexAttribArray(sbd.position_attrib);
  ctx.glDisableVertexAttribArray(sbd.texture_coords_attrib);
  if (FLAGS_disp_rotate != 0) {
    ctx.glDisableVertexAttribArray(sbd.angle_attrib);
    ctx.glDisableVertexAttribArray(sbd.center_attrib);
  }
  ctx.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  ctx.glBindBuffer(GL_ARRAY_BUFFER, 0);
  OPENGL_ERROR_CHECK();

  return true;

fail:
  return false;
}

void SDLDisplayLeaf::draw_texture_in_rect(shader_picture_base_data &spdb,
                                          GLuint texture, Rect rect) {
  if (texture) {
    bool ret = configure_vertexinfo(ctx, spdb.base_data.vertex,
                                    sizeof(spdb.base_data.vertex),
                                    spdb.base_data.vertex_buffer, rect, true);
    if (ret)
      draw_texture(spdb, texture);
  }
}

void SDLDisplayLeaf::SubRun() {
  if (!SDLPrepare()) {
    sdl_prepared = -1;
    return;
  }
  sdl_prepared = 1;

  Format f = static_cast<Format>(FLAGS_format);
  int drm_fmt = get_drm_fmt(f);
  AVPixelFormat pix_fmt = get_av_pixel_fmt(f);
  const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
  int bytes_per_pixel = opengl_type_size(sd.pic_base_data.type);
  if (!(desc->flags & AV_PIX_FMT_FLAG_PLANAR))
    bytes_per_pixel *= desc->nb_components;
  SDL_Event event;
  std::vector<std::shared_ptr<NpuRectList>> fresh_face_vec;
  fresh_face_vec.resize(face_mtxs.size());
  // ctx.glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  // ctx.glClear(GL_COLOR_BUFFER_BIT);
  ctx.glLineWidth(4);
  Rect rect;
  float x_scale = (float)sd.picture_width / sd.pic_base_data.width;
  float y_scale = (float)sd.picture_height / sd.pic_base_data.height;
  int x_offset = -(sd.picture_width >> 1);
  int y_offset = (sd.picture_height >> 1);

  bool swap_wh = (FLAGS_disp_rotate == 90 || FLAGS_disp_rotate == 270);
  int win_w = sd.window_width;
  int win_h = sd.window_height;
  if (swap_wh)
    std::swap<int>(win_w, win_h);
  int *mask = (int *)malloc(win_w * 96 * 4);
  GLuint mask_texture = 0;
  Rect mask_rect = {-win_w / 2, win_h / 2, (uint32_t)win_w, 96};
#if RKNNCASCADE
  const char *title = " RK1808 4X SSD Demo";
#else
  const char *title = " Rockchip Image Process Demo";
#endif
  SDLFont title_font(title_color, 48);
  GLuint title_texture = 0;
  int title_w = 0, title_h = 0;
  if (mask) {
    int *pixel = mask;
    for (int i = 0; i < win_w * 96; i++) {
      *pixel = 0x7f001631;
      pixel++;
    }
    ctx.glGenTextures(1, &mask_texture);
    ctx.glBindTexture(GL_TEXTURE_2D, mask_texture);
    ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ctx.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, win_w, 96, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, mask);
  }
  title_texture = title_font.GetFontTexture(ctx, (char *)title, strlen(title),
                                            32, &title_w, &title_h);
  Rect title_rect = {mask_rect.x, mask_rect.y - 24, (uint32_t)title_w,
                     (uint32_t)title_h};
  av_assert0(mask_texture);
  av_assert0(title_texture);

  while (!req_exit_) {
    SDL_PumpEvents();
    while (!SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT,
                           SDL_LASTEVENT)) {
      std::shared_ptr<JoinBO> sjb = PopBO(false);
      JoinBO *pjb = sjb.get();
      if (!pjb) {
        if (req_exit_)
          break;
        SDL_Delay(10);
        SDL_PumpEvents();
        continue;
      }

      get_face_rects(fresh_face_vec);

      // int64_t start_time = av_gettime_relative();
      // int64_t now1 = start_time, now2;
      EGLint img_attrs[] = {EGL_WIDTH,
                            pjb->width,
                            EGL_HEIGHT,
                            pjb->height,
                            EGL_LINUX_DRM_FOURCC_EXT,
                            drm_fmt,
                            EGL_DMA_BUF_PLANE0_FD_EXT,
                            pjb->fd,
                            EGL_DMA_BUF_PLANE0_OFFSET_EXT,
                            0,
                            EGL_DMA_BUF_PLANE0_PITCH_EXT,
                            pjb->width * bytes_per_pixel,
                            EGL_NONE};
      EGLDisplay dpy = ctx.eglGetCurrentDisplay();
      EGLImageKHR egl_image = EGL_NO_IMAGE_KHR;
      bool failed = true;

      ctx.glBindTexture(GL_TEXTURE_2D, sd.pic_base_data.texture_name[0]);
      OPENGL_ERROR_CHECK();
      egl_image = ctx.eglCreateImageKHR(dpy, EGL_NO_CONTEXT,
                                        EGL_LINUX_DMA_BUF_EXT, NULL, img_attrs);
      if (egl_image == EGL_NO_IMAGE_KHR) {
        av_log(NULL, AV_LOG_FATAL,
               "Failed creating EGL image: 0x%04x;"
               "bytes_per_pixel: %d\n",
               ctx.eglGetError(), bytes_per_pixel);
        continue;
      }
      ctx.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, egl_image);
      EGL_ERROR_CHECK()

      if (!draw_texture(sd.pic_base_data, 0))
        goto fail;
      ctx.eglDestroyImageKHR(dpy, egl_image);
      failed = false;

      for (std::shared_ptr<NpuRectList> &nrl : fresh_face_vec) {
        if (!nrl)
          continue;
        for (NpuRect &nr : nrl->list) {
          rect.x = nr.left;
          rect.y = -nr.top;
          rect.w = nr.right - nr.left;
          rect.h = nr.bottom - nr.top;
          rect.x = rect.x * x_scale + x_offset;
          rect.y = rect.y * y_scale + y_offset;
          rect.w *= x_scale;
          rect.h *= y_scale;
          // printf("x2_scale: %f, y2_scale: %f; offset: %d, %d\n",
          //     x_scale, y_scale, x_offset, y_offset);
#if 1
          if (sdl_font) {
            char *word = (char *)nr.word.c_str();
            int fontw, fonth;
            GLuint font_texture = sdl_font->GetFontTexture(
                ctx, word, strlen(word), 32 /*bytes_per_pixel * 8*/, &fontw,
                &fonth);
            if (font_texture) {
              Rect font_rect = {rect.x + 2, rect.y + 2, (uint32_t)fontw,
                                (uint32_t)fonth};
              // bool ret =
              //     configure_vertexinfo(ctx, font_vertex, sizeof(font_vertex),
              //                          font_vertex_buffer, font_rect, true);
              bool ret = configure_vertexinfo(ctx, rgba_sd.base_data.vertex,
                                              sizeof(rgba_sd.base_data.vertex),
                                              rgba_sd.base_data.vertex_buffer,
                                              font_rect, true);
              if (ret)
                draw_texture(rgba_sd, font_texture);
              ctx.glDeleteTextures(1, &font_texture);
            }
          }
#endif
          draw_rect(rect);
        }
      }
      if (mask_texture)
        draw_texture_in_rect(rgba_sd, mask_texture, mask_rect);
      if (title_texture)
        draw_texture_in_rect(rgba_sd, title_texture, title_rect);
      SDL_GL_SwapWindow(window);
      // now2 = av_gettime_relative();
      // printf("renderer consume: %lld ms\n\n", (now2 - now1) / 1000);

    fail:
      SDL_PumpEvents();
      if (failed) {
        ctx.eglDestroyImageKHR(dpy, egl_image);
      }
    }
    if (event.type == SDL_QUIT)
      break;
    else
      printf("event type: %d\n", event.type);
  }

  if (mask_texture)
    ctx.glDeleteTextures(1, &mask_texture);
  if (title_texture)
    ctx.glDeleteTextures(1, &title_texture);

  SDLDestroy();
}
#endif

#if HAVE_ROCKX

#include <rockx.h>

class DrawOperation;
typedef void *(*npu_process)(std::vector<rockx_handle_t> &npu_handles,
                             rockx_image_t *input_image);
typedef int (*result_in)(void *npu_out, void *disp_ptr, int disp_w, int disp_h,
                         SDL_Renderer *render);
class NpuModelLeaf : public LeafHandler {
public:
  NpuModelLeaf(int draw_idx, int64_t timeout);
  virtual ~NpuModelLeaf();
  bool Prepare(std::string model_name);
  virtual void SubRun() override;

  npu_process img_func;
  std::vector<rockx_handle_t> npu_handles;

  DrawOperation *draw;
  result_in show_func;
  int draw_index;
  int64_t disp_timeout;
};

class NpuResult {
public:
  NpuResult()
      : got_time(av_gettime_relative() / 1000), timeout(200), index(-1),
        leaf(nullptr), npu_out(nullptr) {}
  ~NpuResult() {
    if (npu_out)
      free(npu_out);
  }
  void Draw(void *disp_ptr, int disp_w, int disp_h, SDL_Renderer *render) {
    (*(leaf->show_func))(npu_out, disp_ptr, disp_w, disp_h, render);
  }
  int64_t got_time; // ms
  int64_t timeout;  // ms
  int index;
  NpuModelLeaf *leaf;
  void *npu_out;
};

class DrawOperation {
public:
  DrawOperation(int num) : gather_func(nullptr) {
    mtxs.resize(num);
    results.resize(num);
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);
  }
  ~DrawOperation() = default;
  void Set(int idx, std::shared_ptr<NpuResult> nr) {
    mtxs[idx].lock();
    results[idx] = nr;
    mtxs[idx].unlock();
  }
  void Draw(void *disp_ptr, int disp_w, int disp_h) {
    static auto sdl_fmt = get_sdl_format(static_cast<Format>(FLAGS_format));
    static int depth = get_format_bits(static_cast<Format>(FLAGS_format));
    std::vector<std::shared_ptr<NpuResult>> draw_rs;
    int64_t now = av_gettime_relative() / 1000;
    for (size_t i = 0; i < mtxs.size(); i++) {
      spinlock_mutex &smtx = mtxs[i];
      smtx.lock();
      std::shared_ptr<NpuResult> &nr = results[i];
      if (nr) {
        if (now - nr->got_time < nr->timeout)
          draw_rs.push_back(nr);
        else
          results[i].reset();
      }
      smtx.unlock();
    }
    if (draw_rs.empty())
      return;
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
        disp_ptr, disp_w, disp_h, depth, disp_w * depth / 8, sdl_fmt);
    if (!surface) {
      av_log(NULL, AV_LOG_FATAL,
             "SDL_CreateRGBSurfaceWithFormatFrom failed at line %d\n",
             __LINE__);
      return;
    }
    SDL_Renderer *render = SDL_CreateSoftwareRenderer(surface);
    if (!render) {
      av_log(NULL, AV_LOG_FATAL,
             "SDL_CreateSoftwareRenderer failed at line %d\n", __LINE__);
      return;
    }
    std::vector<void *> npu_outs;
    npu_outs.resize(mtxs.size(), nullptr);
    for (auto &nr : draw_rs) {
      nr->Draw(disp_ptr, disp_w, disp_h, render);
      npu_outs[nr->index] = nr->npu_out;
    }
    if (gather_func)
      gather_func((void *)&npu_outs, disp_ptr, disp_w, disp_h, render);
    SDL_RenderPresent(render);
    SDL_DestroyRenderer(render);
    SDL_FreeSurface(surface);
  }

  // SDL_Renderer *render;
  std::vector<spinlock_mutex> mtxs;
  std::vector<std::shared_ptr<NpuResult>> results;
  result_in gather_func;
};
#else
class DrawOperation {
public:
  void Draw(void *disp_ptr, int disp_w, int disp_h) {}
};
#endif // HAVE_ROCKX

enum csi_path_mode { VOP_PATH = 0, BYPASS_PATH };

enum vop_pdaf_mode {
  VOP_HOLD_MODE = 0,
  VOP_NORMAL_MODE,
  VOP_PINGPONG_MODE,
  VOP_BYPASS_MODE,
  VOP_BACKGROUND_MODE,
  VOP_ONEFRAME_MODE,
  VOP_ONEFRAME_NOSEND_MODE
};

enum vop_pdaf_type {
  VOP_PDAF_TYPE_DEFAULT = 0,
  VOP_PDAF_TYPE_HBLANK,
  VOP_PDAF_TYPE_VBLANK,
};

#include <xf86drm.h>
#include <xf86drmMode.h>

struct type_name {
  unsigned int type;
  const char *name;
};

static const char *util_lookup_type_name(unsigned int type,
                                         const struct type_name *table,
                                         unsigned int count) {
  unsigned int i;

  for (i = 0; i < count; i++)
    if (table[i].type == type)
      return table[i].name;

  return NULL;
}

static const struct type_name encoder_type_names[] = {
    {DRM_MODE_ENCODER_NONE, "none"},   {DRM_MODE_ENCODER_DAC, "DAC"},
    {DRM_MODE_ENCODER_TMDS, "TMDS"},   {DRM_MODE_ENCODER_LVDS, "LVDS"},
    {DRM_MODE_ENCODER_TVDAC, "TVDAC"}, {DRM_MODE_ENCODER_VIRTUAL, "Virtual"},
    {DRM_MODE_ENCODER_DSI, "DSI"},     {DRM_MODE_ENCODER_DPMST, "DPMST"},
    {DRM_MODE_ENCODER_DPI, "DPI"},
};

const char *util_lookup_encoder_type_name(unsigned int type) {
  return util_lookup_type_name(type, encoder_type_names,
                               FF_ARRAY_ELEMS(encoder_type_names));
}

static const struct type_name connector_status_names[] = {
    {DRM_MODE_CONNECTED, "connected"},
    {DRM_MODE_DISCONNECTED, "disconnected"},
    {DRM_MODE_UNKNOWNCONNECTION, "unknown"},
};

const char *util_lookup_connector_status_name(unsigned int status) {
  return util_lookup_type_name(status, connector_status_names,
                               FF_ARRAY_ELEMS(connector_status_names));
}

static const struct type_name connector_type_names[] = {
    {DRM_MODE_CONNECTOR_Unknown, "unknown"},
    {DRM_MODE_CONNECTOR_VGA, "VGA"},
    {DRM_MODE_CONNECTOR_DVII, "DVI-I"},
    {DRM_MODE_CONNECTOR_DVID, "DVI-D"},
    {DRM_MODE_CONNECTOR_DVIA, "DVI-A"},
    {DRM_MODE_CONNECTOR_Composite, "composite"},
    {DRM_MODE_CONNECTOR_SVIDEO, "s-video"},
    {DRM_MODE_CONNECTOR_LVDS, "LVDS"},
    {DRM_MODE_CONNECTOR_Component, "component"},
    {DRM_MODE_CONNECTOR_9PinDIN, "9-pin DIN"},
    {DRM_MODE_CONNECTOR_DisplayPort, "DP"},
    {DRM_MODE_CONNECTOR_HDMIA, "HDMI-A"},
    {DRM_MODE_CONNECTOR_HDMIB, "HDMI-B"},
    {DRM_MODE_CONNECTOR_TV, "TV"},
    {DRM_MODE_CONNECTOR_eDP, "eDP"},
    {DRM_MODE_CONNECTOR_VIRTUAL, "Virtual"},
    {DRM_MODE_CONNECTOR_DSI, "DSI"},
    {DRM_MODE_CONNECTOR_DPI, "DPI"},
};

const char *util_lookup_connector_type_name(unsigned int type) {
  return util_lookup_type_name(type, connector_type_names,
                               FF_ARRAY_ELEMS(connector_type_names));
}

class DRMDisplayLeaf : public LeafHandler {
  // copy from modetest.c, my lazy way
  struct crtc {
    drmModeCrtc *crtc;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
    drmModeModeInfo *mode;
  };

  struct encoder {
    drmModeEncoder *encoder;
  };

  struct connector {
    drmModeConnector *connector;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
    char *name;

    drmModeCrtc *saved_crtc;
  };

  struct fb {
    drmModeFB *fb;
  };

  struct plane {
    drmModePlane *plane;
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
  };

  struct resources {
    drmModeRes *res;
    drmModePlaneRes *plane_res;

    struct crtc *crtcs;
    struct encoder *encoders;
    struct connector *connectors;
    struct fb *fbs;
    struct plane *planes;
  };

  struct device {
    int fd;
    struct resources *resources;
  };

  struct property_arg {
    uint32_t obj_id;
    uint32_t obj_type;
    char name[DRM_PROP_NAME_LEN + 1];
    uint32_t prop_id;
    uint64_t value;
  };

  static void free_resources(struct resources *res);
  struct resources *get_resources(struct device *pdev);

  static drmModeConnector *get_connector_first_match(struct resources *res,
                                                     const char *key);
  static drmModeEncoder *get_encoder_by_id(struct resources *res, uint32_t id);
  static drmModeEncoder *get_encoder_by_key(struct resources *res,
                                            const char *key);
  static int get_crtc_index(struct resources *res, uint32_t id);
  static int get_crtc_index_by_connector(struct resources *res,
                                         drmModeConnector *conn);

  static void store_saved_crtc(struct resources *res);
  static void restore_saved_crtc(int fd, struct resources *res);
  void set_property(struct device *devi, struct property_arg *p);

  bool WaitPageFlip(int timeout);

public:
  DRMDisplayLeaf(int drmfd, const char *connector_key, const char *encoder_key,
                 int width, int height, uint32_t rotate = 0);
  virtual ~DRMDisplayLeaf();

  virtual bool Prepare() override;
  virtual void SubRun() override;

  int drm_fd;
  RockchipRga rga;
  JoinBO jbs[2]; // display buffer, if source w/h do not equal with final
  // display w/h
  char connector_name_key[16];
  char encoder_type_key[16];
  int scale_width, scale_height;
  uint32_t scale_rotate;
  struct device dev;
  uint32_t plane_id;
  uint32_t crtc_id;
  drmModeModeInfo cur_mode;
  drmModeCrtc *cur_crtc;
  uint32_t conn_id;
  JoinBO crtc_jb;
  uint32_t crtc_fb_id;
  JoinBO hdmi_jb;
  uint32_t hdmi_fb_id;

  bool waiting_for_flip;
  drmEventContext drm_evctx;

  DrawOperation *sub_draw;
};

struct DRMDisplayLeaf::resources *
DRMDisplayLeaf::get_resources(struct DRMDisplayLeaf::device *pdev) {
  struct resources *res;
  int i;

  res = (struct resources *)calloc(1, sizeof(*res));
  if (res == 0)
    return NULL;

  drmSetClientCap(pdev->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

  res->res = drmModeGetResources(pdev->fd);
  if (!res->res) {
    fprintf(stderr, "drmModeGetResources failed: %s\n", strerror(errno));
    goto error;
  }

  res->crtcs =
      (struct crtc *)calloc(res->res->count_crtcs, sizeof(*res->crtcs));
  res->encoders = (struct encoder *)calloc(res->res->count_encoders,
                                           sizeof(*res->encoders));
  res->connectors = (struct connector *)calloc(res->res->count_connectors,
                                               sizeof(*res->connectors));
  res->fbs = (struct fb *)calloc(res->res->count_fbs, sizeof(*res->fbs));

  if (!res->crtcs || !res->encoders || !res->connectors || !res->fbs)
    goto error;

#define get_resource(_res, __res, type, Type)                                  \
  do {                                                                         \
    for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {                \
      (_res)->type##s[i].type =                                                \
          drmModeGet##Type(pdev->fd, (_res)->__res->type##s[i]);               \
      if (!(_res)->type##s[i].type)                                            \
        fprintf(stderr, "could not get %s %i: %s\n", #type,                    \
                (_res)->__res->type##s[i], strerror(errno));                   \
    }                                                                          \
  } while (0)

  get_resource(res, res, crtc, Crtc);
  get_resource(res, res, encoder, Encoder);
  get_resource(res, res, connector, Connector);
  get_resource(res, res, fb, FB);

  /* Set the name of all connectors based on the type name and the per-type ID.
   */
  for (i = 0; i < res->res->count_connectors; i++) {
    struct connector *sconnector = &res->connectors[i];
    drmModeConnector *conn = sconnector->connector;

    asprintf(&sconnector->name, "%s-%u",
             util_lookup_connector_type_name(conn->connector_type),
             conn->connector_type_id);
  }

#define get_properties(_res, __res, type, Type)                                \
  do {                                                                         \
    for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {                \
      struct type *obj = &res->type##s[i];                                     \
      unsigned int j;                                                          \
      obj->props = drmModeObjectGetProperties(pdev->fd, obj->type->type##_id,  \
                                              DRM_MODE_OBJECT_##Type);         \
      if (!obj->props) {                                                       \
        fprintf(stderr, "could not get %s %i properties: %s\n", #type,         \
                obj->type->type##_id, strerror(errno));                        \
        continue;                                                              \
      }                                                                        \
      obj->props_info = (drmModePropertyRes **)calloc(                         \
          obj->props->count_props, sizeof(*obj->props_info));                  \
      if (!obj->props_info)                                                    \
        continue;                                                              \
      for (j = 0; j < obj->props->count_props; ++j)                            \
        obj->props_info[j] =                                                   \
            drmModeGetProperty(pdev->fd, obj->props->props[j]);                \
    }                                                                          \
  } while (0)

  get_properties(res, res, crtc, CRTC);
  get_properties(res, res, connector, CONNECTOR);

  for (i = 0; i < res->res->count_crtcs; ++i)
    res->crtcs[i].mode = &res->crtcs[i].crtc->mode;

  res->plane_res = drmModeGetPlaneResources(pdev->fd);
  if (!res->plane_res) {
    fprintf(stderr, "drmModeGetPlaneResources failed: %s\n", strerror(errno));
    return res;
  }

  res->planes = (struct plane *)calloc(res->plane_res->count_planes,
                                       sizeof(*res->planes));
  if (!res->planes)
    goto error;

  get_resource(res, plane_res, plane, Plane);
  get_properties(res, plane_res, plane, PLANE);

  return res;

error:
  free_resources(res);
  return NULL;
}

void DRMDisplayLeaf::free_resources(struct DRMDisplayLeaf::resources *res) {
  int i;

  if (!res)
    return;

#define free_resource(_res, __res, type, Type)                                 \
  do {                                                                         \
    if (!(_res)->type##s)                                                      \
      break;                                                                   \
    for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {                \
      if (!(_res)->type##s[i].type)                                            \
        break;                                                                 \
      drmModeFree##Type((_res)->type##s[i].type);                              \
    }                                                                          \
    free((_res)->type##s);                                                     \
  } while (0)

#define free_properties(_res, __res, type)                                     \
  do {                                                                         \
    for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {                \
      drmModeFreeObjectProperties(res->type##s[i].props);                      \
      free(res->type##s[i].props_info);                                        \
    }                                                                          \
  } while (0)

  if (res->res) {
    free_properties(res, res, crtc);

    free_resource(res, res, crtc, Crtc);
    free_resource(res, res, encoder, Encoder);

    for (i = 0; i < res->res->count_connectors; i++)
      free(res->connectors[i].name);

    free_resource(res, res, connector, Connector);
    free_resource(res, res, fb, FB);

    drmModeFreeResources(res->res);
  }

  if (res->plane_res) {
    free_properties(res, plane_res, plane);

    free_resource(res, plane_res, plane, Plane);

    drmModeFreePlaneResources(res->plane_res);
  }

  free(res);
}

drmModeConnector *
DRMDisplayLeaf::get_connector_first_match(struct resources *res,
                                          const char *key) {
  struct connector *sconnector;
  for (int i = 0; i < res->res->count_connectors; i++) {
    sconnector = &res->connectors[i];

    if (strstr(sconnector->name, key))
      return sconnector->connector;
  }

  return NULL;
}

drmModeEncoder *DRMDisplayLeaf::get_encoder_by_id(struct resources *res,
                                                  uint32_t id) {
  drmModeRes *dmr = res->res;
  drmModeEncoder *encoder;
  int i;

  for (i = 0; i < dmr->count_encoders; i++) {
    encoder = res->encoders[i].encoder;
    if (encoder && encoder->encoder_id == id)
      return encoder;
  }

  return NULL;
}

drmModeEncoder *DRMDisplayLeaf::get_encoder_by_key(struct resources *res,
                                                   const char *key) {
  drmModeRes *dmr = res->res;
  drmModeEncoder *encoder;
  int i;

  for (i = 0; i < dmr->count_encoders; i++) {
    encoder = res->encoders[i].encoder;
    if (encoder) {
      const char *type = util_lookup_encoder_type_name(encoder->encoder_type);
      if (strstr(type, key))
        return encoder;
    }
  }

  return NULL;
}

int DRMDisplayLeaf::get_crtc_index(struct resources *res, uint32_t id) {
  drmModeRes *dmr = res->res;
  int i;

  for (i = 0; i < dmr->count_crtcs; ++i) {
    drmModeCrtc *crtc = res->crtcs[i].crtc;
    if (crtc && crtc->crtc_id == id)
      return i;
  }

  return -1;
}

int DRMDisplayLeaf::get_crtc_index_by_connector(struct resources *res,
                                                drmModeConnector *conn) {
  int crtc_idx = -1;
  uint32_t possible_crtcs = ~0;
  uint32_t active_crtcs = 0;
  uint32_t crtcs_for_connector = 0;
  int idx;
  for (int j = 0; j < conn->count_encoders; ++j) {
    drmModeEncoder *encoder = get_encoder_by_id(res, conn->encoders[j]);
    if (!encoder)
      continue;

    crtcs_for_connector |= encoder->possible_crtcs;

    idx = get_crtc_index(res, encoder->crtc_id);
    if (idx >= 0)
      active_crtcs |= 1 << idx;
  }
  possible_crtcs &= crtcs_for_connector;
  if (!possible_crtcs)
    return -1;
  if (possible_crtcs & active_crtcs)
    crtc_idx = ffs(possible_crtcs & active_crtcs);
  else
    crtc_idx = ffs(possible_crtcs);
  return crtc_idx - 1;
}

void DRMDisplayLeaf::store_saved_crtc(struct resources *res) {
  drmModeRes *dmr = res->res;
  for (int i = 0; i < dmr->count_connectors; i++) {
    struct connector *connector = &res->connectors[i];
    drmModeConnector *conn = connector->connector;
    if (!conn)
      continue;
    if (conn->connection != DRM_MODE_CONNECTED)
      continue;
    if (conn->count_modes <= 0)
      continue;
    int crtc_idx = get_crtc_index_by_connector(res, conn);
    if (crtc_idx >= 0)
      connector->saved_crtc = res->crtcs[crtc_idx].crtc;
  }
}

void DRMDisplayLeaf::restore_saved_crtc(int fd, struct resources *res) {
  drmModeRes *dmr = res->res;
  for (int i = 0; i < dmr->count_connectors; i++) {
    struct connector *connector = &res->connectors[i];
    drmModeCrtc *saved_crtc = connector->saved_crtc;
    if (!saved_crtc)
      continue;
    drmModeConnector *conn = connector->connector;
    drmModeSetCrtc(fd, saved_crtc->crtc_id, saved_crtc->buffer_id,
                   saved_crtc->x, saved_crtc->y, &conn->connector_id, 1,
                   &saved_crtc->mode);
  }
}

void DRMDisplayLeaf::set_property(struct device *devi, struct property_arg *p) {
  drmModeObjectProperties *props = NULL;
  drmModePropertyRes **props_info = NULL;
  const char *obj_type;
  int ret;
  int i;

  p->obj_type = 0;
  p->prop_id = 0;

#define find_object(_res, __res, type, Type)                                   \
  do {                                                                         \
    for (i = 0; i < (int)(_res)->__res->count_##type##s; ++i) {                \
      struct type *obj = &(_res)->type##s[i];                                  \
      if (obj->type->type##_id != p->obj_id)                                   \
        continue;                                                              \
      p->obj_type = DRM_MODE_OBJECT_##Type;                                    \
      obj_type = #Type;                                                        \
      props = obj->props;                                                      \
      props_info = obj->props_info;                                            \
    }                                                                          \
  } while (0)

  find_object(devi->resources, res, crtc, CRTC);
  if (p->obj_type == 0)
    find_object(devi->resources, res, connector, CONNECTOR);
  if (p->obj_type == 0)
    find_object(devi->resources, plane_res, plane, PLANE);
  if (p->obj_type == 0) {
    fprintf(stderr, "Object %i not found, can't set property\n", p->obj_id);
    return;
  }

  if (!props) {
    fprintf(stderr, "%s %i has no properties\n", obj_type, p->obj_id);
    return;
  }

  for (i = 0; i < (int)props->count_props; ++i) {
    if (!props_info[i])
      continue;
    if (strcmp(props_info[i]->name, p->name) == 0)
      break;
  }

  if (i == (int)props->count_props) {
    fprintf(stderr, "%s %i has no %s property\n", obj_type, p->obj_id, p->name);
    return;
  }

  p->prop_id = props->props[i];

  ret = drmModeObjectSetProperty(devi->fd, p->obj_id, p->obj_type, p->prop_id,
                                 p->value);
  if (ret < 0)
    fprintf(stderr, "failed to set %s %i property %s to %" PRIu64 ": %s\n",
            obj_type, p->obj_id, p->name, p->value, strerror(errno));
}

static bool add_fb(int fd, JoinBO *jb, uint32_t *fb_id) {
  static Format f = static_cast<Format>(FLAGS_format);
  static int drm_fmt = get_drm_fmt(f);
  int fourcc_fmt = drm_fmt;
  int width = jb->width;
  int height = jb->height;

  uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};

  if (FLAGS_drm_raw8_mode) {
    fourcc_fmt = drm_fmt = DRM_FORMAT_RGB332;
    width = jb->width * get_format_bits(f) / 8;
  }

  switch (drm_fmt) {
  case DRM_FORMAT_NV12:
  case DRM_FORMAT_NV16:
    handles[0] = jb->bo.handle;
    pitches[0] = width;
    offsets[0] = 0;
    handles[1] = jb->bo.handle;
    pitches[1] = pitches[0];
    offsets[1] = pitches[0] * height;
    break;
  case DRM_FORMAT_RGB888:
    handles[0] = jb->bo.handle;
    pitches[0] = width * 3;
    offsets[0] = 0;
#if DRAW_BY_OPENGLES
    // shit: format endian EGL seems different from DRM
    fourcc_fmt = DRM_FORMAT_BGR888;
#endif
    break;
  case DRM_FORMAT_XRGB8888:
    handles[0] = jb->bo.handle;
    pitches[0] = width * 4;
    offsets[0] = 0;
    break;
  case DRM_FORMAT_RGB332:
    handles[0] = jb->bo.handle;
    pitches[0] = width;
    offsets[0] = 0;
    break;
  default:
    av_log(NULL, AV_LOG_FATAL, "unknown drm format %d\n", drm_fmt);
    return false;
  }

  int ret = drmModeAddFB2(fd, width, height, fourcc_fmt, handles, pitches,
                          offsets, fb_id, 0);
  if (ret) {
    av_log(NULL, AV_LOG_FATAL, "drmModeAddFB2 failed, %m\n");
    av_log(NULL, AV_LOG_FATAL, "width=%d height=%d fourcc=%c%c%c%c handle=%d\n",
           width, height, (fourcc_fmt >> 24) & 0xFF, (fourcc_fmt >> 16) & 0xFF,
           (fourcc_fmt >> 8) & 0xFF, fourcc_fmt & 0xFF, jb->bo.handle);
    return false;
  }
  return true;
}

static void rm_fb(int fd, uint32_t &fb_id) {
  if (fb_id <= 0)
    return;
  drmModeRmFB(fd, fb_id);
  fb_id = 0;
}

DRMDisplayLeaf::DRMDisplayLeaf(int drmfd, const char *connector_key,
                               const char *encoder_key, int width, int height,
                               uint32_t rotate)
    : drm_fd(drmfd), scale_width(width), scale_height(height),
      scale_rotate(rotate), waiting_for_flip(false), sub_draw(nullptr) {
  if (connector_key)
    snprintf(connector_name_key, sizeof(connector_name_key), "%s",
             connector_key);
  else
    connector_name_key[0] = 0;
  if (encoder_key)
    snprintf(encoder_type_key, sizeof(encoder_type_key), "%s", encoder_key);
  else
    encoder_type_key[0] = 0;

  memset(&jbs[0].bo, 0, sizeof(jbs[0].bo));
  jbs[0].fd = -1;
  jbs[0].bo.fd = -1;
  memset(&jbs[1].bo, 0, sizeof(jbs[1].bo));
  jbs[1].fd = -1;
  jbs[1].bo.fd = -1;
  memset(&dev, 0, sizeof(dev));
  dev.fd = -1;

  crtc_jb.fd = -1;
  memset(&crtc_jb.bo, 0, sizeof(crtc_jb.bo));
  crtc_jb.bo.fd = -1;
  crtc_fb_id = 0;

  hdmi_jb.fd = -1;
  memset(&hdmi_jb.bo, 0, sizeof(hdmi_jb.bo));
  hdmi_jb.bo.fd = -1;
  hdmi_fb_id = 0;
}

DRMDisplayLeaf::~DRMDisplayLeaf() {
  free_resources(dev.resources);
  // if (dev.fd >= 0) {
  //     drmClose(dev.fd);
  //     dev.fd = -1;
  // }
  for (int i = 0; i < 2; i++)
    FreeJoinBO(drm_fd, rga, &jbs[i]);
  FreeJoinBO(drm_fd, rga, &crtc_jb);
  rm_fb(drm_fd, crtc_fb_id);
  FreeJoinBO(drm_fd, rga, &hdmi_jb);
  rm_fb(drm_fd, hdmi_fb_id);
}

bool DRMDisplayLeaf::Prepare() {
  if (drmSetClientCap(drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1)) {
    av_log(NULL, AV_LOG_FATAL, "Failed to set universal planes cap %m\n");
    return false;
  }
  if (drmSetClientCap(drm_fd, DRM_CLIENT_CAP_ATOMIC, 1)) {
    av_log(NULL, AV_LOG_FATAL, "Failed set drm atomic cap %m\n");
    return false;
  }
  // get drm infomation
  dev.fd = drm_fd;
  dev.resources = get_resources(&dev);
  if (!dev.resources)
    return false;

  // choose connector matchs key name
  drmModeConnector *dmc =
      get_connector_first_match(dev.resources, connector_name_key);
  if (!dmc) {
    av_log(NULL, AV_LOG_FATAL, "Fail to find connector to match %s\n",
           connector_name_key);
    return false;
  }
  if (dmc->connection != DRM_MODE_CONNECTED || dmc->count_modes <= 0) {
    av_log(NULL, AV_LOG_FATAL, "Connector to match %s is not ready\n",
           connector_name_key);
    return false;
  }
  conn_id = dmc->connector_id;
  drmModeEncoder *enc = NULL;
  if (dmc->encoder_id > 0)
    enc = get_encoder_by_id(dev.resources, dmc->encoder_id);
  else
    enc = get_encoder_by_key(dev.resources, encoder_type_key);
  if (!enc) {
    av_log(NULL, AV_LOG_WARNING, "Encoder is not ready for %s\n",
           connector_name_key);
    return false;
  }
  int idx = -1;
  if (enc->crtc_id > 0)
    idx = get_crtc_index(dev.resources, enc->crtc_id);
  else
    idx = get_crtc_index_by_connector(dev.resources, dmc);
  if (idx < 0) {
    av_log(NULL, AV_LOG_FATAL, "CRTC is not ready\n");
    return false;
  }
  cur_crtc = dev.resources->crtcs[idx].crtc;
  crtc_id = cur_crtc->crtc_id;
  cur_mode = cur_crtc->mode;
  plane_id = 0;
  for (uint32_t i = 0; i < dev.resources->plane_res->count_planes; i++) {
    drmModePlanePtr ovr = dev.resources->planes[i].plane;
    if (!ovr)
      continue;
    if ((ovr->possible_crtcs & (1 << idx)) &&
        (ovr->crtc_id == 0 || ovr->crtc_id == crtc_id)) {
      plane_id = ovr->plane_id;
      break;
    }
  }
  if (plane_id == 0) {
    av_log(NULL, AV_LOG_FATAL, "PLANE is not ready\n");
    return false;
  }
  av_log(NULL, AV_LOG_INFO,
         "drmfd: %d, conn_id: %d, encoder_id: %d, crtc_id: %d<idx: %d>, "
         "plane_id: %d, mode w/h: %d, %d\n",
         dev.fd, conn_id, enc->encoder_id, crtc_id, idx, plane_id,
         cur_mode.hdisplay, cur_mode.vdisplay);
  if (cur_mode.hdisplay == 0 && cur_mode.vdisplay == 0) {
    // set the first mode
    drmModeModeInfoPtr first_mode = &dmc->modes[0];
    cur_mode = *first_mode;
  }
  if (FLAGS_drm_raw8_mode) {
    int pdaf_work_mode = VOP_NORMAL_MODE;
    int pdaf_type = VOP_PDAF_TYPE_VBLANK;
    int vop_csi_path = VOP_PATH;
    struct property_arg p;
    memset(&p, 0, sizeof(p));
    p.obj_id = crtc_id;
    snprintf(p.name, sizeof(p.name), "WORK_MODE");
    p.value = pdaf_work_mode;
    set_property(&dev, &p);
    memset(&p, 0, sizeof(p));
    p.obj_id = crtc_id;
    snprintf(p.name, sizeof(p.name), "PDAF_TYPE");
    p.value = pdaf_type;
    set_property(&dev, &p);
    memset(&p, 0, sizeof(p));
    p.obj_id = conn_id;
    snprintf(p.name, sizeof(p.name), "CSI-TX-PATH");
    p.value = vop_csi_path;
    set_property(&dev, &p);
  }

  if (scale_rotate == 90 || scale_rotate == 270)
    std::swap<int>(scale_width, scale_height);

  if (scale_width > cur_mode.hdisplay || scale_height > cur_mode.vdisplay) {
    av_log(NULL, AV_LOG_WARNING,
           "Input width <%d> or height <%d>"
           " is larger than DRM display <%d> <%d>\n",
           scale_width, scale_height, cur_mode.hdisplay, cur_mode.vdisplay);
  }
  av_assert0(cur_mode.hdisplay > 0 && cur_mode.vdisplay > 0);
  av_log(NULL, AV_LOG_INFO, "mode <%d> x <%d>, scale <%d> x <%d>\n",
         cur_mode.hdisplay, cur_mode.vdisplay, scale_width, scale_height);

  if (FLAGS_drm_raw8_mode) {
    Format f = static_cast<Format>(FLAGS_format);
    if (!AllocJoinBO(drm_fd, rga, &crtc_jb,
                     cur_mode.hdisplay * 8 / get_format_bits(f),
                     cur_mode.vdisplay))
      return false;
  } else {
    if (!AllocJoinBO(drm_fd, rga, &crtc_jb, cur_mode.hdisplay,
                     cur_mode.vdisplay))
      return false;
  }

  if (!add_fb(drm_fd, &crtc_jb, &crtc_fb_id))
    return false;
  int ret =
      drmModeSetCrtc(dev.fd, crtc_id, crtc_fb_id, 0, 0, &conn_id, 1, &cur_mode);
  if (ret) {
    av_log(NULL, AV_LOG_WARNING, "Fail to drmModeSetCrtc, %d\n", ret);
    return false;
  }
  if (scale_width > 0 || scale_rotate != 0) {
    for (int i = 0; i < 2; i++) {
      if (!AllocJoinBO(drm_fd, rga, &jbs[i], scale_width, scale_height))
        return false;
    }
  }

  int force_connect_id = 0; // hack for hdmi, set hdmi after dsi
  char *force_connect_id_str = getenv("ForceConnectID");
  if (force_connect_id_str)
    force_connect_id = atoi(force_connect_id_str);
  if (force_connect_id != 0) {
    drmModeConnector *hdmi_dmc =
        get_connector_first_match(dev.resources, "HDMI");
    if (!hdmi_dmc || !hdmi_dmc->count_modes ||
        (int)hdmi_dmc->connector_id != force_connect_id) {
      av_log(NULL, AV_LOG_FATAL, "Connector to match HDMI is not ready\n");
      return false;
    }
    drmModeModeInfo hdmi_mode = hdmi_dmc->modes[0];
    int hdmi_idx = get_crtc_index_by_connector(dev.resources, hdmi_dmc);
    if (hdmi_idx < 0) {
      av_log(NULL, AV_LOG_FATAL, "CRTC for HDMI is not ready\n");
      return false;
    }
    drmModeCrtc *hdmi_crtc = dev.resources->crtcs[hdmi_idx].crtc;
    if (!AllocJoinBO(drm_fd, rga, &hdmi_jb, hdmi_mode.hdisplay,
                     hdmi_mode.vdisplay))
      return false;
    if (!add_fb(drm_fd, &hdmi_jb, &hdmi_fb_id))
      return false;
    ret = drmModeSetCrtc(dev.fd, hdmi_crtc->crtc_id, hdmi_fb_id, 0, 0,
                         &hdmi_dmc->connector_id, 1, &hdmi_mode);
    if (ret) {
      av_log(NULL, AV_LOG_WARNING, "Fail to drmModeSetCrtc for HDMI, %m\n");
      av_assert0(0);
    }
  }
  return LeafHandler::Prepare();
}

#define UNUSED __attribute__((unused))
static void drm_flip_handler(int fd UNUSED, unsigned int frame UNUSED,
                             unsigned int sec UNUSED, unsigned int usec UNUSED,
                             void *data) {
  *((bool *)data) = false;
}

bool DRMDisplayLeaf::WaitPageFlip(int timeout) {
  while (waiting_for_flip) {
    struct pollfd drm_pollfd;
    drm_pollfd.fd = dev.fd;
    drm_pollfd.events = POLLIN | (POLLHUP | POLLERR);
    drm_pollfd.revents = 0;
    if (poll(&drm_pollfd, 1, timeout) < 0) {
      av_log(NULL, AV_LOG_ERROR, "DRM poll error");
      return false;
    }

    if (drm_pollfd.revents & (POLLHUP | POLLERR)) {
      av_log(NULL, AV_LOG_ERROR, "DRM poll hup or error");
      return false;
    }

    if (drm_pollfd.revents & POLLIN) {
      /* Page flip? If so, drmHandleEvent will unset wdata->waiting_for_flip */
      drmHandleEvent(dev.fd, &drm_evctx);
    } else {
      /* Timed out and page flip didn't happen */
      av_log(NULL, AV_LOG_ERROR, "Dropping frame while waiting_for_flip\n");
      return false;
    }
  }
  return true;
}

static std::pair<uint32_t, bool> &
get_fb_id(int dmafd, std::map<JoinBO *, std::pair<uint32_t, bool>> &jb_fb_map,
          JoinBO *jb) {
  static std::pair<uint32_t, bool> empty(0, false);
  auto iter = jb_fb_map.find(jb);
  if (iter != jb_fb_map.end()) {
    return iter->second;
  }
  uint32_t fb_id = 0;
  if (!add_fb(dmafd, jb, &fb_id))
    return empty;
  jb_fb_map[jb] = std::pair<uint32_t, bool>(fb_id, false);
  return jb_fb_map[jb];
}

static void
clear_fb_id(int dmafd,
            std::map<JoinBO *, std::pair<uint32_t, bool>> &jb_fb_map) {
  for (auto &pair : jb_fb_map) {
    rm_fb(dmafd, pair.second.first);
  }
}

static bool rga_blit_jb(RockchipRga &rga, JoinBO *src_jb, JoinBO *dst_jb,
                        int rotation = 0) {
  rga_info_t src, dst;
  static int format = get_rga_format(static_cast<Format>(FLAGS_format));

  memset(&src, 0, sizeof(src));
  src.fd = src_jb->fd;
  src.mmuFlag = 1;
  src.rotation = rotation;
  rga_set_rect(&src.rect, 0, 0, src_jb->width, src_jb->height, src_jb->width,
               src_jb->height, format);

  memset(&dst, 0, sizeof(dst));
  dst.fd = dst_jb->fd;
  dst.mmuFlag = 1;
  rga_set_rect(&dst.rect, 0, 0, dst_jb->width, dst_jb->height, dst_jb->width,
               dst_jb->height, format);

  int ret = rga.RkRgaBlit(&src, &dst, NULL);
  if (ret) {
    av_log(NULL, AV_LOG_ERROR, "RkRgaBlit error : %s\n", strerror(ret));
    return false;
  }
  return true;
}

void DRMDisplayLeaf::SubRun() {
  std::map<JoinBO *, std::pair<uint32_t, bool>> jb_fb_map;
  std::shared_ptr<JoinBO> render_jb;
  uint32_t render_fb_id = 0;
  int cur_jb_id = 0;
  bool do_rga_blit = (jbs[0].fd >= 0);
  // bool drm_ready = false;
  int rotation = 0;

  if (do_rga_blit && set_rga_ratation(scale_rotate, &rotation)) {
    exit(-1);
  }
  memset(&drm_evctx, 0, sizeof(drm_evctx));
  drm_evctx.version = DRM_EVENT_CONTEXT_VERSION;
  drm_evctx.page_flip_handler = drm_flip_handler;
  store_saved_crtc(dev.resources);

  while (!req_exit_) {
    int ret;
    int render_jb_id = (cur_jb_id + 1) % 2;
    std::shared_ptr<JoinBO> sjb = PopBO();
    JoinBO *jb = sjb.get();
    if (!jb)
      continue;
    if (sub_draw)
      sub_draw->Draw(jb->bo.ptr, jb->width, jb->height);
    if (do_rga_blit) {
      if (!rga_blit_jb(rga, jb, &jbs[render_jb_id], rotation))
        continue;
    }
    // !FLAGS_drm_raw8_mode &&
    if (!WaitPageFlip(1000)) {
      continue;
    }
    if (do_rga_blit) {
      static auto do_nothing = [](JoinBO *jj UNUSED) {};
      render_jb.reset(&jbs[render_jb_id], do_nothing);
    } else {
      render_jb = sjb;
    }
#if 0
    static int dump_fd = -1;
    static int frame_num = 0;
    char *path = "/tmp/dump.yuyv";
    DumpJoinBOToFile(render_jb.get(), path,
                     render_jb->width * render_jb->height * 2, 300, 450,
                     dump_fd, frame_num);
#endif
    std::pair<uint32_t, bool> &render_fb =
        get_fb_id(dev.fd, jb_fb_map, render_jb.get());
    render_fb_id = render_fb.first;
    if (render_fb_id == 0)
      continue;
    // printf("render_fb_id : %d<ready:%d>, render_fb w/h : %d/%d\n",
    //        render_fb_id, render_fb.second,
    //        render_jb->width, render_jb->height);
    if (!render_fb.second) {
      static Format f = static_cast<Format>(FLAGS_format);
      int w = FLAGS_drm_raw8_mode ? render_jb->width * get_format_bits(f) / 8
                                  : render_jb->width;
      av_log(NULL, AV_LOG_INFO,
             "drmModeSetPlane www: %d; render_jb->width: %d; "
             "get_format_bits(f): %d\n",
             w, render_jb->width, get_format_bits(f));
      ret = drmModeSetPlane(dev.fd, plane_id, crtc_id, render_fb_id, 0, 0, 0, w,
                            render_jb->height, 0, 0, w << 16,
                            render_jb->height << 16);
      if (ret)
        av_log(NULL, AV_LOG_FATAL, "drmModeSetPlane failed, %m\n");
      render_fb.second = (ret == 0);
    }
    if (!render_fb.second)
      continue;
    if (FLAGS_debug)
      printf("!!! drmModePageFlip fb id = %d\n", render_fb_id);
    //!! drmModeSetPlane work some problems with drmModePageFlip
    ret = drmModePageFlip(dev.fd, crtc_id, render_fb_id,
                          DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);
    if (ret == 0) {
      cur_jb_id = render_jb_id;
      waiting_for_flip = true;
    } else {
      av_log(NULL, AV_LOG_WARNING, "Could not queue pageflip: %m\n");
    }
  }

  WaitPageFlip(10000);
  restore_saved_crtc(dev.fd, dev.resources);
  clear_fb_id(dev.fd, jb_fb_map);
}

#if RKNNCASCADE
class NpuUsbLeaf : public LeafHandler {
public:
  NpuUsbLeaf(int drmfd, std::shared_ptr<RK::RknnCascade> c, std::vector<int> d,
             std::vector<std::pair<int, int>> offsets, int w, int h);
  virtual ~NpuUsbLeaf();
  virtual bool Prepare() override;
  virtual void SubRun() override;

  int drm_fd;
  std::shared_ptr<RK::RknnCascade> cascade;
  std::vector<int> device_ids;
  std::vector<std::pair<int, int>> coord_offsets;
  SDLDisplayLeaf *disp_leaf;
  RockchipRga rga;
  int scale_width, scale_height;
  JoinBO scale_jb;
  std::vector<void *> sub_image_datas;
};

NpuUsbLeaf::NpuUsbLeaf(int drmfd, std::shared_ptr<RK::RknnCascade> c,
                       std::vector<int> d,
                       std::vector<std::pair<int, int>> offsets, int w, int h)
    : drm_fd(drmfd), cascade(c), device_ids(d), coord_offsets(offsets),
      scale_width(w), scale_height(h) {}

NpuUsbLeaf::~NpuUsbLeaf() {
  FreeJoinBO(drm_fd, rga, &scale_jb);
  for (void *data : sub_image_datas)
    free(data);
}

bool NpuUsbLeaf::Prepare() {
  if (scale_width > 0 &&
      !AllocJoinBO(drm_fd, rga, &scale_jb, scale_width, scale_height))
    return false;
  int bpp_num, bpp_den;
  get_format_rational(static_cast<Format>(FLAGS_format), bpp_num, bpp_den);
  for (size_t i = 0; i < device_ids.size(); i++) {
    void *image_data = malloc(FLAGS_npu_piece_width * FLAGS_npu_piece_height *
                              bpp_num / bpp_den);
    if (!image_data)
      return false;
    sub_image_datas.push_back(image_data);
  }
  return LeafHandler::Prepare();
}

void NpuUsbLeaf::SubRun() {
  uint8_t *dst_data[4];
  int dst_linesizes[4];
  uint8_t *src_data[4];
  int src_linesizes[4];
  enum AVPixelFormat pix_fmt =
      get_av_pixel_fmt(static_cast<Format>(FLAGS_format));
  int bpp_num, bpp_den;

  get_format_rational(static_cast<Format>(FLAGS_format), bpp_num, bpp_den);

  memset(dst_data, 0, sizeof(dst_data));
  memset(dst_linesizes, 0, sizeof(dst_linesizes));
  dst_linesizes[0] = FLAGS_npu_piece_width * bpp_num / bpp_den;
  memset(src_data, 0, sizeof(src_data));
  memset(src_linesizes, 0, sizeof(src_linesizes));

  int skip_frame = 0;
  while (!req_exit_) {
    std::shared_ptr<JoinBO> sjb = PopBO();
    JoinBO *jb = sjb.get();
    if (!jb)
      continue;
    if ((skip_frame++) % 3) {
      continue;
    } else {
      skip_frame = 0;
    }
    if (scale_width > 0 && !rga_blit_jb(rga, jb, &scale_jb))
      continue;
    for (size_t i = 0; i < device_ids.size(); i++) {
      std::pair<int, int> &xy = coord_offsets[i];
      // do not support yuv
      dst_data[0] = (uint8_t *)sub_image_datas[i];
      uint8_t *src_ptr;
      if (scale_width > 0) {
        src_ptr = ((uint8_t *)scale_jb.bo.ptr);
        src_linesizes[0] = scale_jb.width * bpp_num / bpp_den;
      } else {
        src_ptr = ((uint8_t *)jb->bo.ptr);
        src_linesizes[0] = jb->width * bpp_num / bpp_den;
      }
      src_data[0] =
          src_ptr + xy.first * bpp_num / bpp_den + xy.second * src_linesizes[0];
      av_image_copy(dst_data, dst_linesizes, (const uint8_t **)src_data,
                    (const int *)src_linesizes, pix_fmt, FLAGS_npu_piece_width,
                    FLAGS_npu_piece_height);
    }
    for (size_t i = 0; i < device_ids.size(); i++) {
      int device_id = device_ids[i];
      if (cascade->setInput(sub_image_datas[i],
                            FLAGS_npu_piece_width * FLAGS_npu_piece_height *
                                bpp_num / bpp_den,
                            device_id) < 0) {
        av_log(NULL, AV_LOG_INFO, "device id %d fail to set input\n",
               device_id);
      }
    }
  }
}

class NpuOutPutLeaf;
static void output_processer(NpuOutPutLeaf *npl, int index, int device_id);
class NpuOutPutLeaf : public LeafHandler {
public:
  NpuOutPutLeaf(std::shared_ptr<RK::RknnCascade> c, std::vector<int> d,
                std::vector<std::pair<int, int>> offsets,
                SDLDisplayLeaf *sdlleaf, float xs, float ys);
  // virtual bool Prepare() override;
  virtual void SubRun() override;

  std::shared_ptr<RK::RknnCascade> cascade;
  std::vector<int> device_ids;
  std::vector<std::pair<int, int>> coord_offsets;
  SDLDisplayLeaf *disp_leaf;
  bool coord_need_adjust;
  float x_scale, y_scale;
  std::vector<std::thread *> output_threads;
  friend void output_processer(NpuOutPutLeaf *npl, int index, int device_id);
};

NpuOutPutLeaf::NpuOutPutLeaf(std::shared_ptr<RK::RknnCascade> c,
                             std::vector<int> d,
                             std::vector<std::pair<int, int>> offsets,
                             SDLDisplayLeaf *sdlleaf, float xs, float ys)
    : cascade(c), device_ids(d), coord_offsets(offsets), disp_leaf(sdlleaf),
      x_scale(xs), y_scale(ys) {
  coord_need_adjust = !offsets.empty();
}
void NpuOutPutLeaf::SubRun() {
  int index = 0;
  for (int device_id : device_ids) {
    std::thread *th = new std::thread(output_processer, this, index, device_id);
    if (!th) {
      av_log(NULL, AV_LOG_FATAL, "Fail to create thread for npu get output\n");
      request_exit = 1;
      break;
    }
    output_threads.push_back(th);
    index++;
  }
  while (!req_exit_) {
    msleep(200);
  }
  for (std::thread *th : output_threads) {
    th->join();
    delete th;
  }
  output_threads.clear();
}

void output_processer(NpuOutPutLeaf *npl, int index, int device_id) {
  SDLDisplayLeaf *disp_leaf = npl->disp_leaf;
  std::shared_ptr<RK::RknnCascade> cascade = npl->cascade;
  int x = 0, y = 0;
  av_log(NULL, AV_LOG_INFO, "Output processer start, index=%d, device_id=%d\n",
         index, device_id);
  if (npl->coord_need_adjust) {
    std::pair<int, int> &xy = npl->coord_offsets[index];
    x = xy.first;
    y = xy.second;
  }

  while (!npl->is_req_exit() && !cascade->getClosed()) {
    void *data = NULL;
    int length = 0;
    if (cascade->getOutput(&data, &length, device_id) < 0)
      continue;

    if (length != sizeof(detect_result_group_t))
      continue;

    std::shared_ptr<NpuRectList> nr_list = std::make_shared<NpuRectList>();
    if (!nr_list)
      continue;
    nr_list->device_id = device_id;
    nr_list->timeout = 500; // 1s

    detect_result_group_t *group = (detect_result_group_t *)data;
    for (int i = 0; i < group->count; i++) {
      int left = group->results[i].box.left;
      int top = group->results[i].box.top;
      int right = group->results[i].box.right;
      int bottom = group->results[i].box.bottom;
      float prop = group->results[i].prop;
      const char *label = group->results[i].name;
      if (FLAGS_debug)
        printf("device_id = %d, result %2d: (%4d, %4d, %4d, %4d), %4.2f, %s\n",
               device_id, i, left, top, right, bottom, prop, label);
      NpuRect nr;
      // static char words[OBJ_NAME_MAX_SIZE + 16];
      // snprintf(words, sizeof(words), "%s %d%%", label, (int)(prop * 100));
      nr.word = label;
      // printf("word: %s\n", nr.word.c_str());
      // printf("x1_scale: %f, y1_scale: %f; offset: %d, %d\n",
      //         npl->x_scale, npl->y_scale, x, y);
      nr.left = (left + x) * npl->x_scale;
      nr.top = (top + y) * npl->y_scale;
      nr.right = (right + x) * npl->x_scale;
      nr.bottom = (bottom + y) * npl->y_scale;
      nr_list->list.push_back(nr);
    }
    nr_list->got_time = av_gettime_relative() / 1000LL;
    if (disp_leaf)
      disp_leaf->SetFaceRectAt(index, nr_list);
  }
  av_log(NULL, AV_LOG_INFO, "Output processer exit, device_id=%d\n", device_id);
}
#endif

RockchipRga global_rga;

typedef bool (*image_process)(JoinBO *in, JoinBO *out);
class ImageFilterLeaf : public LeafHandler {
public:
  ImageFilterLeaf(int drmfd);
  virtual ~ImageFilterLeaf();
  bool Prepare(int max_buffer_num, int w, int h, Format f);
  virtual void SubRun() override;

  void AddLeaf(LeafHandler *h) {
    std::lock_guard<std::mutex> _lg(leaf_mutex);
    leaf_handler_list.push_back(h);
  }
  //   void RemoveLeaf(LeafHandler *h) {
  //     std::lock_guard<std::mutex> _lg(leaf_mutex);
  //     leaf_handler_list.remove(h);
  //   }
  bool StartLeafs() {
    std::list<LeafHandler *> leafs;
    leaf_mutex.lock();
    leafs = leaf_handler_list;
    leaf_mutex.unlock();
    for (auto l : leafs) {
      l->Start();
    }
    return true;
  }
  void StopAndRemoveLeafs() {
    std::list<LeafHandler *> leafs;
    leaf_mutex.lock();
    leafs = leaf_handler_list;
    leaf_mutex.unlock();
    for (auto l : leafs)
      l->Stop();
    leaf_mutex.lock();
    for (auto l : leaf_handler_list)
      delete l;
    leaf_mutex.unlock();
  }
  void dispatch_buffer(std::shared_ptr<JoinBO> jb) {
    for (auto h : leaf_handler_list)
      h->PushBO(jb);
  }

  int drm_fd;
  image_process func;
  spinlock_mutex mtx;
  std::list<JoinBO *> buffer_list;

  std::mutex leaf_mutex;
  std::list<LeafHandler *> leaf_handler_list;
};

ImageFilterLeaf::ImageFilterLeaf(int drmfd) : drm_fd(drmfd), func(nullptr) {}
ImageFilterLeaf::~ImageFilterLeaf() {
  StopAndRemoveLeafs();
  for (JoinBO *jb : buffer_list)
    FreeJoinBO(drm_fd, global_rga, jb);
}

bool ImageFilterLeaf::Prepare(int max_buffer_num, int w, int h, Format f) {
  assert(max_buffer_num > 1);
  for (int i = 0; i < max_buffer_num; i++) {
    JoinBO *jb = new JoinBO();
    assert(jb);
    if (!AllocJoinBO(drm_fd, global_rga, jb, w, h))
      return false;
    jb->fmt = f;
    buffer_list.push_back(jb);
  }
  return LeafHandler::Prepare();
}

void ImageFilterLeaf::SubRun() {
  while (!req_exit_) {
    JoinBO *output_jb = nullptr;
    std::shared_ptr<JoinBO> sjb = PopBO();
    JoinBO *input_jb = sjb.get();
    if (!input_jb)
      continue;
    mtx.lock();
    if (!buffer_list.empty()) {
      output_jb = buffer_list.front();
      buffer_list.pop_front();
    }
    mtx.unlock();
    if (!output_jb)
      continue;
    static auto recyle_fun = [this](JoinBO *jj) {
      mtx.lock();
      buffer_list.push_back(jj);
      mtx.unlock();
    };
    std::shared_ptr<JoinBO> out(output_jb, recyle_fun);
    int ret = (*func)(input_jb, output_jb);
    if (ret)
      dispatch_buffer(out);
  }
}

static bool bo_scale(JoinBO *in, JoinBO *out) {
  rga_info_t src, dst;
  memset(&src, 0, sizeof(src));
  memset(&dst, 0, sizeof(dst));
  src.fd = in->fd;
  src.mmuFlag = 1;
  rga_set_rect(&src.rect, 0, 0, in->width, in->height, in->width, in->height,
               get_rga_format(in->fmt));
  dst.fd = out->fd;
  dst.mmuFlag = 1;
  rga_set_rect(&dst.rect, 0, 0, out->width, out->height, out->width,
               out->height, get_rga_format(out->fmt));
  int ret = global_rga.RkRgaBlit(&src, &dst, NULL);
  if (ret) {
    av_log(NULL, AV_LOG_ERROR, "RkRgaBlit error : %s\n", strerror(ret));
    return false;
  }
  return true;
}

#if HAVE_ROCKX
NpuModelLeaf::NpuModelLeaf(int draw_idx, int64_t timeout)
    : img_func(nullptr), draw(nullptr), show_func(nullptr),
      draw_index(draw_idx), disp_timeout(timeout) {}

NpuModelLeaf::~NpuModelLeaf() {
  for (auto handle : npu_handles)
    rockx_destroy(handle);
}

bool NpuModelLeaf::Prepare(std::string model_name) {
  std::vector<rockx_module_t> models;
  void *config = nullptr;
  size_t config_size = 0;
  if (model_name == "face_landmark") {
    models.push_back(ROCKX_MODULE_FACE_DETECTION);
    models.push_back(ROCKX_MODULE_FACE_LANDMARK_68);
  } else if (model_name == "pose_finger21") {
    models.push_back(ROCKX_MODULE_POSE_FINGER_21);
  } else if (model_name == "pose_finger3") {
    models.push_back(ROCKX_MODULE_POSE_FINGER_3);
  } else if (model_name == "pose_body") {
    models.push_back(ROCKX_MODULE_POSE_BODY);
  } else if (model_name == "object_detect") {
    models.push_back(ROCKX_MODULE_OBJECT_DETECTION);
  } else {
    av_log(NULL, AV_LOG_FATAL, "TODO: %s\n", model_name.c_str());
    return false;
  }
  npu_handles.resize(models.size(), nullptr);
  for (size_t i = 0; i < models.size(); i++) {
    rockx_handle_t npu_handle = nullptr;
    rockx_module_t model = models[i];
    rockx_ret_t ret = rockx_create(&npu_handle, model, config, config_size);
    if (ret != ROCKX_RET_SUCCESS) {
      av_log(NULL, AV_LOG_FATAL, "init rockx module %d error=%d\n", model, ret);
      return false;
    }
    npu_handles[i] = npu_handle;
  }
  return LeafHandler::Prepare();
}

void NpuModelLeaf::SubRun() {
  while (!req_exit_) {
    std::shared_ptr<JoinBO> sjb = PopBO();
    JoinBO *jb = sjb.get();
    if (!jb)
      continue;
    rockx_image_t input_image;
    input_image.width = jb->width;
    input_image.height = jb->height;
    input_image.data = (uint8_t *)jb->bo.ptr;
    input_image.pixel_format = ROCKX_PIXEL_FORMAT_BGR888;
    void *npu_out = img_func(npu_handles, &input_image);
    if (npu_out) {
#if 0
      static char name[64];
      static int dump_fd = -1;
      sprintf(name, "/tmp/dump_%d_%d.rgb888", jb->width, jb->height);
      dump_fd = open(name, O_RDWR | O_CREAT | O_CLOEXEC);
      assert(dump_fd >= 0);
      write(dump_fd, jb->bo.ptr, jb->width * jb->height * 3);
      close(dump_fd);
#endif
      auto nr = std::make_shared<NpuResult>();
      nr->leaf = this;
      nr->npu_out = npu_out;
      nr->timeout = disp_timeout;
      nr->index = draw_index;
      draw->Set(draw_index, nr);
    }
  }
}

#include <SDL2/SDL2_gfxPrimitives.h>

static SDL_Texture *load_texture(SDL_Surface *sur, SDL_Renderer *render,
                                 SDL_Rect *texture_dimensions) {
  SDL_Texture *texture = SDL_CreateTextureFromSurface(render, sur);
  assert(texture);
  Uint32 pixelFormat;
  int access;
  texture_dimensions->x = 0;
  texture_dimensions->y = 0;
  SDL_QueryTexture(texture, &pixelFormat, &access, &texture_dimensions->w,
                   &texture_dimensions->h);
  return texture;
}

static void *face_landmark_process(std::vector<rockx_handle_t> &npu_handles,
                                   rockx_image_t *input_image) {
  rockx_handle_t face_det_handle = npu_handles[0];
  rockx_object_array_t face_array;
  memset(&face_array, 0, sizeof(rockx_object_array_t));

  rockx_ret_t ret =
      rockx_face_detect(face_det_handle, input_image, &face_array, nullptr);
  if (ret != ROCKX_RET_SUCCESS) {
    printf("rockx_face_detect error %d\n", ret);
    return nullptr;
  }
  // select the max big face
  rockx_object_t face;
  int area = 0;
  for (int i = 0; i < face_array.count; i++) {
    rockx_object_t &f = face_array.object[i];
    int left = f.box.left;
    int top = f.box.top;
    int right = f.box.right;
    int bottom = f.box.bottom;
    // float score = f.score;
    // printf("box=(%d %d %d %d) score=%f\n", left, top, right, bottom, score);
    int a = (right - left) * (bottom - top);
    if (area < a && top > 5) {
      area = a;
      face = f;
    }
  }
  if (area <= 8)
    return nullptr;
  rockx_handle_t face_landmark_handle = npu_handles[1];
  rockx_face_landmark_t *out_landmark =
      (rockx_face_landmark_t *)malloc(sizeof(rockx_face_landmark_t));
  memset(out_landmark, 0, sizeof(rockx_face_landmark_t));

  ret = rockx_face_landmark(face_landmark_handle, input_image, &face.box,
                            out_landmark);

  // printf("landmarks_count = %d, euler_angles = {Pitch:%.1f, Yaw:%.1f,
  // Roll:%.1f}\n", out_landmark->landmarks_count,
  // out_landmark->euler_angles[0], out_landmark->euler_angles[1],
  // out_landmark->euler_angles[2]);

  return out_landmark;
}

static int face_landmark_draw(void *npu_out, void *disp_ptr UNUSED, int disp_w,
                              int disp_h, SDL_Renderer *render) {
  static const std::vector<std::pair<int, int>> landmarkPairs = {
      {0, 1},   {1, 2},   {2, 3},   {3, 4},   {4, 5},   {5, 6},   {6, 7},
      {7, 8},   {8, 9},   {9, 10},  {10, 11}, {11, 12}, {12, 13}, {13, 14},
      {14, 15}, {15, 16}, {17, 18}, {18, 19}, {19, 20}, {20, 21}, {22, 23},
      {23, 24}, {24, 25}, {25, 26}, {27, 28}, {28, 29}, {29, 30}, {31, 32},
      {32, 33}, {33, 34}, {34, 35}, {36, 37}, {37, 38}, {38, 39}, {39, 40},
      {40, 41}, {41, 36}, {42, 43}, {43, 44}, {44, 45}, {45, 46}, {46, 47},
      {47, 42}, {48, 49}, {49, 50}, {50, 51}, {51, 52}, {52, 53}, {53, 54},
      {54, 55}, {55, 56}, {56, 57}, {57, 58}, {58, 59}, {59, 48}, {60, 61},
      {61, 62}, {62, 63}, {63, 64}, {64, 65}, {65, 66}, {66, 67}, {67, 60}};
  static int w = FLAGS_npu_piece_width;
  static int h = FLAGS_npu_piece_height;
  static float scale_w = (float)disp_w / (float)w;
  static float scale_h = (float)disp_h / (float)h;

  rockx_face_landmark_t *landmark = (rockx_face_landmark_t *)npu_out;
#if 1
  // let this leak
  static SDL_Surface *sur_point =
      SDL_LoadBMP("/usr/local/share/app_demo/resource/dot_mobilenet.bmp");
  assert(sur_point);
  SDL_Rect point_texture_dimension;
  SDL_Texture *point_texture =
      load_texture(sur_point, render, &point_texture_dimension);
  assert(point_texture);

  SDL_SetRenderDrawBlendMode(render, SDL_BLENDMODE_BLEND);
  SDL_Rect dst_dimension;
  dst_dimension.w = point_texture_dimension.w;
  dst_dimension.h = point_texture_dimension.h;
  for (int j = 0; j < landmark->landmarks_count; j++) {
    float x0 = landmark->landmarks[j].x;
    float y0 = landmark->landmarks[j].y;
    if (x0 > 0 && y0 > 0) {
      dst_dimension.x = x0 * scale_w - dst_dimension.w / 2;
      dst_dimension.y = y0 * scale_h - dst_dimension.h / 2;
      SDL_RenderCopy(render, point_texture, &point_texture_dimension,
                     &dst_dimension);
      // SDL_RenderDrawPoint(render, landmark->landmarks[j].x *
      // scale_w,landmark->landmarks[j].y * scale_h);
    }
  }
  SDL_DestroyTexture(point_texture);
#else
  SDL_SetRenderDrawBlendMode(render, SDL_BLENDMODE_NONE);
  SDL_SetRenderDrawColor(render, 0xFF, 0xEB, 0x06, 0xFF);
  int count = landmark->landmarks_count;
  for (size_t j = 0; j < landmarkPairs.size(); j++) {
    const std::pair<int, int> &landmarkPair = landmarkPairs[j];
    int idx1 = landmarkPair.first;
    int idx2 = landmarkPair.second;
    if (idx1 >= count || idx2 >= count)
      break;
    float x0 = landmark->landmarks[idx1].x * scale_w;
    float y0 = landmark->landmarks[idx1].y * scale_h;
    float x1 = landmark->landmarks[idx2].x * scale_w;
    float y1 = landmark->landmarks[idx2].y * scale_h;
    if (x0 > 0 && y0 > 0 && x1 > 0 && y1 > 0) {
      SDL_RenderDrawLine(render, x0, y0, x1, y1);
    }
  }
#endif
  return 0;
}

static void *pose_finger_process(std::vector<rockx_handle_t> &npu_handles,
                                 rockx_image_t *input_image) {
  rockx_handle_t pose_finger_handle = npu_handles[0];
  rockx_keypoints_t *finger =
      (rockx_keypoints_t *)malloc(sizeof(rockx_keypoints_t));
  assert(finger);
  memset(finger, 0, sizeof(rockx_keypoints_t));

  rockx_ret_t ret = rockx_pose_finger(pose_finger_handle, input_image, finger);
  if (ret != ROCKX_RET_SUCCESS) {
    printf("rockx_pose_finger21 error %d\n", ret);
    free(finger);
    return nullptr;
  }
  // printf("finger count: %d\n", finger->count);
  if (finger->count <= 0) {
    free(finger);
    return nullptr;
  }
  return finger;
}

static int pose_finger_draw(void *npu_out, void *disp_ptr UNUSED, int disp_w,
                            int disp_h, SDL_Renderer *render) {
  static const std::vector<std::pair<int, int>> posePairs21 = {
      {0, 1},   {1, 2},   {2, 3},  {3, 4},   {0, 5},   {5, 6},  {6, 7},
      {7, 8},   {0, 9},   {9, 10}, {10, 11}, {11, 12}, {0, 13}, {13, 14},
      {14, 15}, {15, 16}, {0, 17}, {17, 18}, {18, 19}, {19, 20}};
  static const std::vector<std::pair<int, int>> posePairs3 = {{0, 1}, {1, 2}};
  static int w = FLAGS_npu_piece_width;
  static int h = FLAGS_npu_piece_height;
  SDL_SetRenderDrawBlendMode(render, SDL_BLENDMODE_NONE);
  SDL_SetRenderDrawColor(render, 0xFF, 0x06, 0xEB, 0xFF);
  static float scale_w = (float)disp_w / w;
  static float scale_h = (float)disp_h / h;
  rockx_keypoints_t *finger = (rockx_keypoints_t *)npu_out;
  // printf("finger count: %d\n", finger->count);
  const std::vector<std::pair<int, int>> *posePairs = nullptr;
  int thick = 3;
  if (finger->count == 21) {
    posePairs = &posePairs21;
  } else if (finger->count == 3) {
    posePairs = &posePairs3;
    thick = 9;
  } else
    return -1;
  for (size_t j = 0; j < posePairs->size(); j++) {
    const std::pair<int, int> &posePair = (*posePairs)[j];
    float x0 = finger->points[posePair.first].x;
    float y0 = finger->points[posePair.first].y;
    float x1 = finger->points[posePair.second].x;
    float y1 = finger->points[posePair.second].y;
    if (x0 > 0 && y0 > 0 && x1 > 0 && y1 > 0) {
      x0 *= scale_w;
      y0 *= scale_h;
      x1 *= scale_w;
      y1 *= scale_h;
      // printf("[%d, %d] x0, y0 (%f, %f); x1, y1 (%f, %f)\n", disp_w, disp_h,
      // x0, y0, x1, y1);
      thickLineRGBA(render, x0, y0, x1, y1, thick, 255 /*b*/, 138 /*r*/,
                    78 /*g*/, 255);
    }
  }
  for (int i = 0; i < finger->count; i++) {
    float x0 = finger->points[i].x;
    float y0 = finger->points[i].y;
    if (x0 > 0 && y0 > 0) {
      x0 *= scale_w;
      y0 *= scale_h;
      filledCircleRGBA(render, x0, y0, thick + 3, 255, 138, 78, 255);
    }
  }
  return 0;
}

static void *pose_body_process(std::vector<rockx_handle_t> &npu_handles,
                               rockx_image_t *input_image) {
  rockx_handle_t pose_body_handle = npu_handles[0];
  rockx_keypoints_array_t body_array;
  memset(&body_array, 0, sizeof(rockx_keypoints_array_t));

  rockx_ret_t ret =
      rockx_pose_body(pose_body_handle, input_image, &body_array, nullptr);
  if (ret != ROCKX_RET_SUCCESS) {
    printf("rockx_pose_body error %d\n", ret);
    return nullptr;
  }
  // printf("body count: %d\n", body_array.count);
  if (body_array.count <= 0)
    return nullptr;

  // select the bigest body
  int index = -1;
  float area = 0;
  for (int i = 0; i < body_array.count; i++) {
    rockx_keypoints_t &body = body_array.keypoints[i];
    float l = 0, t = 0, r = 0, b = 0;
    for (int j = 0; j < body.count; j++) {
      float x = body.points[j].x;
      float y = body.points[j].y;
      if (x < 0)
        x = 0;
      if (y < 0)
        y = 0;
      if (x < l)
        l = x;
      if (y < t)
        t = y;
      if (x > r)
        r = x;
      if (y > b)
        b = y;
      // printf("boxy[%d] x,y : %f, %f\n", j, x, y);
    }
    if (l >= 0 && t >= 0 && r > 0 && b > 0) {
      float a = (r - l) * (b - t);
      if (area < a) {
        area = a;
        index = i;
      }
    }
    // printf("\n[%d]::\nl,t,r,b : %f,%f,%f,%f; a:%f\n\n", i, l,t,r,b, area);
  }

  rockx_keypoints_t *ret_body = nullptr;
  if (index >= 0 && area > 8) {
    ret_body = (rockx_keypoints_t *)malloc(sizeof(rockx_keypoints_t));
    if (!ret_body)
      return nullptr;
    *ret_body = body_array.keypoints[index];
  }

  return ret_body;
}

static int pose_body_draw(void *npu_out, void *disp_ptr, int disp_w, int disp_h,
                          SDL_Renderer *render) {
  const std::vector<std::pair<int, int>> posePairs = {
      {1, 2}, {1, 5},  {2, 3},   {3, 4},  {5, 6},   {6, 7},
      {1, 8}, {8, 9},  {9, 10},  {1, 11}, {11, 12}, {12, 13},
      {1, 0}, {0, 14}, {14, 16}, {0, 15}, {15, 17}};
  static int w = FLAGS_npu_piece_width;
  static int h = FLAGS_npu_piece_height;
  SDL_SetRenderDrawBlendMode(render, SDL_BLENDMODE_NONE);
  // SDL_SetRenderDrawColor(render, 0xFF, 0x06, 0xEB, 0xFF);
  static float scale_w = (float)disp_w / w;
  static float scale_h = (float)disp_h / h;
  rockx_keypoints_t *body = (rockx_keypoints_t *)npu_out;
  for (size_t j = 0; j < posePairs.size(); j++) {
    const std::pair<int, int> &posePair = posePairs[j];
    float x0 = body->points[posePair.first].x;
    float y0 = body->points[posePair.first].y;
    float x1 = body->points[posePair.second].x;
    float y1 = body->points[posePair.second].y;
    if (x0 > 0 && y0 > 0 && x1 > 0 && y1 > 0) {
      x0 *= scale_w;
      y0 *= scale_h;
      x1 *= scale_w;
      y1 *= scale_h;
      // printf("[%d, %d] body x0, y0 (%f, %f); x1, y1 (%f, %f)\n", disp_w,
      // disp_h, x0, y0, x1, y1); SDL_RenderDrawLine(render, x0, y0, x1, y1);
      thickLineRGBA(render, x0, y0, x1, y1, 5, 255 /*b*/, 78 /*r*/, 138 /*g*/,
                    255);
    }
  }
  for (int i = 0; i < body->count; i++) {
    float x0 = body->points[i].x;
    float y0 = body->points[i].y;
    if (x0 > 0 && y0 > 0) {
      x0 *= scale_w;
      y0 *= scale_h;
      filledCircleRGBA(render, x0, y0, 8, 255, 78, 138, 255);
    }
  }
  // crop
  float x2 = body->points[2].x;
  float y2 = body->points[2].y;
  float x5 = body->points[5].x;
  float y5 = body->points[5].y;
  float x4 = body->points[4].x;
  float y4 = body->points[4].y;
  float x7 = body->points[7].x;
  float y7 = body->points[7].y;
  if (((x2 > 0 && y2 > 0) || (x5 > 0 && y5 > 0)) &&
      ((x4 > 0 && y4 > 0) || (x7 > 0 && y7 > 0))) {
    float jian_y = h;
    if (x2 > 0 && y2 > 0)
      jian_y = y2;
    if ((x5 > 0 && y5 > 0) && jian_y > y5)
      jian_y = y5;
    float shou_x = 0, shou_y = h;
    if (x4 > 0 && y4 > 0) {
      shou_x = x4;
      shou_y = y4;
    }
    if ((x7 > 0 && y7 > 0) && shou_y > y7) {
      shou_x = x7;
      shou_y = y7;
    }
    // printf("shou_y: %f, jian_y: %f\n", shou_y, jian_y);
    if (shou_y <= jian_y + 20) {
      static int ww = 100;
      static int dst_x = disp_w - ww * 2;
      static int dst_y = ww;
      int x = shou_x * scale_w - ww;
      if (x < 0)
        x = 0;
      int y = shou_y * scale_h - ww * 2;
      if (y < 0)
        y = 0;
      static int bits = get_format_bits(static_cast<Format>(FLAGS_format));
      int pitch = disp_w * bits / 8;
      uint8_t *dest = (uint8_t *)disp_ptr + dst_y * pitch + dst_x * bits / 8;
      uint8_t *src = (uint8_t *)disp_ptr + y * pitch + x * bits / 8;
      for (int i = 0; i < ww * 2; i++) {
        memmove(dest + pitch * i, src + pitch * i, ww * 2 * bits / 8);
      }
    }
  }
  return 0;
}

static void *ssd_process(std::vector<rockx_handle_t> &npu_handles,
                         rockx_image_t *input_image) {
  rockx_handle_t object_det_handle = npu_handles[0];
  rockx_object_array_t *object_array =
      (rockx_object_array_t *)malloc(sizeof(rockx_object_array_t));
  assert(object_array);
  memset(object_array, 0, sizeof(rockx_object_array_t));

  rockx_ret_t ret = rockx_object_detect(object_det_handle, input_image,
                                        object_array, nullptr);
  if (ret != ROCKX_RET_SUCCESS) {
    printf("rockx_object_detect error %d\n", ret);
    free(object_array);
    return nullptr;
  }
  // printf("object_array count: %d\n", object_array->count);
  if (object_array->count <= 0) {
    free(object_array);
    return nullptr;
  }
  return object_array;
}

static int ssd_draw(void *npu_out, void *disp_ptr UNUSED, int disp_w,
                    int disp_h, SDL_Renderer *render) {
  static int w = FLAGS_npu_piece_width;
  static int h = FLAGS_npu_piece_height;

  static SDLFont *sdl_font = nullptr;

  if (!sdl_font) {
    // let this leak
    sdl_font = new SDLFont(red, 16);
    assert(sdl_font);
  }

  SDL_SetRenderDrawBlendMode(render, SDL_BLENDMODE_NONE);
  SDL_SetRenderDrawColor(render, 0xFF, 0x10, 0xEB, 0xFF);
  rockx_object_array_t *object_array = (rockx_object_array_t *)npu_out;
  for (int i = 0; i < object_array->count; i++) {
    int left = object_array->object[i].box.left;
    int top = object_array->object[i].box.top;
    int right = object_array->object[i].box.right;
    int bottom = object_array->object[i].box.bottom;
    // int cls_idx = object_array->object[i].cls_idx;
    const char *cls_name =
        OBJECT_DETECTION_LABELS_91[object_array->object[i].cls_idx];
    // float score = object_array->object[i].score;
    // printf("~~~ box=(%d %d %d %d) cls_name=%s, score=%f\n", left, top, right,
    // bottom, cls_name, score);
    SDL_Rect rect = {left * disp_w / w, top * disp_h / h,
                     (right - left) * disp_w / w, (bottom - top) * disp_h / h};
    SDL_RenderDrawRect(render, &rect);
    int fontw = 0, fonth = 0;
    SDL_Surface *name = sdl_font->GetFontPicture(
        (char *)cls_name, strlen(cls_name), 32, &fontw, &fonth);
    if (name) {
      SDL_Rect texture_dimension;
      SDL_Texture *texture = load_texture(name, render, &texture_dimension);
      SDL_FreeSurface(name);
      SDL_Rect dst_dimension;
      dst_dimension.x = rect.x;
      dst_dimension.y = rect.y - 18;
      dst_dimension.w = texture_dimension.w;
      dst_dimension.h = texture_dimension.h;
      SDL_RenderCopy(render, texture, &texture_dimension, &dst_dimension);
      SDL_DestroyTexture(texture);
    }
    // putText(orig_img, cls_name, cv::Point(left, top - 8), 1, 1,
    // colorArray[cls_idx%10], 2);
  }
  return 0;
}

class mode_param {
public:
  std::string rockx_name;
  npu_process img_func;
  result_in show_func;
  int64_t disp_timeout;
};

static std::map<std::string, mode_param> model_map;

static void AddModelToMap(std::string key, std::string rockx_name,
                          npu_process img_func, result_in show_func,
                          int64_t disp_timeout) {
  mode_param param;
  param.rockx_name = rockx_name;
  param.img_func = img_func;
  param.show_func = show_func;
  param.disp_timeout = disp_timeout;
  model_map[key] = param;
}

static bool CreateNpuModel(ImageFilterLeaf *ifl, DrawOperation *draw,
                           std::list<std::string> model_names) {
  if (model_map.empty()) {
    AddModelToMap("ssd", "object_detect", ssd_process, ssd_draw, 300);
    AddModelToMap("pose_finger21", "pose_finger21", pose_finger_process,
                  pose_finger_draw, 300);
    AddModelToMap("pose_finger3", "pose_finger3", pose_finger_process,
                  pose_finger_draw, 2000);
    AddModelToMap("pose_body", "pose_body", pose_body_process, pose_body_draw,
                  300);
    AddModelToMap("face_landmark", "face_landmark", face_landmark_process,
                  face_landmark_draw, 300);
  }
  av_log(NULL, AV_LOG_INFO, "CreateNpuModel\n");
  bool ret;
  int index = 0;
  for (auto &name : model_names) {
    mode_param param = model_map[name];
    NpuModelLeaf *leaf = new NpuModelLeaf(index++, param.disp_timeout);
    assert(leaf);
    ret = leaf->Prepare(param.rockx_name);
    assert(ret);
    leaf->draw = draw;
    leaf->img_func = param.img_func;
    leaf->show_func = param.show_func;
    ifl->AddLeaf(leaf);
  }

  return true;
}

#endif // HAVE_ROCKX

static void quit_program(const char *func, int line) {
  av_log(NULL, AV_LOG_FATAL, "quit at %s : %d\n", func, line);
  exit(-1);
}
#define QUIT_PROGRAM() quit_program(__FUNCTION__, __LINE__)

static DISPLAY_TIMING disp_time;
static volatile int received_sigterm = 0;
static struct termios oldtty;
static int restore_tty = 0;
// termios.h
static int read_key(void) {
  unsigned char ch;
  int n = 1;
  struct timeval tv;
  fd_set rfds;

  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  n = select(1, &rfds, NULL, NULL, &tv);
  if (n > 0) {
    n = read(0, &ch, 1);
    if (n == 1)
      return ch;

    return n;
  }
  return -1;
}

static int check_keyboard_interaction() {
  int key;

  if (!FLAGS_tty)
    return 0;

  /* read_key() returns 0 on EOF */
  key = read_key();
  if (key == 'q')
    return AVERROR_EXIT;
  if (key == '+')
    av_log_set_level(av_log_get_level() + 10);
  if (key == '-')
    av_log_set_level(av_log_get_level() - 10);
  if (key == 'd')
    av_log_set_level(AV_LOG_DEBUG);
  if (key == 'h') {
    fprintf(stderr, "log level: %d\n", av_log_get_level());
  }
  if (key == '?') {
    fprintf(stderr, "key    function\n"
                    "?      show this help\n"
                    "+      increase verbosity\n"
                    "-      decrease verbosity\n"
                    "d      cycle through available debug modes\n"
                    "h      dump\n"
                    "q      quit\n");
  }

  return 0;
}

static void sigterm_handler(int sig) {
  received_sigterm = sig;
#if 0
    if (restore_tty)
        tcsetattr(0, TCSANOW, &oldtty);
    exit(-1);
#else
  request_exit = 1;
#endif
}

static void term_init() {
  if (FLAGS_tty) {
    struct termios tty;
    if (tcgetattr(0, &tty) == 0) {
      oldtty = tty;
      restore_tty = 1;

      tty.c_iflag &=
          ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
      tty.c_oflag |= OPOST;
      tty.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
      tty.c_cflag &= ~(CSIZE | PARENB);
      tty.c_cflag |= CS8;
      tty.c_cc[VMIN] = 1;
      tty.c_cc[VTIME] = 0;

      tcsetattr(0, TCSANOW, &tty);
    }
  }

  signal(SIGQUIT, sigterm_handler); /* Quit (POSIX).  */

  signal(SIGINT, sigterm_handler);
  signal(SIGTERM, sigterm_handler);
  signal(SIGXCPU, sigterm_handler);
  signal(SIGPIPE, SIG_IGN);
}

int main(int argc, char **argv) {
  std::string version(CODE_ONE_LINE);
  gflags::SetVersionString(version);
  std::string usage("This program does image process by some processor.");
  usage += "\n\n\tSample usage:\n\t";
  usage += argv[0];
  usage +=
      " -input=\"/userdata/a.mp4 /userdata/b.mp4 \" -processor=npu -disp -debug"
      " -disp_time=\"post_processor\" -disp_rotate=90 -width=xxxx -height=xxxx"
      " -slice_num=xxx -join_round_buffer_num=xxx";
  gflags::SetUsageMessage(usage);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  const char *module = "rockchip";
  int global_drm_fd = drmOpen(module, NULL);
  if (global_drm_fd < 0) {
    fprintf(stderr, "drm failed to open device '%s': %s\n", module,
            strerror(errno));
    QUIT_PROGRAM();
  }

  // av_log_set_flags(AV_LOG_SKIP_REPEATED);
  // if (FLAGS_debug)
  //     av_log_set_level(AV_LOG_DEBUG);

  int slice_num = 0;
  double frame_rate = 0.0f;
  std::string token;
  std::istringstream tokenStream(FLAGS_input);

  std::vector<std::unique_ptr<Input>> input_vector;
  std::unique_ptr<Join> join(new Join(global_drm_fd));
#if RKNNCASCADE
  std::shared_ptr<RK::RknnCascade> cascade;
#endif

  if (FLAGS_join_round_buffer_num < 2)
    QUIT_PROGRAM();

  if (!join.get())
    QUIT_PROGRAM();

  if (!join->Prepare(FLAGS_width, FLAGS_height, FLAGS_slice_num,
                     FLAGS_join_round_buffer_num))
    QUIT_PROGRAM();

#if RKNNCASCADE
  // npu start is slow, load it first
  std::vector<int> device_ids;
  std::vector<std::pair<int, int>> offsets;
  uint32_t npu_w = 0, npu_h = 0;
  if (FLAGS_processor == "npu") {
    npu_w = FLAGS_npu_piece_width * join->GetLineSliceNum();
    npu_h = FLAGS_npu_piece_height * join->GetLineSliceNum();
    if (npu_w == FLAGS_width && npu_h == FLAGS_height) {
      // do not need middle buffer
      npu_w = 0;
      npu_h = 0;
    }
    cascade = RK::RknnCascade::create();
    if (!cascade || cascade->open() < 0)
      QUIT_PROGRAM();
    while (true) {
      int ret = cascade->findDevices(device_ids);
      av_log(NULL, AV_LOG_INFO, "Finding device, num = %d\n", ret);
      if (ret == 4)
        break;
      av_log(NULL, AV_LOG_INFO, "Wait all 4 npu devices ready ...\n");
      sleep(1);
    }
    if (FLAGS_npu_data_source == "usb") {
      // test set input crop
      int x = 0, y = 0;
      for (int i = 0; i < (int)device_ids.size(); i++) {
        x = (i % join->GetLineSliceNum()) * FLAGS_npu_piece_width;
        y = (i / join->GetLineSliceNum()) * FLAGS_npu_piece_height;
        av_log(NULL, AV_LOG_INFO, "Find device id = %d, set crop w/h: %d/%d\n",
               device_ids[i], FLAGS_npu_piece_width, FLAGS_npu_piece_height);
        cascade->setInputCrop(0, 0, FLAGS_npu_piece_width,
                              FLAGS_npu_piece_height, device_ids[i]);
        offsets.push_back(std::pair<int, int>(x, y));
      }
    } else if (FLAGS_npu_data_source == "drm") {
      // test set input crop
      int x = 0, y = 0;
      for (int i = 0; i < (int)device_ids.size(); i++) {
        x = (i % join->GetLineSliceNum()) * FLAGS_npu_piece_width;
        y = (i / join->GetLineSliceNum()) * FLAGS_npu_piece_height;
        av_log(NULL, AV_LOG_INFO,
               "Find device id = %d, set crop x/y,w/h: %d/%d, %d/%d\n",
               device_ids[i], x, y, FLAGS_npu_piece_width,
               FLAGS_npu_piece_height);
        cascade->setInputCrop(x, y, FLAGS_npu_piece_width,
                              FLAGS_npu_piece_height, device_ids[i]);
      }
      // test start mipi stream
      for (int i = (int)device_ids.size() - 1; i >= 0; i--) {
        int ret = cascade->startMipiStream(0, 0, 0, device_ids[i]);
        if (ret)
          av_log(NULL, AV_LOG_ERROR,
                 "Npu device id %d start mipi stream"
                 "failed\n",
                 device_ids[i]);
        msleep(500);
      }
    } else {
      QUIT_PROGRAM();
    }
    // load model
    if (cascade->loadModel(FLAGS_npu_model_path.c_str()) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Fail to load npu model : %s\n",
             FLAGS_npu_model_path.c_str());
      QUIT_PROGRAM();
    }
  }
#endif
  av_register_all();
  avformat_network_init();
  while (std::getline(tokenStream, token, ' ')) {
    Input *i = nullptr;
    if (!strncmp(token.c_str(), "/dev/video", 10)) {
      i = new CameraInput(global_drm_fd, token);
    } else {
      i = new Input(token);
    }
    std::unique_ptr<Input> input(i);
    if (input.get() && input->Prepare()) {
      if (input->fps > frame_rate)
        frame_rate = input->fps;
      input->set_slice_index(slice_num);
      input->set_join_handler(join.get());
      if (input->realtime)
        input->Start();
      input_vector.push_back(std::move(input));
    } else
      QUIT_PROGRAM();
    slice_num++;
    if (slice_num >= FLAGS_slice_num)
      break;
  }
  if (slice_num == 0)
    QUIT_PROGRAM();
  join->set_frame_rate(frame_rate);

  std::list<LeafHandler *> leafs;

  DrawOperation *draw = nullptr;
#if HAVE_ROCKX
  ImageFilterLeaf *ifl = nullptr;
  if (FLAGS_processor == "npu" && FLAGS_npu_data_source == "none" &&
      !FLAGS_npu_model_name.empty()) {
    ifl = new ImageFilterLeaf(global_drm_fd);
    assert(ifl);
    ifl->func = bo_scale;
    leafs.push_back(ifl);
    join->AddLeaf(ifl);
    if (!ifl->Prepare(2, FLAGS_npu_piece_width, FLAGS_npu_piece_height,
                      Format::RGB888))
      QUIT_PROGRAM();

    std::list<std::string> model_names;
    std::string mname;
    std::istringstream mnameStream(FLAGS_npu_model_name);
    while (std::getline(mnameStream, mname, '+'))
      model_names.push_back(mname);
    int num = model_names.size();
    if (num == 0)
      QUIT_PROGRAM();

    draw = new DrawOperation(num);
    assert(draw);
    if (!CreateNpuModel(ifl, draw, model_names))
      QUIT_PROGRAM();
    ifl->StartLeafs();
  }
#endif // HAVE_ROCKX

  SDLFont *sdl_font = nullptr;
  SDLDisplayLeaf *sdlleaf = nullptr;
  DRMDisplayLeaf *drmleaf = nullptr;
  const char *drm_conn_type = FLAGS_drm_conn_type.c_str();
  if (FLAGS_disp) {
#if DRAW_BY_DRM && !RKNNCASCADE
    drmleaf = new DRMDisplayLeaf(global_drm_fd, drm_conn_type, drm_conn_type,
                                 FLAGS_width, FLAGS_height, FLAGS_disp_rotate);
    if (!drmleaf)
      QUIT_PROGRAM();
    if (drmleaf) {
      drmleaf->sub_draw = draw;
      leafs.push_back(drmleaf);
      join->AddLeaf(drmleaf);
      if (!drmleaf->Prepare())
        QUIT_PROGRAM();
    }
#else
    sdlleaf = new SDLDisplayLeaf(global_drm_fd);
    if (!sdlleaf)
      QUIT_PROGRAM();
    if (sdlleaf) {
      sdl_font = new SDLFont(red, 24);
      sdlleaf->sdl_font = sdl_font;
#if RKNNCASCADE
      sdlleaf->SetFaceSliceNum(device_ids.size());
#endif
      leafs.push_back(sdlleaf);
      join->AddLeaf(sdlleaf);
      if (!sdlleaf->Prepare())
        QUIT_PROGRAM();
    }
#endif
  }

#if RKNNCASCADE // DRAW_BY_OPENGLES
  LeafHandler *npu_output_leaf = nullptr;
  if (FLAGS_processor == "npu") {
    LeafHandler *npu_input_leaf = nullptr;
    if (FLAGS_npu_data_source == "usb") {
      npu_input_leaf = new NpuUsbLeaf(global_drm_fd, cascade, device_ids,
                                      offsets, npu_w, npu_h);
    } else if (FLAGS_npu_data_source == "drm") {
      npu_input_leaf =
          new DRMDisplayLeaf(global_drm_fd, "DSI", "DSI", npu_w, npu_h); //"eDP"
    }
    if (!npu_input_leaf)
      QUIT_PROGRAM();
    if (npu_input_leaf) {
      leafs.push_back(npu_input_leaf);
      join->AddLeaf(npu_input_leaf);
      if (!npu_input_leaf->Prepare())
        QUIT_PROGRAM();
      sleep(1);
    }
    npu_output_leaf =
        new NpuOutPutLeaf(cascade, device_ids, offsets, sdlleaf,
                          npu_w ? ((float)FLAGS_width / npu_w) : 1.0f,
                          npu_h ? ((float)FLAGS_height / npu_h) : 1.0f);
    if (!npu_output_leaf)
      QUIT_PROGRAM();
    if (npu_output_leaf) {
      if (!npu_output_leaf->Prepare())
        QUIT_PROGRAM();
    }
  }
#endif
  if (leafs.empty()) {
    DummyLeaf *leaf = new DummyLeaf();
    if (leaf) {
      leafs.push_back(leaf);
      join->AddLeaf(leaf);
      if (!leaf->Prepare())
        QUIT_PROGRAM();
    }
  }

  disp_time = !FLAGS_disp_time.compare("pre_processor") ? PRE_PROCESSOR
                                                        : POST_PROCESSOR;
  term_init();

  for (auto &input : input_vector) {
    input->Start();
  }

#if RKNNCASCADE
  if (npu_output_leaf)
    npu_output_leaf->Start();
#endif
  join->Start();
  for (auto lh : leafs)
    lh->Start();

  if (sdlleaf) {
    while (!sdlleaf->sdl_prepared) {
      printf("sdlleaf->sdl_prepared: %d\n", sdlleaf->sdl_prepared);
      msleep(200);
    }
    if (sdlleaf->sdl_prepared < 0) {
      av_log(NULL, AV_LOG_ERROR, "sdl display prepare failed\n");
      QUIT_PROGRAM();
    }
  }

  while (!received_sigterm) {
    /* if 'q' pressed, exits */
    if (check_keyboard_interaction() < 0 || request_exit)
      break;
    else
      msleep(100);
  }

  for (auto &input : input_vector) {
    input->Stop();
  }

#if RKNNCASCADE
  if (cascade) {
    av_log(NULL, AV_LOG_INFO, "close cascade\n");
    cascade->close();
  }
  if (npu_output_leaf) {
    av_log(NULL, AV_LOG_INFO, "stop npu_output_leaf\n");
    npu_output_leaf->Stop();
    delete npu_output_leaf;
  }
#endif

  for (auto lh : leafs)
    join->RemoveLeaf(lh);
  av_log(NULL, AV_LOG_INFO, "stop join\n");
  join->Stop();

  for (auto lh : leafs) {
    lh->Stop();
    delete lh;
  }
  av_log(NULL, AV_LOG_INFO, "stop leafs\n");

#if HAVE_ROCKX
  if (draw) {
    delete draw;
  }
#endif

  if (restore_tty)
    tcsetattr(0, TCSANOW, &oldtty);

  join.reset();
  input_vector.clear();
  avformat_network_deinit();
  drmClose(global_drm_fd);
  if (sdl_font)
    delete sdl_font;

#if RKNNCASCADE
  cascade.reset();
#endif

  av_log(NULL, AV_LOG_INFO, "exit main\n");
  return 0;
}
