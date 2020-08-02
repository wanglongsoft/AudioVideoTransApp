package com.soft.function;

import android.media.AudioRecord;
import android.util.Log;


public class AudioRecordHelper {
    private final String TAG = "AudioRecordControl";
    private AudioRecord audioRecord;
    private int audioSource;
    private int sampleRateInHz;
    private int channelConfig;
    private int audioFormat;
    private int bufferSize;
    private int bufferMinSize;
    private int audioState;
    private boolean isRecording = false;
    private RecordRunnable recordRunnable;
    private AudioRecordHelper.OnAudioRawCapture audioRawCapture;

    public AudioRecordHelper(int audioSource, int sampleRateInHz, int channelConfig, int audioFormat) {
        this.audioSource = audioSource;
        this.sampleRateInHz = sampleRateInHz;
        this.channelConfig = channelConfig;
        this.audioFormat = audioFormat;
        this.bufferMinSize = AudioRecord.getMinBufferSize(this.sampleRateInHz, this.channelConfig, this.audioFormat);
        Log.d(TAG, "AudioRecordControl audioSource: " + this.audioSource + " sampleRateInHz: " + this.sampleRateInHz);
        Log.d(TAG, "AudioRecordControl channelConfig: "+ this.channelConfig + " audioFormat: " + this.audioFormat + " bufferMinSize: " + this.bufferMinSize);

        //FFmpeg 音频编码，
        // 双声道 * 2，每个样本占 2 字节
        //每个音频帧包含的样本数：1024(实验数据)，this.bufferSize最好为音频帧所占字节数的整数倍，且大于 getMinBufferSize
        adjustBufferSize(1024 * 2 * 2, this.bufferMinSize);
        Log.d(TAG, "AudioRecordHelper bufferSize: " + this.bufferSize);
        audioRecord = new AudioRecord(this.audioSource, this.sampleRateInHz, this.channelConfig, this.audioFormat, this.bufferSize);
        audioState = audioRecord.getState();
        if(AudioRecord.STATE_INITIALIZED == audioState) {
            Log.d(TAG, "AudioRecordControl: init success");
        } else {
            Log.d(TAG, "AudioRecordControl: init fail, state: " + audioState);
        }
    }

    private void adjustBufferSize(int audio_frame_size, int min_buffer_size) {
        //this.bufferSize 大于等于min_buffer_size且是audio_frame_size的最小整数倍
        if(min_buffer_size % audio_frame_size == 0) {
            this.bufferSize = min_buffer_size;
        } else {
            if(audio_frame_size > min_buffer_size) {
                this.bufferSize = audio_frame_size;
            } else {
                this.bufferSize = (min_buffer_size / audio_frame_size + 1) * audio_frame_size;
            }
        }
    }

    public void startRecord() {
        if(null == audioRawCapture) {
            Log.d(TAG, "startRecord : audioRawCapture == null, return");
            return;
        }

        if(true == isRecording) {
            Log.d(TAG, "startRecord : audioRawCapture == true, return");
            return;
        }

        if(AudioRecord.STATE_INITIALIZED == audioState) {
            Log.d(TAG, "startRecord success");
            if(null != audioRecord) {
                recordRunnable = new RecordRunnable(bufferSize, audioRawCapture);
                audioRecord.startRecording();
                audioState = audioRecord.getState();
                isRecording = true;
                new Thread(recordRunnable).start();
            } else {
                Log.d(TAG, "startRecord: audioRecord == null, return");
            }
        } else {
            Log.d(TAG, "startRecord: start fail, state: " + audioState);
        }
    }

    public void stopRecord() {
        Log.d(TAG, "stopRecord isRecording: " + isRecording);
        if(isRecording) {
            isRecording = false;
            if(null != audioRecord) {
                audioRecord.stop();
                audioRecord.release();
                audioState = audioRecord.getState();
                audioRecord = null;
            }
        }
    }

    public class RecordRunnable implements Runnable {

        private int buffreSize;
        private OnAudioRawCapture audioRawCapture;

        public RecordRunnable(int bufferSize, OnAudioRawCapture audioRawCapture) {
            this.buffreSize = bufferSize;
            this.audioRawCapture = audioRawCapture;
        }

        @Override
        public void run() {
            Log.d(TAG, "RecordRunnable, record .....");
            byte[] data = new byte[this.buffreSize];
            while (isRecording) {
                int len = audioRecord.read(data, 0, this.buffreSize);
                this.audioRawCapture.onCapture(data, len);
            }
            //标志音频数据采集结束
            this.audioRawCapture.onCapture(null, -1);
        }
    }

    public void setAudioRawCaptureListener(OnAudioRawCapture audioRawCapture) {
        this.audioRawCapture = audioRawCapture;
    }

    public interface OnAudioRawCapture {
        void onCapture(byte[] data, int data_length);
    }
}
