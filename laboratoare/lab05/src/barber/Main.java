package barber;

import java.util.concurrent.Semaphore;

public class Main {
    public final static int TOTAL_CHAIRS = 3;
    public final static int TOTAL_CLIENTS = 7;

    // Client served and unserved statuses
    public final static int SERVED_CLIENT = 1;
    public final static int UNSERVED_CLIENT = 2;

    public static int[] leftClients = new int[TOTAL_CLIENTS];

    // Semaphores to manage barber and client synchronization
    public static Semaphore clients = new Semaphore(0);         // clients waiting to be served
    public static Semaphore barberReady = new Semaphore(0);     // barber ready to serve a client
    public static Semaphore chairsMutex = new Semaphore(1);     // mutex for accessing chairs

    public static int freeChairs = TOTAL_CHAIRS;

    public static void main(String[] args) throws InterruptedException {
        Thread barberThread = new Barber();
        Thread[] clientThreads = new Client[TOTAL_CLIENTS];

        for (int i = 0; i < TOTAL_CLIENTS; i++) {
            clientThreads[i] = new Client(i);
        }

        barberThread.start();
        for (Thread clientThread : clientThreads) {
            clientThread.start();
            Thread.sleep(100); // slight delay between client arrivals
        }

        barberThread.join();
        for (Thread clientThread : clientThreads) {
            clientThread.join();
        }

        int unservedClients = 0;
        for (int client : leftClients) {
            if (client == UNSERVED_CLIENT) {
                unservedClients++;
            }
        }

        System.out.println("There were " + unservedClients + " unserved clients");
    }
}
