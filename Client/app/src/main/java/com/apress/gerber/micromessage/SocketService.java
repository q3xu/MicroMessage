package com.apress.gerber.micromessage;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

/**
 * 接收消息
 */
public class SocketService extends Service {
    private static final String TAG = "SocketService";

    public static Socket socket;
    public static TCPSend tcpSend;
    public static boolean isConnect = false;
    private static boolean isRecv = false;
    private TCPReceiver tcpReceiver;
    private RecvThread recvThread;

    @Override
    public void onCreate() {
        Log.i(TAG, "onCreate");
        super.onCreate();
        recvThread = new RecvThread();

    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "onDestroy");
        super.onDestroy();
        close();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "onStartCommand");
        if (!isRecv) {
            recvThread.start();
            isRecv = true;
        }
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void initSocket() {
        try {
            Log.v(TAG, "iniSocket");
            String server_ip = getString(R.string.SERVER_IP);
            int server_port = Integer.parseInt(getString(R.string.SERVER_PORT));
            socket = new Socket(server_ip, server_port);
            Log.i(TAG, "isBound" + socket.isBound() + " isConnected" + socket.isConnected());
            if (socket.isConnected() && !socket.isClosed())
                isConnect = true;
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    private class RecvThread extends Thread {
        @Override
        public void run() {
            Log.i(TAG, "start service...");
            if (!isConnect)
                initSocket();
            if (isConnect) {
                tcpSend = new TCPSend(socket);
                tcpReceiver = new TCPReceiver(socket);
                tcpReceiver.recvHandle();
            }
            else
                Log.i(TAG, "socket create failed.");
        }
    };

    /**
     * 关闭连接
     */
    public void close() {
        tcpSend.close();

        if (socket.isInputShutdown()) { //判断输入流是否为打开状态
            try {
                socket.shutdownInput();  //关闭输入流
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        if (socket.isOutputShutdown()) {  //判断输出流是否为打开状态
            try {
                socket.shutdownOutput(); //关闭输出流
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        if (socket.isConnected()) {  //判断是否为连接状态
            try {
                socket.close();  //关闭socket
                isConnect = false;
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        isRecv = false;
    }
}
