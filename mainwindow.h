#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>

namespace Ui {
class MainWindow;
}

class Worker; // forward declaration
class Status; // forward declaration

// main window class
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_startButton_clicked();
    void threadDone();
    void on_stopButton_clicked();

private:
    Ui::MainWindow *ui;
    QThread* thread;
    Status* quit;
    Worker* worker;
};

// worker class containing code to run in thread
class Worker : public QObject
{
    Q_OBJECT

public:
    Worker( Status *quit_status );
    ~Worker();

private:
    Status *quit;

signals:
    void valueChanged(const QString &value);
    void finished();

public slots:
    void doWork();
};

// class to keep track of the status of another class outside of it
class Status : public QObject
{
    Q_OBJECT

public:
    bool getStatus();

public slots:
    void setStatus( bool status );
    void setTrue();

private:
    bool mStatus;
    QMutex mMutex;
};

#endif // MAINWINDOW_H
