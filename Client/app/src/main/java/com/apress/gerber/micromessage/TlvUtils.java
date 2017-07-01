package com.apress.gerber.micromessage;

import android.animation.TypeConverter;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public abstract class TlvUtils {

    /**
     * 将16进制字符串转换为TLV对象列表
     * @param hexString
     * @return
     */
    public static List<Tlv> resolverTlvList(String hexString) {
        List<Tlv> tlvs = new ArrayList<Tlv>();

        int position = 0;
        while (position != hexString.length()) {
            String _hexTag = hexString.substring(position, position + 4);
            position += _hexTag.length();
            int _tl = (int)TypeConversion.hexStringToLong(_hexTag);

            String _hexLen = hexString.substring(position, position + 4);
            position += _hexLen.length();

            int _vl = (int)TypeConversion.hexStringToLong(_hexLen);
            String _hexValue = hexString.substring(position, position + _vl * 2);
            position += _hexValue.length();
            tlvs.add(new Tlv(_tl, _vl, _hexValue));
        }
        return tlvs;
    }

    /**
     * 将16进制字符串转换为<T-V> MAP
     * @param hexString
     * @return
     */
    public static Map<Integer, String> resolverTvMap(String hexString) {

        Map<Integer, String> tvs = new HashMap<Integer, String>();

        int position = 0;
        while (position != hexString.length()) {
            String _hexTag = hexString.substring(position, position + 4);
            position += _hexTag.length();
            int _tl = (int)TypeConversion.hexStringToLong(_hexTag);

            String _hexLen = hexString.substring(position, position + 4);
            position += _hexLen.length();

            int _vl = (int)TypeConversion.hexStringToLong(_hexLen);
            String _hexValue = hexString.substring(position, position + _vl * 2);
            position += _hexValue.length();

            tvs.put(_tl, _hexValue);
        }
        return tvs;
    }

    /**
     * 将TLV对象转换为16进制字符串
     * @param
     * @return hexString
     */
    public static String builderTlvString(Tlv tlv) {
        String hexString = "";
        hexString += TypeConversion.intToHexString(tlv.getTag(), 2);
        hexString += TypeConversion.intToHexString(tlv.getLength(), 2);
        hexString += TypeConversion.string2HexString(tlv.getValue());
        return hexString;
    }
}