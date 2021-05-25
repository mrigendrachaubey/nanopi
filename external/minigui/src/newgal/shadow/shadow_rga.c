#if ENABLE_RGA

#include <pthread.h>
#include <fcntl.h>

#include "shadow_rga.h"
#include "../drmcon/drm_display.h"

static bo_t g_bo;
static int g_src_fd;

typedef struct {
    pthread_mutex_t mtx;
    int front_fb;
    int width;
    int height;
    bo_t bo[2];
    int fb[2];
} video_offscreen_fb;

static video_offscreen_fb video_fb;

void *shadow_rga_g_bo_ptr()
{
    return g_bo.ptr;
}

#define DIVIDE_SUM(a, b) (((a)/(b)) + ((a)%(b)))

int shadow_rga_init(int size)
{
    int ret;
    struct bo bo;
    int width, height, bpp;
    int i;

    pthread_mutex_init(&video_fb.mtx, NULL);
    video_fb.front_fb = 0;
    video_fb.fb[0] = video_fb.fb[1] = -1;

    getdrmdispinfo(&bo, &width, &height);
    getdrmdispbpp(&bpp);

    ret = c_RkRgaInit();
    if (ret) {
        printf("c_RkRgaInit error : %s\n", strerror(errno));
        return ret;
    }
    height = DIVIDE_SUM(DIVIDE_SUM(size, bpp), width) * 8;
    ret = c_RkRgaGetAllocBuffer(&g_bo, width, height, bpp);
    if (ret) {
        printf("c_RkRgaGetAllocBuffer error : %s\n", strerror(errno));
        return ret;
    }
    printf("size = %d, g_bo.size = %d\n", size, g_bo.size);
    ret = c_RkRgaGetMmap(&g_bo);
    if (ret) {
        printf("c_RkRgaGetMmap error : %s\n", strerror(errno));
        return ret;
    }
    ret = c_RkRgaGetBufferFd(&g_bo, &g_src_fd);
    if (ret) {
        printf("c_RkRgaGetBufferFd error : %s\n", strerror(errno));
        return ret;
    }

    i = 2;
    while (i-- > 0) {
        bo_t *cur_bo = &video_fb.bo[i];
        ret = c_RkRgaGetAllocBuffer(cur_bo, width, height, bpp);
        if (ret) {
            printf("c_RkRgaGetAllocBuffer error : %s\n", strerror(errno));
            return ret;
        }
        printf("size = %d, cur_bo->size = %d\n", size, cur_bo->size);
        ret = c_RkRgaGetMmap(cur_bo);
        if (ret) {
            printf("c_RkRgaGetMmap error : %s\n", strerror(errno));
            return ret;
        }
        ret = c_RkRgaGetBufferFd(cur_bo, &video_fb.fb[i]);
        if (ret) {
            printf("c_RkRgaGetBufferFd error : %s\n", strerror(errno));
            return ret;
        }
        video_fb.width = width;
        video_fb.height = height;
    }

    return 0;
}

void shadow_rga_exit(void)
{
    int ret;
    int i;
    close(g_src_fd);
    ret = c_RkRgaUnmap(&g_bo);
    if (ret)
        printf("c_RkRgaUnmap error : %s\n", strerror(errno));
    ret = c_RkRgaFree(&g_bo);
    if (ret)
        printf("c_RkRgaFree error : %s\n", strerror(errno));

    i = 2;
    while (i-- > 0) {
        if (video_fb.fb[i] < 0)
            continue;
        close(video_fb.fb[i]);
        video_fb.fb[i] = -1;
        ret = c_RkRgaUnmap(&video_fb.bo[i]);
        if (ret)
            printf("c_RkRgaUnmap error : %s\n", strerror(errno));
        ret = c_RkRgaFree(&video_fb.bo[i]);
        if (ret)
            printf("c_RkRgaFree error : %s\n", strerror(errno));
    }
    pthread_mutex_destroy(&video_fb.mtx);
}

void shadow_rga_refresh(int fd, int offset, int src_w, int src_h,
                        int dst_w, int dst_h, int rotate)
{
    int ret;
    int bpp;
    rga_info_t src;
    rga_info_t src1;
    rga_info_t dst;
    int srcFormat;
    int src1Format;
    int dstFormat;

    memset(&src, 0, sizeof(rga_info_t));
    src.fd = -1; //g_src_fd;
    // The head exist minigui info, need make a offset.
    src.virAddr = g_bo.ptr + offset;
    src.mmuFlag = 1;

    memset(&src1, 0, sizeof(rga_info_t));
    src1.fd = -1;
    src1.mmuFlag = 1;

    memset(&dst, 0, sizeof(rga_info_t));
    dst.fd = fd;
    dst.mmuFlag = 1;

    getdrmdispbpp(&bpp);
    if (bpp == 32) {
        srcFormat = RK_FORMAT_RGBA_8888;
        dstFormat = RK_FORMAT_RGBA_8888;
    } else {
        srcFormat = RK_FORMAT_RGB_565;
        dstFormat = RK_FORMAT_RGB_565;
    }
    src1Format = RK_FORMAT_YCbCr_420_P;

    rga_set_rect(&src1.rect, 0, 0, video_fb.width, video_fb.height,
                 video_fb.width, video_fb.height, src1Format);
    src1.rotation = rotate;
    rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dstFormat);

    pthread_mutex_lock(&video_fb.mtx);
    src1.fd = video_fb.fb[video_fb.front_fb];
    pthread_mutex_unlock(&video_fb.mtx);
    ret = c_RkRgaBlit(&src1, &dst, NULL);
    if (ret)
        printf("c_RkRgaBlit1 error : %s\n", strerror(errno));

    rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, srcFormat);
    src.rotation = rotate;
    src.blend = 0xff0105;
    rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dstFormat);
    ret = c_RkRgaBlit(&src, &dst, NULL);
    if (ret)
        printf("c_RkRgaBlit2 error : %s\n", strerror(errno));
}

int yuv_draw(char *src_ptr, int src_fd, int format, int src_w, int src_h) {
    int ret;
    rga_info_t src;

    rga_info_t dst;
    memset(&src, 0, sizeof(rga_info_t));
    src.fd = -1; //rga_src_fd;
    src.virAddr = src_ptr;
    src.mmuFlag = 1;

    memset(&dst, 0, sizeof(rga_info_t));
    dst.fd = video_fb.fb[video_fb.front_fb ^ 1];
    dst.mmuFlag = 1;

    rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, format);

    // resize as a rect with src_w and src_h
    video_fb.width = src_w;
    video_fb.height = src_h;
    rga_set_rect(&dst.rect, 0, 0, video_fb.width, video_fb.height, video_fb.width, video_fb.height, RK_FORMAT_YCrCb_420_P);
    // do copy
    ret = c_RkRgaBlit(&src, &dst, NULL);
    if (ret)
        printf("c_RkRgaBlit0 error : %s\n", strerror(errno));
    else {
        pthread_mutex_lock(&video_fb.mtx);
        video_fb.front_fb ^= 1;
        pthread_mutex_unlock(&video_fb.mtx);
    }
    return ret;
}

#endif