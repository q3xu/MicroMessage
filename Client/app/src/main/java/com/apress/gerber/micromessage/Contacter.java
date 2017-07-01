package com.apress.gerber.micromessage;

/**
 * Created by q3xu on 2017/6/24.
 */

public class Contacter {

    private String name;

    private  int imageId;

    public Contacter(String name, int imageId) {
        this.name = name;
        this.imageId = imageId;
    }

    public String getName() {
        return name;
    }

    public int getImageId() {
        return imageId;
    }
}
