//
// Created by maomao on 3/29/21.
//
// Client side C
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "json-c/json.h"
#define PORT 6970
#define MAX 1024
#define STRING 100
int main(int argc, char const *argv[]) {
    if (argc == 2){
        int sock = 0, valread;
        char username[STRING];
        char password[STRING];
        printf("Enter Username:");
        scanf("%s", username);
        printf("Enter Password:");
        scanf("%s", password);

        struct sockaddr_in serv_addr;
        char uname_pass[STRING];
        sprintf(uname_pass,"%s,%s\n",username,password);
        char buffer[MAX] = {0};
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\n Socket creation error \n");
            return -1;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);

        // Convert IPv4 and IPv6 addresses from text to binary form
        if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
            printf("\nInvalid address/ Address not supported \n");
            return -1;
        }

        if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            printf("\nConnection Failed \n");
            return -1;
        }
        send(sock, uname_pass, strlen(uname_pass), 0);
        printf("Sent Credentials\n");
        valread = read(sock, buffer, MAX);
        printf("%s\n", buffer);
        struct json_object *parsed_json;
        struct json_object *ip;
        struct json_object *permission;
        char parsed_ip[STRING];
        char parsed_permission[STRING];

        parsed_json = json_tokener_parse(buffer);
        json_object_object_get_ex(parsed_json, "Ip", &ip);
        json_object_object_get_ex(parsed_json, "Permission", &permission);

        strcpy(parsed_ip,json_object_get_string(ip));
        strcpy(parsed_permission,json_object_get_string(permission));

        printf("%s,%s\n",parsed_ip,parsed_permission);
        if (!strcmp(parsed_ip,"unameerror")){
            printf("Wrong Username or Password\n");
        }
        else if (!strcmp(parsed_ip,"servererror")){
            printf("Server Assignment Failed\n");
        }
        else {
            char *args[] = {"./dumb_client", json_object_get_string(ip), json_object_get_string(permission), NULL};
            execvp(args[0], args);
        }
        return 0;
    }
    else{
        printf("Usage: ./login_client <ip address>\n");
    }
}
