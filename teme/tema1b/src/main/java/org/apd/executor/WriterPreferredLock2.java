package org.apd.executor;

public class WriterPreferredLock2 {
    private int readers = 0;
    private int writers = 0; // Numărul de scriitori activi
    private int waitingWriters = 0; // Numărul de scriitori în așteptare

    public synchronized void read(Runnable task) {
        try {
            // Așteaptă dacă există scriitori activi sau în așteptare
            while (writers > 0 || waitingWriters > 0) {
                wait();
            }
            readers++; // Un cititor a intrat
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        try {
            task.run(); // Execută sarcina de citire
        } finally {
            synchronized (this) {
                readers--;
                if (readers == 0) {
                    notifyAll(); // Notifică scriitorii în așteptare
                }
            }
        }
    }

    public synchronized void write(Runnable task) {
        try {
            waitingWriters++; // Crește numărul de scriitori în așteptare
            // Așteaptă dacă există cititori sau un alt scriitor activ
            while (readers > 0 || writers > 0) {
                wait();
            }
            waitingWriters--;
            writers++; // Scriitorul a intrat
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        try {
            task.run(); // Execută sarcina de scriere
        } finally {
            synchronized (this) {
                writers--;
                notifyAll(); // Notifică cititorii și scriitorii în așteptare
            }
        }
    }
}