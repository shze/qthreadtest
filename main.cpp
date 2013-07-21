#include "mainwindow.h"
#include <QApplication>

// Description of this project:
// Writing a worker object that can be placed in a QThread and run is relatively easy. What is not
// easy is the case where the worker has to be stopped while running, because of user interaction
// e.g. stop button or application close. This project tries to be a complete example for this case.
// current limitation is that the code and ui only allows one thread, but I expect it could be easily
// extended to allow more (famous last words).

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}
