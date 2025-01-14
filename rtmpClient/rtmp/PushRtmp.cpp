﻿#include "PushRtmp.h"
/*****************************************************************
*
* function：PushRtmp
* 作用：构造函数
*
*****************************************************************/
PushRtmp::PushRtmp(int width, int height, int fps, int cameraDeviceIndex)
{
	mCameraDeviceIndex = cameraDeviceIndex;
	m_width = width;
	m_height = height;
	m_fps = fps;
	mOutUrl = "rtmp://127.0.0.1/live/livestream";
	
	//注册所有的编解码器
	avcodec_register_all();

	av_register_all();

	//初始化网络库
	avformat_network_init();

	mPushRtmpThread = new NThread();
	mPushRtmpHandler = new PushRtmpHandler(mPushRtmpThread->getLooper(), this);
	initRtmp();
}

/*****************************************************************
*
* function：~PushRtmp
* 作用：析构函数
*
*****************************************************************/
PushRtmp::~PushRtmp()
{
    
}

/*****************************************************************
*
* function：pushRtmp
* 作用：推流
*
*****************************************************************/
bool PushRtmp::pushRtmp(AVFrame *frame)
{

	if (frame == NULL)
	{
		printf("pushRtmp with null frame!\n");
		return false;
	}
	m_framecnt++;
	Message* message = Message::obtain(PushRtmpHandler::CAPTURE_MESSAGE, frame);
	mPushRtmpHandler->sendMessage(message);
}
/*****************************************************************
*
* function：initRtmp
* 作用：初始化本地rtmp推流配置
*
*****************************************************************/
bool PushRtmp::initRtmp()
{
	int ret = 0;
	m_rtmpStatus = false;
	m_vpts = 0;
	//初始化编码上下文
	//找到编码器
	AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec)
	{
		printf("canot find h264 encoder!\n");
		return false;
	}
	//b 创建编码器上下文
	m_vc = avcodec_alloc_context3(codec);
	if (!m_vc)
	{
		printf("avcodec_alloc_context3 failed!\n");
		return false;
	}

	//c 配置编码器参数
	m_vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; //全局参数
	m_vc->codec_id = codec->id;
	m_vc->thread_count = 8;

	//vc->bit_rate = 50 * 1024*8; //压缩后每秒视频的bit位大小 50kB
	m_vc->bit_rate = 800000;
	m_vc->width = m_width;
	m_vc->height = m_height;
	m_vc->time_base = { 1, m_fps };
	m_vc->framerate = { m_fps, 1 };

	AVDictionary *param;
	param = 0;
	av_dict_set(&param, "preset", "superfast", 0);
	av_dict_set(&param, "profile", "high", 0);
	av_dict_set(&param, "tune", "zerolatency", 0);

	//画面组的大小，多少帧一个关键帧
	//m_vc->gop_size = 250;
	m_vc->gop_size = m_fps * 2;
	m_vc->has_b_frames = 0;
	m_vc->max_b_frames = 0;
	m_vc->qmin = 10;
	m_vc->qmax = 30;
	m_vc->pix_fmt = AV_PIX_FMT_YUV420P;
	//d 打开编码器上下文
	ret = avcodec_open2(m_vc, codec, &param);
	if (ret != 0)
	{
		printf("avcodec_open2 failed!\n");
		avcodec_free_context(&m_vc);
		return false;
	}
	//如果是输入文件 flv可以不传，可以从文件中判断。如果是流则必须传
	//创建输出上下文
	ret = avformat_alloc_output_context2(&m_octx, NULL, "flv", mOutUrl.c_str());
	if (ret < 0)
	{
		printf("avformat_alloc_output_context2 failed,ret=%d\n", ret);
		//avcodec_free_context(&m_vc);
		return false;
	}
	printf("avformat_alloc_output_context2 success!\n");
	//为输出上下文添加音视频流（初始化一个音视频流容器）
	m_out_stream = avformat_new_stream(m_octx, codec);
	if (!m_out_stream)
	{
		printf("未能成功添加音视频流\n");
		//avcodec_free_context(&m_vc);
		//avformat_close_input(&m_octx);
		//ret = AVERROR_UNKNOWN;
		return false;
	}
	m_out_stream->time_base.num = 1;
	m_out_stream->time_base.den = m_fps;
	m_out_stream->codec = m_vc;

	av_dump_format(m_octx, 0, mOutUrl.c_str(), 1);
	//打开IO
	ret = avio_open(&m_octx->pb, mOutUrl.c_str(), AVIO_FLAG_WRITE);
	if (ret < 0)
	{
		printf("avio_open failed:%d\n", ret);
		//avcodec_free_context(&m_vc);
		//avformat_close_input(&m_octx);
		return false;
	}
	printf("avio_open success\n");
	//写入头部信息
	ret = avformat_write_header(m_octx, 0);
	if (ret < 0)
	{
		printf("avformat_write_header failed:%d\n", ret);
		//avcodec_free_context(&m_vc);
		//avformat_close_input(&m_octx);
		return false;
	}
	m_rtmpStatus = true;
	return true;
}

/*****************************************************************
*
* function：releaseAVFrame
* 作用：releaseAVFrame
*
*****************************************************************/
void PushRtmp::releaseAVFrame(AVFrame *avframe)
{
	if (avframe != NULL)
	{
		av_frame_free(&avframe);
		avframe = NULL;
	}
}
/*****************************************************************
*
* function：PushRtmpHandler
* 作用：处理推流的handler
*
*****************************************************************/
PushRtmpHandler::PushRtmpHandler(Looper* looper, PushRtmp *pushRtmp) :Handler(looper)
{
	mPushRtmp = pushRtmp;
}

/*****************************************************************
*
* function：handlerMessage
* 作用：处理发送推流的函数
*
*****************************************************************/
void PushRtmpHandler::handlerMessage(Message *message)
{
	AVFrame *frame = (AVFrame *)message->mObj;
	do
	{
		if (frame == NULL)
		{
			printf("frame is null\n");
			return;
		}
		if (!mPushRtmp->getRtmpStatus())
		{
			printf("restart pushrtmp\n");
			if (!mPushRtmp->initRtmp())
			{
				printf("restart pushrtmp failed\n");
				return;
			}
		}
		int ret = 0;
		timeval startTime;
		timeval endTime;
		AVPacket pack;
		gettimeofday(&startTime, nullptr);
		memset(&pack, 0, sizeof(pack));
		ret = avcodec_send_frame(mPushRtmp->getAVCodecContext(), frame);
		if (ret != 0)
		{
			printf("avcodec_send_frame error\n");
			return;
		}

		ret = avcodec_receive_packet(mPushRtmp->getAVCodecContext(), &pack);
		if (ret != 0 || pack.size > 0)
		{
			printf("avcodec_receive_packet: %d\n", pack.size);
		}
		else
		{
			printf("avcodec_receive_packet error\n");
			return;
		}
		pack.stream_index = mPushRtmp->getOutstream()->index;
		AVRational time_base = mPushRtmp->getOutstream()->time_base;//{ 1, 1000 };
		AVRational r_framerate1 = mPushRtmp->getAVCodecContext()->framerate;
		AVRational time_base_q;
		time_base_q.num = 1;
		time_base_q.den = AV_TIME_BASE;
		int vpts = mPushRtmp->getVpts();
		mPushRtmp->setVptes(++vpts);
		int64_t calc_duration = (double)(AV_TIME_BASE)*(1 / av_q2d(r_framerate1));	//内部时间戳
		pack.pts = av_rescale_q(mPushRtmp->getVpts()*calc_duration, time_base_q, time_base);
		pack.dts = pack.pts;
		pack.duration = av_rescale_q(calc_duration, time_base_q, time_base);
		pack.pos = -1;

		printf("pts:%d,dts:%d,duration:%d\n", pack.pts, pack.dts, pack.duration);

		if (av_interleaved_write_frame(mPushRtmp->getAVFormatContext(), &pack) < 0) //写入图像到视频
		{
			int failedTimes = mPushRtmp->getRtmpPushFailedTimes();
			printf("av_interleaved_write_frame local failed failedTimes%d\n", failedTimes);
			if (failedTimes < 5 * mPushRtmp->getFps())
			{
				return;
			}
			else
			{
				mPushRtmp->setRtmpPushFailedTimes(0);
				avio_close(mPushRtmp->getAVFormatContext()->pb);
				AVCodecContext *m_vc = mPushRtmp->getAVCodecContext();
				if (m_vc != NULL) {
					avcodec_free_context(&m_vc);
				}
				mPushRtmp->setRtmpStatus(false);
			}
		}
		mPushRtmp->reduceFrameCount();
		printf("end pushRtmp now still has %d frame.\n", mPushRtmp->getFrameCount());
	} while (0);
	//释放AVFrame
	mPushRtmp->releaseAVFrame(frame);


}

