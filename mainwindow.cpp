
#include <sys/wait.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <pthread.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <sys/msg.h>
#include <sys/ipc.h>

#include <iostream>
#include <csignal>
#include <cstdlib>
#include <cstring>

#define BUFF 1024
#define NUM_THREADS 5
#define NICK 16

using namespace std;

int now_connected=0;
int connection_socket_descriptor;
char nick[NICK];
int msgid, kom;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct komunikat {
   long typ;
   char wartosc[BUFF];
};




//thread for reading messages from the server.
void* read_handler(void*)
{
    char read_from[BUFF];
    struct komunikat kmt1;
    kmt1.typ = 1;
    while (1) {
        int chars_read = read(connection_socket_descriptor, read_from,BUFF);
        if (chars_read < 0)
        {
            perror("problem with reading");
            exit(1);
        }
        else if (chars_read == 0)
        {
            now_connected=0;
            break;
        }
        else
        {
            //the messages are sent to the main thread through a queue
            strcpy(kmt1.wartosc,read_from);
            pthread_mutex_lock(&mutex);
            if (msgsnd(msgid, &kmt1,sizeof(kmt1.wartosc), 0) == -1)
            {
                perror("Blad przy wysylaniu wiadomości do glownego watku\n");
                exit(1);
            }
            kom++;
            pthread_mutex_unlock(&mutex);

        }
    }
    return 0;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//sets up the connection and maintains it.
void MainWindow::on_connect_clicked()
{

    if(now_connected==0)
    {   kom=0;


        msgid = msgget(IPC_PRIVATE, 0660); //próba utworzenia kolejki komunikatów
        if (msgid == -1){
            perror("Błąd przy próbie stworzenia kolejki komunikatow");
            exit(1);
        }
        struct sockaddr_in server_address, client_address;
        int connect_result;

        int chars_read_to_nick;
        struct hostent* server_host_entity;

        //checking and setting server name
        QString serv_name=this->ui->server_name->toPlainText();
        QByteArray serv_name_helper = serv_name.toLocal8Bit();
        const char *server_name = serv_name_helper.data();
        if(server_name!=NULL)
        {
            server_host_entity = gethostbyname(server_name);
            if (! server_host_entity)
            {
                perror("error with server_host_entity");
                exit(1);
            }
        }

        //creating socket
        connection_socket_descriptor = socket(PF_INET, SOCK_STREAM, 0);
        if (connection_socket_descriptor < 0)
        {
            perror("error with creating socket");
            exit(1);
        }

        //checking and setting port number
        QString port_num=this->ui->port_number->toPlainText();
        QByteArray port_num_helper = port_num.toLocal8Bit();
        const char *port_number = port_num_helper.data();
        int port=atoi(port_number);
        //initialization
        memset(&server_address, 0, sizeof(server_address));
        memset(&client_address, 0, sizeof(client_address));
        server_address.sin_family = AF_INET;
        memcpy(&server_address.sin_addr.s_addr, server_host_entity->h_addr, server_host_entity->h_length);
        server_address.sin_port = htons(port);

        //connection to server
        connect_result = ::connect(connection_socket_descriptor, (struct sockaddr*)&server_address, sizeof(server_address));
        if (connect_result < 0)
        {
            perror("error with connecting socket");
            exit(1);
        }
        //setting nick
        QString nickname=this->ui->nick->toPlainText();
        QByteArray nickname_helper = nickname.toLocal8Bit();
        const char *nickname_nearly_ready = nickname_helper.data();
        char *nickname_ready=(char*)nickname_nearly_ready;
        strcpy(nick,nickname_ready);

        if (strlen(nick) >= NICK-1)
        {
            cout<<"your nick is too long"<<endl;
            exit(1);
        }
        //sending nick to server
        chars_read_to_nick=write(connection_socket_descriptor, nick, NICK);
        if(chars_read_to_nick<0)
        {
            perror("problem with sending nick");
            exit(1);
        }
         now_connected=1;
        //creating thread for read
        pthread_t read_thread;
        if (pthread_create(&read_thread, NULL, read_handler, NULL) != 0) {
            perror("problem with reading");
            exit(1);
        }



        //main loop. Reading on a loop
        struct komunikat kmt;

        while(now_connected)
        {

            pthread_mutex_lock(&mutex);
            if(kom==0)
            {
                pthread_mutex_unlock(&mutex);
                QCoreApplication::processEvents();
                sleep(0.3);
            }else
            {
                if (msgrcv(msgid, &kmt, sizeof(kmt.wartosc), 0, 0) == -1){
                    perror("Błąd przy odbieraniu \n");
                    exit(1);
                }
                kom--;
                QString tip=kmt.wartosc;
                QTextEdit *info3=this->ui->received;
                info3->insertPlainText(tip);
                QString tip1="\n";
                QTextEdit *info4=this->ui->received;
                info4->insertPlainText(tip1);
                info4->moveCursor(QTextCursor::End);
                pthread_mutex_unlock(&mutex);
            }

        }
        pthread_cancel(read_thread);



    }
    else
    {
        QString tip="\nYou are connected\n";
        QTextEdit *info=this->ui->received;
        info->insertPlainText(tip);
        tip="";
        QTextEdit *info2=this->ui->message;
        info2->setPlainText(tip);
    }
}

void MainWindow::on_disconnect_clicked()
{
    if(now_connected==1)
    {
        //close the socket
        printf("\nBye\n");
        ::close(connection_socket_descriptor);


        now_connected=0;
    }
    else
    {
        QString tip="\nYou are not connected\n";
        QTextEdit *info=this->ui->received;
        info->insertPlainText(tip);
        info->moveCursor(QTextCursor::End);
        tip="";
        QTextEdit *info2=this->ui->message;
        info2->setPlainText(tip);

    }
}

void MainWindow::on_send_clicked()
{
    //writer function
    if(now_connected==1)
    {
        char write_to[BUFF];
        QString messg=this->ui->message->toPlainText();
        QByteArray messg_helper = messg.toLocal8Bit();
        const char *messg_nearly_ready = messg_helper.data();
        char *messg_ready=(char*)messg_nearly_ready;
        strcpy(write_to,messg_ready);


        int chars_write=write(connection_socket_descriptor, write_to, BUFF);
        if (chars_write < 0)
        {
            perror("problem with writing");
            exit(1);
        }
        else{
            if (strcmp(write_to, "exit") == 0)
            {
                on_disconnect_clicked();
            }else{
                //Tworzenie tymczasowej listy do generowania pełnego komunikatu
                int temp_buff_size = strlen(write_to) + strlen(nick)+2;
                char* temp_concat = new char[temp_buff_size];
                strcpy(temp_concat, nick);
                strcat(temp_concat,": ");
                strcat(temp_concat,write_to);
                strcat(temp_concat,"\n");
                QString tip=temp_concat;
                QTextEdit *info=this->ui->received;
                info->insertPlainText(tip);
                info->moveCursor(QTextCursor::End);

                tip="";
                QTextEdit *info2=this->ui->message;
                info2->setPlainText(tip);

            }
        }


    }
    else
    {
        QString tip="\nYou are not connected\n";
        QTextEdit *info=this->ui->received;
        info->insertPlainText(tip);
        info->moveCursor(QTextCursor::End);
        tip="";
        QTextEdit *info2=this->ui->message;
        info2->setPlainText(tip);

    }  

}
