#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_connect_clicked();

    void on_disconnect_clicked();

    void on_send_clicked();

    //int connect(int socket, const struct sockaddr *address,int address_len);

    //int close(int fd);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
