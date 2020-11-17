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
    int last_line;
    static int mallloc_and_free_id = 1;

    QSqlQuery query;
    QSqlQuery query1;
    QString malloc_text = "=m ";
    QString free_text = "=f ";
    QString malloc_memmory_address;
    QString string_malloc_memory_size;
    int malloc_memory_size;
    QString free_memory_address;
    QString arr;
    QDateTime current_date_time =QDateTime::currentDateTime();
    QString current_date =current_date_time.toString("yyyy.MM.dd hh:mm:ss.zzz");

    last_line = (ui->recvTextEdit->document()->lineCount());

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

            query.prepare("insert into malloc_and_free values(:id,'"+current_date+"','malloc','"+malloc_memmory_address+"', :malloc_memory_size)");
            query.bindValue(":id",mallloc_and_free_id);
            query.bindValue(":malloc_memory_size", malloc_memory_size);
            query.exec();

            mallloc_and_free_id++;
            //i = arr.indexOf(malloc_text, i) + 1;
        }

        if((arr.indexOf(free_text,j)!= -1))
        {
            //qDebug()<< "malloc出的字在文件中位置是第几个字节" <<arr.indexOf(malloc_text, i)+1;
            free_memory_address = arr.mid(arr.indexOf(free_text, j)+3, 8);
            //qDebug()<< "malloc的大小是" <<malloc_memory_size;
            ui->analyseTextEdit->append("free为 "+free_memory_address+"");

            query.prepare("insert into malloc_and_free values(:id,'"+current_date+"','free','"+free_memory_address+"', 0)");
            query.bindValue(":id",mallloc_and_free_id);
            query.exec();//

            mallloc_and_free_id++;
        }
//        qDebug() << "mallloc_and_free_id" << mallloc_and_free_id;
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
    query.exec("drop view malloc");
    query.exec("drop view free");
    query.exec("drop view one_unfree");
//    query.exec("drop index malloc_size_index");
//    query.exec("drop index type_index");

    ui->recvTextEdit->clear();
    ui->analyseTextEdit->clear();
    ui->detach_textEdit->clear();

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

///**************************************************************
// * Function Name : detach_zero_memory
// * Description   : Analyze the usage of the first memory
// * Parameters    : min_size
// *                 max_size
// * Returns       : null
// **************************************************************/
//void MainWindow::detach_zero_memory(int min_size, int max_size)
//{
//    QSqlQuery query;
//    QSqlQuery query2;
//    QSqlQuery query3;
//    QSqlQuery query4;
//    int min_malloc_size;
//    int max_malloc_size;
//    int detach_malloc_and_free_id = 1;
//    QDateTime  start_time;
//    QDateTime  end_time;
//    qint64 intervalTimeMS = 0;
//    QString date;
//    QString type;
//    QString malloc_address;
//    int malloc_size;
//    int malloc_parent_id;
//    int exec_count = 0;
//    long sum_malloc_size = 0;
//    long sum_malloc_count = 0;
//    long sum_free_size = 0;
//    long sum_free_count = 0;
//    int start_time_flag = 1;
//    int time_count = 1;
//    int search_flag = 1;
//    int min_search_value = 0;
//    int max_search_value = 2500000;
//    QString excel_postion;
//    int analyse_time_unit = 0;
//    int most_max_size = 0;
//    int most_max_id = 0;
//    QString one_max_time;
//    QString most_max_time;
//    int most_malloc_count = 0;
//    int most_free_count = 0;

//    int sum_table_count = 0;
//    int progress_percentage = 0;
//    int detach_count = 0;

//    QString analyse_memory_messages;

//    detach_count = ui->detach_count_Box->currentText().toInt();

//    query4.exec("select malloc_and_free_id from malloc_and_free order by malloc_and_free_id desc");

//    if(query4.next())
//    {
//        sum_table_count = query4.value(0).toInt();
//    }

//    qDebug()<< "sum_table_count" << sum_table_count;

//    analyse_time_unit = ui->separate_lineEdit->text().toInt();
//    if(analyse_time_unit<=0)
//    {
//        QMessageBox::about(NULL, "提示", "分离时间单位有误");
//        return;
//    }

//    min_malloc_size = min_size * 1024;
//    max_malloc_size = max_size * 1024;

//    if(min_malloc_size > max_malloc_size)
//    {
//        QMessageBox::about(NULL, "提示", "最小参数有误");
//        return;
//    }

//    if(min_malloc_size <0 || max_malloc_size < 0)
//    {
//        QMessageBox::about(NULL, "提示", "最大段参数有误");
//        return;
//    }

//    xlsx.write("A1", "time");
//    xlsx.write("B1", "one_size");


//    query3.exec("create temporary table one_unfree_memory(detach_malloc_and_free_id int primary key, date varchar, type varchar, malloc_address varchar, malloc_size varchar, parent_id int)");

//    while(search_flag)
//    {
//            query.prepare("select * from malloc_and_free where malloc_and_free_id < 50 and malloc_and_free_id < :max_search_value");
//            query.bindValue(":max_search_value", max_search_value);
//            query.exec();


//        query3.exec("BEGIN");

//        while(query.next())
//        {
//            date = query.value(1).toString();
//            type = query.value(2).toString();
//            malloc_address = query.value(3).toString();
//            malloc_size = query.value(4).toInt();
//            malloc_parent_id = query.value(0).toInt();

//            if(start_time_flag == 1)
//            {
//                start_time = QDateTime::fromString(date, "yyyy.MM.dd hh:mm:ss.zzz");
//                start_time_flag = 0;
//            }

//            end_time = QDateTime::fromString(date, "yyyy.MM.dd hh:mm:ss.zzz");

//            intervalTimeMS = start_time.msecsTo(end_time);

//            progress_percentage =  double(sum_exec_count + malloc_parent_id)/double(sum_table_count)/double(detach_count)*100;

//            ui->progressBar->setValue(progress_percentage);

//            if(exec_count % 3000 == 0)
//            {
//                query3.exec("COMMIT");
//                query3.exec("BEGIN");
//            }

//            exec_count++;
//        }
//        query3.exec("COMMIT");

//        if(exec_count)
//        {
//            exec_count = 0;
//            min_search_value += 2500000;
//            max_search_value += 2500000;
//        }
//        else
//        {
//            search_flag = 0;
//            break;
//        }
//    }

//    sum_exec_count += malloc_parent_id;

//    qDebug() << "intervalTimeMS/1000/analyse_time_unit" << intervalTimeMS/1000/analyse_time_unit;
//    qDebug() << "time_count" << time_count;

//    if(time_count <= (intervalTimeMS/1000/analyse_time_unit))
//    {
//        while((time_count-1) <= (intervalTimeMS/1000/analyse_time_unit))
//        {
//            excel_postion = QString("B%1").arg(time_count);
//            xlsx.write(excel_postion, 0);
//            time_count++;
//        }
//    }

//    if(excel_row_count < time_count)
//    {
//        excel_row_count = time_count;
//    }

//    analyse_memory_count++;

//    analyse_memory_messages = "第" + QString::number(analyse_memory_count) + "段内存使用信息";

//    ui->detach_textEdit->append("--------------------------------------------");
//    ui->detach_textEdit->append(analyse_memory_messages);
//    ui->detach_textEdit->append("malloc的次数");
//    ui->detach_textEdit->append(QString::number(sum_malloc_count, 10));
//    ui->detach_textEdit->append("malloc的内存大小");
//    ui->detach_textEdit->append(QString::number(sum_malloc_size, 10));
//    ui->detach_textEdit->append("free的次数");
//    ui->detach_textEdit->append(QString::number(sum_free_count, 10));
//    ui->detach_textEdit->append("free的内存大小");
//    ui->detach_textEdit->append(QString::number(sum_free_size, 10));

//    ui->detach_textEdit->append("最大内存占用值");
//    ui->detach_textEdit->append(QString::number(most_max_size, 10));
//    ui->detach_textEdit->append("最大内存占用值时malloc了多少次");
//    ui->detach_textEdit->append(QString::number(most_malloc_count, 10));
//    ui->detach_textEdit->append("最大内存占用值时free了多少次");
//    ui->detach_textEdit->append(QString::number(most_free_count, 10));
//    ui->detach_textEdit->append("在数据库内id号是");
//    ui->detach_textEdit->append(QString::number(most_max_id, 10));
//    ui->detach_textEdit->append("发生时间是");
//    ui->detach_textEdit->append(most_max_time);

//    ui->detach_textEdit->append("未释放内存");
//    ui->detach_textEdit->append(QString::number(sum_malloc_size-sum_free_size, 10));
//    query3.exec("select * from one_unfree_memory");
//    while(query3.next())
//    {
//        ui->detach_textEdit->append("**********************");
//        detach_malloc_and_free_id = query3.value(0).toInt();
//        date = query3.value(1).toString();
//        type = query3.value(2).toString();
//        malloc_address = query3.value(3).toString();
//        malloc_size = query3.value(4).toInt();
//        malloc_parent_id = query3.value(5).toInt();

//        ui->detach_textEdit->append("序号");
//        ui->detach_textEdit->append(QString::number(detach_malloc_and_free_id, 10));
//        ui->detach_textEdit->append("日期");
//        ui->detach_textEdit->append(date);
//        ui->detach_textEdit->append("类型");
//        ui->detach_textEdit->append(type);
//        ui->detach_textEdit->append("malloc地址");
//        ui->detach_textEdit->append(malloc_address);
//        ui->detach_textEdit->append("malloc大小");
//        ui->detach_textEdit->append(QString::number(malloc_size, 10));
//        ui->detach_textEdit->append("源序号");
//        ui->detach_textEdit->append(QString::number(malloc_parent_id, 10));

//        if(ui->detach_textEdit->document()->lineCount()%100000 == 0)
//        {
//            ui->detach_textEdit->clear();
//        }
//    }

//}


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
    lineSeries_1 = new QLineSeries();
    lineSeries_2 = new QLineSeries();
    lineSeries_3 = new QLineSeries();
    lineSeries_4 = new QLineSeries();
    lineSeries_5 = new QLineSeries();
    lineSeries_6 = new QLineSeries();
    lineSeries_7 = new QLineSeries();
    lineSeries_8 = new QLineSeries();
    lineSeries_9 = new QLineSeries();
    lineSeries_10 = new QLineSeries();

    ui->progressBar->setValue(0);

    lineSeries_1->setUseOpenGL(true);
    lineSeries_2->setUseOpenGL(true);
    lineSeries_3->setUseOpenGL(true);
    lineSeries_4->setUseOpenGL(true);
    lineSeries_5->setUseOpenGL(true);
    lineSeries_6->setUseOpenGL(true);
    lineSeries_7->setUseOpenGL(true);
    lineSeries_8->setUseOpenGL(true);
    lineSeries_9->setUseOpenGL(true);
    lineSeries_10->setUseOpenGL(true);


    // 构建图表对象
    m_chart_1 = new QChart();

    // 构建折线系列
    m_chart_1->addSeries(lineSeries_1);
    m_chart_1->addSeries(lineSeries_2);
    m_chart_1->addSeries(lineSeries_3);
    m_chart_1->addSeries(lineSeries_4);
    m_chart_1->addSeries(lineSeries_5);
    m_chart_1->addSeries(lineSeries_6);
    m_chart_1->addSeries(lineSeries_7);
    m_chart_1->addSeries(lineSeries_8);
    m_chart_1->addSeries(lineSeries_9);
    m_chart_1->addSeries(lineSeries_10);

    // 设置默认坐标轴
    m_chart_1->createDefaultAxes();

    // 设置主题
    m_chart_1->setTheme(QtCharts::QChart::ChartThemeBlueCerulean);

    // 设置系列标题
    m_chart_1->setTitle(QString::fromLocal8Bit("One Chart"));

    // 修改波形图样式
    change_style();

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

    chartView->setRubberBand(QChartView::RectangleRubberBand);


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
    QSqlQuery query2;
    QSqlQuery query3;
    QSqlQuery query4;
    int min_malloc_size;
    int max_malloc_size;
    int detach_malloc_and_free_id = 1;
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

    query4.exec("select malloc_and_free_id from malloc_and_free order by malloc_and_free_id desc");

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
//    xlsx.write("B1", "one_size");
//    xlsx.write("C1", "two_size");
//    xlsx.write("D1", "three_size");
//    xlsx.write("E1", "four_size");
//    xlsx.write("F1", "five_size");
//    xlsx.write("G1", "six_size");
//    xlsx.write("H1", "seven_size");
//    xlsx.write("I1", "eight_size");
//    xlsx.write("J1", "nine_size");
//    xlsx.write("K1", "ten_size");
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


    query3.exec("create temporary table one_unfree_memory(detach_malloc_and_free_id int primary key, date varchar, type varchar, malloc_address varchar, malloc_size varchar, parent_id int)");

    while(search_flag)
    {
//        query.prepare("select * from malloc_and_free where malloc_and_free_id >= :min_search_value and malloc_and_free_id < :max_search_value and ((malloc_size >= :min_malloc_size and malloc_size < :max_malloc_size) or type = 'free')");
//        query.bindValue(":min_search_value", min_search_value);
//        query.bindValue(":max_search_value", max_search_value);
//        query.bindValue(":min_malloc_size", min_malloc_size);
//        query.bindValue(":max_malloc_size", max_malloc_size);
//        query.exec();


        query.prepare("select * from malloc_and_free where malloc_and_free_id >= :min_search_value and malloc_and_free_id < :max_search_value");
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
                    query3.prepare("insert into one_unfree_memory values(:detach_malloc_and_free_id, '"+date+"', '"+type+"', '"+malloc_address+"', :malloc_size, :parent_id)");
                    query3.bindValue(":detach_malloc_and_free_id", detach_malloc_and_free_id);
                    query3.bindValue(":malloc_size", malloc_size);
                    query3.bindValue(":parent_id", malloc_parent_id);
                    query3.exec();

                    sum_malloc_size += malloc_size;

                    if(one_max_size<(sum_malloc_size-sum_free_size))
                    {
                        one_max_size = (sum_malloc_size-sum_free_size);
                        one_max_id = detach_malloc_and_free_id;
                        one_max_time = date;

                    }

                    if(most_max_size<(sum_malloc_size-sum_free_size))
                    {
                        most_max_size = (sum_malloc_size-sum_free_size);
                        most_max_id = detach_malloc_and_free_id;
                        most_max_time = date;
                        most_malloc_count = sum_malloc_count + 1;
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

                            lineSeries_1->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;
                        case 1:
                            excel_postion = QString("C%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_2->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;
                        case 2:
                            excel_postion = QString("D%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_3->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;

                        case 3:
                            excel_postion = QString("E%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_4->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;
                        case 4:
                            excel_postion = QString("F%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_5->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;
                        case 5:
                            excel_postion = QString("G%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_6->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;

                        case 6:
                            excel_postion = QString("H%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_7->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;
                        case 7:
                            excel_postion = QString("I%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_8->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;
                        case 8:
                            excel_postion = QString("J%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_9->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;

                        case 9:
                            excel_postion = QString("K%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_10->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;
                        }


                        one_max_size = 0;

                    }

                    detach_malloc_and_free_id++;
                    sum_malloc_count++;


//                    if(sum_malloc_count%10000 == 0)
//                    {
//                        qDebug()<< "sum_malloc_count" <<  sum_malloc_count;
//                    }
                }
            }


            if(type == "free")
            {
                query2.exec("select * from one_unfree_memory where malloc_address = '"+malloc_address+"' order by detach_malloc_and_free_id desc limit 0,1");

                if(query2.next())
                {
                    malloc_size = query2.value(4).toInt();

                    sum_free_size += malloc_size;

                    if(one_max_size<(sum_malloc_size-sum_free_size))
                    {
                        one_max_size = (sum_malloc_size-sum_free_size);
                        one_max_id = detach_malloc_and_free_id;
                        one_max_time = date;

                    }

                    if(most_max_size<(sum_malloc_size-sum_free_size))
                    {
                        most_max_size = (sum_malloc_size-sum_free_size);
                        most_max_id = detach_malloc_and_free_id;
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

                            lineSeries_1->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;
                        case 1:
                            excel_postion = QString("C%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_2->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;
                        case 2:
                            excel_postion = QString("D%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_3->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;

                        case 3:
                            excel_postion = QString("E%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_4->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;
                        case 4:
                            excel_postion = QString("F%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_5->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;
                        case 5:
                            excel_postion = QString("G%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_6->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;

                        case 6:
                            excel_postion = QString("H%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_7->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;
                        case 7:
                            excel_postion = QString("I%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_8->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;
                        case 8:
                            excel_postion = QString("J%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_9->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;

                        case 9:
                            excel_postion = QString("K%1").arg(time_count+1);
                            xlsx.write(excel_postion, one_max_size);

                            lineSeries_10->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                            break;
                        }

                        one_max_size = 0;

                    }

                    detach_malloc_and_free_id++;
                    sum_free_count++;

                    query3.prepare("delete from one_unfree_memory where detach_malloc_and_free_id = :delete_id");
                    query3.bindValue(":delete_id", query2.value(0).toInt());
                    query3.exec();

//                    if(sum_free_count%10000 == 0)
//                    {
//                        qDebug()<< "sum_free_count" <<  sum_free_count;

//                    }

                }
            }


            if(intervalTimeMS>time_count*analyse_time_unit*1000)
            {
                excel_postion = QString("A%1").arg(time_count+1);
                xlsx.write(excel_postion, time_count);

                if(excel_supplement_flag == 1)
                {
                switch(analyse_memory_count)
                {
                case 0:
                    excel_postion = QString("B%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);

                    lineSeries_1->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                    break;
                case 1:
                    excel_postion = QString("C%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);

                    lineSeries_2->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                    break;
                case 2:
                    excel_postion = QString("D%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);

                    lineSeries_3->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                    break;

                case 3:
                    excel_postion = QString("E%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);

                    lineSeries_4->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                    break;
                case 4:
                    excel_postion = QString("F%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);

                    lineSeries_5->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                    break;
                case 5:
                    excel_postion = QString("G%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);

                    lineSeries_6->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                    break;

                case 6:
                    excel_postion = QString("H%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);

                    lineSeries_7->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                    break;
                case 7:
                    excel_postion = QString("I%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);

                    lineSeries_8->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                    break;
                case 8:
                    excel_postion = QString("J%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);

                    lineSeries_9->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
                    break;

                case 9:
                    excel_postion = QString("K%1").arg(time_count+1);
                    xlsx.write(excel_postion, one_max_size);

                    lineSeries_10->append(intervalTimeMS/1000/analyse_time_unit, one_max_size);
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
        detach_malloc_and_free_id = query3.value(0).toInt();
        date = query3.value(1).toString();
        type = query3.value(2).toString();
        malloc_address = query3.value(3).toString();
        malloc_size = query3.value(4).toInt();
        malloc_parent_id = query3.value(5).toInt();

        ui->detach_textEdit->append("序号");
        ui->detach_textEdit->append(QString::number(detach_malloc_and_free_id, 10));
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

}

///**************************************************************
// * Function Name : detach_two_memory
// * Description   : Analyze the usage of the second memory
// * Parameters    : null
// * Returns       : null
// **************************************************************/
//void MainWindow::detach_two_memory(void)
//{
//    QSqlQuery query;
//    QSqlQuery query1;
//    QSqlQuery query2;
//    QSqlQuery query3;
//    int min_malloc_size;
//    int max_malloc_size;
//    int detach_malloc_and_free_id = 1;
//    QDateTime  start_time;
//    QDateTime  end_time;
//    qint64 intervalTimeMS = 0;
//    QString date;
//    QString type;
//    QString malloc_address;
//    int malloc_size;
//    int malloc_parent_id;
//    int exec_count = 0;
//    int sum_malloc_size = 0;
//    int sum_malloc_count = 0;
//    int sum_free_size = 0;
//    int sum_free_count = 0;
//    int start_time_flag = 1;
//    int time_count = 1;
//    int search_flag = 1;
//    int min_search_value = 0;
//    int max_search_value = 2500000;
//    int two_max_size = 0;
//    QString excel_postion;
//    int analyse_time_unit = 0;
//    int most_max_size = 0;
//    int two_max_id = 0;
//    int most_max_id = 0;
//    QString two_max_time;
//    QString most_max_time;

//    analyse_time_unit = ui->separate_lineEdit->text().toInt();
//    if(analyse_time_unit<=0)
//    {
//        QMessageBox::about(NULL, "提示", "分离时间单位有误");
//        return;
//    }

//    if(ui->two_minlineEdit->text() == NULL || ui->two_minlineEdit->text() == NULL)
//    {
//        QMessageBox::about(NULL, "提示", "第二段参数有误");
//        return;
//    }

//    min_malloc_size = ui->two_minlineEdit->text().toInt() * 1024;
//    max_malloc_size = ui->two_minlineEdit->text().toInt() * 1024;

//    if(min_malloc_size > max_malloc_size)
//    {
//        QMessageBox::about(NULL, "提示", "第二段参数有误");
//        return;
//    }

//    if(min_malloc_size <0 || max_malloc_size < 0)
//    {
//        QMessageBox::about(NULL, "提示", "第二段参数有误");
//        return;
//    }

//    xlsx.write("A1", "time");
//    xlsx.write("C1", "two_size");

//    query1.exec("create table two_detach_memory(detach_malloc_and_free_id int primary key, date varchar, type varchar, malloc_address varchar, malloc_size varchar, parent_id int)");
//    query3.exec("create table two_unfree_memory(detach_malloc_and_free_id int primary key, date varchar, type varchar, malloc_address varchar, malloc_size varchar, parent_id int)");

//    while(search_flag)
//    {
//        query.prepare("select malloc_and_free_id, date, type, malloc_address,malloc_size from malloc_and_free where malloc_and_free_id >= :min_search_value and malloc_and_free_id < :max_search_value");
//        query.bindValue(":min_search_value", min_search_value);
//        query.bindValue(":max_search_value", max_search_value);
//        query.exec();

//        query1.exec("BEGIN");
//        query3.exec("BEGIN");

//        while(query.next())
//        {
//            date = query.value(1).toString();
//            type = query.value(2).toString();
//            malloc_address = query.value(3).toString();
//            malloc_size = query.value(4).toInt();
//            malloc_parent_id = query.value(0).toInt();

//            if(start_time_flag == 1)
//            {
//                start_time = QDateTime::fromString(date, "yyyy.MM.dd hh:mm:ss.zzz");
//                start_time_flag = 0;
//            }

//            end_time = QDateTime::fromString(date, "yyyy.MM.dd hh:mm:ss.zzz");

//            intervalTimeMS = start_time.msecsTo(end_time);


//            if(type == "malloc")
//            {
//                if(malloc_size>=min_malloc_size && malloc_size<max_malloc_size)
//                {
//                    query1.prepare("insert into two_detach_memory values(:detach_malloc_and_free_id, '"+date+"', '"+type+"', '"+malloc_address+"', :malloc_size, :parent_id)");
//                    query1.bindValue(":detach_malloc_and_free_id", detach_malloc_and_free_id);
//                    query1.bindValue(":malloc_size", malloc_size);
//                    query1.bindValue(":parent_id", malloc_parent_id);
//                    query1.exec();

//                    query3.prepare("insert into two_unfree_memory values(:detach_malloc_and_free_id, '"+date+"', '"+type+"', '"+malloc_address+"', :malloc_size, :parent_id)");
//                    query3.bindValue(":detach_malloc_and_free_id", detach_malloc_and_free_id);
//                    query3.bindValue(":malloc_size", malloc_size);
//                    query3.bindValue(":parent_id", malloc_parent_id);
//                    query3.exec();

//                    sum_malloc_size += malloc_size;

//                    if(two_max_size<(sum_malloc_size-sum_free_size))
//                    {
//                        two_max_size = (sum_malloc_size-sum_free_size);
//                        two_max_id = detach_malloc_and_free_id;
//                        two_max_time = date;
//                    }

//                    if(most_max_size<(sum_malloc_size-sum_free_size))
//                    {
//                        most_max_size = (sum_malloc_size-sum_free_size);
//                        most_max_id = detach_malloc_and_free_id;
//                        most_max_time = date;
//                    }

//                    if(intervalTimeMS>time_count*analyse_time_unit*1000)
//                    {
//                        lineSeries_2->append(intervalTimeMS/1000/analyse_time_unit, two_max_size);

//                        excel_postion = QString("A%1").arg(time_count+1);
//                        xlsx.write(excel_postion, time_count);

//                        excel_postion = QString("C%1").arg(time_count+1);
//                        xlsx.write(excel_postion, two_max_size);

//                        two_max_size = 0;
//                        time_count++;
//                    }

//                    detach_malloc_and_free_id++;
//                    sum_malloc_count++;

//                    if(sum_malloc_count%10000 == 0)
//                    {
//                        qDebug()<< "sum_malloc_count" <<  sum_malloc_count;
//                    }

//                }
//            }

//            if(type == "free")
//            {
//                query2.exec("select * from two_unfree_memory where malloc_address = '"+malloc_address+"' order by detach_malloc_and_free_id desc limit 0,1");

//                if(query2.next())
//                {
//                    //                qDebug()<< query2.value(0).toInt() << query2.value(1).toString()  <<query2.value(2).toString()  << query2.value(3).toString()  << query2.value(4).toInt()  << query2.value(5).toInt() ;
//                    malloc_size = query2.value(4).toInt();

//                    query1.prepare("insert into two_detach_memory values(:detach_malloc_and_free_id, '"+date+"', '"+type+"', '"+malloc_address+"', :malloc_size, :parent_id)");
//                    query1.bindValue(":detach_malloc_and_free_id", detach_malloc_and_free_id);
//                    query1.bindValue(":malloc_size", malloc_size);
//                    query1.bindValue(":parent_id", malloc_parent_id);
//                    query1.exec();

//                    sum_free_size += malloc_size;

//                    if(two_max_size<(sum_malloc_size-sum_free_size))
//                    {
//                        two_max_size = (sum_malloc_size-sum_free_size);
//                        two_max_id = detach_malloc_and_free_id;
//                        two_max_time = date;

//                    }

//                    if(most_max_size<(sum_malloc_size-sum_free_size))
//                    {
//                        most_max_size = (sum_malloc_size-sum_free_size);
//                        most_max_id = detach_malloc_and_free_id;
//                        most_max_time = date;
//                    }

//                    if(intervalTimeMS>time_count*analyse_time_unit*1000)
//                    {
//                        lineSeries_2->append(intervalTimeMS/1000/analyse_time_unit, two_max_size);

//                        excel_postion = QString("A%1").arg(time_count+1);
//                        xlsx.write(excel_postion, time_count);

//                        excel_postion = QString("C%1").arg(time_count+1);
//                        xlsx.write(excel_postion, two_max_size);

//                        two_max_size = 0;
//                        time_count++;
//                    }

//                    detach_malloc_and_free_id++;
//                    sum_free_count++;

//                    query3.prepare("delete from two_unfree_memory where detach_malloc_and_free_id = :delete_id");
//                    query3.bindValue(":delete_id", query2.value(0).toInt());
//                    query3.exec();

//                    if(sum_free_count%10000 == 0)
//                    {
//                        qDebug()<< "sum_free_count" <<  sum_free_count;
//                    }


//                }
//            }



//            if(exec_count % 3000 == 0)
//            {
//                query1.exec("COMMIT");
//                query1.exec("BEGIN");
//                query3.exec("COMMIT");
//                query3.exec("BEGIN");
//            }

//            exec_count++;

//        }

//        query1.exec("COMMIT");
//        query3.exec("COMMIT");

//        if(exec_count)
//        {
//            exec_count = 0;
//            min_search_value += 2500000;
//            max_search_value += 2500000;
//        }
//        else
//        {
//            search_flag = 0;
//            break;
//        }
//    }

//    qDebug() << "intervalTimeMS/1000/analyse_time_unit" << intervalTimeMS/1000/analyse_time_unit;
//    qDebug() << "time_count" << time_count;

//    if(time_count <= (intervalTimeMS/1000/analyse_time_unit))
//    {
//        while((time_count-1) <= (intervalTimeMS/1000/analyse_time_unit))
//        {
//            excel_postion = QString("C%1").arg(time_count);
//            xlsx.write(excel_postion, 0);
//            time_count++;
//        }
//    }

//    if(excel_row_count < time_count)
//    {
//        excel_row_count = time_count;
//    }

//    ui->detach_textEdit->append("--------------------------------------------");
//    ui->detach_textEdit->append("第二段内存使用信息");
//    ui->detach_textEdit->append("malloc的次数");
//    ui->detach_textEdit->append(QString::number(sum_malloc_count, 10));
//    ui->detach_textEdit->append("malloc的内存大小");
//    ui->detach_textEdit->append(QString::number(sum_malloc_size, 10));
//    ui->detach_textEdit->append("free的次数");
//    ui->detach_textEdit->append(QString::number(sum_free_count, 10));
//    ui->detach_textEdit->append("free的内存大小");
//    ui->detach_textEdit->append(QString::number(sum_free_size, 10));

//    ui->detach_textEdit->append("最大内存占用值");
//    ui->detach_textEdit->append(QString::number(most_max_size, 10));
//    ui->detach_textEdit->append("在数据库内id号是");
//    ui->detach_textEdit->append(QString::number(most_max_id, 10));
//    ui->detach_textEdit->append("发生时间是");
//    ui->detach_textEdit->append(most_max_time);

//    ui->detach_textEdit->append("未释放内存");
//    ui->detach_textEdit->append(QString::number(sum_malloc_size-sum_free_size, 10));
//    query3.exec("select * from two_unfree_memory");
//    while(query3.next())
//    {
//        ui->detach_textEdit->append("**********************");
//        detach_malloc_and_free_id = query3.value(0).toInt();
//        date = query3.value(1).toString();
//        type = query3.value(2).toString();
//        malloc_address = query3.value(3).toString();
//        malloc_size = query3.value(4).toInt();
//        malloc_parent_id = query3.value(5).toInt();

//        ui->detach_textEdit->append("序号");
//        ui->detach_textEdit->append(QString::number(detach_malloc_and_free_id, 10));
//        ui->detach_textEdit->append("日期");
//        ui->detach_textEdit->append(date);
//        ui->detach_textEdit->append("类型");
//        ui->detach_textEdit->append(type);
//        ui->detach_textEdit->append("malloc地址");
//        ui->detach_textEdit->append(malloc_address);
//        ui->detach_textEdit->append("malloc大小");
//        ui->detach_textEdit->append(QString::number(malloc_size, 10));
//        ui->detach_textEdit->append("源序号");
//        ui->detach_textEdit->append(QString::number(malloc_parent_id, 10));

//        if(ui->detach_textEdit->document()->lineCount()%100000 == 0)
//        {
//            ui->detach_textEdit->clear();
//        }
//    }

//}


///**************************************************************
// * Function Name : detach_three_memory
// * Description   : Analyze the usage of the third memory
// * Parameters    : null
// * Returns       : null
// **************************************************************/
//void MainWindow::detach_three_memory(void)
//{
//    QSqlQuery query;
//    QSqlQuery query1;
//    QSqlQuery query2;
//    QSqlQuery query3;
//    int min_malloc_size;
//    int max_malloc_size;
//    int detach_malloc_and_free_id = 1;
//    QDateTime  start_time;
//    QDateTime  end_time;
//    qint64 intervalTimeMS = 0;
//    QString date;
//    QString type;
//    QString malloc_address;
//    int malloc_size;
//    int malloc_parent_id;
//    int exec_count = 0;
//    int sum_malloc_size = 0;
//    int sum_malloc_count = 0;
//    int sum_free_size = 0;
//    int sum_free_count = 0;
//    int start_time_flag = 1;
//    int time_count = 1;
//    int search_flag = 1;
//    int min_search_value = 0;
//    int max_search_value = 2500000;
//    int three_max_size = 0;
//    QString excel_postion;
//    int analyse_time_unit = 0;
//    int most_max_size = 0;
//    int three_max_id = 0;
//    int most_max_id = 0;
//    QString three_max_time;
//    QString most_max_time;


//    analyse_time_unit = ui->separate_lineEdit->text().toInt();
//    if(analyse_time_unit<=0)
//    {
//        QMessageBox::about(NULL, "提示", "分离时间单位有误");
//        return;
//    }

//    if(ui->two_minlineEdit->text() == NULL || ui->two_minlineEdit->text() == NULL)
//    {
//        QMessageBox::about(NULL, "提示", "第三段参数有误");
//        return;
//    }

//    min_malloc_size = ui->two_minlineEdit->text().toInt() * 1024;
//    max_malloc_size = ui->two_minlineEdit->text().toInt() * 1024;

//    if(min_malloc_size > max_malloc_size)
//    {
//        QMessageBox::about(NULL, "提示", "第三段参数有误");
//        return;
//    }

//    if(min_malloc_size <0 || max_malloc_size < 0)
//    {
//        QMessageBox::about(NULL, "提示", "第三段参数有误");
//        return;
//    }

//    xlsx.write("A1", "time");
//    xlsx.write("D1", "three_size");

//    query1.exec("create table three_detach_memory(detach_malloc_and_free_id int primary key, date varchar, type varchar, malloc_address varchar, malloc_size varchar, parent_id int)");
//    query3.exec("create table three_unfree_memory(detach_malloc_and_free_id int primary key, date varchar, type varchar, malloc_address varchar, malloc_size varchar, parent_id int)");

//    while(search_flag)
//    {
//        query.prepare("select malloc_and_free_id, date, type, malloc_address,malloc_size from malloc_and_free where malloc_and_free_id >= :min_search_value and malloc_and_free_id < :max_search_value");
//        query.bindValue(":min_search_value", min_search_value);
//        query.bindValue(":max_search_value", max_search_value);
//        query.exec();

//        query1.exec("BEGIN");
//        query3.exec("BEGIN");

//        while(query.next())
//        {
//            date = query.value(1).toString();
//            type = query.value(2).toString();
//            malloc_address = query.value(3).toString();
//            malloc_size = query.value(4).toInt();
//            malloc_parent_id = query.value(0).toInt();

//            if(start_time_flag == 1)
//            {
//                start_time = QDateTime::fromString(date, "yyyy.MM.dd hh:mm:ss.zzz");
//                start_time_flag = 0;
//            }

//            end_time = QDateTime::fromString(date, "yyyy.MM.dd hh:mm:ss.zzz");

//            intervalTimeMS = start_time.msecsTo(end_time);

//            if(type == "malloc")
//            {
//                if(malloc_size>=min_malloc_size && malloc_size<max_malloc_size)
//                {
//                    query1.prepare("insert into three_detach_memory values(:detach_malloc_and_free_id, '"+date+"', '"+type+"', '"+malloc_address+"', :malloc_size, :parent_id)");
//                    query1.bindValue(":detach_malloc_and_free_id", detach_malloc_and_free_id);
//                    query1.bindValue(":malloc_size", malloc_size);
//                    query1.bindValue(":parent_id", malloc_parent_id);
//                    query1.exec();

//                    query3.prepare("insert into three_unfree_memory values(:detach_malloc_and_free_id, '"+date+"', '"+type+"', '"+malloc_address+"', :malloc_size, :parent_id)");
//                    query3.bindValue(":detach_malloc_and_free_id", detach_malloc_and_free_id);
//                    query3.bindValue(":malloc_size", malloc_size);
//                    query3.bindValue(":parent_id", malloc_parent_id);
//                    query3.exec();

//                    sum_malloc_size += malloc_size;

//                    if(three_max_size < (sum_malloc_size-sum_free_size))
//                    {
//                        three_max_size = (sum_malloc_size-sum_free_size);
//                        three_max_id = detach_malloc_and_free_id;
//                        three_max_time = date;
//                    }

//                    if(most_max_size<(sum_malloc_size-sum_free_size))
//                    {
//                        most_max_size = (sum_malloc_size-sum_free_size);
//                        most_max_id = detach_malloc_and_free_id;
//                        most_max_time = date;
//                    }

//                    if(intervalTimeMS>time_count*analyse_time_unit*1000)
//                    {
//                        lineSeries_3->append(intervalTimeMS/1000/analyse_time_unit, three_max_size);


//                        excel_postion = QString("A%1").arg(time_count+1);
//                        xlsx.write(excel_postion, time_count);

//                        excel_postion = QString("D%1").arg(time_count+1);
//                        xlsx.write(excel_postion, three_max_size);

//                        three_max_size = 0;
//                        time_count++;
//                    }

//                    detach_malloc_and_free_id++;
//                    sum_malloc_count++;

//                    if(sum_malloc_count%10000 == 0)
//                    {
//                        qDebug()<< "sum_malloc_count" <<  sum_malloc_count;//
//                    }
//                }
//            }

//            if(type == "free")
//            {
//                query2.exec("select * from three_unfree_memory where malloc_address = '"+malloc_address+"' order by detach_malloc_and_free_id desc limit 0,1");

//                if(query2.next())
//                {
//                    malloc_size = query2.value(4).toInt();

//                    query1.prepare("insert into three_detach_memory values(:detach_malloc_and_free_id, '"+date+"', '"+type+"', '"+malloc_address+"', :malloc_size, :parent_id)");
//                    query1.bindValue(":detach_malloc_and_free_id", detach_malloc_and_free_id);
//                    query1.bindValue(":malloc_size", malloc_size);
//                    query1.bindValue(":parent_id", malloc_parent_id);
//                    query1.exec();

//                    sum_free_size += malloc_size;

//                    if(three_max_size<(sum_malloc_size-sum_free_size))
//                    {
//                        three_max_size = (sum_malloc_size-sum_free_size);
//                        three_max_id = detach_malloc_and_free_id;
//                        three_max_time = date;
//                    }

//                    if(most_max_size<(sum_malloc_size-sum_free_size))
//                    {
//                        most_max_size = (sum_malloc_size-sum_free_size);
//                        most_max_id = detach_malloc_and_free_id;
//                        most_max_time = date;
//                    }

//                    if(intervalTimeMS>time_count*analyse_time_unit*1000)
//                    {
//                        lineSeries_3->append(intervalTimeMS/1000/analyse_time_unit, three_max_size);

//                        excel_postion = QString("A%1").arg(time_count+1);
//                        xlsx.write(excel_postion, time_count);

//                        excel_postion = QString("D%1").arg(time_count+1);
//                        xlsx.write(excel_postion, three_max_size);

//                        three_max_size = 0;

//                        time_count++;
//                    }

//                    detach_malloc_and_free_id++;
//                    sum_free_count++;

//                    query3.prepare("delete from three_unfree_memory where detach_malloc_and_free_id = :delete_id");
//                    query3.bindValue(":delete_id", query2.value(0).toInt());
//                    query3.exec();//

//                    if(sum_free_count%10000 == 0)
//                    {
//                        qDebug()<< "sum_free_count" <<  sum_free_count;
//                    }
//                }
//            }


//            if(exec_count % 3000 == 0)
//            {
//                query1.exec("COMMIT");
//                query1.exec("BEGIN");
//                query3.exec("COMMIT");
//                query3.exec("BEGIN");
//            }

//            exec_count++;

//        }

//        query1.exec("COMMIT");
//        query3.exec("COMMIT");

//        if(exec_count)
//        {
//            exec_count = 0;
//            min_search_value += 2500000;
//            max_search_value += 2500000;
//        }
//        else
//        {
//            search_flag = 0;
//            break;
//        }

//    }

//    qDebug() << "intervalTimeMS/1000/analyse_time_unit" << intervalTimeMS/1000/analyse_time_unit;
//    qDebug() << "time_count" << time_count;

//    if(time_count <= (intervalTimeMS/1000/analyse_time_unit))
//    {
//        while((time_count-1) <= (intervalTimeMS/1000/analyse_time_unit))
//        {
//            excel_postion = QString("D%1").arg(time_count);
//            xlsx.write(excel_postion, 0);
//            time_count++;
//        }
//    }

//    if(excel_row_count < time_count)
//    {
//        excel_row_count = time_count;
//    }

//    ui->detach_textEdit->append("--------------------------------------------");
//    ui->detach_textEdit->append("第三段内存使用信息");
//    ui->detach_textEdit->append("malloc的次数");
//    ui->detach_textEdit->append(QString::number(sum_malloc_count, 10));
//    ui->detach_textEdit->append("malloc的内存大小");
//    ui->detach_textEdit->append(QString::number(sum_malloc_size, 10));
//    ui->detach_textEdit->append("free的次数");
//    ui->detach_textEdit->append(QString::number(sum_free_count, 10));
//    ui->detach_textEdit->append("free的内存大小");
//    ui->detach_textEdit->append(QString::number(sum_free_size, 10));

//    ui->detach_textEdit->append("最大内存占用值");
//    ui->detach_textEdit->append(QString::number(most_max_size, 10));
//    ui->detach_textEdit->append("最大内存占用值");
//    ui->detach_textEdit->append(QString::number(most_max_size, 10));
//    ui->detach_textEdit->append("在数据库内id号是");
//    ui->detach_textEdit->append(QString::number(most_max_id, 10));
//    ui->detach_textEdit->append("发生时间是");
//    ui->detach_textEdit->append(most_max_time);

//    ui->detach_textEdit->append("未释放内存");
//    ui->detach_textEdit->append(QString::number(sum_malloc_size-sum_free_size, 10));
//    query3.exec("select * from three_unfree_memory");
//    while(query3.next())
//    {
//        ui->detach_textEdit->append("**********************");
//        detach_malloc_and_free_id = query3.value(0).toInt();
//        date = query3.value(1).toString();
//        type = query3.value(2).toString();
//        malloc_address = query3.value(3).toString();
//        malloc_size = query3.value(4).toInt();
//        malloc_parent_id = query3.value(5).toInt();

//        ui->detach_textEdit->append("序号");
//        ui->detach_textEdit->append(QString::number(detach_malloc_and_free_id, 10));
//        ui->detach_textEdit->append("日期");
//        ui->detach_textEdit->append(date);
//        ui->detach_textEdit->append("类型");
//        ui->detach_textEdit->append(type);
//        ui->detach_textEdit->append("malloc地址");
//        ui->detach_textEdit->append(malloc_address);
//        ui->detach_textEdit->append("malloc大小");
//        ui->detach_textEdit->append(QString::number(malloc_size, 10));
//        ui->detach_textEdit->append("源序号");
//        ui->detach_textEdit->append(QString::number(malloc_parent_id, 10));

//        if(ui->detach_textEdit->document()->lineCount()%100000 == 0)
//        {
//            ui->detach_textEdit->clear();
//        }
//    }

//}

/**************************************************************
 * Function Name : wheelEvent
 * Description   : Zoom in and out of the waveform
 * Parameters    : event
 * Returns       : null
 **************************************************************/
void MainWindow::wheelEvent(QWheelEvent *event)
{
    if (event->delta() > 0) {
        m_chart_1->zoom(1.1);
    } else {
        m_chart_1->zoom(10.0/11);
    }

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
 **************************************************************/
void MainWindow::on_detachButton_clicked()
{


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

    detach_count = ui->detach_count_Box->currentText().toInt();

    // 添加系列
    lineSeries_1 = new QLineSeries();
    lineSeries_2 = new QLineSeries();
    lineSeries_3 = new QLineSeries();
    lineSeries_4 = new QLineSeries();
    lineSeries_5 = new QLineSeries();
    lineSeries_6 = new QLineSeries();
    lineSeries_7 = new QLineSeries();
    lineSeries_8 = new QLineSeries();
    lineSeries_9 = new QLineSeries();
    lineSeries_10 = new QLineSeries();


    if(detach_count>=1)
    {
        min_malloc_size = ui->one_left_lineedit->text().toInt();
        max_malloc_size = ui->one_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("drop table one_unfree_memory");
    }

    if(detach_count>=2)
    {
        min_malloc_size = ui->two_left_lineedit->text().toInt();
        max_malloc_size = ui->two_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("drop table one_unfree_memory");
    }

    if(detach_count>=3)
    {
        min_malloc_size = ui->three_left_lineedit->text().toInt();
        max_malloc_size = ui->three_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("drop table one_unfree_memory");
    }

    if(detach_count>=4)
    {
        min_malloc_size = ui->four_left_lineedit->text().toInt();
        max_malloc_size = ui->four_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("drop table one_unfree_memory");
    }

    if(detach_count>=5)
    {
        min_malloc_size = ui->five_left_lineedit->text().toInt();
        max_malloc_size = ui->five_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("drop table one_unfree_memory");
    }

    if(detach_count>=6)
    {
        min_malloc_size = ui->six_left_lineedit->text().toInt();
        max_malloc_size = ui->six_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("drop table one_unfree_memory");
    }

    if(detach_count>=7)
    {
        min_malloc_size = ui->seven_left_lineedit->text().toInt();
        max_malloc_size = ui->seven_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("drop table one_unfree_memory");
    }

    if(detach_count>=8)
    {
        min_malloc_size = ui->eight_left_lineedit->text().toInt();
        max_malloc_size = ui->eight_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("drop table one_unfree_memory");
    }

    if(detach_count>=9)
    {
        min_malloc_size = ui->nine_left_lineedit->text().toInt();
        max_malloc_size = ui->nine_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("drop table one_unfree_memory");
    }

    if(detach_count>=10)
    {
        min_malloc_size = ui->ten_left_lineedit->text().toInt();
        max_malloc_size = ui->ten_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        query.exec("drop table one_unfree_memory");
    }


    min_malloc_size = 0;
    max_malloc_size = 0;

    ui->progressBar->setValue(100);




//    min_malloc_size = ui->one_left_lineedit->text().toInt();
//    max_malloc_size = ui->one_right_lineedit->text().toInt();

//    qDebug() << "min_malloc_size" << min_malloc_size;
//    qDebug() << "max_malloc_size" << max_malloc_size;

//    detach_one_memory(min_malloc_size, max_malloc_size);

//    query.exec("drop table one_unfree_memory");

//    qDebug() << "-----------------------------------";

//    min_malloc_size = 0;
//    max_malloc_size = 0;

//    ui->progressBar->setValue(100);

//    analyse_memory_count = 0;
//    sum_exec_count = 0;
//    detach_count = 0;


//    // 给系列追加数据
//    detach_one_memory();//
//    detach_two_memory();
//    detach_three_memory();


    //    // 给系列追加数据
    //    for (size_t i = 0; i < 1000; i++)
    //    {
    //        lineSeries_1->append(i, sin(0.6f*i));
    //    }
    lineSeries_1->setUseOpenGL(true);
    lineSeries_2->setUseOpenGL(true);
    lineSeries_3->setUseOpenGL(true);
    lineSeries_4->setUseOpenGL(true);
    lineSeries_5->setUseOpenGL(true);
    lineSeries_6->setUseOpenGL(true);
    lineSeries_7->setUseOpenGL(true);
    lineSeries_8->setUseOpenGL(true);
    lineSeries_9->setUseOpenGL(true);
    lineSeries_10->setUseOpenGL(true);


    qDebug()<<"lineSeries_1->count()" <<lineSeries_1->count();
    qDebug()<<"lineSeries_2->count()" <<lineSeries_2->count();
    qDebug()<<"lineSeries_3->count()" <<lineSeries_3->count();
    qDebug()<<"lineSeries_4->count()" <<lineSeries_4->count();
    qDebug()<<"lineSeries_5->count()" <<lineSeries_5->count();
    qDebug()<<"lineSeries_6->count()" <<lineSeries_6->count();
    qDebug()<<"lineSeries_7->count()" <<lineSeries_7->count();
    qDebug()<<"lineSeries_8->count()" <<lineSeries_8->count();
    qDebug()<<"lineSeries_9->count()" <<lineSeries_9->count();
    qDebug()<<"lineSeries_10->count()" <<lineSeries_10->count();

//    if(lineSeries_1->count() == 0 && lineSeries_2->count() == 0 && lineSeries_3->count() == 0)
//    {
//        QMessageBox::about(NULL, "提示", "有内存未找到");//
//    }

    // 构建图表对象
    m_chart_1 = new QChart();

    // 构建折线系列
    m_chart_1->addSeries(lineSeries_1);
    m_chart_1->addSeries(lineSeries_2);
    m_chart_1->addSeries(lineSeries_3);
    m_chart_1->addSeries(lineSeries_4);
    m_chart_1->addSeries(lineSeries_5);
    m_chart_1->addSeries(lineSeries_6);
    m_chart_1->addSeries(lineSeries_7);
    m_chart_1->addSeries(lineSeries_8);
    m_chart_1->addSeries(lineSeries_9);
    m_chart_1->addSeries(lineSeries_10);

    // 设置默认坐标轴
    m_chart_1->createDefaultAxes();


    //     隐藏系列
    lineSeries_1->hide();
    lineSeries_2->hide();
    lineSeries_3->hide();
    lineSeries_4->hide();
    lineSeries_5->hide();
    lineSeries_6->hide();
    lineSeries_7->hide();
    lineSeries_8->hide();
    lineSeries_9->hide();
    lineSeries_10->hide();

    if(analyse_memory_count>0)
    {
        lineSeries_1->show();
    }
    if(analyse_memory_count>1)
    {
        lineSeries_2->show();
    }
    if(analyse_memory_count>2)
    {
        lineSeries_3->show();
    }
    if(analyse_memory_count>3)
    {
        lineSeries_4->show();
    }
    if(analyse_memory_count>4)
    {
        lineSeries_5->show();
    }
    if(analyse_memory_count>5)
    {
        lineSeries_6->show();
    }
    if(analyse_memory_count>6)
    {
        lineSeries_7->show();
    }
    if(analyse_memory_count>7)
    {
        lineSeries_8->show();
    }
    if(analyse_memory_count>8)
    {
        lineSeries_9->show();
    }
    if(analyse_memory_count>9)
    {
        lineSeries_10->show();
    }

    // 设置主题
    m_chart_1->setTheme(QtCharts::QChart::ChartThemeBlueCerulean);

    // 设置系列标题
    m_chart_1->setTitle(QString::fromLocal8Bit("One Chart"));

    // 修改波形图样式
    change_style();

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

    chartView->setRubberBand(QChartView::RectangleRubberBand);

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
 * Function Name : MainWindow
 * Description   : Deconstructe Function MainWindow
 * Parameters    : null
 * Returns       : null
 **************************************************************/
MainWindow::~MainWindow()
{
    delete ui;
}


/**************************************************************
 * Function Name : on_saveButton_clicked
 * Description   : Save three memory analysis results
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::on_saveButton_clicked()
{
    //Parameter validity check
    if(ui->detach_textEdit->document()->isEmpty())
    {
        QMessageBox::about(NULL, "提示", "无数据");
        return;
    }

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

//    QScreen * screen = QGuiApplication::primaryScreen();
//    QPixmap p = screen->grabWindow(chartView->winId());
//    QImage image = p.toImage();
//    image.save("./log/chart.png");

//    SaveChartViewSvg();

    QString save_excel_name;
    save_excel_name = QString("./log/单位为%1s的内存分析表.xlsx").arg(ui->separate_lineEdit->text().toInt());
    dir.remove(save_excel_name);
    xlsx.saveAs(save_excel_name);


    QMessageBox::about(NULL, "提示", "文件保存成功");//

}

/**************************************************************
 * Function Name : change_style
 * Description   : Modify the overall display effect
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::change_style() {

    //设置整体显示主题
    {
        setMainWindowPalette();
    }

    // 设置图表背景
    {
        m_chart_1->setBackgroundVisible(true);
        m_chart_1->setBackgroundBrush(Qt::lightGray);
        // m_chart->setBackgroundBrush(Qt::transparent);

        QPen penBackground;
        penBackground.setStyle(Qt::DotLine);
        penBackground.setColor(Qt::green);
        m_chart_1->setBackgroundPen(penBackground);
    }

    // 设置图表背景
    {
        m_chart_1->setPlotAreaBackgroundVisible(true);
        m_chart_1->setPlotAreaBackgroundBrush(Qt::gray);

    }

    // 图表
    {
        // 设置图表字体
        QFont fontTitle;
        fontTitle.setFamily(QString::fromLocal8Bit("宋体"));
        fontTitle.setPointSizeF(20.f);
        m_chart_1->setTitleFont(fontTitle);

        // 设置标题刷
        m_chart_1->setTitleBrush(Qt::black);
    }

    {
        // 系列排布
        //        m_chart_1->legend()->setAlignment(Qt::AlignLeft);

    }

    // 系列
    {
        QPen pn1(Qt::green, 1.f);
        lineSeries_1->setPen(pn1);
        lineSeries_1->setName("line_1");

        QPen pn2(Qt::black, 1.f);
        lineSeries_2->setPen(pn2);
        lineSeries_2->setName("line_2");

        QPen pn3(Qt::red, 1.f);
        lineSeries_3->setPen(pn3);
        lineSeries_3->setName("line_3");

        QPen pn4(Qt::yellow, 1.f);
        lineSeries_4->setPen(pn4);
        lineSeries_4->setName("line_4");

        QPen pn5(Qt::blue, 1.f);
        lineSeries_5->setPen(pn5);
        lineSeries_5->setName("line_5");

        QPen pn6(Qt::darkRed, 1.f);
        lineSeries_6->setPen(pn6);
        lineSeries_6->setName("line_6");

        QPen pn7(Qt::darkBlue, 1.f);
        lineSeries_7->setPen(pn7);
        lineSeries_7->setName("line_7");

        QPen pn8(Qt::darkYellow, 1.f);
        lineSeries_8->setPen(pn8);
        lineSeries_8->setName("line_8");

        QPen pn9(Qt::darkGreen, 1.f);
        lineSeries_9->setPen(pn9);
        lineSeries_9->setName("line_9");

        QPen pn10(Qt::cyan, 1.f);
        lineSeries_10->setPen(pn10);
        lineSeries_10->setName("line_10");
    }

    //    // 设置动画
    //    QChart::AnimationOptions aniOptions = QChart::AllAnimations;
    //    m_chart_1->setAnimationOptions(aniOptions);
}


/**************************************************************
 * Function Name : setMainWindowPalette
 * Description   : Construct function setMainWindowPalette
 * Parameters    : ...
 * Returns       : null
 **************************************************************/
void MainWindow::setMainWindowPalette() {
    QChart::ChartTheme theme = QChart::ChartThemeBlueIcy;
    m_chart_1->setTheme(theme);

    // 修改整体背景框
    QPalette pal = window()->palette();
    switch (theme)
    {
    case QtCharts::QChart::ChartThemeLight:
        pal.setColor(QPalette::Window, QRgb(0xf0f0f0));
        pal.setColor(QPalette::WindowText, QRgb(0x404040));
        break;
    case QtCharts::QChart::ChartThemeBlueCerulean:
        pal.setColor(QPalette::Window, QRgb(0xf0f0f0));
        pal.setColor(QPalette::WindowText, QRgb(0x404040));
        break;
    case QtCharts::QChart::ChartThemeDark:
        pal.setColor(QPalette::Window, QRgb(0x121218));
        pal.setColor(QPalette::WindowText, QRgb(0xd6d6d6));
        break;
    case QtCharts::QChart::ChartThemeBrownSand:
        pal.setColor(QPalette::Window, QRgb(0x9e8965));
        pal.setColor(QPalette::WindowText, QRgb(0x404044));
        break;
    case QtCharts::QChart::ChartThemeBlueNcs:
        pal.setColor(QPalette::Window, QRgb(0x18bba));
        pal.setColor(QPalette::WindowText, QRgb(0x404044));
        break;
    case QtCharts::QChart::ChartThemeHighContrast:
        pal.setColor(QPalette::Window, QRgb(0xffab03));
        pal.setColor(QPalette::WindowText, QRgb(0x181818));
        break;
    case QtCharts::QChart::ChartThemeBlueIcy:
        pal.setColor(QPalette::Window, QRgb(0xcee7f0));
        pal.setColor(QPalette::WindowText, QRgb(0x404044));
        break;
    case QtCharts::QChart::ChartThemeQt:
    default:
        pal.setColor(QPalette::Window, QRgb(0xf0f0f0));
        pal.setColor(QPalette::WindowText, QRgb(0x404044));
        break;
    }
    window()->setPalette(pal);
}


/**************************************************************
 * Function Name : SaveChartViewSvg
 * Description   : Convert waveform image to svg image
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::SaveChartViewSvg()
{
    QString filePath = "./log/wave_form.svg";
    if (filePath == "")
        return;
    QSvgGenerator generator;
    generator.setFileName(filePath);
    generator.setSize(QSize(this->width(), this->height()));
    generator.setViewBox(QRect(0, 0, this->width(), this->height()));
    generator.setTitle("SVG Example");
    generator.setDescription("This SVG file is generated by Qt.");
    QPainter painter;
    painter.begin(&generator);
    chartView->render(&painter);
    painter.end();
}
