package org.apd.executor;

import org.apd.storage.EntryResult;

import java.util.List;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class ReaderPreferredLock {
    private final ReentrantReadWriteLock lock = new ReentrantReadWriteLock();

//    public void read(Runnable task) {
//        lock.readLock().lock();
//        try {
//            task.run(); // Execută citirea
//        } finally {
//            lock.readLock().unlock();
//        }
//    }
//
//    public void write(Runnable task) {
//        lock.writeLock().lock();
//        try {
//            task.run(); // Execută scrierea
//        } finally {
//            lock.writeLock().unlock();
//        }
//    }

    public void read(Runnable task) {
        try {
            System.out.println(Thread.currentThread().getName() + " intră în citire.");
            lock.readLock().lock();
            task.run();
        } finally {
            System.out.println(Thread.currentThread().getName() + " finalizează citirea.");
            lock.readLock().unlock();
        }
    }

    public void write(Runnable task) {
        try {
            System.out.println(Thread.currentThread().getName() + " intră în scriere.");
            lock.writeLock().lock();
            task.run();
        } finally {
            System.out.println(Thread.currentThread().getName() + " finalizează scrierea.");
            lock.writeLock().unlock();
        }
    }

    private void handleReaderPreferred(StorageTask task, List<EntryResult> results, ReaderPreferredLock lock) {
        Object sharedDatabase = new Object();
        if (task.isWrite()) {
            lock.write(() -> {
                synchronized (sharedDatabase) { // Adaugă sincronizare explicită la nivel de bază de date.
                    System.out.println("Scriere în ReaderPreferred: " + task);
                    EntryResult result = sharedDatabase.addData(task.index(), task.data());
                    results.add(result);
                    System.out.println("Scriere finalizată: " + result);
                }
            });
        } else {
            lock.read(() -> {
                synchronized (sharedDatabase) { // Citirile sunt și ele sincronizate pentru consistență.
                    System.out.println("Citire în ReaderPreferred: " + task);
                    EntryResult result = sharedDatabase.getClass(task.index());
                    results.add(result);
                    System.out.println("Citire finalizată: " + result);
                }
            });
        }
    }
}