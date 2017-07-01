package com.apress.gerber.micromessage;

import android.net.Network;
import android.util.Log;

import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;

/**
 * 发送
 */
public class TCPSend {
    public static final String TAG = "TCPSend";

    /**
     */
    public TCPSend(Socket socket) {
        try {
            out = socket.getOutputStream();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    OutputStream out = null;
    /**
     * 发送数据
     */
    public void sendMessage(int type, String user, String msg) {
        try {
            Log.i(TAG, "sendMessage: type = " + type);
            byte data[] = TypeConversion.hexString2Bytes(NetworkData.builderNetworkData(type, user, msg));
            if (data.length > 0) {
                out.write(data);
                out.flush();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * 关闭output
     */
    public void close() {
        if (out != null) {
            try {
                out.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }
}
