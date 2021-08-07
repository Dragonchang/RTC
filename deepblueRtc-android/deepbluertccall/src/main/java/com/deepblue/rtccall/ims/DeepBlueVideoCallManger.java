package com.deepblue.rtccall.ims;

import android.app.Application;
import android.util.Log;

import com.deepblue.rtccall.bean.UserBean;
import com.deepblue.rtccall.consts.Urls;
import com.deepblue.rtccall.ims.request.ImsClientSender;
import com.deepblue.rtccall.ims.request.VideoManagerCallBack;
import com.deepblue.rtccall.ims.response.CandidateModel;
import com.deepblue.rtccall.ims.response.ResponseType;
import com.deepblue.rtccall.ims.response.ServerResponse;
import com.deepblue.rtccall.rtc.KurentoRTCEngine;
import com.deepblue.rtccall.rtc.PeerConnectionCallBack;
import com.deepblue.rtccall.rtc.RTCEngineConfig;
import com.deepblue.rtccall.rtc.RTCEngineCreateFactory;
import com.deepblue.rtccall.rtc.RTCEngine;
import com.deepblue.rtccall.ui.ViewEntity;
import com.deepblue.webrtcpeer.rtc_comm.ws.DefaultSocketService;
import com.deepblue.webrtcpeer.rtc_comm.ws.SocketService;
import com.deepblue.webrtcpeer.rtc_peer.PeerConnectionClient;
import com.deepblue.webrtcpeer.rtc_peer.SignalingEvents;
import com.deepblue.webrtcpeer.rtc_peer.SignalingParameters;
import com.deepblue.webrtcpeer.rtc_peer.config.DefaultConfig;

import org.java_websocket.handshake.ServerHandshake;
import org.webrtc.IceCandidate;
import org.webrtc.PeerConnection;
import org.webrtc.SessionDescription;
import org.webrtc.StatsReport;

import java.util.LinkedList;

/**
 * 视频通话管理器
 */
public class DeepBlueVideoCallManger implements SignalingEvents,
        PeerConnectionCallBack, VideoManagerCallBack,
        ImServerMessageCallBack, ImServerConnectStateCallBack{
    private static final String TAG = "videoCallManager";

    private Application application;

    private ImsClientSender mImsClientSender;

    private ImServerCallBack mImsMessageCallBack;

    private CallStateManager mCallStateManger;

    private DefaultConfig mDefaultConfig;

    private KurentoRTCEngine mRTCEngine;

    private ViewEntity mView;

    private static volatile DeepBlueVideoCallManger singleton;

    public static DeepBlueVideoCallManger getInstance(Application application) {
        if (singleton == null) {
            synchronized (DeepBlueVideoCallManger.class) {
                if (singleton == null) {
                    singleton = new DeepBlueVideoCallManger(application);
                }
            }
        }
        return singleton;
    }

    private DeepBlueVideoCallManger(Application application) {
        this.application = application;
        initIMServerManger();
    }

    public void initIMServerManger() {
        this.mImsClientSender = new ImsClientSender(application);
        this.mImsMessageCallBack = new ImServerCallBack();
        this.mImsMessageCallBack.registerImsMessageCallBack(this);
        this.mImsMessageCallBack.registerImsConnectCallBack(this);
        this.mCallStateManger = new CallStateManager();
        this.mDefaultConfig = new DefaultConfig();
        connectIMServer();
    }

    /**
     * 用户上线注册
     */
    public void registerUser(UserBean user) {
        mImsClientSender.register(user);
    }

    /**
     * 创建通话的RTCEngine
     *
     * @param view
     */
    public void createRTCEngine(ViewEntity view, RTCEngineConfig rtcEngineConfig) {
        this.mView = view;
        this.mRTCEngine = RTCEngineCreateFactory.CreateRTCEngine(application.getApplicationContext(),
                view, this, rtcEngineConfig);
    }

    /**
     * 连接IM服务 app启动之后调用
     * 网络断开重连调用
     */
    public void connectIMServer() {
        mImsClientSender.connectIMS(mImsMessageCallBack);
    }

    /**
     * 开始通话
     *
     * @param localUserBean
     * @param remoteUserBean
     */
    public void StartCalling(UserBean localUserBean, UserBean remoteUserBean) {
        if (localUserBean == null || remoteUserBean == null) {
            Log.e(TAG, "StartCalling failed, reason: user info is null");
            return;
        }
        this.mRTCEngine.createPeerConnectionFactory(mDefaultConfig.createPeerConnectionParams());

        this.mRTCEngine.startCall();
    }

    public DefaultConfig getDefaultConfig() {
        return mDefaultConfig;
    }

    @Override
    public boolean isOutgoing() {
        if (mRTCEngine == null) {
            Log.e(TAG, "mRTCEngine is null");
            return false;
        }

        return mRTCEngine.getRTCEngineConfig().isOutgoing;
    }

    /************************PeerConnectionEvents*********************/
    @Override
    public void onLocalDescription(SessionDescription sdp) {
        Log.e(TAG, "onLocalDescription ");
        if (mImsClientSender != null) {
            if (isOutgoing()) {
                mImsClientSender.sendCallOfferSdp(sdp, mRTCEngine.getRTCEngineConfig().localUserBean,
                        mRTCEngine.getRTCEngineConfig().remoteUserBean);
            } else {
                mImsClientSender.sendIncomingOfferSdp(sdp, mRTCEngine.getRTCEngineConfig().remoteUserBean);
            }
        }

    }

    @Override
    public void onIceCandidate(IceCandidate candidate) {
        Log.e(TAG, "onIceCandidate ");
        mImsClientSender.sendLocalIceCandidate(candidate);
    }

    @Override
    public void onIceCandidatesRemoved(IceCandidate[] candidates) {
        Log.e(TAG, "onIceCandidatesRemoved ");
    }

    @Override
    public void onIceConnected() {
        Log.i(TAG, "onIceConnected");
    }

    @Override
    public void onIceDisconnected() {
        Log.e(TAG, "onIceDisconnected");
    }

    @Override
    public void onPeerConnectionClosed() {
        Log.e(TAG, "onPeerConnectionClosed ");
    }

    @Override
    public void onPeerConnectionStatsReady(StatsReport[] reports) {
        Log.e(TAG, "onPeerConnectionStatsReady: " + reports);
    }

    @Override
    public void onPeerConnectionError(String description) {
        Log.e(TAG, "onPeerConnectionError: " + description);
    }

    //************************************signal*****************************************//
    @Override
    public void onSignalConnected(SignalingParameters params) {

    }

    @Override
    public void onRemoteDescription(SessionDescription sdp) {

    }

    @Override
    public void onRemoteIceCandidate(IceCandidate candidate) {

    }

    @Override
    public void onRemoteIceCandidatesRemoved(IceCandidate[] candidates) {

    }

    @Override
    public void onChannelClose() {

    }

    @Override
    public void onChannelError(String description) {

    }

    /*****************************IMS消息响应**********************************************/
    @Override
    public void imsRegisterResponse(ServerResponse serverResponse) {

    }

    @Override
    public void imsIncomingCall(ServerResponse serverResponse) {

    }

    @Override
    public void imsCallResponse(ServerResponse serverResponse) {
        if (serverResponse.getTypeRes() == ResponseType.REJECTED) {
            Log.e(TAG, "对方拒绝接听");
        } else {
            Log.w(TAG, "对方接听");
            SessionDescription sdp = new SessionDescription(SessionDescription.Type.ANSWER,
                    serverResponse.getSdpAnswer());
            mRTCEngine.setRemoteDescription(sdp);

        }
    }

    @Override
    public void imsIceCandidate(ServerResponse serverResponse) {
        CandidateModel candidateModel = serverResponse.getCandidate();
        mRTCEngine.addRemoteIceCandidate(
                new IceCandidate(candidateModel.getSdpMid(), candidateModel.getSdpMLineIndex(),
                        candidateModel.getSdp()));
    }

    @Override
    public void imsStartCommunication(ServerResponse serverResponse) {

    }

    @Override
    public void imsStopCommunication(ServerResponse serverResponse) {

    }

    /*********************ims connect status *************************/
    @Override
    public void onOpen(ServerHandshake serverHandshake) {

    }

    @Override
    public void onClose(int i, String s, boolean b) {

    }

    @Override
    public void onError(Exception e) {

    }
}