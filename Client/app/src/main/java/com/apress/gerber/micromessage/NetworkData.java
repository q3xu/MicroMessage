package com.apress.gerber.micromessage;

import android.animation.TypeConverter;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class NetworkData {

    public static final int MAGIC_CODE          = 0x5457;
    public static final int MSG_VERSION         = 1;
    public static final int MSG_RESERVED        = 0;
    public static final int MSG_HDR_SIZE        = 8;

    public static final int MSG_TYPE_LOGIN      = 1;
    public static final int MSG_TYPE_CONTACT    = 2;
    public static final int MSG_TYPE_LOGOUT     = 3;
    public static final int MSG_TYPE_USERLIST   = 4;
    public static final int MSG_TYPE_LOGINFAIL  = 5;

    public static final int TAG_USER_NAME        = 1;
    public static final int TAG_PASSWARD         = 2;
    public static final int TAG_MSG               = 3;
    public static final int TAG_USER_LIST        = 4;

    private NetworkDataHdr header;
    private Map<Integer, String> tvs;

    // input hexString
    public NetworkData(String hexHdrString) {
        header = new NetworkDataHdr(hexHdrString);
    }

    public void resolverData(String hexString) {
        tvs = TlvUtils.resolverTvMap(hexString);
    }

    public int getMsgType() {
        return header.msgType;
    }

    public int getMsgSize() {
        return header.msgSize;
    }

    public Map<Integer, String> getTvs() {
        return tvs;
    }

    public static String builderNetworkData(int msgType, String username, String msg) {
        String data = "";
        switch (msgType) {
            case MSG_TYPE_CONTACT:
                data = builderNetworkDataContact(username, msg);
                break;
            case MSG_TYPE_LOGIN:
                data = builderNetworkDataLogin(username);
                break;
            case MSG_TYPE_LOGOUT:
                data = builderNetworkDataLogout(username);
                break;
            case MSG_TYPE_LOGINFAIL:
                data = builderNetworkDataLoginFail(username);
                break;
            default:
                break;
        }
        return data;
    }

    private static String builderNetworkDataHdr(int msgType, int size) {
        String hexString = TypeConversion.intToHexString(MAGIC_CODE, 2);
        hexString += TypeConversion.intToHexString(size, 2);
        hexString += TypeConversion.intToHexString(MSG_VERSION, 1);
        hexString += TypeConversion.intToHexString(msgType, 1);
        hexString += TypeConversion.intToHexString(MSG_RESERVED, 2);
        return hexString;
    }

    private static String builderNetworkDataLogin(String username) {
        Tlv tlv = new Tlv(TAG_USER_NAME, username.length(), username);
        String hexData = TlvUtils.builderTlvString(tlv);
        String hexHdr = builderNetworkDataHdr(MSG_TYPE_LOGIN, hexData.length()/2);
        return hexHdr + hexData;
    }

    private static String builderNetworkDataLogout(String username) {
        Tlv tlv = new Tlv(TAG_USER_NAME, username.length(), username);
        String hexData = TlvUtils.builderTlvString(tlv);
        String hexHdr = builderNetworkDataHdr(MSG_TYPE_LOGOUT, hexData.length()/2);
        return hexHdr + hexData;
    }

    private static String builderNetworkDataContact(String username, String msg) {
        Tlv tlv = new Tlv(TAG_USER_NAME, username.length(), username);
        String hexData = TlvUtils.builderTlvString(tlv);
        tlv = new Tlv(TAG_MSG, msg.length(), msg);
        hexData += TlvUtils.builderTlvString(tlv);
        String hexHdr = builderNetworkDataHdr(MSG_TYPE_CONTACT, hexData.length() / 2);
        return hexHdr + hexData;
    }

    private static String builderNetworkDataLoginFail(String username) {
        String hexHdr = builderNetworkDataHdr(MSG_TYPE_LOGINFAIL, 0);
        return hexHdr;
    }

    public class NetworkDataHdr {
        public int msgType;
        public int msgSize;

        public NetworkDataHdr(String hexString) {
            int position = 0;
            String _hexMc = hexString.substring(position, position + 4);
            position += _hexMc.length();

            String _hexLen = hexString.substring(position, position + 4);
            position += _hexLen.length();
            msgSize = (int)TypeConversion.hexStringToLong(_hexLen);

            String _hexVer = hexString.substring(position, position + 2);
            position += _hexVer.length();

            String _hexTp = hexString.substring(position, position + 2);
            position += _hexTp.length();
            msgType = (int)TypeConversion.hexStringToLong(_hexTp);

            String _hexRv = hexString.substring(position, position + 4);
            position += _hexRv.length();
        }
    }
}