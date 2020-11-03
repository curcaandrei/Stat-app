#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio_ext.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/sysmacros.h>
void PrintMenu();
int prelucrare(char *buf);
void PrintStats(struct stat statut);
void cmd(int ok, char *buf, int *logat);
struct stat cmdmystat(char *buf, struct stat cox, int *scanat);
void cmdmyfind(char *basePath, char *find,int *linie);
int logat; //variabila care verifica daca userul este logat
FILE *users;

void main()
{   mkfifo("myfifo",0666);
    users = fopen("users.txt", "r"); //fisierul users
    char buf1[1000], buf2[1000];
    bzero(buf1, sizeof(buf1));
    bzero(buf2, sizeof(buf2));
    int pw, pr; //namedpipe pt scriere comanda catre copil si citire comanda de catre copil
    pid_t pid;
    int pipelogat[2], pipecmd[2]; // unnamed pipe pt transmiterea variabilei logat de la fiu la parinte
    int sockstat[2], sockfind[2];
    while (1)
    { //se va crea un copil la fiecare pas, iar dupa ce termina instructiunile, se foloseste functia kill

        pipe(pipelogat);
        pipe(pipecmd);
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockstat) < 0)
        {
            perror("Err... socketpair");
            exit(1);
        }
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockfind) < 0)
        {
            perror("Err... socketpair");
            exit(1);
        }
        fflush(stdout);
        if ((pid = fork()) < 0)
        {
            perror("Error at fork");
            exit(1);
        }

        if (pid) //parinte
        {
            PrintMenu();
            fflush(stdout);
            pw = open("myfifo", O_WRONLY); //deschidem fifo de scriere
            fgets(buf1, 400, stdin);
            fflush(stdout);
            printf("\n");
            write(pw, buf1, 1000); //scriem in fifo ce se citeste de la tastatura
            close(pw);
            close(pipelogat[1]);
            read(pipelogat[0], &logat, sizeof(int)); //logat=0 initial, se va modifica in copil daca se apeleaza login, si va fi citit aici
            int comanda;
            close(pipecmd[1]);
            read(pipecmd[0], &comanda, sizeof(int));
            close(pipecmd[0]);
            if (comanda == 4)
            {

                char path[100];
                close(sockstat[1]);
                int scanat = 0;
                read(sockstat[0], &scanat, sizeof(int));
                if (scanat == 1)
                {
                    struct stat statut;
                    read(sockstat[0], &statut, sizeof(statut));
                    PrintStats(statut);
                    close(sockstat[0]);
                }
                else
                {
                    printf("\n======INCORRECT PATH======\n\n");
                }
                close(sockstat[0]);
            }
            else if (comanda == 5)
            {   
                
                int findat=-1;
                char *matrice[400];
                read(sockfind[0],&findat,sizeof(int));
                int scanat=0;
                if(findat==0)
                {
                    printf("\n=====Nu s-a gasit niciun astfel de fisier=====\n");
                }
                close(sockfind[0]);
                
            }
        }
        else //copil
        {
            pr = open("myfifo", O_RDONLY);
            read(pr, buf2, 1000); //citim din fifo ce s-a citit de la tastatura prin parinte
            close(pr);
            int comanda = prelucrare(buf2);
            close(pipecmd[0]);
            write(pipecmd[1], &comanda, sizeof(int));
            close(pipecmd[1]);
            if (prelucrare(buf2) <= 3)
            {
                cmd(prelucrare(buf2), buf2, &logat); // aici se prelucreaza comenzile de la tastatura
            }
            else if (comanda == 4)
            {
                close(sockstat[0]);
                struct stat statut;
                int scanat = 0;
                statut = cmdmystat(buf2, statut, &scanat);
                if (scanat == 1)
                {
                    write(sockstat[1], &scanat, sizeof(int));
                    write(sockstat[1], &statut, sizeof(statut));
                    close(sockstat[1]);
                }

                close(sockstat[1]);
            }
            else if(comanda==5)
            {
                char cauta[100];
                int linie=0;
                strcpy(cauta, buf2 + 7);
                strtok(cauta, "\n");
                cmdmyfind("/home", cauta,&linie);
                close(sockfind[0]);
                write(sockfind[1],&linie,sizeof(int));
                close(sockfind[1]);
            }

            close(pipelogat[0]);
            write(pipelogat[1], &logat, sizeof(int)); // aici se trimite valoarea lui logat, daca utilizatorul s a logat
            kill(getpid(), SIGINT);
        }
    }
}

void PrintMenu()
{

    if (logat == 0) //meniu pt cand nu suntem logati
    {
        printf("Puteti folosi comenzile:\n");
        printf("login : *Username*\n");
        printf("quit\n");
        printf("\n");
    }
    if (logat == 2) //meniu pt cand introducem user gresit
    {

        printf("---USER NOT FOUND--- TRY AGAIN\n");
        printf("Puteti folosi comenzile:\n");
        printf("login : *Username*\n");
        printf("quit\n");
        printf("\n");
        logat = 0;
    }
    if (logat == 1) //meniu pt cand suntem logati
    {
        printf("Puteti folosi comenzile:\n");
        printf("myfind filename\n");
        printf("mystat filename\n");
        printf("delogare\n");
        printf("quit\n");
        printf("\n");
    }
    fflush(stdout);
}
int prelucrare(char *buf)
{ //comenzi accesibile daca nu suntem logati
    // 0-quit ; 1-login ; 2-delogare ;
    if (logat == 0)
    {
        if (strcmp(buf, "quit\n") == 0)
            return 0;
        if (strstr(buf, "login : ") - buf == 0)
            return 1;
    }
    if (logat == 1) //comenzi accesibile daca suntem logati
    {
        if (strcmp(buf, "delogare\n") == 0)
            return 2;
        if (strcmp(buf, "quit\n") == 0)
            return 0;
        if (strstr(buf, "mystat ") - buf == 0)
            return 4;
        if (strstr(buf, "myfind ") - buf == 0)
            return 5;
    }
    if (logat == 2)
    {
        if (strcmp(buf, "quit\n") == 0)
            return 0;
        if (strstr(buf, "login : ") - buf == 0)
            return 1;
    }
    return 3;
}
void cmd(int ok, char *buf, int *logat)
{
    if (ok == 0)
        kill(getppid(), SIGINT);
    if (ok == 1)
    { // cod care desparte login : user si verifica daca user apartine fisierului cu useri
        int gasit = 0;
        ssize_t citire;
        size_t len = 0;
        char *line = NULL;
        char *cuvant;
        const char sep[2] = " :";
        cuvant = strtok(buf, sep);
        cuvant = strtok(NULL, sep);
        while ((citire = getline(&line, &len, users)) != -1)
        {
            if (strcmp(line, cuvant) == 0)
            {
                *(logat) = 1;
                gasit = 1;
            }
        }
        if (gasit == 0)
        {
            *(logat) = 2;
        }
        rewind(users);
        fflush(stdout);
    }
    if (ok == 2)
    {
        *(logat) = 0;
    }
    if (ok == 3)
        printf("Wrong command\n\n");
}
struct stat cmdmystat(char *buf, struct stat statut, int *scanat)
{
    char path[100];
    strcpy(path, buf + 7);
    strtok(path, "\n");
    if (stat(path, &statut) == 0)
    {
        *(scanat) = 1;
        return statut;
    }
    else
        *(scanat) = 0;
}
void cmdmyfind(char *basePath, char *find,int *linie)
{   
    char path[1000];
    struct dirent *current;
    DIR *dir = opendir(basePath);
    if (!dir)
        return;
    struct stat stat;
    while ((current = readdir(dir)) != NULL )
    {
        if (strcmp(current->d_name, ".") != 0 && strcmp(current->d_name, "..") != 0 )
        {
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, current->d_name);
            if(strcmp(current->d_name,find)==0){
                (*linie)++;
            printf("%d -> %s\n",*linie,path);      
        }
            cmdmyfind(path, find,linie);
        }
    }

    closedir(dir);
}
void PrintStats(struct stat sb)
{
    printf("=====FILE STATS====================\n");
    //date luate din manualul stat()
    printf("ID of containing device:  [%lx,%lx]\n",
           (long)major(sb.st_dev), (long)minor(sb.st_dev));

    printf("File type:                ");

    switch (sb.st_mode & S_IFMT)
    {
    case S_IFBLK:
        printf("block device\n");
        break;
    case S_IFCHR:
        printf("character device\n");
        break;
    case S_IFDIR:
        printf("directory\n");
        break;
    case S_IFIFO:
        printf("FIFO/pipe\n");
        break;
    case S_IFLNK:
        printf("symlink\n");
        break;
    case S_IFREG:
        printf("regular file\n");
        break;
    case S_IFSOCK:
        printf("socket\n");
        break;
    default:
        printf("unknown?\n");
        break;
    }
    char permisiune[10];
    printf("I-node number:            %ld\n", (long)sb.st_ino);
    if (S_IRUSR & sb.st_mode)
        permisiune[0] = 'r';
    if (S_IWUSR & sb.st_mode)
        permisiune[1] = 'w';
    if (S_IXUSR & sb.st_mode)
        permisiune[2] = 'x';
    if (S_IRGRP & sb.st_mode)
        permisiune[3] = 'r';
    if (S_IWGRP & sb.st_mode)
        permisiune[4] = 'w';
    if (S_IXGRP & sb.st_mode)
        permisiune[5] = 'x';
    if (S_IROTH & sb.st_mode)
        permisiune[6] = 'r';
    if (S_IWOTH & sb.st_mode)
        permisiune[7] = 'w';
    if (S_IXOTH & sb.st_mode)
        permisiune[8] = 'x';
    printf("PERMISSIONS:              %s \n", permisiune);
    printf("Link count:               %ld\n", (long)sb.st_nlink);
    printf("Ownership:                UID=%ld   GID=%ld\n",
           (long)sb.st_uid, (long)sb.st_gid);
    printf("Preferred I/O block size: %ld bytes\n",
           (long)sb.st_blksize);
    printf("File size:                %lld bytes\n",
           (long long)sb.st_size);
    printf("Blocks allocated:         %lld\n",
           (long long)sb.st_blocks);
    printf("Last status change:       %s", ctime(&sb.st_ctime));
    printf("Last file access:         %s", ctime(&sb.st_atime));
    printf("Last file modification:   %s", ctime(&sb.st_mtime));
    printf("=====FILE STATS====================\n\n");
}
