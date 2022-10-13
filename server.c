#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>



#define MAX_NUMBER_OF_ROOMS 100
#define MAX_CLIENTS_IN_ROOM 3
#define SIZE 1024


/*
 * Computer -> 1
 * Elecrical -> 2
 * Civil -> 3
 * Mechanic -> 4
*/

struct Room {
    int id;
    int clients[MAX_CLIENTS_IN_ROOM];
    int port;
    int field;
    int occupied;
    int open;
};

struct Room all_rooms[MAX_NUMBER_OF_ROOMS];
int room_id = 0;
int room_port;
int created_rooms_size = 0;
int answered_clients[MAX_NUMBER_OF_ROOMS * 3] = {0};

int setupServer(int port) {
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 500);
    return server_fd;
}

int find_open_room_index(int field_id){
    for(int i = 0 ; i < MAX_NUMBER_OF_ROOMS; i++){
        if(all_rooms[i].field == field_id && all_rooms[i].occupied < MAX_CLIENTS_IN_ROOM){
            return i;
        }
    }
    return -1;
}


void create_new_room(int field_id, int client_fd){
    struct Room new_room;
    new_room.id = room_id;
    new_room.occupied = 1;
    new_room.field = field_id;
    new_room.port = room_port;
    new_room.clients[0] = client_fd;
    new_room.open = 1;
    all_rooms[created_rooms_size] = new_room;
    room_id++;
    room_port++;
    created_rooms_size++;
    printf("room created for client %d with room id %d of field %d occ: %d\n", client_fd, new_room.id, field_id, new_room.occupied);
}

int compare_string(char* a, char* b){
    int i=0;
    while(1){
        if(a[i] =='\0' || b[i] == '\0'){
            return 1;
        }
        if(a[i] != b[i]){
            return 0;
        }
        i++;
    }
    return 1;
}

int get_fd_index_in_array(int goal_fd, int*arr){
    for(int i=0;i<sizeof(arr);i++){
        if(arr[i] == goal_fd){
            return i;
        }
    }
    return 50;
}

void set_port_to_full_room(struct Room full_room){
    char port_buffer[SIZE];
    char room_port[7];
    char turn;
    bzero(port_buffer, SIZE);
    sprintf(room_port, "%d", full_room.port);
    strcat(port_buffer, room_port);
    for(int i = 0; i < MAX_CLIENTS_IN_ROOM;i++){
        turn = i + 1 + '0';
        port_buffer[4] = turn;
        send(full_room.clients[i], port_buffer, SIZE, 0);
    }
}

int get_field_of_client(int client_fd){
    for(int i=0;i<MAX_NUMBER_OF_ROOMS;i++){
        if(all_rooms[i].open == 1){
            for(int j=0;j<3;j++){
                if(all_rooms[i].clients[j] == client_fd){
                    return all_rooms[i].field;
                }
            }
        }
    }
    return -1;
}

void closing_client(int client_fd){
    for(int i=0;i<MAX_NUMBER_OF_ROOMS;i++){
        for(int j=0;j<3;j++){
            if(all_rooms[i].clients[j] == client_fd){
                all_rooms[i].clients[j] == -10;
                all_rooms[i].occupied -= 1;
                if(all_rooms[i].occupied == 0){
                    all_rooms[i].field = 5;
                    all_rooms[i].open = 0;
                }
            }
        }
    }
}


void handle_room(int client_fd, char* field){
    int curr_field_id;
    if(compare_string(field,"1")){
        curr_field_id = 1;
    }
    if(compare_string(field,"2")){
        curr_field_id =2;
    }
    if(compare_string(field, "3")){
        curr_field_id = 3;
    }
    if(compare_string(field, "4")){
        curr_field_id = 4;
    }

    if(created_rooms_size == 0){
        create_new_room(curr_field_id, client_fd);
        return;
    }
    int open_room_index = find_open_room_index(curr_field_id);
    if(open_room_index != -1) {
        all_rooms[open_room_index].clients[all_rooms[open_room_index].occupied] = client_fd;
        all_rooms[open_room_index].occupied++;
        printf("client %d added to room %d of field %d, occ: %d\n", client_fd, open_room_index, curr_field_id, all_rooms[open_room_index].occupied);
        if(all_rooms[open_room_index].occupied == MAX_CLIENTS_IN_ROOM){
            set_port_to_full_room(all_rooms[open_room_index]);
        }
    }
    else {
        create_new_room(curr_field_id, client_fd);
    }
}

int acceptClient(int server_fd) {
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*) &address_len);
    return client_fd;
}


int main(int argc, char const *argv[]) {
    int server_fd, new_socket, max_sd;
    char buffer[SIZE] = {'\0'};
    fd_set master_set, working_set;
    if(argc < 2){
        printf("ERROR, NO PORT\n");
        return 0;
    }
    room_port = atoi(argv[1]) + 1;
    server_fd = setupServer(atoi(argv[1]));
    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(server_fd, &master_set);

    write(1, "Server is running\n", 18);

    while (1) {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        for (int i = 0; i <= max_sd; i++) {
            if (FD_ISSET(i, &working_set)) {
                
                if (i == server_fd) {  // new clinet
                    new_socket = acceptClient(server_fd);
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    printf("New client connected. fd(id) = %d\n", new_socket);
                }
                else { // client sending msg
                    int bytes_received;
                    bytes_received = recv(i , buffer, 1024, 0);
                    buffer[SIZE-1] = '\0';
                    if (bytes_received == 0) { // EOF
                        printf("client fd = %d closed\n", i);
                        close(i);
                        closing_client(i);
                        answered_clients[i] = 0;
                        FD_CLR(i, &master_set);
                        continue;
                    }
                    else{
                        if(buffer[0] == 'Q'){//for writing in file
                            int file_fd;
                            char file_name[30]={'\0'};
                            memset(file_name, 0, 30);
                            int field_of_client = get_field_of_client(i);
                            if(field_of_client == 1) strcpy(file_name,"Computer.txt");
                            else if(field_of_client == 2) strcpy(file_name,"Electrical.txt");
                            else if(field_of_client == 3) strcpy(file_name,"Civil.txt");
                            else if(field_of_client == 4) strcpy(file_name,"Mechanic.txt");
                            else{printf("no room found\n");}
                            file_fd = open(file_name, O_APPEND | O_RDWR | O_CREAT, 0644);
                            write(file_fd, buffer, strlen(buffer));
                            close(file_fd);
                            // printf("request client %d, wrote in file: %s\n",i, file_name);
                        }
                        else{
                            handle_room(i, buffer);
                        }
                    }
                    
                    memset(buffer, 0, 1024);
                }
            }
        }

    }

    return 0;
}