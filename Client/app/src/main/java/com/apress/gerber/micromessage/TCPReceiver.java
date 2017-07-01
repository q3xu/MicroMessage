package com.apress.gerber.micromessage;

import android.util.Log;

import com.apress.gerber.micromessage.observer.ObserverHolder;

import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.Map;

/**
 * 接收
 */
public class TCPReceiver {

    public static final String TAG = "TCPReceiver";

    private DataInputStream inputStream;

    /**
     * 服务器注册的端口号
     */
    public TCPReceiver(Socket socket) {
        Log.i(TAG, "TCPReceiver");
        try {
            inputStream = new DataInputStream(socket.getInputStream());
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void recvHandle() {
        Log.i(TAG, "recvHandle");
        try {
            // 从Socket当中得到InputStream对象
            while (SocketService.isConnect) {
                byte hdr[] = new byte[NetworkData.MSG_HDR_SIZE];
                inputStream.readFully(hdr);
                NetworkData networkData = new NetworkData(TypeConversion.bytes2HexString(hdr));
                Log.i(TAG, "received msg type = " + networkData.getMsgType());

                if (networkData.getMsgSize() > 0) {
                    byte data[] = new byte[networkData.getMsgSize()];
                    inputStream.readFully(data);
                    networkData.resolverData(TypeConversion.bytes2HexString(data));
                    ObserverHolder.getInstance().notifyObservers(networkData.getTvs(), networkData.getMsgType());
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
