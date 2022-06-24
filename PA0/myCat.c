#include <stdio.h>

int main(int argc, char* argv[]){
    FILE *myfile = fopen(argv[1], "r");
    
    char c = fgetc(myfile);
    while (c != EOF)
    {
        printf ("%c", c);
        c = fgetc(myfile);
    }
    
    fclose(myfile);
    return 0;
    
}