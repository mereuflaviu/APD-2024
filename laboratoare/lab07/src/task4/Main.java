package task4;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ForkJoinPool;
import java.util.concurrent.RecursiveTask;

public class Main {
    static int[][] graph = {
            { 0, 1 }, { 0, 4 }, { 0, 5 }, { 1, 0 }, { 1, 2 }, { 1, 6 },
            { 2, 1 }, { 2, 3 }, { 2, 7 }, { 3, 2 }, { 3, 4 }, { 3, 8 },
            { 4, 0 }, { 4, 3 }, { 4, 9 }, { 5, 0 }, { 5, 7 }, { 5, 8 },
            { 6, 1 }, { 6, 8 }, { 6, 9 }, { 7, 2 }, { 7, 5 }, { 7, 9 },
            { 8, 3 }, { 8, 5 }, { 8, 6 }, { 9, 4 }, { 9, 6 }, { 9, 7 }
    };

    public static void main(String[] args) {
        ArrayList<Integer> partialPath = new ArrayList<>();
        partialPath.add(0); // Start from node 0

        // Create a ForkJoinPool and invoke the root task
        ForkJoinPool forkJoinPool = new ForkJoinPool();
        forkJoinPool.invoke(new PathFindingTask(partialPath, 3)); // Find paths to node 3
        forkJoinPool.shutdown();
    }

    // RecursiveTask for parallel pathfinding
    static class PathFindingTask extends RecursiveTask<Void> {
        private final ArrayList<Integer> partialPath;
        private final int destination;

        public PathFindingTask(ArrayList<Integer> partialPath, int destination) {
            this.partialPath = partialPath;
            this.destination = destination;
        }

        @Override
        protected Void compute() {
            // If the last node in the path is the destination, print the path
            if (partialPath.get(partialPath.size() - 1) == destination) {
                System.out.println(partialPath);
                return null;
            }

            // Get the last node in the current path
            int lastNodeInPath = partialPath.get(partialPath.size() - 1);

            // List to hold tasks for each neighbor
            List<PathFindingTask> tasks = new ArrayList<>();

            // Explore all neighbors of the last node
            for (int[] edge : graph) {
                if (edge[0] == lastNodeInPath) {
                    int nextNode = edge[1];

                    // Avoid cycles by skipping already visited nodes
                    if (partialPath.contains(nextNode)) {
                        continue;
                    }

                    // Create a new path including the next node
                    ArrayList<Integer> newPartialPath = new ArrayList<>(partialPath);
                    newPartialPath.add(nextNode);

                    // Create and fork a new task for the neighbor
                    PathFindingTask task = new PathFindingTask(newPartialPath, destination);
                    tasks.add(task);
                    task.fork();
                }
            }

            // Join all forked tasks
            for (PathFindingTask task : tasks) {
                task.join();
            }

            return null;
        }
    }
}
