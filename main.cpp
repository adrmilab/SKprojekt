#include "mainwindow.h"
#include <QApplication>

#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

#include <iostream>
#include <csignal>
#include <cstdlib>
#include <cstring>

#define BUFF 1024
#define NUM_THREADS 5
#define PORT_NUMBER 1234
#define NICK 16



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    //signal(SIGINT, to_exit);

    return a.exec();
}


