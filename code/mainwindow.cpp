#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QPushButton>
#include <QByteArray>
#include <QTextEdit>
#include <QTextBlock>
#include <QMutex>
#include <QSqlQuery>
#include <QDateTime>
#include <stdlib.h>
#include "mythread.h"
#include <synchapi.h>

/**************Global Data***********/
mythread *thread1;
QMutex mutex;

QString start_memory_address = "";
QString end_memory_address = "";


int cur_line = 0;
int memory_sum = 0;
int free_sum = 0;

MainWindow* instance;
extern void database_search_malloc(void);


/**************************************************************
 * Function Name : MainWindow
 * Description   : Construct function MainWindow
 * Parameters    : ...
 * Returns       : MainWindow
 **************************************************************/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_chart_1(new QChart),
    isStopping(false)
{
    ui->setupUi(this);

    this->setWindowTitle("内存分析编辑器");

    //连接信号和槽
    QObject::connect(&serial, &QSerialPort::readyRead, this, &MainWindow::serialPort_readyRead);

//    QObject::connect(ui->detach_count_Edit, SIGNAL(textChanged(const QString &)), ui->detach_value_Edit, SLOT(update_detach_value()));

    QObject::connect(ui->detach_count_Box, SIGNAL(currentTextChanged(QString)), this, SLOT(update_detach_value(void)));

//        //开启线程
//        thread1 = new mythread(this);
//        thread1->start();

//        //销毁程序时释放线程试用资源
//        connect(this, &MainWindow::destroyed, this, &MainWindow::stopThread);

//        //构建内调函数，供其它线程调本类中成员
//        instance = this;

    //波特率默认选择下拉第四项：115200
    ui->baudrateBox->setCurrentIndex(4);

    ui->progressBar->setRange(0,100);
    ui->progressBar->setValue(0);


//    ui->detach_count_Box->setCurrentIndex(0);


    ui->detach_count_Box->addItem(QWidget::tr("1"));
    ui->detach_count_Box->addItem(QWidget::tr("2"));
    ui->detach_count_Box->addItem(QWidget::tr("3"));
    ui->detach_count_Box->addItem(QWidget::tr("4"));
    ui->detach_count_Box->addItem(QWidget::tr("5"));
    ui->detach_count_Box->addItem(QWidget::tr("6"));
    ui->detach_count_Box->addItem(QWidget::tr("7"));
    ui->detach_count_Box->addItem(QWidget::tr("8"));
    ui->detach_count_Box->addItem(QWidget::tr("9"));
    ui->detach_count_Box->addItem(QWidget::tr("10"));

//    ui->analyseButton->hide();

    QSqlQuery query;
    query.exec("select start_and_end_address from malloc_and_free where id == 1");

    if(query.next())
    {
       start_memory_address = query.value(0).toString();
    }

    query.exec("select start_and_end_address from malloc_and_free where id == 2");

    if(query.next())
    {
       end_memory_address = query.value(0).toString();
    }



}

/**************************************************************
 * Function Name : serialPort_readyRead
 * Description   : When the serial port receives information,
 *                 the function responds to the buffer
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::serialPort_readyRead()
{
    //从接收缓冲区中读取数据
    QByteArray buffer = serial.readAll();
    //从界面中读取以前收到的数据

    QString recv;
    QString arr;
    int last_number = 0;

    recv = QString(buffer);

    ui->recvTextEdit->insertPlainText(recv);
    ui->recvTextEdit->moveCursor(QTextCursor::End);

    last_number = ui->recvTextEdit->document()->lineCount();
//    qDebug()<< "last_number" << last_number;

    data_get();

    //清空以前的显示
    if(last_number >= 100000)
    {
        arr = ui->recvTextEdit->document()->findBlockByLineNumber(last_number-1).text();

        if((arr.length() == 0))
        {
            ui->recvTextEdit->clear();
            ui->analyseTextEdit->clear();
            cur_line = 0;
        }
    }

}


//建立自身调用
MainWindow *MainWindow::getInstance()
{
    return instance;
}
//普通函数
void showLab()
{
    MainWindow::getInstance()->data_get();
}


/**************************************************************
 * Function Name : data_get
 * Description   : Analyze the obtained serial port data
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::data_get()
{
    //从左边显示提取出文本内容
    int i = 0;
    int j = 0;
    int k = 0;
    int last_line = 0;
    static int id = 1;

    QSqlQuery query;
    QSqlQuery query1;
    QString malloc_text = "=m ";
    QString free_text = "=f ";
    QString memory_text = "=mem ";
    QString malloc_memmory_address;
    QString string_malloc_memory_size;
    int malloc_memory_size;
    QString free_memory_address;
    QString arr;
    QDateTime current_date_time =QDateTime::currentDateTime();
    QString current_date =current_date_time.toString("yyyy.MM.dd hh:mm:ss.zzz");

    last_line = (ui->recvTextEdit->document()->lineCount());


//    qDebug()<< "cur_line" << cur_line;
//    qDebug()<< "last_line" << last_line;

    while(cur_line<last_line)
    {
        arr = ui->recvTextEdit->document()->findBlockByLineNumber(cur_line-1).text();

        cur_line++;

        if((arr.indexOf(malloc_text,i)!= -1))
        {
            //qDebug()<< "malloc出的字在文件中位置是第几个字节" <<arr.indexOf(malloc_text, i)+1;
            malloc_memmory_address = arr.mid(arr.indexOf(malloc_text, i)+3, 8);
            //qDebug()<< "malloc提取出的字为" <<malloc_memmory_address;
            string_malloc_memory_size = arr.mid(arr.indexOf(malloc_text, i)+12, 7);
            malloc_memory_size = string_malloc_memory_size.toInt(NULL, 16);
            //qDebug()<< "malloc的大小是" <<malloc_memory_size;

            ui->analyseTextEdit->append("malloc为 "+malloc_memmory_address+"");
            //ui->analyseTextEdit->appendPlainText("\n");
            ui->analyseTextEdit->append("malloc大小是 "+string_malloc_memory_size+"");
            //ui->analyseTextEdit->appendPlainText("\n");

            query.prepare("insert into malloc_and_free values(:id,'"+current_date+"','malloc','"+malloc_memmory_address+"', :malloc_memory_size, '')");
            query.bindValue(":id",id);
            query.bindValue(":malloc_memory_size", malloc_memory_size);
            query.exec();

            id++;
            //i = arr.indexOf(malloc_text, i) + 1;
        }

        else if((arr.indexOf(free_text,j)!= -1))
        {
            //qDebug()<< "malloc出的字在文件中位置是第几个字节" <<arr.indexOf(malloc_text, i)+1;
            free_memory_address = arr.mid(arr.indexOf(free_text, j)+3, 8);
            //qDebug()<< "malloc的大小是" <<malloc_memory_size;
            ui->analyseTextEdit->append("free为 "+free_memory_address+"");

            query.prepare("insert into malloc_and_free values(:id,'"+current_date+"','free','"+free_memory_address+"', 0, '')");
            query.bindValue(":id",id);
            query.exec();//

            id++;
        }


        else if((arr.indexOf(memory_text,k)!= -1))
        {
            start_memory_address = arr.mid(arr.indexOf(memory_text, k)+5, 8);
            end_memory_address = arr.mid(arr.indexOf(memory_text, k)+14, 9);

            query.exec("update malloc_and_free set start_and_end_address = '"+start_memory_address+"' where id = 1");

            query.exec("update malloc_and_free set start_and_end_address = '"+end_memory_address+"' where id = 2");

            qDebug()<< "start_memory_address" <<start_memory_address;
            qDebug()<< "end_memory_address" <<end_memory_address;
        }
//        qDebug() << "id" << id;
    }
}


/**************************************************************
 * Function Name : stopThread
 * Description   : Release the resources required by the thread
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::stopThread()
{
    thread1->quit();

    thread1->wait();

    qDebug()<<"quit success";
}

/**************************************************************
 * Function Name : on_searchButton_clicked
 * Description   : Find out which serial ports are available
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::on_searchButton_clicked()
{
    ui->portNameBox->clear();
    //通过QSerialPortInfo查找可用串口
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->portNameBox->addItem(info.portName());
    }
}

int excel_row_count = 0;
int excel_column_count = 5;
/**************************************************************
 * Function Name : on_openButton_clicked
 * Description   : Open the serial port for data reception
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::on_openButton_clicked()
{
    if(ui->openButton->text()==QString("打开串口"))//
    {
        //设置串口名
        serial.setPortName(ui->portNameBox->currentText());
        //设置波特率
        serial.setBaudRate(ui->baudrateBox->currentText().toInt());
        //设置数据位数
        switch(ui->dataBitsBox->currentIndex())
        {
        case 8: serial.setDataBits(QSerialPort::Data8); break;
        default: break;
        }
        //设置奇偶校验
        switch(ui->ParityBox->currentIndex())
        {
        case 0: serial.setParity(QSerialPort::NoParity); break;
        default: break;
        }
        //设置停止位
        switch(ui->stopBitsBox->currentIndex())
        {
        case 1: serial.setStopBits(QSerialPort::OneStop); break;
        case 2: serial.setStopBits(QSerialPort::TwoStop); break;
        default: break;
        }
        //设置流控制
        serial.setFlowControl(QSerialPort::NoFlowControl);

        //打开串口
        if(!serial.open(QIODevice::ReadWrite))
        {
            QMessageBox::about(NULL, "提示", "无法打开串口！");
            return;
        }

        //下拉菜单控件失能
        ui->portNameBox->setEnabled(false);
        ui->baudrateBox->setEnabled(false);
        ui->dataBitsBox->setEnabled(false);
        ui->ParityBox->setEnabled(false);
        ui->stopBitsBox->setEnabled(false);
        ui->searchButton->setEnabled(false);
        ui->clearButton->setEnabled(false);
        ui->analyseButton->setEnabled(false);
        ui->detachButton->setEnabled(false);
        ui->saveButton->setEnabled(false);

        ui->openButton->setText(QString("关闭串口"));




    }
    else
    {
        //关闭串口
        serial.close();

        QSqlQuery query;
        query.exec("create index malloc_size_index on malloc_and_free(malloc_size)");

        query.exec("create index type_index on malloc_and_free(type)");


        //下拉菜单控件使能
        ui->portNameBox->setEnabled(true);
        ui->baudrateBox->setEnabled(true);
        ui->dataBitsBox->setEnabled(true);
        ui->ParityBox->setEnabled(true);
        ui->stopBitsBox->setEnabled(true);
        ui->searchButton->setEnabled(true);
        ui->clearButton->setEnabled(true);
        ui->analyseButton->setEnabled(true);
        ui->detachButton->setEnabled(true);
        ui->saveButton->setEnabled(true);

        ui->analyseTextEdit->moveCursor(QTextCursor::Start);
        ui->analyseTextEdit->moveCursor(QTextCursor::End);

        ui->openButton->setText(QString("打开串口"));
    }
}


/**************************************************************
 * Function Name : on_clearButton_clicked
 * Description   : Clear the results of memory analysis
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::on_clearButton_clicked()
{
    QSqlQuery query;

    query.exec("drop table one_detach_memory");
    query.exec("drop table one_unfree_memory");
    query.exec("drop table two_detach_memory");
    query.exec("drop table two_unfree_memory");
    query.exec("drop table three_detach_memory");
    query.exec("drop table three_unfree_memory");
    query.exec("drop table memory_pool_table");
    query.exec("drop view malloc");
    query.exec("drop view free");
    query.exec("drop view one_unfree");
//    query.exec("drop index malloc_size_index");
//    query.exec("drop index type_index");

    ui->recvTextEdit->clear();
    ui->analyseTextEdit->clear();
    ui->detach_textEdit->clear();

    if(ui)

//    if(series0 != NULL)
//    {
//        delete series0;
//        series0 = NULL;
//        qDebug()<< "series0";
//    }
//    if(series1 != NULL)
//    {
//        delete series1;
//        series1 = NULL;
//        qDebug()<< "series1";
//    }
//    if(series2 != NULL)
//    {
//        delete series2;
//        series2 = NULL;
//        qDebug()<< "series2";
//    }
//    if(series3 != NULL)
//    {
//        delete series3;
//        series3 = NULL;
//        qDebug()<< "series3";
//    }
//    if(series4 != NULL)
//    {
//        delete series4;
//        series4 = NULL;
//        qDebug()<< "series4";
//    }
//    if(series5 != NULL)
//    {
//        delete series5;
//        series5 = NULL;
//        qDebug()<< "series5";
//    }


    qDebug() << "excel_row_count" << excel_row_count;
    qDebug() << "excel_column_count" << excel_column_count;

    for(int i=1; i< excel_row_count; i++)
    {
        for(int j = 1; j < excel_column_count; j++)
        {
            xlsx.write(i, j, "");
        }
    }
}

/**************************************************************
 * Function Name : on_analyseButton_clicked
 * Description   : Always separate malloc and free from the total table
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::on_analyseButton_clicked()
{
    QSqlQuery query;

//    query.exec("create index type_index on malloc_and_free(type)");

    query.exec("create view malloc as select * from malloc_and_free where type = 'malloc' ");

    query.exec("create view free as select * from malloc_and_free where type = 'free' ");

    QMessageBox::about(NULL, "提示", "分离malloc和free成功");
}

int analyse_memory_count = 0;
int sum_exec_count = 0;



int first_flag = 1;
/**************************************************************
 * Function Name : progress_bar_prepartion
 * Description   : Analyze the usage of the first memory
 * Parameters    : min_size
 *                 max_size
 * Returns       : null
 **************************************************************/
void MainWindow::progress_bar_prepartion(void)
{
    // 添加系列
    ui->progressBar->setValue(0);



    // 构建图表对象
    m_chart_1 = new QChart();


    // 设置默认坐标轴
    m_chart_1->createDefaultAxes();

//    // 设置主题
//    m_chart_1->setTheme(QtCharts::QChart::ChartThemeBlueCerulean);

//    // 设置系列标题
//    m_chart_1->setTitle(QString::fromLocal8Bit("One Chart"));

    // 修改波形图样式
//    change_style();


    if(first_flag == 1)
    {
        first_flag = 0;
        chartView = new ChartView(m_chart_1);
    }
    else
    {
        ui->horizontalLayout_12->removeWidget(chartView);
        delete chartView;
        chartView = new ChartView(m_chart_1);
    }

    chartView->setRenderHint(QPainter::Antialiasing);

//    chartView->setRubberBand(QChartView::RectangleRubberBand);



    ui->horizontalLayout_12->addWidget(chartView);



//        QMessageBox::about(NULL, "提示", "开始分析数据");//
    QMessageBox *box = new QMessageBox(QMessageBox::Information,tr("提示"),tr("开始分析数据"));
    QTimer::singleShot(1000,box,SLOT(accept()));//

    box->exec();//box->show();都可以

//    Sleep(1000);
}



/**************************************************************
 * Function Name : detach_one_memory
 * Description   : Analyze the usage of the first memory
 * Parameters    : min_size
 *                 max_size
 * Returns       : null
 **************************************************************/
void MainWindow::detach_one_memory(int min_size, int max_size)
{
    QSqlQuery query;
    QSqlQuery query1;
    QSqlQuery query2;
    QSqlQuery query3;
    QSqlQuery query4;
    int min_malloc_size;
    int max_malloc_size;
    int id = 1;
    QDateTime  start_time;
    QDateTime  end_time;
    qint64 intervalTimeMS = 0;
    QString date;
    QString type;
    QString malloc_address;
    int malloc_size;
    int malloc_parent_id;
    int exec_count = 0;
    long sum_malloc_size = 0;
    long sum_malloc_count = 0;
    long sum_free_size = 0;
    long sum_free_count = 0;
    int start_time_flag = 1;
    int time_count = 1;
    int search_flag = 1;
    int min_search_value = 0;
    int max_search_value = 2500000;
    int one_max_size = 0;
    QString excel_postion;
    int analyse_time_unit = 0;
    int most_max_size = 0;
    int one_max_id = 0;
    int most_max_id = 0;
    QString one_max_time;
    QString most_max_time;
    int most_malloc_count = 0;
    int most_free_count = 0;

    int sum_table_count = 0;
    int progress_percentage = 0;
    int detach_count = 0;
    int excel_supplement_flag = 1;


    QString analyse_memory_messages;

    detach_count = ui->detach_count_Box->currentText().toInt();

    query4.exec("select id from malloc_and_free order by id desc");

    if(query4.next())
    {
        sum_table_count = query4.value(0).toInt();
    }

    qDebug()<< "sum_table_count" << sum_table_count;
//    sum_table_count = 1716821;

    analyse_time_unit = ui->separate_lineEdit->text().toInt();
    if(analyse_time_unit<=0)
    {
        QMessageBox::about(NULL, "提示", "分离时间单位有误");
        return;
    }

    min_malloc_size = min_size * 1024;
    max_malloc_size = max_size * 1024;

    if(min_malloc_size > max_malloc_size)
    {
        QMessageBox::about(NULL, "提示", "最小参数有误");
        qDebug()<<"min_malloc_size"<<min_malloc_size;
        qDebug()<<"max_malloc_size"<<max_malloc_size;
        return;
    }

    if(min_malloc_size <0 || max_malloc_size < 0)
    {
        QMessageBox::about(NULL, "提示", "最大段参数有误");
        return;
    }

    qDebug()<<"analyse_memory_count"<<analyse_memory_count;

    xlsx.write("A1", "time");
    switch(analyse_memory_count)
    {
    case 0:
        xlsx.write("B1", "one_size");
        break;
    case 1:
        xlsx.write("C1", "two_size");
        break;
    case 2:
        xlsx.write("D1", "three_size");
        break;
    case 3:
        xlsx.write("E1", "four_size");
        break;
    case 4:
        xlsx.write("F1", "five_size");
        break;
    case 5:
        xlsx.write("G1", "six_size");
        break;
    case 6:
        xlsx.write("H1", "seven_size");
        break;
    case 7:
        xlsx.write("I1", "eight_size");
        break;
    case 8:
        xlsx.write("J1", "nine_size");
        break;
    case 9:
        xlsx.write("K1", "ten_size");
        break;
    }

    query3.exec("create table one_unfree_memory(id int primary key, date varchar, type varchar, malloc_address varchar, malloc_size varchar, malloc_and_free_id int)");

    while(search_flag)
    {

        query.prepare("select * from malloc_and_free where id >= :min_search_value and id < :max_search_value");
        query.bindValue(":min_search_value", min_search_value);
        query.bindValue(":max_search_value", max_search_value);
        query.exec();

        query3.exec("BEGIN");

        while(query.next())
        {
            date = query.value(1).toString();
            type = query.value(2).toString();
            malloc_address = query.value(3).toString();
            malloc_size = query.value(4).toInt();
            malloc_parent_id = query.value(0).toInt();

            if(start_time_flag == 1)
            {
                start_time = QDateTime::fromString(date, "yyyy.MM.dd hh:mm:ss.zzz");
                start_time_flag = 0;
            }

            end_time = QDateTime::fromString(date, "yyyy.MM.dd hh:mm:ss.zzz");

            intervalTimeMS = start_time.msecsTo(end_time);

            progress_percentage =  double(sum_exec_count + malloc_parent_id)/double(sum_table_count)/double(detach_count)*100;

            ui->progressBar->setValue(progress_percentage);

            one_max_size = (sum_malloc_size-sum_free_size);

            if(type == "malloc")
            {
                if(malloc_size>=min_malloc_size && malloc_size<max_malloc_size)
                {
                    query3.prepare("insert into one_unfree_memory values(:id, '"+date+"', '"+type+"', '"+malloc_address+"', :malloc_size, :malloc_and_free_id)");
                    query3.bindValue(":id", id);
                    query3.bindValue(":malloc_size", malloc_size);
                    query3.bindValue(":malloc_and_free_id", malloc_parent_id);
                    query3.exec();

                    sum_malloc_size += malloc_size;

                    if(one_max_size<(sum_malloc_size-sum_free_size))
                    {
                        one_max_size = (sum_malloc_size-sum_free_size);
                        one_max_id = id;
                        one_max_time = date;

                    }

                    if(most_max_size<(sum_malloc_size-sum_free_size))
                    {
                        most_max_size = (sum_malloc_size-sum_free_size);
                        most_max_id = id;
                        most_max_time = date;
                        most_malloc_count = sum_malloc_count + 1;
                        most_free_count = sum_free_count + 1;
                    }


                    if(intervalTimeMS>time_count*analyse_time_unit*1000)
                    {
                        excel_supplement_flag = 0;

//                        *set_1 << one_max_size;
//                        categories << QString::number(intervalTimeMS/1000/analyse_time_unit,10);

                        switch(analyse_memory_count)
                        {
                        case 0:
                            excel_postion = QString("B%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;
                        case 1:
                            excel_postion = QString("C%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;
                        case 2:
                            excel_postion = QString("D%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;

                        case 3:
                            excel_postion = QString("E%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;
                        case 4:
                            excel_postion = QString("F%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;
                        case 5:
                            excel_postion = QString("G%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;

                        case 6:
                            excel_postion = QString("H%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;
                        case 7:
                            excel_postion = QString("I%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;
                        case 8:
                            excel_postion = QString("J%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;

                        case 9:
                            excel_postion = QString("K%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;
                        }


                        one_max_size = 0;

                    }

                    id++;
                    sum_malloc_count++;

                }
            }


            if(type == "free")
            {
                query2.exec("select * from one_unfree_memory where malloc_address = '"+malloc_address+"' order by id desc limit 0,1");

                if(query2.next())
                {
                    malloc_size = query2.value(4).toInt();

                    sum_free_size += malloc_size;

                    if(one_max_size<(sum_malloc_size-sum_free_size))
                    {
                        one_max_size = (sum_malloc_size-sum_free_size);
                        one_max_id = id;
                        one_max_time = date;

                    }

                    if(most_max_size<(sum_malloc_size-sum_free_size))
                    {
                        most_max_size = (sum_malloc_size-sum_free_size);
                        most_max_id = id;
                        most_max_time = date;
                        most_free_count = sum_free_count + 1;
                    }

                    if(intervalTimeMS>time_count*analyse_time_unit*1000)
                    {
                        excel_supplement_flag = 0;


                        switch(analyse_memory_count)
                        {
                        case 0:
                            excel_postion = QString("B%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;
                        case 1:
                            excel_postion = QString("C%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;
                        case 2:
                            excel_postion = QString("D%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;

                        case 3:
                            excel_postion = QString("E%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;
                        case 4:
                            excel_postion = QString("F%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;
                        case 5:
                            excel_postion = QString("G%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;

                        case 6:
                            excel_postion = QString("H%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;
                        case 7:
                            excel_postion = QString("I%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;
                        case 8:
                            excel_postion = QString("J%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;

                        case 9:
                            excel_postion = QString("K%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);
                            break;
                        }

                        one_max_size = 0;

                    }

                    id++;
                    sum_free_count++;

                    query3.prepare("delete from one_unfree_memory where id = :delete_id");
                    query3.bindValue(":delete_id", query2.value(0).toInt());
                    query3.exec();

                }
            }


            if(intervalTimeMS>time_count*analyse_time_unit*1000)
            {
                excel_postion = QString("A%1").arg(time_count+1);
                xlsx.write(excel_postion, time_count);

                if(excel_supplement_flag == 1)
                {
//                    *set_1 << one_max_size;
                switch(analyse_memory_count)
                {
                case 0:
                    excel_postion = QString("B%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);
                    break;
                case 1:
                    excel_postion = QString("C%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);
                    break;
                case 2:
                    excel_postion = QString("D%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);
                    break;

                case 3:
                    excel_postion = QString("E%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);
                    break;
                case 4:
                    excel_postion = QString("F%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);
                    break;
                case 5:
                    excel_postion = QString("G%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);
                    break;

                case 6:
                    excel_postion = QString("H%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);
                    break;
                case 7:
                    excel_postion = QString("I%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);
                    break;
                case 8:
                    excel_postion = QString("J%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);
                    break;

                case 9:
                    excel_postion = QString("K%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);
                    break;
                }
                }
                else
                {
                    one_max_size = 0;
                    excel_supplement_flag = 1;
                }

                time_count++;
            }

            if(exec_count % 3000 == 0)
            {
                query3.exec("COMMIT");
                query3.exec("BEGIN");
            }

            exec_count++;
        }
        query3.exec("COMMIT");

        if(exec_count)
        {
            exec_count = 0;
            min_search_value += 2500000;
            max_search_value += 2500000;
        }
        else
        {
            search_flag = 0;
            break;
        }
    }

    sum_exec_count += malloc_parent_id;

    qDebug() << "intervalTimeMS/1000/analyse_time_unit" << intervalTimeMS/1000/analyse_time_unit;
    qDebug() << "time_count" << time_count;


    if(time_count <= (intervalTimeMS/1000/analyse_time_unit))
    {
        while((time_count-1) <= (intervalTimeMS/1000/analyse_time_unit))
        {
            if((time_count-1) != 0)
            {
                switch(analyse_memory_count)
                {
                case 0:
                    excel_postion = QString("B%1").arg(time_count);
                    xlsx.write(excel_postion, 0);
                    break;
                case 1:
                    excel_postion = QString("C%1").arg(time_count);
                    xlsx.write(excel_postion, 0);
                    break;
                case 2:
                    excel_postion = QString("D%1").arg(time_count);
                    xlsx.write(excel_postion, 0);
                    break;
                case 3:
                    excel_postion = QString("E%1").arg(time_count);
                    xlsx.write(excel_postion, 0);
                    break;
                case 4:
                    excel_postion = QString("F%1").arg(time_count);
                    xlsx.write(excel_postion, 0);
                    break;
                case 5:
                    excel_postion = QString("G%1").arg(time_count);
                    xlsx.write(excel_postion, 0);
                    break;
                case 6:
                    excel_postion = QString("H%1").arg(time_count);
                    xlsx.write(excel_postion, 0);
                    break;
                case 7:
                    excel_postion = QString("I%1").arg(time_count);
                    xlsx.write(excel_postion, 0);
                    break;
                case 8:
                    excel_postion = QString("J%1").arg(time_count);
                    xlsx.write(excel_postion, 0);
                    break;
                case 9:
                    excel_postion = QString("K%1").arg(time_count);
                    xlsx.write(excel_postion, 0);
                    break;
                }
            }

            time_count++;
        }

    }

    if(excel_row_count < time_count)
    {
        excel_row_count = time_count;
    }

    analyse_memory_count++;


    analyse_memory_messages = "第" + QString::number(analyse_memory_count) + "段内存使用信息";

    ui->detach_textEdit->append("--------------------------------------------");
    ui->detach_textEdit->append(analyse_memory_messages);
    ui->detach_textEdit->append("malloc的次数");
    ui->detach_textEdit->append(QString::number(sum_malloc_count, 10));
    ui->detach_textEdit->append("malloc的内存大小");
    ui->detach_textEdit->append(QString::number(sum_malloc_size, 10));
    ui->detach_textEdit->append("free的次数");
    ui->detach_textEdit->append(QString::number(sum_free_count, 10));
    ui->detach_textEdit->append("free的内存大小");
    ui->detach_textEdit->append(QString::number(sum_free_size, 10));

    ui->detach_textEdit->append("最大内存占用值");
    ui->detach_textEdit->append(QString::number(most_max_size, 10));
    ui->detach_textEdit->append("最大内存占用值时malloc了多少次");
    ui->detach_textEdit->append(QString::number(most_malloc_count, 10));
    ui->detach_textEdit->append("最大内存占用值时free了多少次");
    ui->detach_textEdit->append(QString::number(most_free_count, 10));
    ui->detach_textEdit->append("在数据库内id号是");
    ui->detach_textEdit->append(QString::number(most_max_id, 10));
    ui->detach_textEdit->append("发生时间是");
    ui->detach_textEdit->append(most_max_time);

    ui->detach_textEdit->append("未释放内存");
    ui->detach_textEdit->append(QString::number(sum_malloc_size-sum_free_size, 10));
    query3.exec("select * from one_unfree_memory");
    while(query3.next())
    {
        ui->detach_textEdit->append("**********************");
        id = query3.value(0).toInt();
        date = query3.value(1).toString();
        type = query3.value(2).toString();
        malloc_address = query3.value(3).toString();
        malloc_size = query3.value(4).toInt();
        malloc_parent_id = query3.value(5).toInt();

        ui->detach_textEdit->append("序号");
        ui->detach_textEdit->append(QString::number(id, 10));
        ui->detach_textEdit->append("日期");
        ui->detach_textEdit->append(date);
        ui->detach_textEdit->append("类型");
        ui->detach_textEdit->append(type);
        ui->detach_textEdit->append("malloc地址");
        ui->detach_textEdit->append(malloc_address);
        ui->detach_textEdit->append("malloc大小");
        ui->detach_textEdit->append(QString::number(malloc_size, 10));
        ui->detach_textEdit->append("源序号");
        ui->detach_textEdit->append(QString::number(malloc_parent_id, 10));

        if(ui->detach_textEdit->document()->lineCount()%100000 == 0)
        {
            ui->detach_textEdit->clear();
        }

    }

    qDebug()<< "intervalTimeMS" << intervalTimeMS;
}


/**************************************************************
 * Function Name : wheelEvent
 * Description   : Zoom in and out of the waveform
 * Parameters    : event
 * Returns       : null
 **************************************************************/
void MainWindow::wheelEvent(QWheelEvent *event)
{
    qreal factor;//缩放因子

    if (event->delta() > 0) {
        factor = 1.1;
    } else {
        factor = double(10.0/11);
    }


//    QRectF rect;
//    rect.setLeft(m_chart_1->plotArea().left()-100);
//    rect.setWidth(m_chart_1->plotArea().width()/factor);
//    rect.setLeft(m_chart_1->plotArea().top()-100);
//    rect.setWidth(m_chart_1->plotArea().height());

//    m_chart_1->zoom(factor);

    m_chart_1->createDefaultAxes();




    //鼠标的当前位置
    QPointF mousePos = mapFromGlobal(QCursor::pos());
    QRectF rect = QRectF(m_chart_1->plotArea().left(),m_chart_1->plotArea().top(),
               m_chart_1->plotArea().width()/factor,m_chart_1->plotArea().height());
    mousePos.setX(m_chart_1->plotArea().center().x());
    mousePos.setY(rect.y());
    rect.moveCenter(mousePos);
    m_chart_1->zoomIn(rect);

    QValueAxis *axisX = dynamic_cast<QValueAxis*>(m_chart_1->axisX());//

    QValueAxis *One_axisX = new QValueAxis();


    One_axisX->setLabelFormat("%#X");

    One_axisX->setMin(axisX->min());

    One_axisX->setMax(axisX->max());

    m_chart_1->setAxisX(One_axisX);

    m_chart_1->axisY()->setVisible(false);

    m_chart_1->update();


    QWidget::wheelEvent(event);
}


/**************************************************************
 * Function Name : update_detach_value
 * Description   : Draw waveform graph
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::update_detach_value()
{
    int detach_count = 0;

    detach_count = ui->detach_count_Box->currentText().toInt();

    qDebug() << "detach_count" << detach_count;

    switch(detach_count)
    {
    case 1:
        ui->one_left_lineedit->setText("0");
        ui->one_right_lineedit->setText("4");
        ui->separate_lineEdit->setText("1");
        ui->widget_one->show();
        ui->widget_two->hide();
        ui->widget_two->hide();
        ui->widget_three->hide();
        ui->widget_four->hide();
        ui->widget_five->hide();
        ui->widget_six->hide();
        ui->widget_seven->hide();
        ui->widget_eight->hide();
        ui->widget_nine->hide();
        ui->widget_ten->hide();

        ui->two_left_lineedit->clear();
        ui->two_right_lineedit->clear();
        ui->three_left_lineedit->clear();
        ui->three_right_lineedit->clear();
        ui->four_left_lineedit->clear();
        ui->four_right_lineedit->clear();
        ui->five_left_lineedit->clear();
        ui->five_right_lineedit->clear();
        ui->six_left_lineedit->clear();
        ui->six_right_lineedit->clear();
        ui->seven_left_lineedit->clear();
        ui->seven_right_lineedit->clear();
        ui->eight_left_lineedit->clear();
        ui->eight_right_lineedit->clear();
        ui->nine_left_lineedit->clear();
        ui->nine_right_lineedit->clear();
        ui->ten_left_lineedit->clear();
        ui->ten_right_lineedit->clear();
        break;

    case 2:
        ui->one_left_lineedit->setText("0");
        ui->one_right_lineedit->setText("4");
        ui->two_left_lineedit->setText("4");
        ui->two_right_lineedit->setText("64");
        ui->separate_lineEdit->setText("1");
        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->hide();
        ui->widget_four->hide();
        ui->widget_five->hide();
        ui->widget_six->hide();
        ui->widget_seven->hide();
        ui->widget_eight->hide();
        ui->widget_nine->hide();
        ui->widget_ten->hide();

        ui->three_left_lineedit->clear();
        ui->three_right_lineedit->clear();
        ui->four_left_lineedit->clear();
        ui->four_right_lineedit->clear();
        ui->five_left_lineedit->clear();
        ui->five_right_lineedit->clear();
        ui->six_left_lineedit->clear();
        ui->six_right_lineedit->clear();
        ui->seven_left_lineedit->clear();
        ui->seven_right_lineedit->clear();
        ui->eight_left_lineedit->clear();
        ui->eight_right_lineedit->clear();
        ui->nine_left_lineedit->clear();
        ui->nine_right_lineedit->clear();
        ui->ten_left_lineedit->clear();
        ui->ten_right_lineedit->clear();
        break;

    case 3:
        ui->one_left_lineedit->setText("0");
        ui->one_right_lineedit->setText("4");
        ui->two_left_lineedit->setText("4");
        ui->two_right_lineedit->setText("64");
        ui->three_left_lineedit->setText("64");
        ui->three_right_lineedit->setText("30000");
        ui->separate_lineEdit->setText("1");
        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->show();
        ui->widget_four->hide();
        ui->widget_five->hide();
        ui->widget_six->hide();
        ui->widget_seven->hide();
        ui->widget_eight->hide();
        ui->widget_nine->hide();
        ui->widget_ten->hide();

        ui->four_left_lineedit->clear();
        ui->four_right_lineedit->clear();
        ui->five_left_lineedit->clear();
        ui->five_right_lineedit->clear();
        ui->six_left_lineedit->clear();
        ui->six_right_lineedit->clear();
        ui->seven_left_lineedit->clear();
        ui->seven_right_lineedit->clear();
        ui->eight_left_lineedit->clear();
        ui->eight_right_lineedit->clear();
        ui->nine_left_lineedit->clear();
        ui->nine_right_lineedit->clear();
        ui->ten_left_lineedit->clear();
        ui->ten_right_lineedit->clear();
        break;

    case 4:
        ui->one_left_lineedit->setText("");
        ui->one_right_lineedit->setText("");
        ui->two_left_lineedit->setText("");
        ui->two_right_lineedit->setText("");
        ui->three_left_lineedit->setText("");
        ui->three_right_lineedit->setText("");
        ui->separate_lineEdit->setText("");
        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->show();
        ui->widget_four->show();
        ui->widget_five->hide();
        ui->widget_six->hide();
        ui->widget_seven->hide();
        ui->widget_eight->hide();
        ui->widget_nine->hide();
        ui->widget_ten->hide();

        ui->five_left_lineedit->clear();
        ui->five_right_lineedit->clear();
        ui->six_left_lineedit->clear();
        ui->six_right_lineedit->clear();
        ui->seven_left_lineedit->clear();
        ui->seven_right_lineedit->clear();
        ui->eight_left_lineedit->clear();
        ui->eight_right_lineedit->clear();
        ui->nine_left_lineedit->clear();
        ui->nine_right_lineedit->clear();
        ui->ten_left_lineedit->clear();
        ui->ten_right_lineedit->clear();
        break;

    case 5:
        ui->one_left_lineedit->setText("");
        ui->one_right_lineedit->setText("");
        ui->two_left_lineedit->setText("");
        ui->two_right_lineedit->setText("");
        ui->three_left_lineedit->setText("");
        ui->three_right_lineedit->setText("");
        ui->separate_lineEdit->setText("");
        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->show();
        ui->widget_four->show();
        ui->widget_five->show();
        ui->widget_six->hide();
        ui->widget_seven->hide();
        ui->widget_eight->hide();
        ui->widget_nine->hide();
        ui->widget_ten->hide();

        ui->six_left_lineedit->clear();
        ui->six_right_lineedit->clear();
        ui->seven_left_lineedit->clear();
        ui->seven_right_lineedit->clear();
        ui->eight_left_lineedit->clear();
        ui->eight_right_lineedit->clear();
        ui->nine_left_lineedit->clear();
        ui->nine_right_lineedit->clear();
        ui->ten_left_lineedit->clear();
        ui->ten_right_lineedit->clear();
        break;


    case 6:
        ui->one_left_lineedit->setText("");
        ui->one_right_lineedit->setText("");
        ui->two_left_lineedit->setText("");
        ui->two_right_lineedit->setText("");
        ui->three_left_lineedit->setText("");
        ui->three_right_lineedit->setText("");
        ui->separate_lineEdit->setText("");
        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->show();
        ui->widget_four->show();
        ui->widget_five->show();
        ui->widget_six->show();
        ui->widget_seven->hide();
        ui->widget_eight->hide();
        ui->widget_nine->hide();
        ui->widget_ten->hide();

        ui->seven_left_lineedit->clear();
        ui->seven_right_lineedit->clear();
        ui->eight_left_lineedit->clear();
        ui->eight_right_lineedit->clear();
        ui->nine_left_lineedit->clear();
        ui->nine_right_lineedit->clear();
        ui->ten_left_lineedit->clear();
        ui->ten_right_lineedit->clear();
        break;


    case 7:
        ui->one_left_lineedit->setText("");
        ui->one_right_lineedit->setText("");
        ui->two_left_lineedit->setText("");
        ui->two_right_lineedit->setText("");
        ui->three_left_lineedit->setText("");
        ui->three_right_lineedit->setText("");
        ui->separate_lineEdit->setText("");
        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->show();
        ui->widget_four->show();
        ui->widget_five->show();
        ui->widget_six->show();
        ui->widget_seven->show();
        ui->widget_eight->hide();
        ui->widget_nine->hide();
        ui->widget_ten->hide();

        ui->eight_left_lineedit->clear();
        ui->eight_right_lineedit->clear();
        ui->nine_left_lineedit->clear();
        ui->nine_right_lineedit->clear();
        ui->ten_left_lineedit->clear();
        ui->ten_right_lineedit->clear();
        break;


    case 8:
        ui->one_left_lineedit->setText("");
        ui->one_right_lineedit->setText("");
        ui->two_left_lineedit->setText("");
        ui->two_right_lineedit->setText("");
        ui->three_left_lineedit->setText("");
        ui->three_right_lineedit->setText("");
        ui->separate_lineEdit->setText("");
        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->show();
        ui->widget_four->show();
        ui->widget_five->show();
        ui->widget_six->show();
        ui->widget_seven->show();
        ui->widget_eight->show();
        ui->widget_nine->hide();
        ui->widget_ten->hide();

        ui->nine_left_lineedit->clear();
        ui->nine_right_lineedit->clear();
        ui->ten_left_lineedit->clear();
        ui->ten_right_lineedit->clear();
        break;

    case 9:
        ui->one_left_lineedit->setText("");
        ui->one_right_lineedit->setText("");
        ui->two_left_lineedit->setText("");
        ui->two_right_lineedit->setText("");
        ui->three_left_lineedit->setText("");
        ui->three_right_lineedit->setText("");
        ui->separate_lineEdit->setText("");
        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->show();
        ui->widget_four->show();
        ui->widget_five->show();
        ui->widget_six->show();
        ui->widget_seven->show();
        ui->widget_eight->show();
        ui->widget_nine->show();
        ui->widget_ten->hide();

        ui->ten_left_lineedit->clear();
        ui->ten_right_lineedit->clear();
        break;

    case 10:
        ui->one_left_lineedit->setText("");
        ui->one_right_lineedit->setText("");
        ui->two_left_lineedit->setText("");
        ui->two_right_lineedit->setText("");
        ui->three_left_lineedit->setText("");
        ui->three_right_lineedit->setText("");
        ui->separate_lineEdit->setText("");
        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->show();
        ui->widget_four->show();
        ui->widget_five->show();
        ui->widget_six->show();
        ui->widget_seven->show();
        ui->widget_eight->show();
        ui->widget_nine->show();
        ui->widget_ten->show();
        break;
    }

}


/**************************************************************
 * Function Name : on_detachButton_clicked
 * Description   : Draw waveform graph
 * Parameters    : null
 * Returns       : null
 **************************************************************///
void MainWindow::on_detachButton_clicked()
{
    on_clearButton_clicked();

    ui->portNameBox->setEnabled(false);
    ui->baudrateBox->setEnabled(false);
    ui->dataBitsBox->setEnabled(false);
    ui->ParityBox->setEnabled(false);
    ui->stopBitsBox->setEnabled(false);
    ui->openButton->setEnabled(false);
    ui->searchButton->setEnabled(false);
    ui->clearButton->setEnabled(false);
    ui->analyseButton->setEnabled(false);
    ui->detachButton->setEnabled(false);
    ui->saveButton->setEnabled(false);

    progress_bar_prepartion();

    int detach_count = 0;
    int min_malloc_size = 0;
    int max_malloc_size = 0;
    QSqlQuery query;
    QString malloc_address;
    QString test_address;
    int malloc_size = 0;
    uint new_start_memory_address = 0;
    uint new_end_memory_address = 0;

    int one_flag = 0;
    int two_flag = 0;
    int three_flag = 0;
    int four_flag = 0;
    int five_flag = 0;
    int six_flag = 0;
    int seven_flag = 0;
    int eight_flag = 0;
    int nine_flag = 0;
    int ten_flag = 0;


    // 添加系列
    series0 = new QLineSeries();
    series1 = new QLineSeries();
    series2 = new QLineSeries();
    series3 = new QLineSeries();
    series4 = new QLineSeries();
    series5 = new QLineSeries();
    series6 = new QLineSeries();
    series7 = new QLineSeries();
    series8 = new QLineSeries();
    series9 = new QLineSeries();
    series10 = new QLineSeries();
    series11 = new QLineSeries();
    series12 = new QLineSeries();
    series13 = new QLineSeries();
    series14 = new QLineSeries();
    series15 = new QLineSeries();
    series16 = new QLineSeries();
    series17 = new QLineSeries();
    series18 = new QLineSeries();
    series19 = new QLineSeries();
    series20 = new QLineSeries();
    series21 = new QLineSeries();


    // 构建图表对象
    m_chart_1 = new QChart();


/***********内存池最初显示状态***************/
    //获取内存池点坐标
    if((ui->start_address_lineEdit->text() == NULL && ui->end_address_lineEdit->text() == NULL))
    {
        new_start_memory_address = start_memory_address.toUInt(NULL, 16);
        new_end_memory_address = end_memory_address.toUInt(NULL, 16);

        *series0 << QPointF(start_memory_address.toUInt(NULL, 16), 100) << QPointF(end_memory_address.toUInt(NULL, 16), 100);
        *series1 << QPointF(start_memory_address.toUInt(NULL, 16), 0) <<  QPointF(end_memory_address.toUInt(NULL, 16), 0);

    }
    else if(ui->start_address_lineEdit->text().toUInt(NULL, 16)<start_memory_address.toUInt(NULL, 16) || ui->end_address_lineEdit->text().toUInt(NULL,16) > end_memory_address.toUInt(NULL, 16))
    {
        new_start_memory_address = start_memory_address.toUInt(NULL, 16);
        new_end_memory_address = end_memory_address.toUInt(NULL, 16);

        *series0 << QPointF(start_memory_address.toUInt(NULL, 16), 100) << QPointF(end_memory_address.toUInt(NULL, 16), 100);
        *series1 << QPointF(start_memory_address.toUInt(NULL, 16), 0) <<  QPointF(end_memory_address.toUInt(NULL, 16), 0);

    }
    else
    {
        new_start_memory_address = ui->start_address_lineEdit->text().toUInt(NULL, 16);
        new_end_memory_address = ui->end_address_lineEdit->text().toUInt(NULL, 16);;
        qDebug() << "new_start_memory_address" << new_start_memory_address;
        qDebug() << "new_end_memory_address" << new_end_memory_address;

        // 为系列赋值
        *series0 << QPointF(new_start_memory_address, 100) << QPointF(new_end_memory_address, 100);
        *series1 << QPointF(new_start_memory_address, 0) <<  QPointF(new_end_memory_address, 0);

    }

    //绘制内存池
    QAreaSeries *series_0 = new QAreaSeries(series0, series1);
    series_0->setName("free memory");
    QPen pen_0(0x3D9140);
    pen_0.setWidth(0);
    series_0->setPen(pen_0);

    QLinearGradient gradient_0(QPointF(0, 0), QPointF(0, 1));
    gradient_0.setColorAt(0.0, 0x3D9140);
    gradient_0.setColorAt(1.0, 0x3D9140);
    gradient_0.setCoordinateMode(QGradient::ObjectBoundingMode);
    series_0->setBrush(gradient_0);

    //上传至图例
    m_chart_1->addSeries(series_0);


    detach_count = ui->detach_count_Box->currentText().toInt();

    if(detach_count>=1)
    {
        /***********第一段内存状态***************/
        min_malloc_size = ui->one_left_lineedit->text().toInt();
        max_malloc_size = ui->one_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("select malloc_address, malloc_size from one_unfree_memory");
        while(query.next())
        {
            malloc_address = query.value(0).toString();
            malloc_size = query.value(1).toInt();

            uint series_start_malloc_address = malloc_address.toUInt(NULL, 16);
            uint series_end_malloc_address = series_start_malloc_address + malloc_size;

            if(series_end_malloc_address < new_start_memory_address || series_start_malloc_address > new_end_memory_address)
            {
                continue;
            }

            if(series_start_malloc_address < new_start_memory_address && series_end_malloc_address >= new_start_memory_address)
            {
                series_start_malloc_address = new_start_memory_address;
            }

            if(series_start_malloc_address <= new_end_memory_address && series_end_malloc_address > new_end_memory_address)
            {
                series_end_malloc_address = new_end_memory_address;
            }

            series2->append(series_start_malloc_address, 0);
            series2->append(series_start_malloc_address, 100);
            series2->append(series_end_malloc_address, 100);
            series2->append(series_end_malloc_address, 0);

            series3->append(series_start_malloc_address, 0);
            series3->append(series_start_malloc_address, 0);
            series3->append(series_end_malloc_address, 0);
            series3->append(series_end_malloc_address, 0);

            one_flag++;

        }

        query.exec("drop table one_unfree_memory");

        QAreaSeries *series_1 = new QAreaSeries(series2, series3);
        series_1->setName("one_memory");
        QPen pen_1(0xFF0000);
        pen_1.setWidth(0);
        series_1->setPen(pen_1);

        QLinearGradient gradient_1(QPointF(0, 0), QPointF(0, 1));
        gradient_1.setColorAt(0.0, 0xFF0000);
        gradient_1.setColorAt(1.0, 0xFF0000);
        gradient_1.setCoordinateMode(QGradient::ObjectBoundingMode);
        series_1->setBrush(gradient_1);

        if(one_flag != 0)
        {
            m_chart_1->addSeries(series_1);

            one_flag = 0;
        }
    }


    if(detach_count>=2)
    {
        /***********第二段内存状态***************/
        min_malloc_size = ui->two_left_lineedit->text().toInt();
        max_malloc_size = ui->two_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("select malloc_address, malloc_size from one_unfree_memory");
        while(query.next())
        {
            malloc_address = query.value(0).toString();
            malloc_size = query.value(1).toInt();

            uint series_start_malloc_address = malloc_address.toUInt(NULL, 16);
            uint series_end_malloc_address = series_start_malloc_address + malloc_size;

            if(series_end_malloc_address < new_start_memory_address || series_start_malloc_address > new_end_memory_address)
            {
                continue;
            }

            if(series_start_malloc_address < new_start_memory_address && series_end_malloc_address >= new_start_memory_address)
            {
                series_start_malloc_address = new_start_memory_address;
            }

            if(series_start_malloc_address <= new_end_memory_address && series_end_malloc_address > new_end_memory_address)
            {
                series_end_malloc_address = new_end_memory_address;
            }

            series4->append(series_start_malloc_address, 0);
            series4->append(series_start_malloc_address, 100);
            series4->append(series_end_malloc_address, 100);
            series4->append(series_end_malloc_address, 0);

            series5->append(series_start_malloc_address, 0);
            series5->append(series_start_malloc_address, 0);
            series5->append(series_end_malloc_address, 0);
            series5->append(series_end_malloc_address, 0);

            two_flag++;

        }

        query.exec("drop table one_unfree_memory");

        QAreaSeries *series_2 = new QAreaSeries(series4, series5);
        series_2->setName("two_memory");
        QPen pen_2(0x00FFFF);
        pen_2.setWidth(0);
        series_2->setPen(pen_2);

        QLinearGradient gradient_2(QPointF(0, 0), QPointF(0, 1));
        gradient_2.setColorAt(0.0, 0x00FFFF);
        gradient_2.setColorAt(1.0, 0x00FFFF);
        gradient_2.setCoordinateMode(QGradient::ObjectBoundingMode);
        series_2->setBrush(gradient_2);

        if(two_flag != 0)
        {
            m_chart_1->addSeries(series_2);

            two_flag = 0;
        }
    }



    if(detach_count>=3)
    {
        /***********第三段内存状态***************/
        min_malloc_size = ui->three_left_lineedit->text().toInt();
        max_malloc_size = ui->three_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("select malloc_address, malloc_size from one_unfree_memory");
        while(query.next())
        {
            malloc_address = query.value(0).toString();
            malloc_size = query.value(1).toInt();

            uint series_start_malloc_address = malloc_address.toUInt(NULL, 16);
            uint series_end_malloc_address = series_start_malloc_address + malloc_size;

            if(series_end_malloc_address < new_start_memory_address || series_start_malloc_address > new_end_memory_address)
            {
                continue;
            }

            if(series_start_malloc_address < new_start_memory_address && series_end_malloc_address >= new_start_memory_address)
            {
                series_start_malloc_address = new_start_memory_address;
            }

            if(series_start_malloc_address <= new_end_memory_address && series_end_malloc_address > new_end_memory_address)
            {
                series_end_malloc_address = new_end_memory_address;
            }

            series6->append(series_start_malloc_address, 0);
            series6->append(series_start_malloc_address, 100);
            series6->append(series_end_malloc_address, 100);
            series6->append(series_end_malloc_address, 0);

            series7->append(series_start_malloc_address, 0);
            series7->append(series_start_malloc_address, 0);
            series7->append(series_end_malloc_address, 0);
            series7->append(series_end_malloc_address, 0);

            three_flag++;
        }

        query.exec("drop table one_unfree_memory");

        QAreaSeries *series_3 = new QAreaSeries(series6, series7);
        series_3->setName("three_memory");
        QPen pen_3(0xFFFF00);
        pen_3.setWidth(0);
        series_3->setPen(pen_3);

        QLinearGradient gradient_3(QPointF(0, 0), QPointF(0, 1));
        gradient_3.setColorAt(0.0, 0xFFFF00);
        gradient_3.setColorAt(1.0, 0xFFFF00);
        gradient_3.setCoordinateMode(QGradient::ObjectBoundingMode);
        series_3->setBrush(gradient_3);

        if(three_flag != 0)
        {
            m_chart_1->addSeries(series_3);

            three_flag = 0;
        }
    }



    if(detach_count>=4)
    {
        /***********第四段内存状态***************/
        min_malloc_size = ui->four_left_lineedit->text().toInt();
        max_malloc_size = ui->four_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("select malloc_address, malloc_size from one_unfree_memory");
        while(query.next())
        {
            malloc_address = query.value(0).toString();
            malloc_size = query.value(1).toInt();

            uint series_start_malloc_address = malloc_address.toUInt(NULL, 16);
            uint series_end_malloc_address = series_start_malloc_address + malloc_size;

            if(series_end_malloc_address < new_start_memory_address || series_start_malloc_address > new_end_memory_address)
            {
                continue;
            }

            if(series_start_malloc_address < new_start_memory_address && series_end_malloc_address >= new_start_memory_address)
            {
                series_start_malloc_address = new_start_memory_address;
            }

            if(series_start_malloc_address <= new_end_memory_address && series_end_malloc_address > new_end_memory_address)
            {
                series_end_malloc_address = new_end_memory_address;
            }

            series8->append(series_start_malloc_address, 0);
            series8->append(series_start_malloc_address, 100);
            series8->append(series_end_malloc_address, 100);
            series8->append(series_end_malloc_address, 0);

            series9->append(series_start_malloc_address, 0);
            series9->append(series_start_malloc_address, 0);
            series9->append(series_end_malloc_address, 0);
            series9->append(series_end_malloc_address, 0);

            four_flag++;
        }

        query.exec("drop table one_unfree_memory");

        QAreaSeries *series_4 = new QAreaSeries(series8, series9);
        series_4->setName("four_memory");
        QPen pen_4(0x696969);
        pen_4.setWidth(0);
        series_4->setPen(pen_4);

        QLinearGradient gradient_4(QPointF(0, 0), QPointF(0, 1));
        gradient_4.setColorAt(0.0, 0x696969);
        gradient_4.setColorAt(1.0, 0x696969);
        gradient_4.setCoordinateMode(QGradient::ObjectBoundingMode);
        series_4->setBrush(gradient_4);



        if(four_flag != 0)
        {
            m_chart_1->addSeries(series_4);

            four_flag = 0;
        }
    }

    if(detach_count>=5)
    {
        /***********第五段内存状态***************/
        min_malloc_size = ui->five_left_lineedit->text().toInt();
        max_malloc_size = ui->five_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("select malloc_address, malloc_size from one_unfree_memory");
        while(query.next())
        {
            malloc_address = query.value(0).toString();
            malloc_size = query.value(1).toInt();

            uint series_start_malloc_address = malloc_address.toUInt(NULL, 16);
            uint series_end_malloc_address = series_start_malloc_address + malloc_size;

            if(series_end_malloc_address < new_start_memory_address || series_start_malloc_address > new_end_memory_address)
            {
                continue;
            }

            if(series_start_malloc_address < new_start_memory_address && series_end_malloc_address >= new_start_memory_address)
            {
                series_start_malloc_address = new_start_memory_address;
            }

            if(series_start_malloc_address <= new_end_memory_address && series_end_malloc_address > new_end_memory_address)
            {
                series_end_malloc_address = new_end_memory_address;
            }

            series10->append(series_start_malloc_address, 0);
            series10->append(series_start_malloc_address, 100);
            series10->append(series_end_malloc_address, 100);
            series10->append(series_end_malloc_address, 0);

            series11->append(series_start_malloc_address, 0);
            series11->append(series_start_malloc_address, 0);
            series11->append(series_end_malloc_address, 0);
            series11->append(series_end_malloc_address, 0);

            five_flag++;
        }

        query.exec("drop table one_unfree_memory");

        QAreaSeries *series_5 = new QAreaSeries(series10, series11);
        series_5->setName("five_memory");
        QPen pen_5(0x000080);
        pen_5.setWidth(0);
        series_5->setPen(pen_5);

        QLinearGradient gradient_5(QPointF(0, 0), QPointF(0, 1));
        gradient_5.setColorAt(0.0, 0x000080);
        gradient_5.setColorAt(1.0, 0x000080);
        gradient_5.setCoordinateMode(QGradient::ObjectBoundingMode);
        series_5->setBrush(gradient_5);

        if(five_flag != 0)
        {
            m_chart_1->addSeries(series_5);


            five_flag = 0;
        }
    }



    if(detach_count>=6)
    {
        /***********第六段内存状态***************/
        min_malloc_size = ui->six_left_lineedit->text().toInt();
        max_malloc_size = ui->six_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("select malloc_address, malloc_size from one_unfree_memory");
        while(query.next())
        {
            malloc_address = query.value(0).toString();
            malloc_size = query.value(1).toInt();

            uint series_start_malloc_address = malloc_address.toUInt(NULL, 16);
            uint series_end_malloc_address = series_start_malloc_address + malloc_size;

            if(series_end_malloc_address < new_start_memory_address || series_start_malloc_address > new_end_memory_address)
            {
                continue;
            }

            if(series_start_malloc_address < new_start_memory_address && series_end_malloc_address >= new_start_memory_address)
            {
                series_start_malloc_address = new_start_memory_address;
            }

            if(series_start_malloc_address <= new_end_memory_address && series_end_malloc_address > new_end_memory_address)
            {
                series_end_malloc_address = new_end_memory_address;
            }

            series12->append(series_start_malloc_address, 0);
            series12->append(series_start_malloc_address, 100);
            series12->append(series_end_malloc_address, 100);
            series12->append(series_end_malloc_address, 0);

            series13->append(series_start_malloc_address, 0);
            series13->append(series_start_malloc_address, 0);
            series13->append(series_end_malloc_address, 0);
            series13->append(series_end_malloc_address, 0);

            six_flag++;
        }

        query.exec("drop table one_unfree_memory");

        QAreaSeries *series_6 = new QAreaSeries(series12, series13);
        series_6->setName("six_memory");
        QPen pen_6(0xFF6A6A);
        pen_6.setWidth(0);
        series_6->setPen(pen_6);

        QLinearGradient gradient_6(QPointF(0, 0), QPointF(0, 1));
        gradient_6.setColorAt(0.0, 0xFF6A6A);
        gradient_6.setColorAt(1.0, 0xFF6A6A);
        gradient_6.setCoordinateMode(QGradient::ObjectBoundingMode);
        series_6->setBrush(gradient_6);

        if(six_flag != 0)
        {
            m_chart_1->addSeries(series_6);

            six_flag = 0;
        }
    }


    if(detach_count>=7)
    {
        /***********第七段内存状态***************/
        min_malloc_size = ui->seven_left_lineedit->text().toInt();
        max_malloc_size = ui->seven_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("select malloc_address, malloc_size from one_unfree_memory");
        while(query.next())
        {
            malloc_address = query.value(0).toString();
            malloc_size = query.value(1).toInt();

            uint series_start_malloc_address = malloc_address.toUInt(NULL, 16);
            uint series_end_malloc_address = series_start_malloc_address + malloc_size;

            if(series_end_malloc_address < new_start_memory_address || series_start_malloc_address > new_end_memory_address)
            {
                continue;
            }

            if(series_start_malloc_address < new_start_memory_address && series_end_malloc_address >= new_start_memory_address)
            {
                series_start_malloc_address = new_start_memory_address;
            }

            if(series_start_malloc_address <= new_end_memory_address && series_end_malloc_address > new_end_memory_address)
            {
                series_end_malloc_address = new_end_memory_address;
            }

            series14->append(series_start_malloc_address, 0);
            series14->append(series_start_malloc_address, 100);
            series14->append(series_end_malloc_address, 100);
            series14->append(series_end_malloc_address, 0);

            series15->append(series_start_malloc_address, 0);
            series15->append(series_start_malloc_address, 0);
            series15->append(series_end_malloc_address, 0);
            series15->append(series_end_malloc_address, 0);

            seven_flag++;
        }

        query.exec("drop table one_unfree_memory");

        QAreaSeries *series_7 = new QAreaSeries(series14, series15);
        series_7->setName("seven_memory");
        QPen pen_7(0xC71585);
        pen_7.setWidth(0);
        series_7->setPen(pen_7);

        QLinearGradient gradient_7(QPointF(0, 0), QPointF(0, 1));
        gradient_7.setColorAt(0.0, 0xC71585);
        gradient_7.setColorAt(1.0, 0xC71585);
        gradient_7.setCoordinateMode(QGradient::ObjectBoundingMode);
        series_7->setBrush(gradient_7);

        if(seven_flag != 0)
        {
            m_chart_1->addSeries(series_7);


            seven_flag = 0;
        }
    }

    if(detach_count>=8)
    {
        /***********第八段内存状态***************/
        min_malloc_size = ui->eight_left_lineedit->text().toInt();
        max_malloc_size = ui->eight_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("select malloc_address, malloc_size from one_unfree_memory");
        while(query.next())
        {
            malloc_address = query.value(0).toString();
            malloc_size = query.value(1).toInt();

            uint series_start_malloc_address = malloc_address.toUInt(NULL, 16);
            uint series_end_malloc_address = series_start_malloc_address + malloc_size;

            if(series_end_malloc_address < new_start_memory_address || series_start_malloc_address > new_end_memory_address)
            {
                continue;
            }

            if(series_start_malloc_address < new_start_memory_address && series_end_malloc_address >= new_start_memory_address)
            {
                series_start_malloc_address = new_start_memory_address;
            }

            if(series_start_malloc_address <= new_end_memory_address && series_end_malloc_address > new_end_memory_address)
            {
                series_end_malloc_address = new_end_memory_address;
            }

            series16->append(series_start_malloc_address, 0);
            series16->append(series_start_malloc_address, 100);
            series16->append(series_end_malloc_address, 100);
            series16->append(series_end_malloc_address, 0);

            series17->append(series_start_malloc_address, 0);
            series17->append(series_start_malloc_address, 0);
            series17->append(series_end_malloc_address, 0);
            series17->append(series_end_malloc_address, 0);

            eight_flag++;
        }

        query.exec("drop table one_unfree_memory");

        QAreaSeries *series_8 = new QAreaSeries(series16, series17);
        series_8->setName("eight_memory");
        QPen pen_8(0xFFA07A);
        pen_8.setWidth(0);
        series_8->setPen(pen_8);

        QLinearGradient gradient_8(QPointF(0, 0), QPointF(0, 1));
        gradient_8.setColorAt(0.0, 0xFFA07A);
        gradient_8.setColorAt(1.0, 0xFFA07A);
        gradient_8.setCoordinateMode(QGradient::ObjectBoundingMode);
        series_8->setBrush(gradient_8);


        if(eight_flag != 0)
        {
            m_chart_1->addSeries(series_8);


            eight_flag = 0;
        }

    }


    if(detach_count>=9)
    {
        /***********第九段内存状态***************/
        min_malloc_size = ui->nine_left_lineedit->text().toInt();
        max_malloc_size = ui->nine_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("select malloc_address, malloc_size from one_unfree_memory");
        while(query.next())
        {
            malloc_address = query.value(0).toString();
            malloc_size = query.value(1).toInt();

            uint series_start_malloc_address = malloc_address.toUInt(NULL, 16);
            uint series_end_malloc_address = series_start_malloc_address + malloc_size;

            if(series_end_malloc_address < new_start_memory_address || series_start_malloc_address > new_end_memory_address)
            {
                continue;
            }

            if(series_start_malloc_address < new_start_memory_address && series_end_malloc_address >= new_start_memory_address)
            {
                series_start_malloc_address = new_start_memory_address;
            }

            if(series_start_malloc_address <= new_end_memory_address && series_end_malloc_address > new_end_memory_address)
            {
                series_end_malloc_address = new_end_memory_address;
            }

            series18->append(series_start_malloc_address, 0);
            series18->append(series_start_malloc_address, 100);
            series18->append(series_end_malloc_address, 100);
            series18->append(series_end_malloc_address, 0);

            series19->append(series_start_malloc_address, 0);
            series19->append(series_start_malloc_address, 0);
            series19->append(series_end_malloc_address, 0);
            series19->append(series_end_malloc_address, 0);

            nine_flag++;
        }

        query.exec("drop table one_unfree_memory");

        QAreaSeries *series_9 = new QAreaSeries(series18, series19);
        series_9->setName("nine_memory");
        QPen pen_9(0x8B4500);
        pen_9.setWidth(0);
        series_9->setPen(pen_9);

        QLinearGradient gradient_9(QPointF(0, 0), QPointF(0, 1));
        gradient_9.setColorAt(0.0, 0x8B4500);
        gradient_9.setColorAt(1.0, 0x8B4500);
        gradient_9.setCoordinateMode(QGradient::ObjectBoundingMode);
        series_9->setBrush(gradient_9);


        if(nine_flag != 0)
        {
            m_chart_1->addSeries(series_9);

            nine_flag = 0;
        }

    }


    if(detach_count>=10)
    {
        /***********第十段内存状态***************/
        min_malloc_size = ui->ten_left_lineedit->text().toInt();
        max_malloc_size = ui->ten_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("select malloc_address, malloc_size from one_unfree_memory");
        while(query.next())
        {
            malloc_address = query.value(0).toString();
            malloc_size = query.value(1).toInt();

            uint series_start_malloc_address = malloc_address.toUInt(NULL, 16);
            uint series_end_malloc_address = series_start_malloc_address + malloc_size;

            if(series_end_malloc_address < new_start_memory_address || series_start_malloc_address > new_end_memory_address)
            {
                continue;
            }

            if(series_start_malloc_address < new_start_memory_address && series_end_malloc_address >= new_start_memory_address)
            {
                series_start_malloc_address = new_start_memory_address;
            }

            if(series_start_malloc_address <= new_end_memory_address && series_end_malloc_address > new_end_memory_address)
            {
                series_end_malloc_address = new_end_memory_address;
            }

            series20->append(series_start_malloc_address, 0);
            series20->append(series_start_malloc_address, 100);
            series20->append(series_end_malloc_address, 100);
            series20->append(series_end_malloc_address, 0);

            series21->append(series_start_malloc_address, 0);
            series21->append(series_start_malloc_address, 0);
            series21->append(series_end_malloc_address, 0);
            series21->append(series_end_malloc_address, 0);

            ten_flag++;
        }

        query.exec("drop table one_unfree_memory");

        QAreaSeries *series_10 = new QAreaSeries(series18, series19);
        series_10->setName("ten_memory");
        QPen pen_10(0xFFA500);
        pen_10.setWidth(0);
        series_10->setPen(pen_10);

        QLinearGradient gradient_10(QPointF(0, 0), QPointF(0, 1));
        gradient_10.setColorAt(0.0, 0xFFA500);
        gradient_10.setColorAt(1.0, 0xFFA500);
        gradient_10.setCoordinateMode(QGradient::ObjectBoundingMode);
        series_10->setBrush(gradient_10);

        if(ten_flag != 0)
        {
            m_chart_1->addSeries(series_10);

            ten_flag = 0;
        }

    }



    min_malloc_size = 0;
    max_malloc_size = 0;

    ui->progressBar->setValue(100);








    m_chart_1->createDefaultAxes();

    QValueAxis *axisX = dynamic_cast<QValueAxis*>(m_chart_1->axisX());//

    QValueAxis *One_axisX = new QValueAxis();


    One_axisX->setLabelFormat("%#X");

    One_axisX->setMin(axisX->min());

    One_axisX->setMax(axisX->max());

    m_chart_1->setAxisX(One_axisX);

    m_chart_1->axisY()->setVisible(false);



//        qreal cur_x_min = axisX->min();

//        qreal cur_x_max = axisX->max();

//        axisX->setRange(cur_x_min + 20, cur_x_max + 20);


//        //![3]
//        min_x_value = 0;
//        max_x_value = 9074785;
//        QValueAxis *axisX = new QValueAxis();
//        axisX->setLabelFormat("%d");

//        axisX->setMin(min_x_value);
//        axisX->setMax(max_x_value);

//        m_chart_1->setAxisX(axisX);

//    m_chart_1->createDefaultAxes();

//    m_chart_1->axisX()->setLabelFormat();
//    m_chart_1->axisY()->setLabelsVisible(false);

//    m_chart_1->axisY()->setRange(0,2500);


//    // 构建坐标轴并绑定到棒系列
//    QBarCategoryAxis* axisX = new QBarCategoryAxis(this);

//    axisX->append(categories);
//    axisX->setTitleText("axisX");

//    QValueAxis* axisY = new QValueAxis(this);
//    axisY->setRange(0, 2056);
//    axisY->setTitleText("axisY");//



//    //绑定坐标轴与系列
//    m_chart_1->setAxisX(axisX, barSeries);

//    m_chart_1->setAxisY(axisY, barSeries);



    if(first_flag == 1)
    {
        first_flag = 0;
        chartView = new ChartView(m_chart_1);
    }
    else
    {
        ui->horizontalLayout_12->removeWidget(chartView);
        delete chartView;
        chartView = new ChartView(m_chart_1);
    }

    chartView->setRenderHint(QPainter::Antialiasing);

    ui->horizontalLayout_12->addWidget(chartView);

    ui->portNameBox->setEnabled(true);
    ui->baudrateBox->setEnabled(true);
    ui->dataBitsBox->setEnabled(true);
    ui->ParityBox->setEnabled(true);
    ui->stopBitsBox->setEnabled(true);
    ui->openButton->setEnabled(true);
    ui->searchButton->setEnabled(true);
    ui->clearButton->setEnabled(true);
    ui->analyseButton->setEnabled(true);
    ui->detachButton->setEnabled(true);
    ui->saveButton->setEnabled(true);

//    // 设置动画
//    ui->one_widget->setRenderHint(QPainter::Antialiasing, true);

    analyse_memory_count = 0;
    sum_exec_count = 0;
    detach_count = 0;


    QMessageBox::about(NULL, "提示", "数据分析完成");//
}



/**************************************************************
 * Function Name : on_saveButton_clicked
 * Description   : Save three memory analysis results
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::on_saveButton_clicked()
{
//    //Parameter validity check
//    if(ui->detach_textEdit->document()->isEmpty())
//    {
//        QMessageBox::about(NULL, "提示", "无数据");
//        return;
//    }

    //New save folder
    QString filename;
    filename = QString("单位为%1s的内存分析结果.txt").arg(ui->separate_lineEdit->text().toInt());
    QDir dir;
    if(!dir.exists("log")){
        dir.mkdir("log");
    }
    dir="log";
    dir.remove(filename);
    dir.remove("./log/chart.png");
    dir.remove("./log/wave_form.svg");
    QString path = dir.filePath(filename);
    QFile file;
    file.setFileName(path);

    if(file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Append))
    {
        QTextStream out(&file);

        out << ui->detach_textEdit->toPlainText();
    }
    else
    {
        QMessageBox::about(NULL, "提示", "文件保存失败");//
        return;
    }
    file.close();


    QString save_excel_name;
    save_excel_name = QString("./log/单位为%1s的内存分析表.xlsx").arg(ui->separate_lineEdit->text().toInt());
    dir.remove(save_excel_name);
    xlsx.saveAs(save_excel_name);


    QString date;
    QString save_db_name;
    QSqlQuery query;

    query.exec("select date from malloc_and_free where id = 1");

    if(query.next())
    {
        date = query.value(0).toString();
    }

    date = date.mid(0, 10) + "-" + date.mid(11, 2) + "." + date.mid(14, 2) + "." + date.mid(17, 2) + "." + date.mid(20, 3);

    qDebug() << "date" << date;

    save_db_name = "./log/" + date + ".db";


    qDebug() << "save_db_name" << save_db_name;

    QFile file1("./MyDataBase.db");

    file1.copy("./log/MyDataBase.db");

    QFile file2("./log/MyDataBase.db");

    file2.rename(save_db_name);


    file1.copy("./log/MyDataBase.db");

//    file2.rename("./log/2020.11.25.db");




    QMessageBox::about(NULL, "提示", "文件保存成功");//

}



/**************************************************************
 * Function Name : MainWindow
 * Description   : Deconstructe Function MainWindow
 * Parameters    : null
 * Returns       : null
 **************************************************************/
MainWindow::~MainWindow()
{

    QSqlDatabase database;
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("MyDataBase.db");
    database.close();

    QFile delete_file("./MyDataBase.db");
    if (delete_file.exists())
    {
        qDebug()<< "数据库查找成功";

        if(delete_file.remove())
        {
            qDebug() << "删除成功";
        }
        else
        {
            qDebug() << "删除失败";
//            QMessageBox::about(NULL, "提示", "未删除数据库");
            return;
        }
    }
    else
    {
        qDebug()<< "数据库查找失败";
    }

    delete ui;
}


