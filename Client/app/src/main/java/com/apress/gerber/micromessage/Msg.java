package com.apress.gerber.micromessage;

/**
 * Created by q3xu on 2017/6/23.
 */

public class Msg {
    public static final int TYPE_RECEIVED = 0;

    public static final int TYPE_SENT = 1;

    private String content;

    private String username;

    private int type;

    public Msg(String content, String name, int type) {
        this.content = content;
        this.username = name;
        this.type = type;
    }

    public String getContent() {
        return content;
    }

    public String getUsername() { return username; }

    public int getType() {
        return type;
    }
}
