/*
** Copyright (c) 2012 The Linux Foundation. All rights reserved.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*#error uncomment this for compiler test!*/

#define ALOG_NDEBUG 0
#define ALOG_NIDEBUG 0
#define LOG_TAG "QCameraHWI_Record"
#include <utils/Log.h>
#include <utils/threads.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "QCameraStream.h"
#include "QCameraVideoMirror.h"


#define LIKELY(exp)   __builtin_expect(!!(exp), 1)
#define UNLIKELY(exp) __builtin_expect(!!(exp), 0)

/* QCameraStream_record class implementation goes here*/
/* following code implement the video streaming capture & encoding logic of this class*/
// ---------------------------------------------------------------------------
// QCameraStream_record createInstance()
// ---------------------------------------------------------------------------
namespace android {


QCameraStream* QCameraStream_record::createInstance(int cameraId,
                                      camera_mode_t mode)
{
  ALOGV("%s: BEGIN", __func__);
  QCameraStream* pme = new QCameraStream_record(cameraId, mode);
  ALOGV("%s: END", __func__);
  return pme;
}

// ---------------------------------------------------------------------------
// QCameraStream_record deleteInstance()
// ---------------------------------------------------------------------------
void QCameraStream_record::deleteInstance(QCameraStream *ptr)
{
  ALOGV("%s: BEGIN", __func__);
  if (ptr){
    ptr->release();
    delete ptr;
    ptr = NULL;
  }
  ALOGV("%s: END", __func__);
}

// ---------------------------------------------------------------------------
// QCameraStream_record Constructor
// ---------------------------------------------------------------------------
QCameraStream_record::QCameraStream_record(int cameraId,
                                           camera_mode_t mode)
  :QCameraStream(cameraId,mode),
   mDebugFps(false),
   mVideoMirrorMode(LGE_VIDEO_MIRROR_OFF),
   mEncodeBufferCount(0),
   mVideoMirrorWidth(0),
   mVideoMirrorHeight(0),
   mVideoMirrorStride(0),
   mVideoMirrorCbcrOffset(0)
{
  mHalCamCtrl = NULL;
  char value[PROPERTY_VALUE_MAX];
  ALOGV("%s: BEGIN", __func__);

  property_get("persist.debug.sf.showfps", value, "0");
  mDebugFps = atoi(value);

  ALOGV("%s: END", __func__);
}

// ---------------------------------------------------------------------------
// QCameraStream_record Destructor
// ---------------------------------------------------------------------------
QCameraStream_record::~QCameraStream_record() {
  ALOGV("%s: BEGIN", __func__);
  if(mActive) {
    stop();
  }
  if(mInit) {
    release();
  }
  mInit = false;
  mActive = false;
  ALOGV("%s: END", __func__);

}

// ---------------------------------------------------------------------------
// QCameraStream_record Callback from mm_camera
// ---------------------------------------------------------------------------
static void record_notify_cb(mm_camera_ch_data_buf_t *bufs_new,
                              void *user_data)
{
  QCameraStream_record *pme = (QCameraStream_record *)user_data;
  mm_camera_ch_data_buf_t *bufs_used = 0;
  ALOGV("%s: BEGIN", __func__);

  /*
  * Call Function Process Video Data
  */
  pme->processRecordFrame(bufs_new);
  ALOGV("%s: END", __func__);
}

// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------
status_t QCameraStream_record::init()
{
  status_t ret = NO_ERROR;
  ALOGV("%s: BEGIN", __func__);
  mInit = true;
  ALOGV("%s: END", __func__);
  return ret;
}
// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------

status_t QCameraStream_record::start()
{
  status_t ret = NO_ERROR;
  ALOGE("%s: BEGIN", __func__);

  if (mActive)
    return NO_ERROR;
  ret = initEncodeBuffers();
  if (NO_ERROR!=ret) {
    ALOGE("%s ERROR: Buffer Allocation Failed\n",__func__);
    return ret;
  }
  Mutex::Autolock l(&mHalCamCtrl->mRecordLock);
  mHalCamCtrl->mReleasedRecordingFrame = false;

  mHalCamCtrl->mStartRecording  = true;
  mActive = true;

  ALOGV("%s: END", __func__);
  return ret;
}

// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------
void QCameraStream_record::stop()
{
  status_t ret = NO_ERROR;
  ALOGE("%s: BEGIN", __func__);
  mHalCamCtrl->mStartRecording  = false;
  Mutex::Autolock l(&mHalCamCtrl->mRecordLock);
  Mutex::Autolock lock(mStopCallbackLock);
  {
        mHalCamCtrl->mRecordFrameLock.lock();
        mHalCamCtrl->mReleasedRecordingFrame = true;
        mHalCamCtrl->mRecordWait.signal();
        mHalCamCtrl-> mRecordFrameLock.unlock();
  }

  releaseEncodeBuffer();
  mActive = false;
  ALOGV("%s: END", __func__);

}
// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------
void QCameraStream_record::release()
{
  if (mActive)
    stop();
  else
    releaseEncodeBuffer();
  mInit = false;
}

status_t QCameraStream_record::processRecordFrame(void *data)
{
  ALOGE("%s : BEGIN",__func__);
  ALOGE("%s : END",__func__);
  return NO_ERROR;
}

//Record Related Functions
// Caller holds mRecordLock; stop cannot free this pool during a frame copy.
bool QCameraStream_record::prepareVideoMirror(const struct msm_frame *frame, int index)
{
    if (mVideoMirrorMode == LGE_VIDEO_MIRROR_OFF)
        return true;
    if (!frame || index < 0 || index >= mEncodeBufferCount
            || !mHalCamCtrl->mRecordingMemory.camera_memory[index])
        return false;
    camera_memory_t *output = mHalCamCtrl->mRecordingMemory.camera_memory[index];
    if (frame->y_off != 0 || frame->cbcr_off != mVideoMirrorCbcrOffset
            || !lgeMirrorVideo420(reinterpret_cast<const uint8_t *>(frame->buffer),
                frame->ion_alloc.len, static_cast<uint8_t *>(output->data), output->size,
                mVideoMirrorWidth, mVideoMirrorHeight, mVideoMirrorStride,
                mVideoMirrorCbcrOffset, mVideoMirrorMode)) {
        ALOGE("LG virtual front: invalid encoder mirror frame layout");
        return false;
    }
#ifdef USE_ION
    struct ion_flush_data flush;
    memset(&flush, 0, sizeof(flush));
    flush.vaddr = output->data;
    flush.fd = mHalCamCtrl->mRecordingMemory.fd[index];
    flush.handle = mHalCamCtrl->mRecordingMemory.ion_info_fd[index].handle;
    flush.length = mHalCamCtrl->mRecordingMemory.alloc[index].len;
    if (mHalCamCtrl->cache_ops(mHalCamCtrl->mRecordingMemory.main_ion_fd[index],
            &flush, ION_IOC_CLEAN_CACHES) < 0)
        return false;
#endif
    return true;
}

camera_memory_t *QCameraStream_record::getMirroredVideoBuffer(int index)
{
    if (mVideoMirrorMode == LGE_VIDEO_MIRROR_OFF || index < 0 || index >= mEncodeBufferCount)
        return NULL;
    return mHalCamCtrl->mRecordingMemory.camera_memory[index];
}

status_t QCameraStream_record::initEncodeBuffers()
{
    releaseEncodeBuffer();
    // The original single-output HAL initializes only metadata slots. Start a
    // new encoder pool from a fully defined heap, including failure paths.
    memset(&mHalCamCtrl->mRecordingMemory, 0, sizeof(mHalCamCtrl->mRecordingMemory));
    const int count = mHalCamCtrl->mPreviewMemory.buffer_count;
    if (count <= 0 || count > MM_CAMERA_MAX_NUM_FRAMES)
        return BAD_VALUE;
    mEncodeBufferCount = count;
    const char *flip = mHalCamCtrl->mParameters.get("lg-virtual-front-video-mirror");
    mVideoMirrorMode = LGE_VIDEO_MIRROR_OFF;
    if (flip && !strcmp(flip, "flip-h"))
        mVideoMirrorMode = LGE_VIDEO_MIRROR_HORIZONTAL;
    else if (flip && !strcmp(flip, "flip-v"))
        mVideoMirrorMode = LGE_VIDEO_MIRROR_VERTICAL;

    if (mVideoMirrorMode != LGE_VIDEO_MIRROR_OFF) {
        int outputs = 0;
        if (cam_config_get_parm(mCameraId, MM_CAMERA_PARM_VFE_OUTPUT_ENABLE, &outputs) != MM_CAMERA_OK
                || outputs != 1 || (mHalCamCtrl->mPreviewFormat != CAMERA_YUV_420_NV21
                    && mHalCamCtrl->mPreviewFormat != CAMERA_YUV_420_NV12)) {
            releaseEncodeBuffer();
            return BAD_VALUE;
        }
        cam_ctrl_dimension_t dimension;
        memset(&dimension, 0, sizeof(dimension));
        if (cam_config_get_parm(mCameraId, MM_CAMERA_PARM_DIMENSION, &dimension) != MM_CAMERA_OK) {
            releaseEncodeBuffer();
            return BAD_VALUE;
        }
        mVideoMirrorWidth = dimension.display_width;
        mVideoMirrorHeight = dimension.display_height;
        mVideoMirrorStride = mHalCamCtrl->mPreviewMemory.stride[0];
        mVideoMirrorCbcrOffset = dimension.display_frame_offset.mp[0].len;
        if (!mHalCamCtrl->mPreviewMemory.private_buffer_handle[0]) {
            releaseEncodeBuffer();
            return BAD_VALUE;
        }
        const int size = mHalCamCtrl->mPreviewMemory.private_buffer_handle[0]->size;
        if (mVideoMirrorWidth <= 0 || mVideoMirrorHeight <= 0
                || (mVideoMirrorWidth & 1) || (mVideoMirrorHeight & 1)
                || mVideoMirrorStride < mVideoMirrorWidth
                || static_cast<size_t>(mVideoMirrorCbcrOffset) < static_cast<size_t>(mVideoMirrorStride) * mVideoMirrorHeight
                || static_cast<size_t>(size) < mVideoMirrorCbcrOffset
                    + static_cast<size_t>(mVideoMirrorStride) * (mVideoMirrorHeight / 2)) {
            releaseEncodeBuffer();
            return BAD_VALUE;
        }
        for (int index = 0; index < count; ++index) {
            if (!mHalCamCtrl->mPreviewMemory.private_buffer_handle[index]
                    || mHalCamCtrl->mPreviewMemory.private_buffer_handle[index]->size != size
                    || mHalCamCtrl->mPreviewMemory.stride[index] != mVideoMirrorStride) {
                releaseEncodeBuffer();
                return BAD_VALUE;
            }
        }
        if (mHalCamCtrl->initHeapMem(&mHalCamCtrl->mRecordingMemory, count, size,
                0, mVideoMirrorCbcrOffset, MSM_PMEM_VIDEO, NULL, NULL, 0, NULL) < 0) {
            releaseEncodeBuffer();
            return NO_MEMORY;
        }
        for (int index = 0; index < count; ++index) {
            camera_memory_t *memory = mHalCamCtrl->mRecordingMemory.camera_memory[index];
            if (mHalCamCtrl->mRecordingMemory.fd[index] < 0 || !memory
                    || !memory->data || memory->data == MAP_FAILED || memory->size < static_cast<size_t>(size)) {
                releaseEncodeBuffer();
                return NO_MEMORY;
            }
            memset(memory->data, 0, memory->size);
        }
        ALOGI("LG virtual front: encoder pixel mirror enabled (%s)", flip);
    }
    for (int index = 0; index < count; ++index) {
        if (!mHalCamCtrl->mStoreMetaDataInFrame)
            continue;
        camera_memory_t *memory = mHalCamCtrl->mGetMemory(-1,
                sizeof(struct encoder_media_buffer_type), 1, this);
        if (!memory || !memory->data || memory->data == MAP_FAILED) {
            if (memory)
                memory->release(memory);
            releaseEncodeBuffer();
            return NO_MEMORY;
        }
        memset(memory->data, 0, sizeof(struct encoder_media_buffer_type));
        mHalCamCtrl->mRecordingMemory.metadata_memory[index] = memory;
        struct encoder_media_buffer_type *packet =
                static_cast<struct encoder_media_buffer_type *>(memory->data);
        native_handle_t *handle = native_handle_create(1, 3);
        if (!handle) {
            releaseEncodeBuffer();
            return NO_MEMORY;
        }
        packet->meta_handle = handle;
        packet->buffer_type = kMetadataBufferTypeCameraSource;
        if (mVideoMirrorMode != LGE_VIDEO_MIRROR_OFF) {
            handle->data[0] = mHalCamCtrl->mRecordingMemory.fd[index];
            handle->data[1] = 0;
            handle->data[2] = mHalCamCtrl->mRecordingMemory.size;
            handle->data[3] = (uint32_t)mHalCamCtrl->mRecordingMemory.camera_memory[index]->data;
        } else {
            handle->data[0] = mHalCamCtrl->mPreviewMemory.private_buffer_handle[index]->fd;
            handle->data[1] = 0;
            handle->data[2] = mHalCamCtrl->mPreviewMemory.private_buffer_handle[index]->size;
            handle->data[3] = (uint32_t)mHalCamCtrl->mPreviewMemory.camera_memory[index]->data;
        }
    }
    return NO_ERROR;
}

void QCameraStream_record::releaseEncodeBuffer()
{
    for (int index = 0; index < mEncodeBufferCount; ++index) {
        camera_memory_t *memory = mHalCamCtrl->mRecordingMemory.metadata_memory[index];
        if (!memory)
            continue;
        struct encoder_media_buffer_type *packet =
                static_cast<struct encoder_media_buffer_type *>(memory->data);
        if (packet->meta_handle)
            native_handle_delete(const_cast<native_handle_t *>(packet->meta_handle));
        memory->release(memory);
        mHalCamCtrl->mRecordingMemory.metadata_memory[index] = NULL;
    }
    if (mVideoMirrorMode != LGE_VIDEO_MIRROR_OFF)
        mHalCamCtrl->releaseHeapMem(&mHalCamCtrl->mRecordingMemory);
    mVideoMirrorMode = LGE_VIDEO_MIRROR_OFF;
    mEncodeBufferCount = 0;
}

void QCameraStream_record::releaseRecordingFrame(const void *opaque)
{
    Mutex::Autolock rLock(&mHalCamCtrl->mRecordFrameLock);
    mHalCamCtrl->mReleasedRecordingFrame = true;
    mHalCamCtrl->mRecordWait.signal();
    ALOGE("%s, Signaling from-",__func__);
}

void QCameraStream_record::debugShowVideoFPS() const
{

}

status_t  QCameraStream_record::takeLiveSnapshot(){
	return true;
}

}//namespace android
