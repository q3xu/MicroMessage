package com.apress.gerber.micromessage;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.Toast;

import com.apress.gerber.micromessage.observer.IObservable;
import com.apress.gerber.micromessage.observer.IObserver;
import com.apress.gerber.micromessage.observer.ObserverHolder;

import java.util.List;
import java.util.Map;

/**
 * Created by q3xu on 2017/6/24.
 */

public class LoginActivity extends BaseActivity implements IObserver {
    public static final String TAG = "LoginActivity";

    private EditText accountEdit;

    private EditText passwordEdit;

    private Button login;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);

        ImageManager.initImageManager();

        startService(new Intent(this, SocketService.class));
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(2000);  // wait socketService starting
                }catch (InterruptedException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
        }).start();
        ObserverHolder.getInstance().register(this);

        accountEdit = (EditText) findViewById(R.id.account);
        passwordEdit = (EditText) findViewById(R.id.password);
        login = (Button) findViewById(R.id.login);
        login.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String account = accountEdit.getText().toString();
                String password = passwordEdit.getText().toString();
                if (account.equals("") || password.equals("")) {
                    Toast.makeText(LoginActivity.this, "account or password is invalid, should not be null.",
                            Toast.LENGTH_SHORT).show();
                } else {
                    if (!SocketService.isConnect)
                        Toast.makeText(LoginActivity.this, "connect to server failed.",
                                Toast.LENGTH_SHORT).show();
                    else {
                        SocketService.tcpSend.sendMessage(NetworkData.MSG_TYPE_LOGIN, account, "");
                    }
                }
            }
        });
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
            case NetworkData.MSG_TYPE_USERLIST:  // login success.
                Log.i(TAG, "MSG_TYPE_USERLIST");
                runOnUiThread(new Runnable() {  //切换到主线程更新ui
                    @Override
                    public void run() {

                        @SuppressWarnings("unchecked")
                        Map<Integer, String> tvs = (Map<Integer, String>) object;
                        if (tvs.containsKey(NetworkData.TAG_USER_LIST)) {
                            List<Tlv> tlvs = TlvUtils.resolverTlvList(tvs.get(NetworkData.TAG_USER_LIST));
                            for (Tlv tvl : tlvs) {
                                if (NetworkData.TAG_USER_NAME == tvl.getTag()) {
                                    String username = TypeConversion.hexString2String(tvl.getValue());
                                    Contacter user = new Contacter(username, ImageManager.getRandomImageId());
                                    ContacterManager.addContacter(user);
                                }
                            }
                        }

                        // add login in account
                        String account = accountEdit.getText().toString();
                        Contacter user = new Contacter(account, ImageManager.getRandomImageId());
                        ContacterManager.addContacter(user);

                        // skip to next activity
                        Intent intent = new Intent(LoginActivity.this, MainActivity.class);
                        intent.putExtra("account", account);
                        startActivity(intent);
                        finish();
                    }
                });
                break;

            case NetworkData.MSG_TYPE_LOGINFAIL:  // login failed.
                Log.i(TAG, "MSG_TYPE_LOGINFAIL");
                runOnUiThread(new Runnable() {  //切换到主线程更新ui
                    @Override
                    public void run() {
                        accountEdit.setText("");
                        passwordEdit.setText("");
                        Toast.makeText(LoginActivity.this, "account is already existing.",
                                Toast.LENGTH_SHORT).show();
                    }
                });
                break;

            default:
                break;
        }
    }
}
