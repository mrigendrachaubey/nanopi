
#include "ProxyCameraBuffer.h"
#include "cam_ia_api/cam_ia10_trace.h"

using namespace std;

ProxyCameraBuffer::ProxyCameraBuffer(
    void* vaddr,
    const char* camPixFmt,
    unsigned int width,
    unsigned int height,
    int stride,
    size_t size,
    weak_ptr<CameraBufferAllocator> allocator,
    weak_ptr<ICameraBufferOwener> bufOwener) :
  CameraBuffer(allocator, bufOwener), mVaddr(vaddr),
  mWidth(width), mHeight(height), mBufferSize(size), mCamPixFmt(camPixFmt), mStride(stride) {
  ALOGV("%s: vaddr %p, camPixFmt %s, stride %d", __func__, vaddr, camPixFmt, stride);

}

ProxyCameraBuffer::~ProxyCameraBuffer(void) {
  if (!mAllocator.expired()) {
	
	  shared_ptr<CameraBufferAllocator> spAlloc = mAllocator.lock();

	  if (spAlloc.get()) {
		cout<<"line "<<__LINE__<<" func:"<<__func__<<"\n";
		spAlloc->free(this);
		}
  }
}

void* ProxyCameraBuffer::getHandle(void) const {
  return NULL;
}
void* ProxyCameraBuffer::getPhyAddr(void) const {
  return NULL;
}

void* ProxyCameraBuffer::getVirtAddr(void) const {
  return mVaddr;
}

void ProxyCameraBuffer::setVirtAddr(void* vaddr) {
  mVaddr = vaddr;
}

const char* ProxyCameraBuffer::getFormat(void) {
  return mCamPixFmt;
}

unsigned int ProxyCameraBuffer::getWidth(void) {
  return mWidth;
}

unsigned int ProxyCameraBuffer::getHeight(void) {
  return mHeight;
}

size_t ProxyCameraBuffer::getDataSize(void) const {
  return mDataSize;
}

void ProxyCameraBuffer::setDataSize(size_t size) {
  mDataSize = size;
}

size_t ProxyCameraBuffer::getCapacity(void) const {
  return mBufferSize;
}

void ProxyCameraBuffer::setCapacity(size_t size) {
  mBufferSize = size;
}


unsigned int ProxyCameraBuffer::getStride(void) const {
  return mStride;
}

bool ProxyCameraBuffer::lock(unsigned int usage) {
  UNUSED_PARAM(usage);
  return true;
}

bool ProxyCameraBuffer::unlock(unsigned int usage) {
  UNUSED_PARAM(usage);
  return true;
}


ProxyCameraBufferAllocator::ProxyCameraBufferAllocator(void) {
  ALOGV("%s", __func__);
}

ProxyCameraBufferAllocator::~ProxyCameraBufferAllocator(void) {
  if (mNumBuffersAllocated > 0)
    ALOGE("%s: memory leak; %d camera buffers have not been freed", __func__, mNumBuffersAllocated);
}

shared_ptr<CameraBuffer> ProxyCameraBufferAllocator::alloc(
    const char* camPixFmt,
    unsigned int width,
    unsigned int height,
    unsigned int usage,
    weak_ptr<ICameraBufferOwener> bufOwener) {
  int ret;
  int stride;
  int sharefd;
  void* vaddr;
  int map_fd;
  size_t buffer_size;
  shared_ptr<ProxyCameraBuffer> camBuff;
  unsigned int halPixFmt;

  ALOGV("%s: format %s %dx%d, usage 0x%08x", __func__, camPixFmt, width, height, usage);

  //buffer_size maybe changed by setCapcity().
  buffer_size = calcBufferSize(camPixFmt, width, height);
  stride = width;

  camBuff = shared_ptr<ProxyCameraBuffer>(new ProxyCameraBuffer(NULL, camPixFmt, width, height,
                                                                stride, buffer_size, shared_from_this(), bufOwener));
  if (!camBuff.get())
    ALOGE("%s: Out of memory", __func__);
  else {
    mNumBuffersAllocated++;
    if (camBuff->error()) {
      camBuff.reset();
    }
  }

  return camBuff;
}

void ProxyCameraBufferAllocator::free(CameraBuffer* buffer) {
  if (buffer) {
    ProxyCameraBuffer* camBuff = static_cast<ProxyCameraBuffer*>(buffer);
    ALOGD("%s: index %d", __func__, buffer->getIndex());
    mNumBuffersAllocated--;
  }
}

