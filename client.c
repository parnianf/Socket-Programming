#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

#define SIZE 1024
#define ALARM 60


int f = 0;
char ans1[SIZE]= {'\0'};
char ans2[SIZE]= {'\0'};
char myQuestion[SIZE] = {'\0'};
char sending_server[3*SIZE]= {'\0'};
int best_answer, server_port, server_fd;
int room_port, udp_sockfd, my_turn;
struct sockaddr_in bc_adr_sendto, bc_adr_recvfrom;

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

int connectServer(int port) {
    int fd;
    struct sockaddr_in server_address;
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons(port); 
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) { // checking for errors
        printf("Error in connecting to server\n");
    }

    return fd;
}


char* get_total_rounds(int total_rounds, char ans[SIZE]){
    char rounds[4] = {'\0'};
    sprintf(rounds,"%d",total_rounds);
    if(total_rounds > 9){
        ans[0] = rounds[0];
        ans[1] = rounds[1];
        ans[2] = '*';
    }
    else{
        ans[0] = rounds[0];
        ans[1] = '*';
    }
    return ans;
}

int has_answered = 1;

void alarm_handler(int sig) {
    if(!has_answered){
        char alarm_buff[SIZE] = {'\0'};
        memset(alarm_buff, 0, SIZE);
        char sending_buf[SIZE]={'\0'};
        char delim[2] = {'$', '\0'};
        char ans[SIZE] = {'\0'};
        strcat(alarm_buff,"Time's up\n");
        strcat(sending_buf, get_total_rounds(f, ans));
        strcat(sending_buf, alarm_buff);
        strcat(sending_buf, delim);
        

        if(f == 1 || f == 4 || f == 5 || f == 8 || f == 9){
            strcat(sending_buf, "\n*****Answer round");
            if(f == 1) strcat(sending_buf, "<turn client 3>*****");
            if(f == 4) strcat(sending_buf, "<turn client 1>*****");
            if(f == 5) strcat(sending_buf, "<turn client 3>*****");
            if(f == 8) strcat(sending_buf, "<turn client 1>*****");
            if(f == 9) strcat(sending_buf, "<turn client 2>*****");
        }


        if(f == 2 || f == 6 || f == 10){
            strcat(sending_buf, "\n*****Choose answer");
            if(f == 2) strcat(sending_buf, "<turn client 1>*****\nfirst answer -> 1\nsecond answer -> 2");
            if(f == 6) strcat(sending_buf, "<turn client 2>*****\nfirst answer -> 1\nsecond answer -> 2");
            if(f == 10) strcat(sending_buf, "<turn client 3>*****\nfirst answer -> 1\nsecond answer -> 2");
        }

        if(f == 0 ||f == 3 || f == 7){
            strcat(sending_buf, "\n*****Question round");
            if(f == 3) strcat(sending_buf, "<turn client 2>*****");
            if(f == 7) strcat(sending_buf, "<turn client 3>*****");
        }

        alarm(0);
        sendto(udp_sockfd, sending_buf, strlen(sending_buf), 0,(struct sockaddr *)&bc_adr_sendto, sizeof(struct sockaddr_in));   
    }
}

void recieve_data(char buffer[SIZE], char room_answer[SIZE]){
    memset(buffer, 0, SIZE);
    socklen_t bc_adr_len = sizeof(bc_adr_recvfrom);
    recvfrom(udp_sockfd, buffer, SIZE, 0, (struct sockaddr *)&bc_adr_recvfrom, &bc_adr_len);

    if(buffer[0] != 'r'){
        char stf[3] = {'\0'};
        char type_round[300] = {'\0'};
        int ind = 0;
        while(buffer[ind] != '*'){
            stf[ind] = buffer[ind];
            ind++;
        }
        f = atoi(stf);        
        ind++;
        int new_ind = 0;
        while(buffer[ind] != '$'){
            room_answer[new_ind] = buffer[ind];
            ind++;
            new_ind++;
        }
        ind++;
        int type_ind = 0;
        while(buffer[ind] != '\0'){
            type_round[type_ind] = buffer[ind];
            ind++;
            type_ind++;
        }
        if((f == 1 && my_turn == 1) || (f == 5 && my_turn == 2) || (f == 9 && my_turn == 3)){
            strcpy(ans1, room_answer);
        }
        if((f == 2 && my_turn == 1) || (f == 6 && my_turn == 2) || (f == 10 && my_turn == 3)){
            strcpy(ans2, room_answer);
        }
        f++;//f is total round passed till now
        printf("%s\n", room_answer);
        printf("%s\n", type_round);
        
        if((f == 1 && my_turn == 2) || (f == 2 && my_turn == 3) || (f == 5 && my_turn == 1) || (f == 6 && my_turn == 3)||
            (f == 9 && my_turn == 1) || (f == 10 && my_turn == 2)){//should be answered in this cases
            printf("<YOUR TURN NOW>\n");
            printf("Alarm Started, You Have 1 Minute.\n");
            has_answered = 0;
            alarm(ALARM);
        }
        if((f == 0 && my_turn == 1) || (f == 3 && my_turn == 1) || (f == 4 && my_turn == 2)||
        (f == 7 && my_turn == 2) || (f == 8 && my_turn == 3) || (f == 11 && my_turn == 3)){
            printf("<YOUR TURN NOW>\n");
        }
    }
    else if (buffer[0] == 'r'){
        char res_brod[SIZE*3] = {'\0'};
        int round_res_ind = 2;
        while(buffer[round_res_ind] != '\0'){
            res_brod[round_res_ind-2] = buffer[round_res_ind];
            round_res_ind++;
        }
        printf("%s\n", res_brod);
    }
}

void send_result_to_server(char best_opt_buffer[SIZE], int my_turn_char){
    if(atoi(best_opt_buffer) == 1) best_answer = 1;
    else if(atoi(best_opt_buffer) == 2) best_answer = 2;
    strcat(sending_server, "Question: ");
    strcat(sending_server, myQuestion);
    strcat(sending_server, "\nAnswers: \n");
    strcat(sending_server, "-*");
    if(best_answer == 1) {
        strcat(sending_server, ans1);
        strcat(sending_server, "\b-");
        strcat(sending_server, ans2);
    }
    else if(best_answer == 2) {
        strcat(sending_server, ans2);
        strcat(sending_server, "\n-");
        strcat(sending_server, ans1);
    }
    send(server_fd, sending_server, strlen(sending_server), 0);
    printf("Result has sent to server.\n");
}


int main(int argc, char const *argv[]) {
    printf("Choose Your Field By Writing A Number:\n");
    printf("Computer engineering -> 1\n");
    printf("Electrical engineering -> 2\n");
    printf("Civil engineering -> 3\n");
    printf("Mechanic engineering -> 4\n");
    signal(SIGALRM, alarm_handler);
    siginterrupt(SIGALRM, 1);
    int fd;
    char buff[SIZE] = {0};
    char my_turn_char;
    if(argc < 2){
        printf("ERROR, NO PORT\n");
        return 0;
    }
    server_port = atoi(argv[1]);
    server_fd = connectServer(server_port);
    fd_set master_set, working_set;
    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set); //connection with server
    FD_SET(0, &master_set); //standard input
    int max_sd = server_fd;
    
    while(1){//TCP with server
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);
        if(FD_ISSET(server_fd, &working_set)){//room is ready
            bzero(buff, SIZE);
            recv(server_fd, buff, SIZE, 0);
            if(buff[4] == '1') my_turn = 1;
            else if(buff[4] == '2') my_turn = 2;
            else if(buff[4] == '3') my_turn = 3;
            my_turn_char = buff[4];
            buff[4] = '\0';
            room_port = atoi(buff);
            break;
        }
        else { // standard input
            read(0, buff, SIZE);
            send(server_fd, buff, strlen(buff), 0);
            memset(buff, 0, SIZE);
            printf("Trying To Connect To Room...\n");
        }
        memset(buff, 0, SIZE);
    }

    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_sockfd < 0)
        printf("ERROR - creating socket\n");
    
    int broadcast_en = 1, opt_en = 1;
    setsockopt(udp_sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_en, sizeof(broadcast_en)); 
    setsockopt(udp_sockfd, SOL_SOCKET, SO_REUSEPORT, &opt_en, sizeof(opt_en));
    

    bc_adr_recvfrom.sin_family = AF_INET;
    bc_adr_recvfrom.sin_addr.s_addr = inet_addr("192.168.1.255");
    bc_adr_recvfrom.sin_port = htons(room_port);
    bc_adr_sendto.sin_family = AF_INET;
    bc_adr_sendto.sin_addr.s_addr = inet_addr("192.168.1.255"); 
    bc_adr_sendto.sin_port = htons(room_port);

    
    if(bind(udp_sockfd, (struct sockaddr*) &bc_adr_recvfrom, sizeof(bc_adr_recvfrom)) < 0) 
        printf("ERROR on binding\n");

    char buffer[SIZE] = {'\0'};
    max_sd = 500;
    FD_ZERO(&master_set);
    FD_SET(udp_sockfd, &master_set);
    FD_SET(0, &master_set);



    write(1, "UDP Room is running, ", 22);
    if(my_turn == 1) printf("your turn is \"1\"\n<YOUR TURN NOW>\n");
    if(my_turn == 2) printf("your turn is \"2\"\n");
    if(my_turn == 3) printf("your turn is \"3\"\n");
    
    printf("*****Ask Question<turn client 1>*****");
    
    
    while (1) {//UDP for broadcasting
        char room_answer[SIZE] = {'\0'};
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);
        if(FD_ISSET(udp_sockfd, &working_set)){ //recieving
            alarm(0);
            recieve_data(buffer, room_answer);            
        }
        
        if(FD_ISSET(0, &working_set)) { //sending and broadcasting
            
            alarm(0);
            if( (f == 0 && my_turn != 1) || (f == 4 && my_turn != 2) || (f == 8 && my_turn != 3)){
                continue;
            }
            else if ((f == 1 && my_turn != 2) || (f == 2 && my_turn != 3) || (f == 5 && my_turn != 1) || 
            (f == 6 && my_turn != 3) || (f == 9 && my_turn != 1) || (f == 10 && my_turn != 2)){
                continue;
            }
            else if((f==3 && my_turn!=1) || (f==7 && my_turn!=2) || (f==11 && my_turn!=3)){
                continue;
            } 


            memset(buffer, 0, SIZE);
            char sending_buf[SIZE]={'\0'};
            char delim[2] = {'$', '\0'};
            char ans[SIZE] = {'\0'};
            char ts[4] = {my_turn_char,':',' ' ,'\0'};
            read(0, buffer, SIZE);
            strcat(sending_buf, get_total_rounds(f, ans));
            strcat(sending_buf, "client ");
            strcat(sending_buf, ts);
            strcat(sending_buf, buffer);
            strcat(sending_buf, delim);



            if((f == 3 && my_turn == 1) || (f == 7 && my_turn == 2) || (f == 11 && my_turn == 3)){
                send_result_to_server(buffer,my_turn_char);
                char show_round_result[SIZE*3] = {'\0'};
                char show_best_ans[SIZE*3]  ={'\0'};
                strcat(show_best_ans, "r*Best Answer=> ");
                if(atoi(buffer) == 1) strcat(show_best_ans, ans1);
                else if(atoi(buffer) == 2) strcat(show_best_ans, ans2);
                sendto(udp_sockfd, show_best_ans, strlen(show_best_ans), 0,(struct sockaddr *)&bc_adr_sendto, sizeof(struct sockaddr_in));   

                strcat(show_round_result,"r*Result of this round is:\n");
                strcat(show_round_result,sending_server);
                sendto(udp_sockfd, show_round_result, strlen(show_round_result), 0,(struct sockaddr *)&bc_adr_sendto, sizeof(struct sockaddr_in));   
                
            }

            if((f == 0 && my_turn==1) || (f==4 && my_turn==2) || (f==8 && my_turn==3)){
                strcpy(myQuestion,buffer);
            }

            if(f == 0 || f == 1 || f == 4 || f == 5 || f == 8 || f ==9){
                strcat(sending_buf, "\n*****Answer");
                if(f == 0) strcat(sending_buf, "<turn client 2>*****");
                if(f == 1) strcat(sending_buf, "<turn client 3>*****");
                if(f == 4) strcat(sending_buf, "<turn client 1>*****");
                if(f == 5) strcat(sending_buf, "<turn client 3>*****");
                if(f == 8) strcat(sending_buf, "<turn client 1>*****");
                if(f == 9) strcat(sending_buf, "<turn client 2>*****");
            }
            if(f == 2 || f == 6 || f == 10){
                strcat(sending_buf, "\n*****Choose Answer");
                if(f == 2) strcat(sending_buf, "<turn client 1>*****\nfirst answer -> 1\nsecond answer -> 2");
                if(f == 6) strcat(sending_buf, "<turn client 2>*****\nfirst answer -> 1\nsecond answer -> 2");
                if(f == 10) strcat(sending_buf, "<turn client 3>*****\nfirst answer -> 1\nsecond answer -> 2");
            }
            if(f == 3 || f == 7){
                strcat(sending_buf, "\n*****Ask Question");
                if(f == 3) strcat(sending_buf, "<turn client 2>*****");
                if(f == 7) strcat(sending_buf, "<turn client 3>*****");
            }


            sendto(udp_sockfd, sending_buf, strlen(sending_buf), 0,(struct sockaddr *)&bc_adr_sendto, sizeof(struct sockaddr_in));   
            alarm(0);
        }

        if(f==12){
            printf("Room Closed.\n");
            break;
        }
    }

    


    return 0;
}