#include "mainwindow.h"
#include <QApplication>
#include <QDateTime>

int main(int argc, char *argv[])
{
    QDateTime current_date_time =QDateTime::currentDateTime();
    QString current_date = current_date_time.toString("yyyyMMdd");
    QString deadline = "20230720";
    QApplication a(argc, argv);
    MainWindow w;
    if(deadline.toInt() - current_date.toInt() < 0){
        w.Overdue();
    }else {
        w.move(400,200);
        w.show();
    }
    return a.exec();
}
