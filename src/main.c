#include <stdio.h>
int main(int argc, char *argv[]) {
    
    //Ausgabe aller Argumente auf der Konsole
    for(int i = 0; i < argc; i++) {
        printf("Argument %d: %s\n", i, argv[i]);
    }

    return 0;
}