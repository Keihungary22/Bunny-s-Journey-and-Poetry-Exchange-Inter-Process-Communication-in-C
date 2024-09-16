#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <sys/msg.h>

#define NUM_BOYS 4
#define MAX_POEMS 100
#define MAX_POEM_LENGTH 100
#define POEM_FILE "poems.txt"

struct message{
    long mtype;
    char mtext[1024];
};

void mother_signal(){
    printf("MAMA: Have fun my son!\n");
}

void child_signal(){
    printf("\n");
    printf("BOY: I arrived in Barátfa!\n");
    printf("BOY: Give me poems!\n");
}

int main(int argc, char* argv[]){
    int motherpipe1[2];
    int motherpipe2[2];
    if(pipe(motherpipe1) == -1){
        perror("pipe error.\n");
        return 1;
    }
    if(pipe(motherpipe2) == -1){
        perror("pipe error.\n");
        return 1;
    }
    char poem1[MAX_POEM_LENGTH], poem2[MAX_POEM_LENGTH], final_poem[MAX_POEM_LENGTH];
    signal(SIGTERM, mother_signal);
    signal(SIGUSR1, child_signal);

    pid_t pid[NUM_BOYS];
    int i;
    srand(time(NULL));
    int chosen_child = rand() % NUM_BOYS;

    for(int i = 0; i < NUM_BOYS; i++){
        pid[i] = fork();
        if(pid[i] < 0){
            printf("fork faild.\n");
            return 1;
        }
        if(pid[i] == 0){
            if(i != chosen_child){
                exit(0);
            }
        }
    }

    if(pid[chosen_child] > 0){
        //mother process
        printf("BOY: The chosen bunny is %i bunny.\n", chosen_child + 1);

        FILE *file = fopen("poems.txt", "r");
        if(file == NULL){
            printf("Cannot open file.\n");
            return 1;
        }

        char *poems[MAX_POEMS];
        char line[MAX_POEM_LENGTH];
        int count = 0;

        while(fgets(line, sizeof(line), file)){
            poems[count] = malloc(strlen(line) + 1);
            if(poems[count] == NULL){
                printf("Memory aloocation error.\n");
                return 1;
            }
            strcpy(poems[count], line);
            count++;
        }
        fclose(file);
        for(int i = 0; i < count ; i++){
            printf("%s", poems[i]);
            // free(poems[i]);
        }
        printf("\n");

        char choice;
        while(1){
            printf("Enter 'a' to add a new poem or 'r' to let the bunny go: ");
            scanf(" %c", &choice);

            if(choice == 'a'){
                char new_poem[MAX_POEM_LENGTH];
                printf("Enter the new poem: ");
                getchar();
                fgets(new_poem, MAX_POEM_LENGTH, stdin);

                FILE *file = fopen("poems.txt", "a");
                if(file == NULL){
                    printf("Cannot open file.\n");
                    return 1;
                }
                fprintf(file, "%d.%s", count + 1, new_poem);
                count++;
                fclose(file);
                printf("The new poem has been added.\n");
            }else if(choice == 'r'){
                break;
            }else{
                printf("Invalid choice. Please enter 'a' or 'r'.\n");
            }
        }
        printf("\n");

        kill(pid[chosen_child], SIGTERM);

        pause();
        sleep(1);
        printf("\n");
        printf("MAMA: I confirmed you arrived!\n");
        printf("MAMA: I will send you two poems!\n");

        srand(time(NULL));
        int poem1_index = rand() % count;
        srand(time(NULL));
        int poem2_index = rand() % count;
        while(poem2_index == poem1_index){
            srand(time(NULL));
            poem2_index = rand() % count;
        }

        close(motherpipe1[0]);
        close(motherpipe2[0]);
        write(motherpipe1[1], poems[poem1_index], 20);
        write(motherpipe2[1], poems[poem2_index], strlen(poems[poem2_index]) + 1);
        close(motherpipe1[1]);
        close(motherpipe2[1]);
        fflush(NULL);

        key_t key = ftok(argv[0], 1);
        int msgid = msgget(key, 0600 | IPC_CREAT);
        if(msgid < 0){
            perror("msgget error.\n");
            return 1;
        }
        struct message message;
        msgrcv(msgid, &message, sizeof(message), 1, 0);
        printf("MAMA: Received poem from child ->  %s\n", message.mtext);

        int status;
        waitpid(pid[chosen_child], &status, 0);

        
    }else{
        //chosen_child process
        pause();
        printf("\n");
        printf("BOY: I am the chosen one.\n");
        printf("BOY: I am heading to Barátfa!\n");
        sleep(1);
        kill(getppid(), SIGUSR1);

        close(motherpipe1[1]);
        close(motherpipe2[1]);
        read(motherpipe1[0], poem1, MAX_POEM_LENGTH);
        read(motherpipe2[0], poem2, MAX_POEM_LENGTH);        
        // poem1[MAX_POEM_LENGTH - 1] = '\0';
        // poem2[MAX_POEM_LENGTH - 1] = '\0';
        close(motherpipe1[0]);
        close(motherpipe2[0]);
        sleep(1);
        printf("BOY: The received poems is following!\n%s%s\n", poem1, poem2);

        srand(time(NULL));
        int choose_one = rand() % 2;
        if(choose_one == 0){
            strcpy(final_poem, poem1);
        }else{
            strcpy(final_poem, poem2);
        }
        printf("BOY: I chose this poem! -> %s\n", final_poem);

        key_t key = ftok(argv[0], 1);
        int msgid = msgget(key, 0600 | IPC_CREAT);
        if(msgid < 0){
            perror("msgget error.\n");
            return 1;
        }
        struct message message;
        message.mtype = 1;
        strncpy(message.mtext, final_poem, sizeof(message.mtext));
        msgsnd(msgid, &message, sizeof(message), 0);

        sleep(1);
        printf("BOY: May I water!\n");
        sleep(1);
        printf("BOY: I will go back to my home.\n");
        sleep(1);
        printf("BOY: I returned home to Mama Bunny.\n");

    }
    return 0;
    // waitpid(pid[chosen_child], NULL, 0);
    
}