package task1;

import java.util.ArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class Main {
    static int[][] graph = {
            {0, 1}, {0, 4}, {0, 5}, {1, 0}, {1, 2}, {1, 6},
            {2, 1}, {2, 3}, {2, 7}, {3, 2}, {3, 4}, {3, 8},
            {4, 0}, {4, 3}, {4, 9}, {5, 0}, {5, 7}, {5, 8},
            {6, 1}, {6, 8}, {6, 9}, {7, 2}, {7, 5}, {7, 9},
            {8, 3}, {8, 5}, {8, 6}, {9, 4}, {9, 6}, {9, 7}
    };

    public static void main(String[] args) {
        ArrayList<Integer> partialPath = new ArrayList<>();
        ExecutorService executor = Executors.newFixedThreadPool(4);

        // Start path from node 0 and find paths to node 3
        partialPath.add(0);
        executor.submit(new RunnableTask(executor, partialPath, 3));

        executor.shutdown();
    }

    static class RunnableTask implements Runnable {
        private final ExecutorService executor;
        private final ArrayList<Integer> path;
        private final int destination;

        public RunnableTask(ExecutorService executor, ArrayList<Integer> path, int destination) {
            this.executor = executor;
            this.path = path;
            this.destination = destination;
        }

        @Override
        public void run() {
            // If the last node in the path is the destination, print the path
            if (path.get(path.size() - 1) == destination) {
                System.out.println(path);
                return;
            }

            // Get the last node in the current path
            int lastNode = path.get(path.size() - 1);

            // Explore all neighbors of the last node
            for (int[] edge : Main.graph) {
                if (edge[0] == lastNode) {
                    int nextNode = edge[1];

                    // Avoid cycles by skipping already visited nodes
                    if (path.contains(nextNode)) continue;

                    // Create a new path including the next node
                    ArrayList<Integer> newPath = new ArrayList<>(path);
                    newPath.add(nextNode);

                    // Submit the new task to the executor
                    executor.submit(new RunnableTask(executor, newPath, destination));
                }
            }
        }
    }
}
