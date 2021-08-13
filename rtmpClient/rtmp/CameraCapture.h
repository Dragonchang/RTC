#ifndef H_CAMERA_CAPTURE
#define H_CAMERA_CAPTURE
#include <unistd.h>
#include <iostream>
#include <sys/time.h>

#include "../handlerThread/Handler.h"
#include "../handlerThread/Message.h"
#include "../handlerThread/Looper.h"
#include "../handlerThread/NThread.h"
#include "./PushRtmp.h"

#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/opencv.hpp>

#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/avutil.h"  
#include "libavcodec/avcodec.h"  
#include "libavformat/avformat.h"  
#include "libswscale/swscale.h"  
#include <libavutil/imgutils.h>  
#ifdef __cplusplus
}
#endif

using namespace std;
using namespace cv;
class CameraCaptureHandler;
class PushRtmp;
class CameraCapture
{
public:
	CameraCapture();
	virtual ~CameraCapture();

public:
	void startCameraCapture();
	void doCapture();
	long getFramerate() { return mFramerate; }

private:
	string gstreamerPipeline();
	bool openCamera();
	bool isCISCamera();
	bool isCameraOpen();
	AVFrame* cvmatToAvframe(Mat* image);

private:
	CameraCaptureHandler *mCaptureHandler;
	NThread *mCapturethread;

	VideoCapture *mVideoCapture;

	//�Ƿ������ͷ
	bool mHasOpenCamera;

	//���������
	int mCaptureWidth;
	int mCaptureHeight;
	int mDisplayWidth;
	int mDisplayHeight;
	int mFramerate;
	int mFlipMethod;

	//Ĭ��ʹ��/dev/video1 usb
	int mCameraDeviceIndex;

	PushRtmp *mPushRtmp;

};

class CameraCaptureHandler : public Handler {

public:
	static const int CAPTURE_MESSAGE = 1;

private:
	CameraCapture *mCameraCapture;

public:
	CameraCaptureHandler(Looper* looper, CameraCapture *cameraCapture);

private:
	void handlerMessage(Message *message);
	long getNextFrameDelay(timeval startTime, timeval endTime);
};
#endif