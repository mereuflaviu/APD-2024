package task2;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicInteger;

public class Main {
    static final int N = 10; // Number of nodes
    static final int COLORS = 3; // Number of colors
    static final int[][] graph = {
            { 0, 1 }, { 0, 4 }, { 0, 5 }, { 1, 0 }, { 1, 2 }, { 1, 6 },
            { 2, 1 }, { 2, 3 }, { 2, 7 }, { 3, 2 }, { 3, 4 }, { 3, 8 },
            { 4, 0 }, { 4, 3 }, { 4, 9 }, { 5, 0 }, { 5, 7 }, { 5, 8 },
            { 6, 1 }, { 6, 8 }, { 6, 9 }, { 7, 2 }, { 7, 5 }, { 7, 9 },
            { 8, 3 }, { 8, 5 }, { 8, 6 }, { 9, 4 }, { 9, 6 }, { 9, 7 }
    };

    public static void main(String[] args) {
        int[] colors = new int[N]; // Array to store the colors of the nodes
        AtomicInteger inQueue = new AtomicInteger(0); // Counter to track active tasks
        ExecutorService executor = Executors.newFixedThreadPool(4); // Thread pool

        // Submit the initial task
        inQueue.incrementAndGet();
        executor.submit(new ColoringTask(colors, executor, inQueue, 0));

        // Shutdown the executor after all tasks are completed
        executor.shutdown();
    }

    static class ColoringTask implements Runnable {
        private final ExecutorService executor;
        private final AtomicInteger inQueue;
        private final int[] colors;
        private final int step;

        public ColoringTask(int[] colors, ExecutorService executor, AtomicInteger inQueue, int step) {
            this.executor = executor;
            this.inQueue = inQueue;
            this.colors = colors.clone(); // Clone the array to avoid conflicts
            this.step = step;
        }

        @Override
        public void run() {
            // If we reached the last node, print the valid coloring
            if (step == N) {
                printColors(colors);
                int left = inQueue.decrementAndGet();
                if (left == 0) {
                    executor.shutdown(); // Shut down the executor if all tasks are completed
                }
                return;
            }

            // Try all possible colors for the current node
            for (int i = 0; i < COLORS; i++) {
                colors[step] = i; // Assign color i to the current node
                if (verifyColors(colors, step)) {
                    inQueue.incrementAndGet();
                    executor.submit(new ColoringTask(colors, executor, inQueue, step + 1));
                }
            }

            // Decrement the counter when this task is done
            int left = inQueue.decrementAndGet();
            if (left == 0) {
                executor.shutdown();
            }
        }

        // Verify if the current coloring is valid
        private boolean verifyColors(int[] colors, int step) {
            for (int i = 0; i < step; i++) {
                if (colors[i] == colors[step] && isEdge(i, step)) {
                    return false;
                }
            }
            return true;
        }

        // Check if there is an edge between two nodes
        private boolean isEdge(int a, int b) {
            for (int[] edge : graph) {
                if (edge[0] == a && edge[1] == b) {
                    return true;
                }
            }
            return false;
        }

        // Print the coloring of the graph
        private void printColors(int[] colors) {
            StringBuilder result = new StringBuilder();
            for (int color : colors) {
                result.append(color).append(" ");
            }
            System.out.println(result);
        }
    }
}
