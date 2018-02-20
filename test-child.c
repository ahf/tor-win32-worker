#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    printf("First line! Next one should show up in a second.\n");
    sleep(1);

    printf("Second line! Next one should show up in two seconds.\n");
    sleep(2);

    printf("Third line! The rest of the line will arrive in two seconds. ");
    sleep(2);

    printf("Woh! The line is complete.\n");
    sleep(4);

    printf("This is the last line :-)\n");
    sleep(1);

    return EXIT_SUCCESS;
}
