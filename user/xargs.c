#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    char *args[MAXARG];  //this variable is used in exec

    char xargs_argv[MAXARG][MAXARG];  //temp variable uesd to storage parament
    memset(xargs_argv, '\0', MAXARG * MAXARG); //initialize
    for (int i = 1; i < argc; i++)
    {
        strcpy(xargs_argv[i - 1], argv[i]); //storage xargs parament
    }

    char read_char;
    int flag[2] = {argc - 1, 0};// xargs_argv[flag[0]][flag[1]]
    while (read(0, &read_char, 1))
    {
        //read normal char except " " and "\n"
        if (read_char != ' ' && read_char != '\n')
        {
            xargs_argv[flag[0]][flag[1]] = read_char;
            flag[1]++;
            continue;
        }
        else
        {
            if (read_char != '\n')//if " "
            {
                flag[0]++;//skip " "
            }
            else  //if "\n"
            {
                for (int i = 0; i < MAXARG - 1; i++)
                {
                    args[i] = xargs_argv[i];//put the paraments' address in args
                }
                args[MAXARG - 1] = 0;//the last must be 0, otherwise exec will not be executed(tested)

                //Child exec
                if (fork() == 0)
                {
                    exec(argv[1], args);
                    exit(0);
                }
                else
                {
                    wait(0); //Wati child

                    //reinitialize
                    memset(xargs_argv, '\0', MAXARG * MAXARG);
                    for (int i = 1; i < argc; i++)
                    {
                        strcpy(xargs_argv[i - 1], argv[i]);
                    }
                    flag[0] = argc - 1;
                    flag[1] = 0;
                }
            }
        }
    }

    exit(0);
}
