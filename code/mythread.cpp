#include "mythread.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMutex>

extern QMutex mutex;
extern void showLab();
extern mythread *thread1;



mythread::mythread(QObject *parent) : QThread(parent)
{

}
int number= 0;

void mythread::run()
{
        showLab();
}
