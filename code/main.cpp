/***************************************************************
* Filename     : main.cpp
* Description   : Build window application objects, add UI and
*                 functions, and create database and total table
*                 at the same time
* Version      : 1.0
* History       :
* penghongran 2020-11-30  finished
**************************************************************/
#include "mainwindow.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>

int main(int argc, char *argv[])
{
    //Create a memory analysis object
    QApplication a(argc, argv);

    //Establish a connection to the database
    QSqlDatabase database;
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("Working_database.db");
    if (!database.open())
    {
        qDebug() << "Error: Failed to connect database." << database.lastError();
    }
    else
    {
        qDebug() << "Succeed to connect database." ;
    }

    //Add database complete table
    QSqlQuery query;
    query.exec("PRAGMA synchronous = OFF");
    query.exec("create table malloc_and_free(id int primary key,"
               " date varchar, type varchar, malloc_address varchar,"
               "malloc_size int,  start_and_end_address varchar)");

    //Draw UI and functions for memory analysis objects
    MainWindow w;
    w.show();

    return a.exec();
}
