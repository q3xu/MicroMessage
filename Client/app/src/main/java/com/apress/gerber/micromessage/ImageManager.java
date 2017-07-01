package com.apress.gerber.micromessage;

import android.app.Activity;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

/**
 * Created by q3xu on 2017/6/24.
 */

public class ImageManager {
    public static Map<Integer, Integer> images = new HashMap<Integer, Integer>();


    public static void initImageManager() {
        images.put(0, R.drawable.apple_pic);
        images.put(1, R.drawable.banana_pic);
        images.put(2, R.drawable.cherry_pic);
        images.put(3, R.drawable.grape_pic);
        images.put(4, R.drawable.mango_pic);
        images.put(5, R.drawable.orange_pic);
        images.put(6, R.drawable.pear_pic);
        images.put(7, R.drawable.pineapple_pic);
        images.put(8, R.drawable.strawberry_pic);
        images.put(9, R.drawable.watermelon_pic);
    }

    public static Integer getRandomImageId() {
        Random random = new Random();
        return getImageId(random.nextInt(images.size()));
    }

    private static Integer getImageId(Integer key) {
        return images.get(key);
    }
}
