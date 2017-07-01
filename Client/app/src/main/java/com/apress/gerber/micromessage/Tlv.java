package com.apress.gerber.micromessage;

public class Tlv {

    /** 子域Tag标签 */
    private int tag;

    /** 子域取值的长度 */
    private int length;

    /** 子域取值 */
    private String value;

    public Tlv(int tag, int length, String value) {
        this.tag = tag;
        this.length = length;
        this.value = value;
    }

    public int getTag() {
        return tag;
    }

    public void setTag(int tag) {
        this.tag = tag;
    }

    public int getLength() {
        return length;
    }

    public void setLength(int length) {
        this.length = length;
    }

    public String getValue() {
        return value;
    }

    public void setValue(String value) {
        this.value = value;
    }

    @Override
    public String toString() {
        return "tag=[" + this.tag + "]," + "length=[" + this.length + "]," + "value=[" + this.value + "]";
    }
}