package com.apress.gerber.micromessage;

import android.app.Activity;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by q3xu on 2017/6/24.
 */

public class ContacterManager {
    public static List<Contacter> contacters = new ArrayList<>();

    public static void addContacter(Contacter contacter) {
        if (contacter != null)
            contacters.add(contacter);
    }

    public static void removeContacter(Contacter contacter) {
        if (contacter != null)
            contacters.remove(contacter);
    }

    public static Contacter findContacter(String name) {
        for (Contacter contacter : contacters) {
            if (contacter.getName().equals(name))
                return contacter;
        }
        return null;
    }

    public static void removeAll() {
        contacters.clear();
    }
}
