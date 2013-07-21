#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTimer>
#include <QPlainTextEdit>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    thread(NULL),
    quit(NULL),
    worker(NULL)
{
    ui->setupUi(this);
    ui->plainTextEdit->appendPlainText( "done ui setup" );
}

MainWindow::~MainWindow()
{
    // if thread is still running, abort it. for some reason the application seems to close alright
    // without leaving running threads behind, even when it is closed while a thread is running and
    // this piece of code is missing.
    if( ! quit->getStatus()) {
        on_stopButton_clicked();
    }

    delete ui;
}

void MainWindow::on_startButton_clicked()
{
    // start button is only enabled if thread is not running

    ui->plainTextEdit->appendPlainText( "startButton -->" );
    ui->startButton->setEnabled( false);
    ui->stopButton->setEnabled( true);

    // http://mayaposch.wordpress.com/2011/11/01/how-to-really-truly-use-qthreads-the-full-explanation/
    // this is a good example, the problem is, that a running thread cannot be stopped, and starting
    // the worker again starts another thread, so that multiple threads can run in parallel
    // note 1: extending this example worker with an abort() is not possible, as we would need to check
    // if the worker still exists before calling abort(); checking if the thread still exists seem not
    // possible: if thread ptr is not null, isRunning() can return true or false, or it can crash the
    // application with a misaligned ptr error; this was tried before here:
    // http://fabienpn.wordpress.com/2013/05/01/qt-thread-simple-and-stable-with-sources/
    // but I cannot get it to run without crashes.
    // note 2: removing the deleteLater() calls seems to cause problems because calling
    // QMetaObject::invokeMethod() cannot find the slots anymore
    // note 3: possible way to go implemented below: keep automatic delete with connect, move the quit state
    // of the worker into a separate object under control of the main thread, so that the status can be always
    // checked. this object is also used to abort the worker on user's request. the idea for note 3 comes
    // from: http://stackoverflow.com/questions/4897912/how-to-properly-interrupt-a-qthread-infinite-loop
    thread = new QThread;
    quit = new Status;
    worker = new Worker( quit);
    worker->moveToThread(thread);
    connect(worker, SIGNAL(valueChanged(QString)), ui->plainTextEdit, SLOT(appendPlainText(QString)));
    connect(thread, SIGNAL(started()), worker, SLOT(doWork())); // start work when thread starts
    connect(worker, SIGNAL(finished()), quit, SLOT(setTrue())); // set quit to true when work is done
    connect(worker, SIGNAL(finished()), this, SLOT(threadDone())); // call threadDone() to clean up
    connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}

// this is called, if the thread finishes without user clicking on the stop button
void MainWindow::threadDone()
{
    // disable stop button
    ui->plainTextEdit->appendPlainText( "stopButton || via threadDone()" );
    ui->stopButton->setEnabled( false);

    // enable start button
    ui->startButton->setEnabled( true);
}

void MainWindow::on_stopButton_clicked()
{
    // problem: while the user clicks the stop button and this methods processes this event, the thread
    // may finish and call the threadDone() method.
    // solution: use Status which uses a mutex-guarded variable internally to store the state of theworker

    // disable stop button
    ui->plainTextEdit->appendPlainText( "stopButton || via user!" );
    ui->stopButton->setEnabled( false);

    // abort the worker
    quit->setTrue();
    // wait is blocking, so set timeout for wait and process events until wait returns true when the thread
    // is done
    while( ! thread->wait( 200)) {
        QCoreApplication::processEvents();
    }

    // no need to reenable the start button, because threadDone() will be called when the worker finishes
    // and it will do the reenabling
}

Worker::Worker( Status *quit_status ) :
    quit( quit_status)
{
    // object must be constructed in order to be able to connect signals, so any emit in the constructor
    // cannot be received by any object. so, there's no point in emitting anything here.
}

Worker::~Worker()
{
    emit valueChanged( "Worker destructor" );
}

void Worker::doWork()
{
    // important: no ui stuff in here: this should run in a separate thread, ui changes can only be
    // made from the main (ui) thread!

    emit valueChanged( "Worker::doWork() start" );
    quit->setStatus( false);

    for (int i = 0; i < 10; i ++) {
        if ( quit->getStatus() ) {
            emit valueChanged( "Worker::doWork() loop abort!");
            break;
        }

        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(1000);
        QEventLoop loop;
        connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        timer.start();
        loop.exec();

        emit valueChanged( "Worker::doWork() loop " + QString::number(i) );
    }

    emit valueChanged( "Worker::doWork() loop done");
    emit finished();
}

bool Status::getStatus()
{
    mMutex.lock();
    bool status = mStatus;
    mMutex.unlock();

    return status;
}

void Status::setStatus( bool status )
{
    mMutex.lock();
    mStatus = status;
    mMutex.unlock();
}

void Status::setTrue()
{
    setStatus( true);
}
