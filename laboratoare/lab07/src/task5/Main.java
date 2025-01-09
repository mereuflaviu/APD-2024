package task5;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ForkJoinPool;
import java.util.concurrent.RecursiveTask;

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
        int[] colors = new int[N]; // Array to hold colors for each node
        ForkJoinPool forkJoinPool = new ForkJoinPool(); // Create ForkJoinPool

        // Submit the root task
        forkJoinPool.invoke(new ColoringTask(colors, 0));
        forkJoinPool.shutdown();
    }

    // RecursiveTask for parallel graph coloring
    static class ColoringTask extends RecursiveTask<Void> {
        private final int[] colors;
        private final int step;

        public ColoringTask(int[] colors, int step) {
            this.colors = colors.clone(); // Clone the colors array to avoid conflicts
            this.step = step;
        }

        @Override
        protected Void compute() {
            // If all nodes are colored, print the valid coloring
            if (step == N) {
                printColors(colors);
                return null;
            }

            // List to store child tasks
            List<ColoringTask> tasks = new ArrayList<>();

            // Try all possible colors for the current node
            for (int i = 0; i < COLORS; i++) {
                colors[step] = i; // Assign color i to the current node
                if (isValid(colors, step)) {
                    // Create a new task for the next node and fork it
                    ColoringTask task = new ColoringTask(colors, step + 1);
                    tasks.add(task);
                    task.fork();
                }
            }

            // Join all the forked tasks
            for (ColoringTask task : tasks) {
                task.join();
            }

            return null;
        }

        // Check if the current coloring is valid
        private boolean isValid(int[] colors, int step) {
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

        // Print the colors of the graph
        private void printColors(int[] colors) {
            StringBuilder result = new StringBuilder();
            for (int color : colors) {
                result.append(color).append(" ");
            }
            System.out.println(result);
        }
    }
}
