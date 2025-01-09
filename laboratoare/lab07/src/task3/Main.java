package task3;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicInteger;

public class Main {
    public static int N = 4; // Size of the chessboard (N x N)

    public static void main(String[] args) {
        int[] graph = new int[N]; // Array to represent the positions of queens
        AtomicInteger inQueue = new AtomicInteger(0); // Counter for active tasks
        ExecutorService executor = Executors.newFixedThreadPool(4); // Thread pool with 4 threads

        // Start the initial task
        inQueue.incrementAndGet();
        executor.submit(new NQueensTask(graph, executor, inQueue, 0));

        // Shut down the executor when all tasks are done
        executor.shutdown();
    }

    static class NQueensTask implements Runnable {
        private final int[] graph;
        private final ExecutorService executor;
        private final AtomicInteger inQueue;
        private final int step;

        public NQueensTask(int[] graph, ExecutorService executor, AtomicInteger inQueue, int step) {
            this.graph = graph.clone(); // Clone to avoid modifying shared data
            this.executor = executor;
            this.inQueue = inQueue;
            this.step = step;
        }

        @Override
        public void run() {
            // If we placed queens in all rows, print the solution
            if (step == N) {
                printQueens(graph);
                int left = inQueue.decrementAndGet();
                if (left == 0) {
                    executor.shutdown(); // Shut down if all tasks are complete
                }
                return;
            }

            // Try placing a queen in every column of the current row
            for (int i = 0; i < N; i++) {
                graph[step] = i; // Place queen at column `i` of row `step`
                if (isSafe(graph, step)) {
                    inQueue.incrementAndGet(); // Increment task counter
                    executor.submit(new NQueensTask(graph, executor, inQueue, step + 1));
                }
            }

            // Decrement the task counter when the task is finished
            int left = inQueue.decrementAndGet();
            if (left == 0) {
                executor.shutdown();
            }
        }

        // Check if the current position of queens is valid
        private boolean isSafe(int[] graph, int step) {
            for (int i = 0; i < step; i++) {
                if (graph[i] == graph[step] || Math.abs(graph[i] - graph[step]) == Math.abs(i - step)) {
                    return false;
                }
            }
            return true;
        }

        // Print the positions of the queens
        private void printQueens(int[] sol) {
            StringBuilder result = new StringBuilder();
            for (int i = 0; i < sol.length; i++) {
                result.append("(").append(sol[i] + 1).append(", ").append(i + 1).append("), ");
            }
            result = new StringBuilder(result.substring(0, result.length() - 2)); // Remove last comma
            System.out.println("[" + result + "]");
        }
    }
}
