package com.deepblue.rtccall.ui;

import org.webrtc.EglBase;
import org.webrtc.VideoCapturer;
import org.webrtc.VideoRenderer;

public interface ViewEntity {

    void setSwappedFeeds(boolean swappedFeed);

    VideoCapturer createVideoCapturer();

    EglBase.Context getEglBaseContext();

    VideoRenderer.Callbacks getLocalProxyRenderer();

    VideoRenderer.Callbacks getRemoteProxyRenderer();
}