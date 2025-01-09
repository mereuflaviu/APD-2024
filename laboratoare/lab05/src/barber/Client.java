package barber;

public class Client extends Thread {
    private final int id;

    public Client(int id) {
        super();
        this.id = id;
    }

    @Override
    public void run() {
        try {
            // Try to find a free chair in the waiting area
            Main.chairsMutex.acquire();
            if (Main.freeChairs > 0) {
                // Occupy a chair and wait for the barber
                Main.freeChairs--;
                System.out.println("Client " + id + " is waiting for haircut. Available seats: " + Main.freeChairs);

                // Notify barber that a client is waiting
                Main.clients.release();
                Main.chairsMutex.release();

                // Wait for the barber to be ready
                Main.barberReady.acquire();

                // Client is served by the barber
                System.out.println("Client " + id + " is served by the barber");
                Main.leftClients[id] = Main.SERVED_CLIENT;

            } else {
                // No chairs available, client leaves
                Main.chairsMutex.release();
                System.out.println("Client " + id + " left unserved");
                Main.leftClients[id] = Main.UNSERVED_CLIENT;
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}
