#include <mpi.h>
#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>

// ============================================================================
// MACRO-URI SCHELET
// ============================================================================
#define TRACKER_RANK  0
#define MAX_FILES     10
#define MAX_FILENAME  15
#define HASH_SIZE     32
#define MAX_CHUNKS    100

// ============================================================================
// Tag-uri suplimentare (folosite pentru comunicare)
static const int TAG_INIT      = 100;   // client -> tracker, anunt fisiere
static const int TAG_SWARM_REQ = 110;   //  cere swarm
static const int TAG_SEG_REQ   = 120;   //  cere segment
static const int TAG_UPDATE    = 130;   //  trimite update
static const int TAG_DONE      = 140;   //  anunta final

static const int MAX_FILENAME_LEN = 32;
static const int HASH_LENGTH      = 32;

// ============================================================================
// Structuri ajutătoare
// ============================================================================

// informatii segment
struct SegmentInfo {
    bool        owned;          // true = chunk descarcat
    std::string segmentHash;    // hash-ul (32 caractere)
    SegmentInfo() : owned(false), segmentHash("") {}  //init cu valori
};

// Descrie un peer din swarm (cine are segmentele)
struct SwarmPeer {
    int              peerRank;   // rank-ul peer-ului
    std::vector<int> hasSegment; // 1 = are segmentul, 0 = nu
    SwarmPeer() : peerRank(-1) {}
};

// Fisier la un peer: nume, segmente, swarm
struct TorrentFile {
    std::string         fileName;
    int                 totalSegments;
    int                 ownedCount;
    std::vector<SegmentInfo> segments;
    std::vector<SwarmPeer>   swarm;

    TorrentFile() : totalSegments(0), ownedCount(0) {}
};

// Fișiere globale la peer
static int                     g_numOwnedFiles  = 0;
static int                     g_numWantedFiles = 0;
static std::vector<TorrentFile>g_filesOnClient;  // vector cu fișiere (deținute + dorite)

// Mutex dacă vrem sa protejam ceva
static pthread_mutex_t         g_mutexDownload  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t         g_mutexUpload    = PTHREAD_MUTEX_INITIALIZER;

// ============================================================================
// Semnaturi din schelet
// ============================================================================
void *download_thread_func(void *arg);  // thread download
void *upload_thread_func(void *arg);    // thread upload
void tracker(int numtasks, int rank);   
void peer(int numtasks, int rank);      

// ============================================================================
// funcții "helper"
void loadPeerDataFromFile   (int myRank);
void sendClientFilesToTracker(int tag, int myRank, int howMany);
void askTrackerForSwarm     (int myRank);
void requestSegmentFromPeer (int seedRank, int fileIdx, int segIdx, int myRank);
int  pickMissingSegmentIndex(int fileIdx);
int  chooseRandomSeeder     (int fileIdx, int segIdx);
void saveDownloadedFile     (int myRank, int fileIdx);

// Structuri pt tracker
struct GlobalFile {
    std::string               fileName;
    int                       totalSegments;
    std::vector<SegmentInfo>  segments;     // Lista segmentelor 
    std::vector<SwarmPeer>    swarm;        // Lista peers care partajează
    GlobalFile() : totalSegments(0) {}
};

void handleSwarmRequest(int senderRank, const std::string &requestedFile,
                        const std::vector<GlobalFile> &allFiles);
void handleClientUpdate (int senderRank, std::vector<GlobalFile> &allFiles, int tag);

// ============================================================================
//  Thread: download_thread_func
// ============================================================================
void *download_thread_func(void *arg)
{
    int Rank = *(int*)arg;
    MPI_Status status;
    srand(static_cast<unsigned>(time(nullptr)) ^ Rank);

    // 1 - Trimitem fisierele pe care le avem (seed) la tracker
    sendClientFilesToTracker(TAG_INIT, Rank, g_numOwnedFiles);

    // 2 - Astept confirmare
    {
        char ackBuf[8];
        MPI_Recv(ackBuf, 8, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD, &status);
        // Nu ne pasă de conținut
    }

    // 3 - Cere swarm initial
    askTrackerForSwarm(Rank);

    // 4 - Download fisiere dorite
    int fileIndex = g_numOwnedFiles;
    while (fileIndex < (int)g_filesOnClient.size()) {
        int steps = 10; // descarca 10 segmente, apoi update
        while (steps > 0) {
            int missing = pickMissingSegmentIndex(fileIndex);
            if (missing < 0) {
                // fisier complet descarcat => salvare
                saveDownloadedFile(Rank, fileIndex);
                fileIndex++;
                break;
            } else {
                int seed = chooseRandomSeeder(fileIndex, missing);
                if (seed < 0) {
                    // nimeni nu are, imposibil in teorie :))))
                    break;
                }
                requestSegmentFromPeer(seed, fileIndex, missing, Rank); //cere segment lipsa de la peer
                steps--;
            }
        }

        //If all files are downloaded, exit the loop
        if (fileIndex >= (int)g_filesOnClient.size()) {
            break;
        }

        // trimite update la tracker
        {
            std::string upd("update");
            MPI_Send(upd.c_str(), MAX_FILENAME_LEN, MPI_CHAR, 
                     TRACKER_RANK, TAG_UPDATE, MPI_COMM_WORLD);
            //  information about the number of files downloaded so far
            int howMany = fileIndex; 
            if (howMany < 0) howMany = 0;
            sendClientFilesToTracker(TAG_UPDATE, Rank, howMany);
            
            //asteapta confirmare
            char ack2[8];
            MPI_Recv(ack2, 8, MPI_CHAR, TRACKER_RANK, TAG_UPDATE, MPI_COMM_WORLD, &status);
        }
    }

    // 5 - notifica terminarea
    {
        std::string doneMsg("I'm done");
        MPI_Send(doneMsg.c_str(), MAX_FILENAME_LEN, MPI_CHAR, 
                 TRACKER_RANK, TAG_DONE, MPI_COMM_WORLD);
    }

    return nullptr;
}

// ============================================================================
//  Thread: upload_thread_func
// ============================================================================
void *upload_thread_func(void *arg)
{
    int Rank = *(int*)arg;
    MPI_Status status;

    while(true) {
        char requestedName[MAX_FILENAME_LEN];
        memset(requestedName, 0, sizeof(requestedName));    // Initialize the filename buffer
        MPI_Recv(requestedName, MAX_FILENAME_LEN, MPI_CHAR,
                 MPI_ANY_SOURCE, TAG_SEG_REQ, MPI_COMM_WORLD, &status);

        // dacă tracker trimite "TERMINATION" => stop the thread
        if (status.MPI_SOURCE == TRACKER_RANK &&
            strcmp(requestedName, "TERMINATION") == 0) {
            break;
        }

        int srcRank = status.MPI_SOURCE;    // Source rank
        int segIndex = -1;  // Segment index
        MPI_Recv(&segIndex, 1, MPI_INT, srcRank, TAG_SEG_REQ, MPI_COMM_WORLD, &status);

        bool found = false;

        // Loop through all files owned by the peer
        for (size_t i = 0; i < g_filesOnClient.size(); i++) {
            if (g_filesOnClient[i].fileName == requestedName) {
                if (segIndex >= 0 && segIndex < g_filesOnClient[i].totalSegments) {
                     // If the segment index is valid, send its hash
                    const std::string &hval = g_filesOnClient[i].segments[segIndex].segmentHash;
                    MPI_Send(hval.c_str(), HASH_SIZE, MPI_CHAR, srcRank, 777, MPI_COMM_WORLD);
                } else {
                    std::string dummy(HASH_SIZE, '?');
                    MPI_Send(dummy.c_str(), HASH_SIZE, MPI_CHAR, srcRank, 777, MPI_COMM_WORLD);
                }
                found = true;   //file found
                break;
            }
        }
        if (!found) {
            std::string emptyH(HASH_SIZE, 'X');
            MPI_Send(emptyH.c_str(), HASH_SIZE, MPI_CHAR, srcRank, 777, MPI_COMM_WORLD);
        }
    }

    return nullptr;
}

// ============================================================================
//  tracker
// ============================================================================
void tracker(int numtasks, int rank)
{
    MPI_Status status;
    std::vector<GlobalFile> knownFiles;
    knownFiles.reserve(64);

    // 1 - primim init (TAG_INIT) de la toti peers
    for (int c = 1; c < numtasks; c++) {
        handleClientUpdate(c, knownFiles, TAG_INIT);
    }

    // 2) trimitem "ACK" la toti
    for (int c = 1; c < numtasks; c++) {
        char ackBuf[8] = "ACK";
        MPI_Send(ackBuf, 8, MPI_CHAR, c, 0, MPI_COMM_WORLD);
    }

    // 3) aici manageriem cererile swarm, update, done
    int activePeers = numtasks - 1;     // Number of active peers fara tracker
    while (activePeers > 0) {
        char incomingF[MAX_FILENAME_LEN];
        MPI_Recv(incomingF, MAX_FILENAME_LEN, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG,
                 MPI_COMM_WORLD, &status);

        int sender = status.MPI_SOURCE;     //sender rank
        int t      = status.MPI_TAG;        //tag-ul mesajului

        switch(t) {
        case TAG_SWARM_REQ:
            {
                std::string rq(incomingF);
                handleSwarmRequest(sender, rq, knownFiles);
            }
            break;
        case TAG_UPDATE:
            handleClientUpdate(sender, knownFiles, TAG_UPDATE);
            break;
        case TAG_DONE:
            activePeers--;  // Decrease the count of active peers
            break;
        default:
            break;
        }
    }

    // 4) trimitem "TERMINATION" la toti peers
    std::string stopMsg("TERMINATION");
    for (int c = 1; c < numtasks; c++) {
        MPI_Send(stopMsg.c_str(), MAX_FILENAME_LEN, MPI_CHAR, c, TAG_SEG_REQ, MPI_COMM_WORLD);
    }
}

// ============================================================================
//  peer
// ============================================================================
void peer(int numtasks, int rank)
{
    // 1 - citim fisiere (in<rank>.txt)
    loadPeerDataFromFile(rank);

    // 2 - cream thread-urile
    pthread_t download_thread, upload_thread;
    void *status;
    int r;

    r = pthread_create(&download_thread, nullptr, download_thread_func, &rank);
    if (r) {
        std::cerr << "Eroare la crearea thread-ului de download\n";
        std::exit(-1);
    }
    r = pthread_create(&upload_thread, nullptr, upload_thread_func, &rank);
    if (r) {
        std::cerr << "Eroare la crearea thread-ului de upload\n";
        std::exit(-1);
    }

    // 3 - așteptăm finalizarea
    r = pthread_join(download_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_join(upload_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de upload\n");
        exit(-1);
    }
}

// ============================================================================
// MAIN (din schelet)
// ============================================================================
int main (int argc, char *argv[])
{
    int numtasks, rank;
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI nu are suport pentru multi-threading\n");
        exit(-1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == TRACKER_RANK) {
        tracker(numtasks, rank);
    } else {
        peer(numtasks, rank);
    }

    MPI_Finalize();
    return 0;
}

// ============================================================================
// Implementări "helper" 
// ============================================================================
void loadPeerDataFromFile(int myRank) {
    char fname[64];
    sprintf(fname, "in%d.txt", myRank);     // Construim numele fisierului
    FILE* fin = fopen(fname, "r");
    if (!fin) {
        std::cerr << "[Peer" << myRank << "] Eroare la deschidere " << fname << std::endl;
        exit(1);
    }
    // 1 - citim nr fisiere owned
    fscanf(fin, "%d\n", &g_numOwnedFiles);

    // Citim fisierele seed
    for (int i = 0; i < g_numOwnedFiles; i++) {
        TorrentFile tFile;
        char nm[MAX_FILENAME_LEN];
        int segCount;
        fscanf(fin, "%s %d\n", nm, &segCount);  // Citim numele fișierului și numărul de segmente
        tFile.fileName = nm;    // Setăm numele fișierului
        tFile.totalSegments = segCount;
        tFile.ownedCount    = segCount;
        tFile.segments.resize(segCount);    //resize vector

        //segment info
        for (int s = 0; s < segCount; s++) {
            char buff[HASH_LENGTH+1];   // Buffer pentru hash-ul segmentului
            memset(buff, 0, sizeof(buff));
            fread(buff, 1, HASH_LENGTH, fin);
            fgetc(fin); // elimin newline \n
            tFile.segments[s].owned       = true;
            tFile.segments[s].segmentHash = buff;
        }
        g_filesOnClient.push_back(tFile);
    }

    // 2 - nr fișiere dorite
    fscanf(fin, "%d", &g_numWantedFiles);

    // Extindem vectorul g_filesOnClient
    for (int i = 0; i < g_numWantedFiles; i++) {
        TorrentFile tFile;
        char nm[MAX_FILENAME_LEN];
        fscanf(fin, "%s", nm);
        tFile.fileName = nm;
        tFile.totalSegments = 0;    // Init nr total de segmente la 0
        tFile.ownedCount    = 0;
        g_filesOnClient.push_back(tFile);
    }
    fclose(fin);
}

void sendClientFilesToTracker(int tag, int myRank, int howMany) {
    // Trimite numărul de fișiere deținute complet de acest peer către tracker
    MPI_Send(&howMany, 1, MPI_INT, TRACKER_RANK, tag, MPI_COMM_WORLD);

    // Pentru fiecare fișier deținut:
    for (int i = 0; i < howMany; i++) {
        char buf[MAX_FILENAME_LEN]; // Buffer pentru numele fișierului
        memset(buf, 0, sizeof(buf)); // Inițializează buffer-ul
        strcpy(buf, g_filesOnClient[i].fileName.c_str()); // Copiază numele fișierului în buffer

        // Trimite numele fișierului către tracker
        MPI_Send(buf, MAX_FILENAME_LEN, MPI_CHAR, TRACKER_RANK, tag, MPI_COMM_WORLD);

        int segCount = g_filesOnClient[i].ownedCount; // Numărul de segmente deținute
        // Trimite numărul de segmente ale fișierului către tracker
        MPI_Send(&segCount, 1, MPI_INT, TRACKER_RANK, tag, MPI_COMM_WORLD);

        // Pentru fiecare segment al fișierului:
        for (int s = 0; s < segCount; s++) {
            const std::string &h = g_filesOnClient[i].segments[s].segmentHash; // Hash-ul segmentului
            // Trimite hash-ul segmentului către tracker
            MPI_Send(h.c_str(), HASH_SIZE, MPI_CHAR, TRACKER_RANK, tag, MPI_COMM_WORLD);
        }
    }
}

void askTrackerForSwarm(int myRank) {
    MPI_Status status;

    // Iterăm prin fișierele dorite (începând de la cele care nu sunt încă deținute)
    for (size_t idx = g_numOwnedFiles; idx < g_filesOnClient.size(); idx++) {
        // Trimite cerere pentru swarm-ul fișierului către tracker
        MPI_Send(g_filesOnClient[idx].fileName.c_str(), MAX_FILENAME_LEN, MPI_CHAR,
                 TRACKER_RANK, TAG_SWARM_REQ, MPI_COMM_WORLD);

        int swarmSize = 0, segCount = 0;
        // Primește dimensiunea swarm-ului și numărul de segmente ale fișierului
        MPI_Recv(&swarmSize, 1, MPI_INT, TRACKER_RANK, TAG_SWARM_REQ, MPI_COMM_WORLD, &status);
        MPI_Recv(&segCount, 1, MPI_INT, TRACKER_RANK, TAG_SWARM_REQ, MPI_COMM_WORLD, &status);

        // Actualizează informațiile despre fișier: numărul total de segmente
        g_filesOnClient[idx].totalSegments = segCount;
        g_filesOnClient[idx].segments.resize(segCount);

        // Inițializează segmentele ca notOwned și hash-urile empty
        for (int s = 0; s < segCount; s++) {
            g_filesOnClient[idx].segments[s].owned = false;
            g_filesOnClient[idx].segments[s].segmentHash = "";
        }
        g_filesOnClient[idx].ownedCount = 0; 

        // Actualizează informațiile despre swarm
        g_filesOnClient[idx].swarm.resize(swarmSize);
        for (int p = 0; p < swarmSize; p++) {
            int pid = -1;
            // Primește rank-ul peer-ului din swarm
            MPI_Recv(&pid, 1, MPI_INT, TRACKER_RANK, TAG_SWARM_REQ, MPI_COMM_WORLD, &status);
            g_filesOnClient[idx].swarm[p].peerRank = pid;

            // Primește informațiile despre segmentele deținute de peer
            g_filesOnClient[idx].swarm[p].hasSegment.resize(segCount);
            MPI_Recv(g_filesOnClient[idx].swarm[p].hasSegment.data(), 
                     segCount, MPI_INT, TRACKER_RANK, TAG_SWARM_REQ, MPI_COMM_WORLD, &status);
        }
    }
}


void requestSegmentFromPeer(int seedRank, int fileIdx, int segIdx, int myRank) {
    MPI_Status status;
    // 1) trimitem nume
    MPI_Send(g_filesOnClient[fileIdx].fileName.c_str(), MAX_FILENAME_LEN, MPI_CHAR,
             seedRank, TAG_SEG_REQ, MPI_COMM_WORLD);
    // 2) Trimite indexul segmentului cerut 
    MPI_Send(&segIdx, 1, MPI_INT, seedRank, TAG_SEG_REQ, MPI_COMM_WORLD);

    // 3) primim hash de la peer
    char recvHash[HASH_SIZE + 2];
    memset(recvHash, 0, sizeof(recvHash));
    MPI_Recv(recvHash, HASH_SIZE, MPI_CHAR, seedRank, 777, MPI_COMM_WORLD, &status);

    // Actualizează informațiile despre segment: marcat ca descărcat și salvează hash-ul
    g_filesOnClient[fileIdx].segments[segIdx].owned = true;
    g_filesOnClient[fileIdx].segments[segIdx].segmentHash = recvHash;
    g_filesOnClient[fileIdx].ownedCount++;  // Crește numărul de segmente deținute
}

int pickMissingSegmentIndex(int fileIdx) {
    // Iterăm prin toate segmentele fișierului
    for (int i = 0; i < g_filesOnClient[fileIdx].totalSegments; i++) {
        // Dacă segmentul nu este deținut, returnăm indexul acestuia
        if (!g_filesOnClient[fileIdx].segments[i].owned) {
            return i;  // Primul segment lipsă găsit
        }
    }
    
    return -1;
}


int chooseRandomSeeder(int fileIdx, int segIdx) {
    std::vector<int> seeds;
    // Iterăm prin toți peers din swarm-ul fișierului specificat
    for (auto &p : g_filesOnClient[fileIdx].swarm) {
        // Dacă peer-ul are segmentul specificat, adăugăm rank-ul său în lista seeds
        if (p.hasSegment[segIdx] == 1) {
            seeds.push_back(p.peerRank);
        }
    }
    // Dacă niciun peer nu are segmentul, returnăm -1
    if (seeds.empty()) return -1;

    // Alegem un peer aleatoriu din lista seeds
    int pos = rand() % seeds.size();
    return seeds[pos];
}


void saveDownloadedFile(int myRank, int fileIdx) {
    // Construim numele fișierului de ieșire 
    char outName[64];
    sprintf(outName, "client%d_%s", myRank, g_filesOnClient[fileIdx].fileName.c_str());

    
    FILE* fout = fopen(outName, "w");
    if (!fout) {
        std::cerr << "Eroare la salvare " << outName << std::endl;
        return;
    }

    // Scriem hash-urile segmentelor în fișier, unul pe linie
    for (int i = 0; i < g_filesOnClient[fileIdx].totalSegments; i++) {
        fwrite(g_filesOnClient[fileIdx].segments[i].segmentHash.c_str(), 1, HASH_SIZE, fout);
        fputc('\n', fout);  // Adăugăm o nouă linie după fiecare hash
    }

    fclose(fout);
}


// ============================================================================
// handleSwarmRequest, handleClientUpdate (tracker logic)
// ============================================================================
void handleSwarmRequest(int senderRank, const std::string &requestedFile,
                        const std::vector<GlobalFile> &allFiles)
{
    // Iterăm prin lista de fișiere cunoscute pentru a găsi fișierul cerut de client
    for (auto &f : allFiles) {
        if (f.fileName == requestedFile) {  
            int swarmCount = (int)f.swarm.size();   
            int segCount   = f.totalSegments;      
            
            // Trimitem numărul de peers și numărul de segmente clientului
            MPI_Send(&swarmCount, 1, MPI_INT, senderRank, TAG_SWARM_REQ, MPI_COMM_WORLD);
            MPI_Send(&segCount,   1, MPI_INT, senderRank, TAG_SWARM_REQ, MPI_COMM_WORLD);

            // Trimitem detalii despre fiecare peer din swarm
            for (int s = 0; s < swarmCount; s++) {
                int pid = f.swarm[s].peerRank;  
                MPI_Send(&pid, 1, MPI_INT, senderRank, TAG_SWARM_REQ, MPI_COMM_WORLD);
                // Trimitem vectorul care indică ce segmente deține peer-ul
                MPI_Send(f.swarm[s].hasSegment.data(), segCount, MPI_INT, 
                         senderRank, TAG_SWARM_REQ, MPI_COMM_WORLD);
            }
            return;  
        }
    }
    // Dacă fișierul nu este găsit în lista de fișiere cunoscute
    int zero = 0;  // Semnalizăm absența fișierului
    MPI_Send(&zero, 1, MPI_INT, senderRank, TAG_SWARM_REQ, MPI_COMM_WORLD);
    MPI_Send(&zero, 1, MPI_INT, senderRank, TAG_SWARM_REQ, MPI_COMM_WORLD);
}


void handleClientUpdate(int senderRank, std::vector<GlobalFile> &allFiles, int tag)
{
    MPI_Status status;
    int howMany = 0;

    // Primește numărul de fișiere raportate de client
    MPI_Recv(&howMany, 1, MPI_INT, senderRank, tag, MPI_COMM_WORLD, &status);

    while (howMany--) {
        char fBuf[MAX_FILENAME_LEN];
        memset(fBuf, 0, sizeof(fBuf)); // Init buffer

        // Primește numele fișierului de la client
        MPI_Recv(fBuf, MAX_FILENAME_LEN, MPI_CHAR, senderRank, tag, MPI_COMM_WORLD, &status);

        int segCount = 0;

        // Primește numărul de segmente din fișier
        MPI_Recv(&segCount, 1, MPI_INT, senderRank, tag, MPI_COMM_WORLD, &status);

        bool found = false;

        // Verifică dacă fișierul există deja în lista `allFiles`
        for (size_t i = 0; i < allFiles.size(); i++) {
            if (allFiles[i].fileName == fBuf) { // Fișierul este deja cunoscut
                int swarmPos = -1;

                // Verifică dacă clientul este deja parte din swarm-ul acestui fișier
                for (size_t s = 0; s < allFiles[i].swarm.size(); s++) {
                    if (allFiles[i].swarm[s].peerRank == senderRank) {
                        swarmPos = (int)s;
                        break;
                    }
                }

                // Dacă clientul nu este în swarm il adauga
                if (swarmPos < 0) {
                    SwarmPeer sp;
                    sp.peerRank = senderRank;
                    sp.hasSegment.resize(allFiles[i].totalSegments, 0); 
                    allFiles[i].swarm.push_back(sp);
                    swarmPos = (int)allFiles[i].swarm.size() - 1;
                }

                // Actualizează segmentele deținute de client
                for (int seg = 0; seg < segCount; seg++) {
                    char tmp[HASH_SIZE + 1];
                    memset(tmp, 0, sizeof(tmp));

                    // Primește hash-ul segmentului
                    MPI_Recv(tmp, HASH_SIZE, MPI_CHAR, senderRank, tag, MPI_COMM_WORLD, &status);

                    // Marchează segmentul ca deținut de client
                    if (seg < (int)allFiles[i].swarm[swarmPos].hasSegment.size()) {
                        allFiles[i].swarm[swarmPos].hasSegment[seg] = 1;
                    }
                }

                found = true;
                break; 
            }
        }

        // Dacă fișierul nu este găsit, il adauga ca nou
        if (!found) {
            GlobalFile gf;
            gf.fileName      = fBuf;     // Numele fișierului
            gf.totalSegments = segCount; // Numărul total de segmente
            gf.segments.resize(segCount); // Redimensionează vectorul de segmente

            SwarmPeer sp;
            sp.peerRank = senderRank;
            sp.hasSegment.resize(segCount, 0); // Inițializează vectorul de segmente

            // Primește hash-urile segmentelor și marchează-le ca deținute
            for (int seg = 0; seg < segCount; seg++) {
                char tmp[HASH_SIZE + 1];
                memset(tmp, 0, sizeof(tmp));
                MPI_Recv(tmp, HASH_SIZE, MPI_CHAR, senderRank, tag, MPI_COMM_WORLD, &status);
                sp.hasSegment[seg]      = 1;
                gf.segments[seg].owned  = true;
                gf.segments[seg].segmentHash = tmp; // Salvează hash-ul segmentului
            }

            // Adaugă clientul la swarm-ul fișierului
            gf.swarm.push_back(sp);

            // Adaugă fișierul în lista globală
            allFiles.push_back(gf);
        }
    }

    // Trimite un "ACK" clientului pentru confirmare
    {
        char ackBuf[8] = "ACK";
        MPI_Send(ackBuf, 8, MPI_CHAR, senderRank, tag, MPI_COMM_WORLD);
    }
}

