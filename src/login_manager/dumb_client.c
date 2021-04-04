//
// Created by maomao on 3/29/21.
//
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]){
    if (!strcmp(argv[2],"view")){ // strcomp returns 0 if true so !
        printf("pretending to connect to,%s to view\n",argv[1]);
    }
    else if (!strcmp(argv[2],"control")){
        printf("pretending to connect to %s with control\n",argv[1]);
    }
    else{
        printf("Invalid lol XD\n");
    }

}
