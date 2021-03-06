package com.apress.gerber.micromessage.observer;

/**
 * 被观察者
 */

public interface IObservable {

    /**
     * 向消息订阅队列添加订阅者
     */
    void deleteObserver(IObserver observer);

    /**
     * 删除指定订阅者
     */
    void addObserver(IObserver observer);

    /**
     * 通知消息订阅队列里的每一个订阅者
     * @param data  给订阅者传递的参数
     * @param flag 标示
     */
    void notifyObservers(Object data, int flag);

}
