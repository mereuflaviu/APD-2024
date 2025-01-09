package readersWriters.writerPriority;

public class Writer extends Thread {
    private final int id;
    private final int number_of_writes;
    private final long wait_time_ms;
    private final ReaderWriterSharedVars shared_vars;
    private double start_time;
    private double completion_time;

    public Writer(int id, int number_of_writes, long wait_time_ms, ReaderWriterSharedVars shared_vars) {
        this.id = id;
        this.number_of_writes = number_of_writes;
        this.wait_time_ms = wait_time_ms;
        this.shared_vars = shared_vars;
    }

    @Override
    public void run() {
        this.start_time = System.currentTimeMillis() / 1000.0;

        for (int i = 0; i < number_of_writes; i++) {
            try {
                // Start writer access synchronization
                shared_vars.writersMutex.acquire();
                if (shared_vars.writers == 0) {
                    shared_vars.readWrite.acquire(); // First writer locks the resource
                }
                shared_vars.writers++;
                shared_vars.writersMutex.release();

                // Perform writing
                write();

                // End writer access synchronization
                shared_vars.writersMutex.acquire();
                shared_vars.writers--;
                if (shared_vars.writers == 0) {
                    shared_vars.readWrite.release(); // Last writer releases the resource
                }
                shared_vars.writersMutex.release();

            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        }

        this.completion_time = System.currentTimeMillis() / 1000.0 - this.start_time;
        System.out.println("Writer with ID = " + this.id + " ended after " + completion_time + "s");
    }

    public double getCompletion_time() {
        return completion_time;
    }

    public void write() {
        int new_value = shared_vars.shared_value + 3;
        System.out.println(Utils.get_current_time_str() + " | Writer " + this.id + " writes " + shared_vars.shared_value);
        shared_vars.shared_value = new_value;

        try {
            sleep(wait_time_ms);
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
    }
}
