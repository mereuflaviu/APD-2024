package org.apd.executor;

import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

public class WriterPreferredLock1 {
    private final ReentrantLock lock = new ReentrantLock();
    private final Condition condition = lock.newCondition();
    private int readers = 0;
    private boolean writerActive = false;

    public void read(Runnable task) {
        lock.lock();
        try {
            while (writerActive) {
                condition.await(); // Cititorii așteaptă dacă un scriitor este activ
            }
            readers++;
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        } finally {
            lock.unlock();
        }

        try {
            task.run(); // Execută sarcina de citire
        } finally {
            lock.lock();
            readers--;
            if (readers == 0) {
                condition.signalAll(); // Notifică scriitorii
            }
            lock.unlock();
        }
    }

    public void write(Runnable task) {
        lock.lock();
        try {
            while (writerActive || readers > 0) {
                condition.await(); // Așteaptă până când resursa este liberă
            }
            writerActive = true;
            task.run();
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        } finally {
            writerActive = false;
            condition.signalAll(); // Notifică cititorii și scriitorii
            lock.unlock();
        }
}}