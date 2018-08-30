package me.linjw.opensldemo;

import android.Manifest;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {
    private OpenSLDemo mOpenSLDemo = new OpenSLDemo();

    private Thread mRecordThread;
    private Thread mPlayThread;

    private TextView mButtonRecord;
    private TextView mButtonPlay;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mButtonRecord = (TextView) findViewById(R.id.record);
        mButtonPlay = (TextView) findViewById(R.id.play);

        ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.RECORD_AUDIO,
                Manifest.permission.WRITE_EXTERNAL_STORAGE}, 0);
    }

    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.record:
                if (mRecordThread == null) {
                    startRecort();
                } else {
                    stopRecort();
                }
                break;
            case R.id.play:
                if (mPlayThread == null) {
                    startPlay();
                } else {
                    stopPlay();
                }
                break;
        }
    }

    private void startRecort() {
        mButtonRecord.setText(getString(R.string.recording));
        mButtonPlay.setEnabled(false);

        mRecordThread = new Thread(new Runnable() {
            @Override
            public void run() {
                mOpenSLDemo.startRecord();
            }
        });
        mRecordThread.start();
    }

    private void stopRecort() {
        mButtonRecord.setText(getString(R.string.not_recording));
        mButtonPlay.setEnabled(true);

        mOpenSLDemo.stopRecord();

        try {
            mRecordThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        mRecordThread = null;
    }

    private void startPlay() {
        mButtonRecord.setEnabled(false);
        mButtonPlay.setText(getString(R.string.playing));

        mPlayThread = new Thread(new Runnable() {
            @Override
            public void run() {
                mOpenSLDemo.startPlay();

                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        stopPlay();
                    }
                });
            }
        });
        mPlayThread.start();
    }


    private void stopPlay() {
        mButtonRecord.setEnabled(true);
        mButtonPlay.setText(getString(R.string.not_playing));

        mOpenSLDemo.stopPlay();

        if (mPlayThread == null) {
            return;
        }

        try {
            mPlayThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        mPlayThread = null;
    }
}
