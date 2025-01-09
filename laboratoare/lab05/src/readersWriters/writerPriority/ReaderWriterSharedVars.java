package readersWriters.writerPriority;

import java.util.concurrent.Semaphore;

public class ReaderWriterSharedVars {
    volatile int shared_value;

    // Semaphores for controlling access
    public Semaphore readWrite = new Semaphore(1); // Semaphore to control access to the shared resource
    public Semaphore readersMutex = new Semaphore(1); // Mutex for updating reader count
    public Semaphore writersMutex = new Semaphore(1); // Mutex for updating writer count

    // Counters
    public int readers = 0;
    public int writers = 0; // To track if a writer is active or waiting

    ReaderWriterSharedVars(int init_shared_value) {
        this.shared_value = init_shared_value;
    }
}
