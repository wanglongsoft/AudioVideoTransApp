package com.soft.function;

import android.content.res.AssetManager;
import android.view.Surface;

public class FunctionControl {
    static {
        System.loadLibrary("native-lib");
    }

    private static volatile FunctionControl instance;

    private FunctionControl() {
    }

    public static FunctionControl getInstance() {
        if(null == instance) {
            synchronized (FunctionControl.class) {
                if(null == instance) {
                    instance = new FunctionControl();
                }
            }
        }
        return instance;
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native void setSurface(Surface surface);
    public native void setSurfaceSize(int width, int height);
    public native void saveAssetManager(AssetManager manager);
    public native void rendSurface(byte[] data, int width, int height, int video_rotation);
    public native void rendAudio(byte[] data, int length);
    public native void rendAudioFromPcmFile();
    public native void releaseResources();
}
