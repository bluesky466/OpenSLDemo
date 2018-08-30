package me.linjw.opensldemo;

/**
 * Created by linjw on 18-8-22.
 */

public class OpenSLDemo {
    static {
        System.loadLibrary("opensldemo");
    }

    public native void startRecord();
    public native void stopRecord();

    public native void startPlay();
    public native void stopPlay();
}
