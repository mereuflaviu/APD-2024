package org.apd.executor;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

public class ThreadPool {
    private final BlockingQueue<Runnable> taskQueue;
    private final Worker[] workers;
    private volatile boolean isShutdown = false;

    public ThreadPool(int numThreads) {
        taskQueue = new LinkedBlockingQueue<>();
        workers = new Worker[numThreads];

        for (int i = 0; i < numThreads; i++) {
            workers[i] = new Worker();
            new Thread(workers[i], "Thread-" + i).start();
        }
    }

    public void submit(Runnable task) {
        if (!isShutdown) {
            taskQueue.offer(task);
        }
    }

    public void shutdown() {
        isShutdown = true;
    }

    public void awaitTermination() {
        for (Worker worker : workers) {
            worker.stop();
        }
    }

    private class Worker implements Runnable {
        private volatile boolean running = true;

        @Override
        public void run() {
            while (running || !taskQueue.isEmpty()) {
                try {
                    Runnable task = taskQueue.poll();
                    if (task != null) {
                        task.run();
                    }
                } catch (Exception e) {
                    System.err.println("Eroare în worker: " + e.getMessage());
                }
            }
        }

        public void stop() {
            running = false;
        }
    }
}
