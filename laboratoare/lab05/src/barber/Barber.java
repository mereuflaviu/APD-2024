package barber;

public class Barber extends Thread {
    @Override
    public void run() {
        int servedClients = 0;

        while (true) {
            try {
                // Wait for a client to arrive
                Main.clients.acquire();

                // Acquire lock to update the number of free chairs
                Main.chairsMutex.acquire();
                Main.freeChairs++; // A client is leaving the waiting area to be served
                Main.chairsMutex.release();

                // Signal that the barber is ready to cut hair
                Main.barberReady.release();

                // Simulate haircut duration
                System.out.println("Barber is cutting hair...");
                Thread.sleep(100);

                System.out.println("Barber served client");
                servedClients++;

            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}
