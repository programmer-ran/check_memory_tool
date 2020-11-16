#include "mainwindow.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    QFile delete_file("./MyDataBase.db");
    if (delete_file.exists())
    {
        qDebug()<< "数据库查找成功";

//        if(delete_file.remove())
//        {
//            qDebug() << "删除成功";
//        }
//        else
//        {
//            qDebug() << "删除失败";
//            QMessageBox::about(NULL, "提示", "未初始化成功");
//            return NULL;
//        }
    }
    else
    {
        qDebug()<< "数据库查找失败";
    }
//
    //建立并打开数据库
    QSqlDatabase database;
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("MyDataBase.db");
    if (!database.open())
    {
        qDebug() << "Error: Failed to connect database." << database.lastError();
    }
    else
    {
        qDebug() << "Succeed to connect database." ;
    }

    QSqlQuery query;
    query.exec("PRAGMA synchronous = OFF");
    query.exec("create table malloc_and_free(malloc_and_free_id int primary key, date varchar, type varchar, malloc_address varchar, malloc_size int)");

    MainWindow w;
    w.show();

    return a.exec();
}
