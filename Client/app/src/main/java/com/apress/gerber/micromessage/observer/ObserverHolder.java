package com.apress.gerber.micromessage.observer;

import java.util.Map;

/**
 * 订阅模式管理类
 */
public class ObserverHolder {

    private static ObserverHolder instance;

    private EventObservable observable;

    private ObserverHolder() {
        observable = new EventObservable();
    }

    public static ObserverHolder getInstance() {
        if (instance == null) {
            synchronized (ObserverHolder.class) {
                if (instance == null) {
                    instance = new ObserverHolder();
                }
            }
        }
        return instance;
    }

    /**
     * 将消息接收者注册进来
     * (如果将Activity作为订阅者在此注册的时候，切记在onDestroy()里面移除注册，否则可能导致内存泄露)
     * @param observer
     */
    public void register(IObserver observer){
        observable.addObserver(observer);
    }

    /**
     * 将消息接收者移除注册
     */
    public void unregister(IObserver observer){
        observable.deleteObserver(observer);
    }

    /**
     * 给订阅者发生消息
     * @param data
     */
    public void notifyObservers(Map<Integer, String> data, int flag){
        observable.notifyObservers(data,flag);
    }


}
