package org.apd.executor;

import org.apd.storage.EntryResult;
import org.apd.storage.SharedDatabase;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/* DO NOT MODIFY THE METHODS SIGNATURES */
public class TaskExecutor {
    private final SharedDatabase sharedDatabase;

    public TaskExecutor(int storageSize, int blockSize, long readDuration, long writeDuration) {
        sharedDatabase = new SharedDatabase(storageSize, blockSize, readDuration, writeDuration);
    }

    public List<EntryResult> ExecuteWork(int numberOfThreads, List<StorageTask> tasks, LockType lockType) {
        ThreadPool threadPool = new ThreadPool(numberOfThreads);
        List<EntryResult> results = Collections.synchronizedList(new ArrayList<>());

        ReaderPreferredLock readerLock = new ReaderPreferredLock();
        WriterPreferredLock1 writerLock1 = new WriterPreferredLock1();
        WriterPreferredLock2 writerLock2 = new WriterPreferredLock2();

        for (StorageTask task : tasks) {
            threadPool.submit(() -> {
                try {
                    switch (lockType) {
                        case ReaderPreferred -> handleReaderPreferred(task, results, readerLock);
                        case WriterPreferred1 -> handleWriterPreferred1(task, results, writerLock1);
                        case WriterPreferred2 -> handleWriterPreferred2(task, results, writerLock2);
                        default -> throw new IllegalArgumentException("LockType necunoscut!");
                    }
                } catch (Exception e) {
                    System.err.println("Eroare la procesarea task-ului: " + task + " - " + e.getMessage());
                }
            });
        }

        threadPool.shutdown();
        threadPool.awaitTermination();

        return results;
    }

    private void handleReaderPreferred(StorageTask task, List<EntryResult> results, ReaderPreferredLock lock) {
        if (task.isWrite()) {
            lock.write(() -> {
                System.out.println("Scriere în ReaderPreferred: " + task);
                EntryResult result = sharedDatabase.addData(task.index(), task.data());
                results.add(result);
                System.out.println("Scriere finalizată: " + result);
            });
        } else {
            lock.read(() -> {
                System.out.println("Citire în ReaderPreferred: " + task);
                EntryResult result = sharedDatabase.getData(task.index());
                results.add(result);
                System.out.println("Citire finalizată: " + result);
            });
        }
    }

    private void handleWriterPreferred1(StorageTask task, List<EntryResult> results, WriterPreferredLock1 lock) {
        if (task.isWrite()) {
            lock.write(() -> {
                System.out.println("Scriere în WriterPreferred1: " + task);
                EntryResult result = sharedDatabase.addData(task.index(), task.data());
                results.add(result);
                System.out.println("Scriere finalizată: " + result);
            });
        } else {
            lock.read(() -> {
                System.out.println("Citire în WriterPreferred1: " + task);
                EntryResult result = sharedDatabase.getData(task.index());
                results.add(result);
                System.out.println("Citire finalizată: " + result);
            });
        }
    }

    private void handleWriterPreferred2(StorageTask task, List<EntryResult> results, WriterPreferredLock2 lock) {
        if (task.isWrite()) {
            lock.write(() -> {
                System.out.println("Scriere în WriterPreferred2: " + task);
                EntryResult result = sharedDatabase.addData(task.index(), task.data());
                results.add(result);
                System.out.println("Scriere finalizată: " + result);
            });
        } else {
            lock.read(() -> {
                System.out.println("Citire în WriterPreferred2: " + task);
                EntryResult result = sharedDatabase.getData(task.index());
                results.add(result);
                System.out.println("Citire finalizată: " + result);
            });
        }
    }

    public List<EntryResult> ExecuteWorkSerial(List<StorageTask> tasks) {
        return tasks.stream().map(task -> {
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
    }
}
