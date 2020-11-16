#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QThread>

class mythread : public QThread
{
    Q_OBJECT
public:
    explicit mythread(QObject *parent = nullptr);
    void run();//protect 虚函数,线程处理函数,不能直接调用，需要通过start函数进行间接调用

signals:

};

#endif // MYTHREAD_H
