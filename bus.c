/**
 * 2. PROJEKT DO PREDMETU IOS
 * --------------------------
 * AUTOR: Jakub Sadilek
 * LOGIN: xsadil07
 * DNE: 1.5.2018
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define LOCKED 0
#define UNLOCKED 1
#define DETECTED 1
#define SEM_NAME1 "/xsadil07_semaphore1"
#define SEM_NAME2 "/xsadil07_semaphore2"
#define SEM_NAME3 "/xsadil07_semaphore3"
#define SEM_NAME4 "/xsadil07_semaphore4"
#define SEM_NAME5 "/xsadil07_semaphore5"

/**Struktura pro argumenty.*/
typedef struct {
    int R;
    int C;
    int ART;
    int ABT;
} args;

/**Inicializace semaforu.*/
sem_t *sem_station = NULL;
sem_t *sem_boarding = NULL;
sem_t *sem_writing = NULL;
sem_t *sem_all_in = NULL;
sem_t *sem_leaving = NULL;

/**Inicializace sdilenych promenych.*/
int *action = NULL;
int *waiting = NULL;
int *boarded = NULL;
int *removed = NULL;
int *riders = NULL;
int *error = NULL;
pid_t *pids = NULL;

/**Ukazatel na soubor pro zapis logu.*/
FILE *log_file;

/**Inicializace ID pro sdilene promene.*/
int shm_action = 0;
int shm_waiting = 0;
int shm_boarded = 0;
int shm_removed = 0;
int shm_riders = 0;
int shm_error = 0;
pid_t shm_pids = 0;

/**Kontrola argumentu, funkce vrati strukturu s argumenty.*/
args check_args(int argc, char **argv){
    args arguments;
    if (argc != 5){ //Kontrola chybneho poctu zadanych argumentu.
        fprintf(stderr, "Spatny pocet zadanych argumentu.\n");
        exit(1);
    }

    char *erra, *errb, *errc, *errd;
    //Prirazeni argumentu do promnenych.
    int r = strtol(argv[1], &erra, 10);
    int c = strtol(argv[2], &errb, 10);
    int art = strtol(argv[3], &errc, 10);
    int abt = strtol(argv[4], &errd, 10);
    //Kontrola povoleneho rozsahu a spravnosti.
    if (r <= 0 || c <= 0 || art < 0 || art > 1000 || abt < 0 || abt > 1000 || *erra != '\0' || *errb != '\0' || *errc != '\0' || *errd != '\0'){
        fprintf(stderr, "Spatne zadane hodnoty argumentu.\n");
        exit(1);
    }
    //Prirazeni promnenych do struktury.
    arguments.R = r;
    arguments.C = c;
    arguments.ART = art;
    arguments.ABT = abt;
    //Vratime strukturu s argumenty.
    return arguments;
}

/**Funkce zavre vsechny semafory. Patri k funkci clean_all*/
void unlink_semaphores(void){
    sem_close(sem_station);
    sem_close(sem_boarding);
    sem_close(sem_writing);
    sem_close(sem_all_in);
    sem_close(sem_leaving);

    sem_unlink(SEM_NAME1);
    sem_unlink(SEM_NAME2);
    sem_unlink(SEM_NAME3);
    sem_unlink(SEM_NAME4);
    sem_unlink(SEM_NAME5);
}

/**Funkce zavre semafory, soubor a sdilenou pamet.*/
void clean_all (void){
    unlink_semaphores();
    fclose(log_file);
    shmctl(shm_action, IPC_RMID, NULL);
    shmctl(shm_waiting, IPC_RMID, NULL);
    shmctl(shm_boarded, IPC_RMID, NULL);
    shmctl(shm_removed, IPC_RMID, NULL);
    shmctl(shm_riders, IPC_RMID, NULL);
    shmctl(shm_error, IPC_RMID, NULL);
    shmctl(shm_pids, IPC_RMID, NULL);
}

/**Funkce inicializuje semafory, sdil. pamet a priradi jim pocatecni hodnoty.*/
void init(args arguments){
    //Uzamceni/odemceni semaforu.
    sem_station = sem_open(SEM_NAME1, O_CREAT | O_EXCL, 0666, UNLOCKED);
    sem_boarding = sem_open(SEM_NAME2, O_CREAT | O_EXCL, 0666, LOCKED);
    sem_writing = sem_open(SEM_NAME3, O_CREAT | O_EXCL, 0666, UNLOCKED);
    sem_all_in = sem_open(SEM_NAME4, O_CREAT | O_EXCL, 0666, LOCKED);
    sem_leaving = sem_open(SEM_NAME5, O_CREAT | O_EXCL, 0666, LOCKED);

    //Kontrola zda se vsechny semafory otevrely spravne.
    if (sem_station == SEM_FAILED || sem_boarding == SEM_FAILED || sem_writing  == SEM_FAILED || sem_all_in == SEM_FAILED || sem_leaving == SEM_FAILED){
        fclose(log_file);
        unlink_semaphores();
        fprintf(stderr, "Vytvoreni semaforu selhalo.\n");
        exit(1);
    }

    //Vytvoreni sdilene pameti
    shm_waiting = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    shm_action = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    shm_removed = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    shm_boarded = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    shm_riders = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    shm_error = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    shm_pids = shmget(IPC_PRIVATE, arguments.R * sizeof(pid_t), IPC_CREAT | 0666);

    //Kontrola jestli sdilena pamet byla vytvorena v poradku.
    if (shm_waiting == -1 || shm_action == -1 || shm_removed == -1 || shm_boarded == -1 || shm_error == -1 || shm_riders == -1 || shm_pids == -1){
        clean_all();
        fprintf(stderr, "Chyba pri vytvareni sdilene pameti.\n");
        exit(1);
    }

    //Mapovani sdilene pameti.
    action = shmat(shm_action, NULL, 0);
    waiting = shmat(shm_waiting, NULL, 0);
    removed = shmat(shm_removed, NULL, 0);
    boarded = shmat(shm_boarded, NULL, 0);
    riders = shmat(shm_riders, NULL, 0);
    error = shmat(shm_error, NULL, 0);
    pids = shmat(shm_pids, NULL, 0);
    
    //Kontrola jestli mapovani probjehlo uspesne.
    if (action == NULL || waiting == NULL || removed == NULL || boarded == NULL || error == NULL || riders == NULL || pids == NULL){
        clean_all();
        fprintf(stderr, "Chyba pri mapovani.\n");
        exit(1);
    }
}

/**Funkce simuluje jizdu autobusu*/
void ride(args arguments){
    sem_wait(sem_writing);  //Piseme BUS START.
    (*action)++;
    fprintf(log_file, "%d\t: BUS\t: start\n", *action);
    sem_post(sem_writing);

    while (*removed < arguments.R){  //Cyklime dokud neodvezeme vsechny
        if((*error) == DETECTED && (*removed) == (*riders)) //Pokud error pri gen., odvezeme vygen. ridery a koncime.
            exit(1);
        sem_wait(sem_station);  //Zamykame nastupiste.
        sem_wait(sem_writing);  //Piseme BUS ARRIVAL.
        (*action)++;
        fprintf(log_file, "%d\t: BUS\t: arrival\n", *action);
        sem_post(sem_writing);

        if (*waiting == 0){         //Pokud nikdo neceka:
            sem_wait(sem_writing);  //Piseme BUS DEPART.
            (*action)++;
            fprintf(log_file, "%d\t: BUS\t: depart\n", *action);
            sem_post(sem_writing);
            sem_post(sem_station);  //Odemykame nastupiste.
        }
        else {
            sem_wait(sem_writing);  //Piseme START BOARDING.
            (*action)++;
            fprintf(log_file, "%d\t: BUS\t: start boarding: %d\n", *action, *waiting);
            sem_post(sem_writing);
            sem_post(sem_boarding); //Otvirame dvere autobusu.
            sem_wait(sem_all_in);   //Cekame az vsichni budou uvnitr nebo max. kapacita.
            sem_wait(sem_writing);  //Piseme END BOARDING a DEPART.
            (*action)++;
            fprintf(log_file, "%d\t: BUS\t: end boarding: %d\n", *action, *waiting);
            (*action)++;
            fprintf(log_file, "%d\t: BUS\t: depart\n", *action);
            sem_post(sem_writing);
            sem_post(sem_station);  //Odemykame nastupiste.
        }

        if (arguments.ABT != 0) //Simulujeme jizdu.
            usleep((rand() % arguments.ABT) * 1000);

        sem_wait(sem_writing);  //Piseme END.
        (*action)++;
        fprintf(log_file, "%d\t: BUS\t: end\n", *action);
        if (*boarded != 0){ //Pokud mame cestujici tak:
            sem_post(sem_leaving);  //Otvirame dvere.
            sem_wait(sem_all_in);   //Cekame na signal az vystoupi vsichni.
        }
        sem_post(sem_writing);
    }
    sem_wait(sem_writing);  //Vsichni jsou odvezeni, piseme FINISH.
    (*action)++;
    fprintf(log_file, "%d\t: BUS\t: finish\n", *action);
    sem_post(sem_writing);
    exit(0);
}

/**Funkce popisuje chovani ridera.*/
void rider(args arguments, int id){
    sem_wait(sem_writing);  //Piseme START.
    (*action)++;
    fprintf(log_file, "%d\t: RID %d\t: start\n", *action, id);
    sem_post(sem_writing);
    sem_wait(sem_station);  //Pokud je odemceno nastupiste, vejdeme.
    sem_wait(sem_writing);  //Piseme ENTER.
    (*waiting)++;
    (*action)++;
    fprintf(log_file, "%d\t: RID %d\t: enter: %d\n", *action, id, *waiting);
    sem_post(sem_writing);
    sem_post(sem_station);
    sem_wait(sem_boarding); //Cekame na otevreni dveri autobusu.
    sem_wait(sem_writing);  //Piseme BOARDING.
    (*waiting)--;
    (*boarded)++;
    (*action)++;    //Pokud je plna kapacita nebo nikdo uz neceka davame signal k odjezdu.
    if (*waiting == 0 || *boarded == arguments.C)
        sem_post(sem_all_in);
    else    //Pokud muze jeste nekdo nastupit otevreme mu dvere.
        sem_post(sem_boarding);
    fprintf(log_file, "%d\t: RID %d\t: boarding\n", *action, id);
    sem_post(sem_writing);
    (*removed)++;
    sem_wait(sem_leaving);  //Cekame na signal k vystoupeni.
    (*boarded)--;
    if (*boarded != 0)  //Pokud jeste nekdo vystupuje po nas, otevreme mu.
        sem_post(sem_leaving);
    else    //Jinak davame signal k odjezdu autobusu.
        sem_post(sem_all_in);
    sem_wait(sem_writing);  //Piseme FINISH.
    (*action)++;
    fprintf(log_file, "%d\t: RID %d\t: finish\n", *action, id);
    sem_post(sem_writing);
    exit(0);
}

int main(int argc, char **argv){
    args arguments = check_args(argc, argv);    //Kontrola arg.
    log_file = fopen("bus.out", "w"); //Otevreme soubor pro zapis.
    if (log_file == NULL){  //Kontrolujeme uspesne otevreni souboru.
        fprintf(stderr, "Nepodarilo se otevrit soubor bus.out\n");
        exit(1);
    }
    init(arguments); //Inicializujeme semafory, pamet..
    setbuf(log_file, NULL); //Zabranime vyrovnavani vystupu.
    srand(time(0)); //Nastavime nahodne hodnoty pro rand(), podle casu.
    pid_t process_bus = fork(); //Vytvorime proces BUS.

    if (process_bus == 0) {    //BUS.
        ride(arguments);    //Simulace jizdy.
    }
    else if (process_bus < 0){  //Chyba.
        clean_all();
        fprintf(stderr, "Chyba pri forku BUS.\n");
        exit(1);
    }
    
    pid_t process_generator = fork();   //Vytvorime proces pro generovani rideru.

    if (process_generator == 0) {    //RIDER GENERATOR.
        pid_t process_rider;
        for(int id = 1; id <= arguments.R; id++){   //Generujeme podle zadaneho arg.
            if (arguments.ART != 0) //Uspime podle arg.
                usleep((rand() % arguments.ART) * 1000);

            process_rider = fork(); //Vytvorime proces rider.
            if(process_rider == 0){ //Rider.
                (*riders)++;
                rider(arguments, id);   //Simulace ridera.
            }
            else if (process_rider < 0){    //Chyba.
                *error = DETECTED;     //Nastavime priznak erroru.
                fprintf(stderr, "Chyba pri generovani rideru.\n");
                exit(1);
            }
            pids[id - 1] = process_rider;   //Ulozime pid processu pro pozdejsi cekani.
        }
        for (int i = 0; i < arguments.R; i++){
            waitpid(pids[i], NULL, 0);    //Cekame na vsechny ridery az skonci.
        }
        exit(0);
    } else if (process_generator < 0){  //Chyba.
        clean_all();
        fprintf(stderr, "Chyba pri forku generatoru rideru.\n");
        exit(1);
    }
    waitpid(process_generator, NULL, 0); //Cekame na proces generator.
    waitpid(process_bus, NULL, 0);  //Cekame na proces bus.
    clean_all();    //Uklidime.
    if (*error == DETECTED) //Podle chyby vracime navratovou hodnotu.
        exit(1);
    else
        exit(0);
}
