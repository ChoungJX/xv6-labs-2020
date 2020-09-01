#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int p[2];

    char message[1] = "a"; // setting the "pingpong" byte

    pipe(p); // create a pipe

    if (fork() == 0) //if the child
    {

        char child_get_message[1] = "0";  // Initialize variable
        read(p[0], child_get_message, 1); //read from pipe

        if (child_get_message[0] == message[0]) //if read is successful
        {
            printf("%d: received ping\n", getpid());
        }

        close(p[0]);

        write(p[1], child_get_message, 1); //pong
        close(p[1]);
        exit(0);
    }
    else //parent
    {
        write(p[1], message, 1); // ping
        close(p[1]);
        wait(0); //wait the child

        char parent_get_message[1] = "0"; // Initialize variable
        read(p[0], parent_get_message, 1);

        if (parent_get_message[0] == message[0]) //if read is successful
        {
            printf("%d: received pong\n", getpid());
        }
        exit(0);
    }
    exit(0);
}