package com.apress.gerber.micromessage;

import com.apress.gerber.micromessage.observer.IObservable;
import com.apress.gerber.micromessage.observer.IObserver;
import com.apress.gerber.micromessage.observer.ObserverHolder;
import com.google.android.gms.appindexing.Action;
import com.google.android.gms.appindexing.AppIndex;
import com.google.android.gms.appindexing.Thing;
import com.google.android.gms.common.api.GoogleApiClient;

import android.net.Network;
import android.net.Uri;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.content.Intent;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;

import java.net.Socket;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;


public class MainActivity extends BaseActivity implements IObserver {
    public static final String TAG = "MainActivity";

    public String username;
    ExecutorService executorService = Executors.newSingleThreadExecutor();

    private List<Msg> msgList = new ArrayList<Msg>();

    private EditText inputText;

    private Button send;

    private RecyclerView msgRecyclerView;

    private MsgAdapter adapter;

    private String inputTextString;
    /**
     * ATTENTION: This was auto-generated to implement the App Indexing API.
     * See https://g.co/AppIndexing/AndroidStudio for more information.
     */
    private GoogleApiClient client;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "onCreate");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Intent intent = getIntent();
        username = intent.getStringExtra("account");

        // 启动服务
        startService(new Intent(this, SocketService.class));
        ObserverHolder.getInstance().register(this);

        // 响应控件
        inputText = (EditText) findViewById(R.id.input_text);
        send = (Button) findViewById(R.id.send);
        msgRecyclerView = (RecyclerView) findViewById(R.id.msg_recycler_view);
        LinearLayoutManager layoutManager = new LinearLayoutManager(this);
        msgRecyclerView.setLayoutManager(layoutManager);
        adapter = new MsgAdapter(msgList);
        msgRecyclerView.setAdapter(adapter);
        send.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                inputTextString = inputText.getText().toString();
                if (!"".equals(inputTextString)) {
                    Msg msg = new Msg(inputTextString, username, Msg.TYPE_SENT);
                    msgList.add(msg);
                    adapter.notifyItemInserted(msgList.size() - 1); // 当有新消息时，刷新ListView中的显示
                    msgRecyclerView.scrollToPosition(msgList.size() - 1); // 将ListView定位到最后一行
                    inputText.setText(""); // 清空输入框中的内容
                    sendMsg();
                }
            }
        });
        // ATTENTION: This was auto-generated to implement the App Indexing API.
        // See https://g.co/AppIndexing/AndroidStudio for more information.
        client = new GoogleApiClient.Builder(this).addApi(AppIndex.API).build();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        ObserverHolder.getInstance().unregister(this);
    }

    @Override
    public void onMessageReceived(IObservable observable, final Object object, int flag) {
        Log.i(TAG, "onMessageReceived");
        switch (flag) {
            case NetworkData.MSG_TYPE_LOGIN:
                Log.i(TAG, "MSG_TYPE_LOGIN");
                runOnUiThread(new Runnable() {  //切换到主线程更新ui
                    @Override
                    public void run() {
                        @SuppressWarnings("unchecked")
                        Map<Integer, String> tvs = (Map<Integer, String>) object;
                        String user = TypeConversion.hexString2String(tvs.get(NetworkData.TAG_USER_NAME));
                        if (ContacterManager.findContacter(user) == null) {
                            Contacter contacter = new Contacter(username, ImageManager.getRandomImageId());
                            ContacterManager.addContacter(contacter);
                        }
                        Toast.makeText(MainActivity.this, user + " has jouined the communication",
                                Toast.LENGTH_SHORT).show();
                    }
                });
                break;

            case NetworkData.MSG_TYPE_CONTACT:
                Log.i(TAG, "MSG_TYPE_CONTACT");
                runOnUiThread(new Runnable() {  //切换到主线程更新ui
                    @Override
                    public void run() {
                        @SuppressWarnings("unchecked")
                        Map<Integer, String> tvs = (Map<Integer, String>) object;
                        String user = TypeConversion.hexString2String(tvs.get(NetworkData.TAG_USER_NAME));
                        String msg = TypeConversion.hexString2String(tvs.get(NetworkData.TAG_MSG));
                        Msg echo = new Msg(msg, user, Msg.TYPE_RECEIVED);
                        msgList.add(echo);
                        adapter.notifyItemInserted(msgList.size() - 1); // 当有新消息时，刷新ListView中的显示
                        msgRecyclerView.scrollToPosition(msgList.size() - 1); // 将ListView定位到最后一行
                    }
                });
                break;

            case NetworkData.MSG_TYPE_LOGOUT:
                Log.i(TAG, "MSG_TYPE_LOGOUT");
                runOnUiThread(new Runnable() {  //切换到主线程更新ui
                    @Override
                    public void run() {
                        @SuppressWarnings("unchecked")
                        Map<Integer, String> tvs = (Map<Integer, String>) object;
                        String user = TypeConversion.hexString2String(tvs.get(NetworkData.TAG_USER_NAME));
                        ContacterManager.removeContacter(ContacterManager.findContacter(user));
                        Toast.makeText(MainActivity.this, user + " has leaved the communication",
                                Toast.LENGTH_SHORT).show();
                    }
                });
                break;

            default:
                break;
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.exit_item:
                Intent intent = new Intent("com.apress.gerber.micromessage.FORCE_OFFLINE"); //发动广播
                sendBroadcast(intent);
                break;
            default:
        }
        return true;
    }

    /**
     * 发送信息
     */
    private void sendMsg() {
        executorService.execute(runnable);
    }

    Runnable runnable = new Runnable() {
        @Override
        public void run() {
            if (SocketService.isConnect)
                SocketService.tcpSend.sendMessage(NetworkData.MSG_TYPE_CONTACT, username, inputTextString);
        }
    };

    /**
     * ATTENTION: This was auto-generated to implement the App Indexing API.
     * See https://g.co/AppIndexing/AndroidStudio for more information.
     */
    public Action getIndexApiAction() {
        Thing object = new Thing.Builder()
                .setName("Main Page") // TODO: Define a title for the content shown.
                // TODO: Make sure this auto-generated URL is correct.
                .setUrl(Uri.parse("http://[ENTER-YOUR-URL-HERE]"))
                .build();
        return new Action.Builder(Action.TYPE_VIEW)
                .setObject(object)
                .setActionStatus(Action.STATUS_TYPE_COMPLETED)
                .build();
    }

    @Override
    public void onStart() {
        super.onStart();

        // ATTENTION: This was auto-generated to implement the App Indexing API.
        // See https://g.co/AppIndexing/AndroidStudio for more information.
        client.connect();
        AppIndex.AppIndexApi.start(client, getIndexApiAction());
    }

    @Override
    public void onStop() {
        super.onStop();

        // ATTENTION: This was auto-generated to implement the App Indexing API.
        // See https://g.co/AppIndexing/AndroidStudio for more information.
        AppIndex.AppIndexApi.end(client, getIndexApiAction());
        client.disconnect();
    }
}
