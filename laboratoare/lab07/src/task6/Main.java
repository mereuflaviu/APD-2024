package task6;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ForkJoinPool;
import java.util.concurrent.RecursiveTask;

public class Main {
    public static int N = 4; // Number of queens and size of the chessboard

    public static void main(String[] args) {
        int[] graph = new int[N]; // Array to represent the board state
        ForkJoinPool forkJoinPool = new ForkJoinPool(); // Create ForkJoinPool

        // Invoke the main task for the N-Queens problem
        forkJoinPool.invoke(new NQueensTask(graph, 0));
        forkJoinPool.shutdown();
    }

    // RecursiveTask for parallelizing the N-Queens problem
    static class NQueensTask extends RecursiveTask<Void> {
        private final int[] graph; // Array to hold the positions of the queens
        private final int step;   // Current row being processed

        public NQueensTask(int[] graph, int step) {
            this.graph = graph.clone(); // Clone the graph to avoid conflicts
            this.step = step;
        }

        @Override
        protected Void compute() {
            // Base case: all queens are placed
            if (step == N) {
                printQueens(graph);
                return null;
            }

            // List to hold subtasks for parallel computation
            List<NQueensTask> tasks = new ArrayList<>();

            // Try placing a queen in each column of the current row
            for (int i = 0; i < N; ++i) {
                int[] newGraph = graph.clone(); // Clone the board state for the new task
                newGraph[step] = i; // Place queen in column i of the current row

                if (isSafe(newGraph, step)) {
                    // Create a new task for the next row
                    NQueensTask task = new NQueensTask(newGraph, step + 1);
                    tasks.add(task); // Add task to the list
                    task.fork(); // Fork the task for parallel execution
                }
            }

            // Wait for all tasks to complete
            for (NQueensTask task : tasks) {
                task.join();
            }

            return null;
        }

        // Check if placing a queen at the current step is safe
        private boolean isSafe(int[] board, int step) {
            for (int i = 0; i < step; i++) {
                // Check if two queens are in the same column or on the same diagonal
                if (board[i] == board[step] ||
                        Math.abs(board[i] - board[step]) == Math.abs(i - step)) {
                    return false;
                }
            }
            return true;
        }

        // Print the board configuration
        private void printQueens(int[] solution) {
            StringBuilder output = new StringBuilder();
            for (int i = 0; i < solution.length; i++) {
                output.append("(").append(solution[i] + 1).append(", ").append(i + 1).append("), ");
            }
            // Remove the trailing comma and space
            output = new StringBuilder(output.substring(0, output.length() - 2));
            System.out.println("[" + output + "]");
        }
    }
}
