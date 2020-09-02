#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


//maybe I need to encapsulate a redirect function next time...

//filter primes
void analyze_primes()
{
    //create a pipe
    int p[2];
    pipe(p);

    int read_number;
    if (read(0, &read_number, 4) == 4)
    {
        printf("prime %d\n", read_number); //output the first number(prime) in queue

        if (fork() == 0) //child
        {
            //redirect standard output
            close(1);
            dup(p[1]);
            close(p[0]);
            close(p[1]);

            //filter primes in this queue and write them to standard output(pipe)
            int next_number;
            while (read(0, &next_number, 4) == 4)
            {
                if (next_number % read_number != 0)
                {
                    write(1, &next_number, 4);
                }
            }

            exit(0);
        }
        else //parent
        {
            //redirect standard input
            close(0);
            dup(p[0]);
            close(p[0]);
            close(p[1]);

            //wait child
            wait(0);

            //next filter
            analyze_primes();
            exit(0);
        }
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    //create a pipe
    int p[2];
    pipe(p);

    if (fork() == 0) //child
    {

        //redirect standard output
        close(1);
        dup(p[1]);
        close(p[0]);
        close(p[1]);

        //wirte 2 to 35 in standard output(pipe)
        for (int i = 2; i < 36; i++)
        {
            write(1, &i, 4);
        }

        exit(0);
    }
    else //parent
    {
        //redirect standard input
        close(0);
        dup(p[0]);
        close(p[0]);
        close(p[1]);

        //wait child
        wait(0);

        //filter primes
        analyze_primes();
        exit(0);
    }

    exit(0);
}