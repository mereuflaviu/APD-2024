package org.apd.executor;

import org.apd.storage.EntryResult;
import org.apd.storage.SharedDatabase;

import org.apd.executor.StorageTask;
import org.apd.executor.LockType;
import org.apd.executor.ThreadPool;
import org.apd.executor.WriterPreferred1;
import org.apd.executor.WriterPreferred2;


import java.util.ArrayList;
import java.util.List;

/* DO NOT MODIFY THE METHODS SIGNATURES */
public class TaskExecutor {
    private final SharedDatabase sharedDatabase;

    public TaskExecutor(int storageSize, int blockSize, long readDuration, long writeDuration) {
        sharedDatabase = new SharedDatabase(storageSize, blockSize, readDuration, writeDuration);
    }

    public List<EntryResult> ExecuteWork(int numberOfThreads, List<StorageTask> tasks, LockType lockType) {
        /* IMPLEMENT HERE THE THREAD POOL, ASSIGN THE TASKS AND RETURN THE RESULTS */
        ThreadPool threadPool = new ThreadPool(numberOfThreads); //aici se creaaza threadpool-ul cu numarul de threaduri care intra ca input in parametru
        List<EntryResult> results = Collections.synchronizedList(new ArrayList<>());

        ReaderPreferredLock readerLock = new ReaderPreferredLock();
        WriterPreferredLock1 writerLock1 = new WriterPreferredLock1();
        WriterPreferredLock2 writerLock2 = new WriterPreferredLock2();

        for (StorageTask task : tasks) {
            threadPool.submit(() -> {
                if (lockType == LockType.ReaderPreferred) {
                    if (task.isWrite()) {
                        readerLock.write(() -> results.add(sharedDatabase.addData(task.index(), task.data())));
                    } else {
                        readerLock.read(() -> results.add(sharedDatabase.getData(task.index())));
                    }
                } else if (lockType == LockType.WriterPreferred1) {
                    if (task.isWrite()) {
                        writerLock1.write(() -> results.add(sharedDatabase.addData(task.index(), task.data())));
                    } else {
                        writerLock1.read(() -> results.add(sharedDatabase.getData(task.index())));
                    }
                } else if (lockType == LockType.WriterPreferred2) {
                    if (task.isWrite()) {
                        writerLock2.write(() -> results.add(sharedDatabase.addData(task.index(), task.data())));
                    } else {
                        writerLock2.read(() -> results.add(sharedDatabase.getData(task.index())));
                    }
                }
            });
        }

        threadPool.shutdown();


        return results;
    }

    public List<EntryResult> ExecuteWorkSerial(List<StorageTask> tasks) {
        var results = tasks.stream().map(task -> {
            try {
                if (task.isWrite()) {
                    return sharedDatabase.addData(task.index(), task.data());
                } else {
                    return sharedDatabase.getData(task.index());
                }
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }).toList();

        return results.stream().toList();
    }
}
