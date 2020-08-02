package com.soft.audiovideotransapp;

import android.Manifest;
import android.media.AudioFormat;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;

import com.soft.function.AudioRecordHelper;
import com.soft.function.BaseActivity;
import com.soft.function.CameraHelper;
import com.soft.function.FunctionControl;

public class MainActivity extends BaseActivity {

    public static final String TAG = "MainActivity";

    private Button mStartEncode;
    private Button mStopEncode;
    private SurfaceView m_surface_view;
    private CameraHelper myCamera;
    private AudioRecordHelper audioRecordHelper;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        requestRunTimePermission(new String[]{
                        Manifest.permission.CAMERA,
                        Manifest.permission.RECORD_AUDIO,
                        Manifest.permission.READ_EXTERNAL_STORAGE,
                        Manifest.permission.WRITE_EXTERNAL_STORAGE},
                null);

        initSurfaceView();
        initCommon();
        cameraPreview();
        audioRecord();
    }


    private void initSurfaceView() {
        m_surface_view = findViewById(R.id.surface_view);
        m_surface_view.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                Log.d(TAG, "surfaceCreated: ");
                FunctionControl.getInstance().saveAssetManager(getAssets());
                FunctionControl.getInstance().setSurface(holder.getSurface());
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                Log.d(TAG, "surfaceChanged: ");
                FunctionControl.getInstance().setSurfaceSize(width, height);
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                Log.d(TAG, "surfaceDestroyed: ");
                FunctionControl.getInstance().releaseResources();
            }
        });
    }

    private void initCommon() {
        mStartEncode = findViewById(R.id.start_encode);
        mStartEncode.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(null != myCamera) {
                    myCamera.startPreview();
                }
                if(null != audioRecordHelper) {
                    audioRecordHelper.startRecord();
                }
            }
        });

        mStopEncode = findViewById(R.id.stop_encode);
        mStopEncode.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(null != myCamera) {
                    myCamera.stopPreview();
                }
                if(null != audioRecordHelper) {
                    audioRecordHelper.stopRecord();
                }
            }
        });
    }

    private void cameraPreview() {
        myCamera = new CameraHelper();
        myCamera.setPictureRawCaptureListener(new CameraHelper.OnPictureRawCapture() {
            @Override
            public void onCapture(byte[] data, int width, int height, int video_rotation) {
                if(data != null) {
                    FunctionControl.getInstance().rendSurface(data, width, height, video_rotation);
                } else {
                    Log.d(TAG, "onCapture: 视频采集结束");
                    FunctionControl.getInstance().rendSurface(null, -1, -1, -1);
                }
            }
        });
    }

    private void audioRecord() {
        audioRecordHelper = new AudioRecordHelper(
                MediaRecorder.AudioSource.MIC,
                44100,
                AudioFormat.CHANNEL_IN_STEREO,
                AudioFormat.ENCODING_PCM_16BIT
                );
        audioRecordHelper.setAudioRawCaptureListener(new AudioRecordHelper.OnAudioRawCapture() {
            @Override
            public void onCapture(byte[] data, int data_length) {
                Log.d(TAG, "onCapture data_length: " + data_length);
                if(data != null) {
                    FunctionControl.getInstance().rendAudio(data, data_length);
                } else {
                    Log.d(TAG, "onCapture: 音频采集结束");
                    FunctionControl.getInstance().rendAudio(null, -1);
                }
            }
        });
    }
}
