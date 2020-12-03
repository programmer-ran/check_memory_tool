/***************************************************************
* Filename     : mainwindow.cpp
* Description   : Add UI and functions to application window objects
* Version      : 1.0
* History       :
* penghongran 2020-11-30  finished
**************************************************************/
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
#include <synchapi.h>

/**************************************************************
 * Function Name : MainWindow
 * Description   : Construct function MainWindow
 * Parameters    : ...
 * Returns       : MainWindow
 **************************************************************/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    memory_pool_chart(new QChart)
{
    ui->setupUi(this);

    //set softwave title
    this->setWindowTitle("内存分析编辑器");

    //Connect signals and slots
    QObject::connect(&serial, &QSerialPort::readyRead, this, &MainWindow::serialPort_readyRead);

    QObject::connect(ui->detach_count_Box, SIGNAL(currentTextChanged(QString)), this, SLOT(update_detach_value(void)));

    //Baud rate default selection drop down the fourth item: 115200
    ui->baudrateBox->setCurrentIndex(4);

    //init progressBar
    ui->progressBar->setRange(0,100);
    ui->progressBar->setValue(0);

    //init detach_count_Box's value
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


    //get memory_pool's start_address and end_address
    QSqlQuery query;

    query.exec("select start_and_end_address from malloc_and_free where id == 1");

    //Every time next is called, the data in the query table is moved to the next
    if(query.next())
    {
        start_memory_address = query.value(0).toString();
    }

    query.exec("select start_and_end_address from malloc_and_free where id == 2");

    //Every time next is called, the data in the query table is moved to the next
    if(query.next())
    {
        end_memory_address = query.value(0).toString();
    }

}

/**************************************************************
 * Function Name : serialPort_readyRead
 * Description   : When the serial port receives information,
 *                 the function responds to the buffer, analyse
 *                 malloc and free messages to database
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::serialPort_readyRead()
{
    QString recv_serialport_data;
    QString recvtextEdit_last_line_data;
    int recvtextEdit_last_line_count = 0;
    QSqlQuery query;

    //Read data from the receive buffer
    QByteArray template_receive_buffer = serial.readAll();

    //Load the data in the buffer to the receiving window
    recv_serialport_data = QString(template_receive_buffer);
    ui->recvTextEdit->insertPlainText(recv_serialport_data);
    ui->recvTextEdit->moveCursor(QTextCursor::End);

    recvtextEdit_last_line_count = ui->recvTextEdit->document()->lineCount();

//    qDebug() << "serial.readBufferSize()" << serial.readBufferSize();
//    qDebug() << "recvtextEdit_last_line_count" << recvtextEdit_last_line_count;

    //According receiving window to analyse malloc and free and save in database
    analyse_receive_window_data();

    //Prevent crashes caused by excessive data in the receiving window and analysis window
    if(recvtextEdit_last_line_count >= 3000)
    {
        recvtextEdit_last_line_data = ui->recvTextEdit->document()->findBlockByLineNumber(recvtextEdit_last_line_count-1).text();

        if((recvtextEdit_last_line_data.length() == 0))
        {
            ui->recvTextEdit->clear();
            ui->analyseTextEdit->clear();
            recvEdit_analyzed_number = 0;
        }

        query.exec("COMMIT");
        query.exec("BEGIN");
    }

}

/**************************************************************
 * Function Name : analyse_receive_window_data
 * Description   : Perform malloc and free extraction according
 *                 to the receiving serial port and load the
 *                 results into the analysis window and
 * database simultaneously
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::analyse_receive_window_data()
{
    int malloc_postion = 0;
    int free_postion = 0;
    int memory_pool_postion = 0;
    static int recvtextEdit_last_line_number = 0;
    static int id = 1;

    QSqlQuery query;
    QString malloc_text = "=m ";
    QString free_text = "=f ";
    QString memory_text = "=mem ";
    QString malloc_memory_address;
    QString string_malloc_memory_size;
    int malloc_memory_size = 0;
    QString free_memory_address;
    QString analyse_data;
    static QString last_analyse_data = "";
    QDateTime current_date_time =QDateTime::currentDateTime();
    QString current_date =current_date_time.toString("yyyy.MM.dd hh:mm:ss.zzz");

    recvtextEdit_last_line_number = (ui->recvTextEdit->document()->lineCount());

//    qDebug()<< "recvEdit_analyzed_number" << recvEdit_analyzed_number;
//    qDebug()<< "recvtextEdit_last_line_number" << recvtextEdit_last_line_number;

    //Analyze the data sent from the serial port line by line
    while(recvEdit_analyzed_number<recvtextEdit_last_line_number)
    {
        analyse_data = ui->recvTextEdit->document()->findBlockByLineNumber(recvEdit_analyzed_number-1).text();
//        qDebug()<< "analyse_data" << analyse_data;

        recvEdit_analyzed_number++;

        if(last_analyse_data == analyse_data)
        {
            continue;
        }

        //Analyze malloc information and synchronize the results to the database and analysis window
        if((analyse_data.indexOf(malloc_text,malloc_postion)!= -1))
        {

            //get malloc_address and malloc_size
            malloc_memory_address = analyse_data.mid(analyse_data.indexOf(malloc_text, malloc_postion)+3, 8);

            string_malloc_memory_size = analyse_data.mid(analyse_data.indexOf(malloc_text, malloc_postion)+12, 6);
            malloc_memory_size = string_malloc_memory_size.toInt(NULL, 16);

            //Synchronize analysis results to analysis window
            ui->analyseTextEdit->append("malloc为 "+malloc_memory_address+"");
            ui->analyseTextEdit->append("malloc大小是 "+string_malloc_memory_size+"");

            //Synchronize analysis results to database
            query.prepare("insert into malloc_and_free values(:id,'"+current_date+"','malloc','"+malloc_memory_address+"', :malloc_memory_size, '')");
            query.bindValue(":id",id);
            query.bindValue(":malloc_memory_size", malloc_memory_size);
            query.exec();

            last_analyse_data = analyse_data;

            id++;
        }

        //Analyze malloc information and synchronize the results to the database and analysis window
        else if((analyse_data.indexOf(free_text, free_postion)!= -1))
        {

            //get free_address
            free_memory_address = analyse_data.mid(analyse_data.indexOf(free_text, free_postion)+3, 8);

            //Synchronize analysis results to analysis window
            ui->analyseTextEdit->append("free为 "+free_memory_address+"");

            //Synchronize analysis results to database
            query.prepare("insert into malloc_and_free values(:id,'"+current_date+"','free','"+free_memory_address+"', 0, '')");
            query.bindValue(":id",id);
            query.exec();

            last_analyse_data = analyse_data;

            id++;
        }

        //Analyze memory_pool information and synchronize the results to the database and analysis window
        else if((analyse_data.indexOf(memory_text, memory_pool_postion)!= -1))
        {
            //get memory_pool start_address and end_address
            start_memory_address = analyse_data.mid(analyse_data.indexOf(memory_text, memory_pool_postion)+5, 8);
            end_memory_address = analyse_data.mid(analyse_data.indexOf(memory_text, memory_pool_postion)+14, 9);

            //Synchronize analysis results to database
            query.exec("update malloc_and_free set start_and_end_address = '"+start_memory_address+"' where id = 1");

            query.exec("update malloc_and_free set start_and_end_address = '"+end_memory_address+"' where id = 2");

            last_analyse_data = analyse_data;

        }
    }
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

    //Find available serial ports through QSerialPortInfo
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->portNameBox->addItem(info.portName());
    }
}


/**************************************************************
 * Function Name : on_openButton_clicked
 * Description   : Open the serial port for data reception
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::on_openButton_clicked()
{
    QSqlQuery query;

    if(ui->openButton->text()==QString("打开串口"))
    {
        //Set the serial port name
        serial.setPortName(ui->portNameBox->currentText());
        //Set the baud rate
        serial.setBaudRate(ui->baudrateBox->currentText().toInt());
        //Set the number of data bits
        switch(ui->dataBitsBox->currentIndex())
        {
        case 8: serial.setDataBits(QSerialPort::Data8); break;
        default: break;
        }
        //set parity check
        switch(ui->ParityBox->currentIndex())
        {
        case 0: serial.setParity(QSerialPort::NoParity); break;
        default: break;
        }
        //set stop bit
        switch(ui->stopBitsBox->currentIndex())
        {
        case 1: serial.setStopBits(QSerialPort::OneStop); break;
        case 2: serial.setStopBits(QSerialPort::TwoStop); break;
        default: break;
        }
        //Set up flow control
        serial.setFlowControl(QSerialPort::NoFlowControl);

        //Open the serial port
        if(!serial.open(QIODevice::ReadWrite))
        {
            QMessageBox::about(NULL, "提示", "无法打开串口！");
            return;
        }

        //Drop-down menu control disabled
        ui->portNameBox->setEnabled(false);
        ui->baudrateBox->setEnabled(false);
        ui->dataBitsBox->setEnabled(false);
        ui->ParityBox->setEnabled(false);
        ui->stopBitsBox->setEnabled(false);
        ui->searchButton->setEnabled(false);
        ui->clearButton->setEnabled(false);
        ui->detachButton->setEnabled(false);
        ui->saveButton->setEnabled(false);

        query.exec("BEGIN");

        ui->openButton->setText(QString("关闭串口"));

    }
    else
    {
        //Close the serial port
        serial.close();

        //create database index
        query.exec("create index malloc_size_index on malloc_and_free(malloc_size)");

        query.exec("create index type_index on malloc_and_free(type)");

        //Drop-down menu control enable
        ui->portNameBox->setEnabled(true);
        ui->baudrateBox->setEnabled(true);
        ui->dataBitsBox->setEnabled(true);
        ui->ParityBox->setEnabled(true);
        ui->stopBitsBox->setEnabled(true);
        ui->searchButton->setEnabled(true);
        ui->clearButton->setEnabled(true);
        ui->detachButton->setEnabled(true);
        ui->saveButton->setEnabled(true);

        ui->analyseTextEdit->moveCursor(QTextCursor::Start);
        ui->analyseTextEdit->moveCursor(QTextCursor::End);

        query.exec("COMMIT");

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

    //clear analyse table and view
    query.exec("drop table one_detach_memory");
    query.exec("drop table one_unfree_memory");
    query.exec("drop table two_detach_memory");
    query.exec("drop table two_unfree_memory");
    query.exec("drop table three_detach_memory");
    query.exec("drop table three_unfree_memory");
    query.exec("drop table memory_pool_table");
    query.exec("drop view malloc");
    query.exec("drop view free");

    //clear text window
    ui->detach_textEdit->clear();


    //clear excel
    for(int i=1; i<= excel_row_count; i++)
    {
        for(int j = 1; j < excel_column_count; j++)
        {
            xlsx.write(i, j, "");
        }
    }
}


/**************************************************************
 * Function Name : progress_bar_prepartion
 * Description   : Draw the scene graph in advance to prevent
 *                 the interface from freezing
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::progress_bar_prepartion(void)
{
    upper_boundary_of_free_memory = new QLineSeries();
    lower_boundary_of_free_memory = new QLineSeries();
    upper_boundary_of_first_segment_memory = new QLineSeries();
    lower_boundary_of_first_segment_memory = new QLineSeries();
    upper_boundary_of_second_segment_memory = new QLineSeries();
    lower_boundary_of_second_segment_memory = new QLineSeries();
    upper_boundary_of_third_segment_memory = new QLineSeries();
    lower_boundary_of_third_segment_memory = new QLineSeries();
    upper_boundary_of_fourth_segment_memory = new QLineSeries();
    lower_boundary_of_fourth_segment_memory = new QLineSeries();
    upper_boundary_of_fifth_segment_memory = new QLineSeries();
    lower_boundary_of_fifth_segment_memory = new QLineSeries();
    upper_boundary_of_sixth_segment_memory = new QLineSeries();
    lower_boundary_of_sixth_segment_memory = new QLineSeries();
    upper_boundary_of_seventh_segment_memory = new QLineSeries();
    lower_boundary_of_seventh_segment_memory = new QLineSeries();
    upper_boundary_of_eighth_segment_memory = new QLineSeries();
    lower_boundary_of_eighth_segment_memory = new QLineSeries();
    upper_boundary_of_ninth_segment_memory = new QLineSeries();
    lower_boundary_of_ninth_segment_memory = new QLineSeries();
    upper_boundary_of_tenth_segment_memory = new QLineSeries();
    lower_boundary_of_tenth_segment_memory = new QLineSeries();

    //init progressBar value
    ui->progressBar->setValue(0);


    upper_boundary_of_free_memory->setUseOpenGL(true);
    lower_boundary_of_free_memory->setUseOpenGL(true);


    upper_boundary_of_first_segment_memory->setUseOpenGL(true);
    lower_boundary_of_first_segment_memory->setUseOpenGL(true);
    upper_boundary_of_second_segment_memory->setUseOpenGL(true);
    lower_boundary_of_second_segment_memory->setUseOpenGL(true);
    upper_boundary_of_third_segment_memory->setUseOpenGL(true);
    lower_boundary_of_third_segment_memory->setUseOpenGL(true);
    upper_boundary_of_fourth_segment_memory->setUseOpenGL(true);
    lower_boundary_of_fourth_segment_memory->setUseOpenGL(true);
    upper_boundary_of_fifth_segment_memory->setUseOpenGL(true);
    lower_boundary_of_fifth_segment_memory->setUseOpenGL(true);
    upper_boundary_of_sixth_segment_memory->setUseOpenGL(true);
    lower_boundary_of_sixth_segment_memory->setUseOpenGL(true);
    upper_boundary_of_seventh_segment_memory->setUseOpenGL(true);
    lower_boundary_of_seventh_segment_memory->setUseOpenGL(true);
    upper_boundary_of_eighth_segment_memory->setUseOpenGL(true);
    lower_boundary_of_eighth_segment_memory->setUseOpenGL(true);
    upper_boundary_of_ninth_segment_memory->setUseOpenGL(true);
    lower_boundary_of_ninth_segment_memory->setUseOpenGL(true);
    upper_boundary_of_tenth_segment_memory->setUseOpenGL(true);
    lower_boundary_of_tenth_segment_memory->setUseOpenGL(true);


    //Build chart objects
    memory_pool_chart = new QChart();


    memory_pool_chart->addSeries(upper_boundary_of_free_memory);
    memory_pool_chart->addSeries(lower_boundary_of_free_memory);
    memory_pool_chart->addSeries(upper_boundary_of_first_segment_memory);
    memory_pool_chart->addSeries(lower_boundary_of_first_segment_memory);
    memory_pool_chart->addSeries(upper_boundary_of_second_segment_memory);
    memory_pool_chart->addSeries(lower_boundary_of_second_segment_memory);
    memory_pool_chart->addSeries(upper_boundary_of_third_segment_memory);
    memory_pool_chart->addSeries(lower_boundary_of_third_segment_memory);
    memory_pool_chart->addSeries(upper_boundary_of_fourth_segment_memory);
    memory_pool_chart->addSeries(lower_boundary_of_fourth_segment_memory);
    memory_pool_chart->addSeries(upper_boundary_of_fifth_segment_memory);
    memory_pool_chart->addSeries(lower_boundary_of_fifth_segment_memory);
    memory_pool_chart->addSeries(upper_boundary_of_sixth_segment_memory);
    memory_pool_chart->addSeries(lower_boundary_of_sixth_segment_memory);
    memory_pool_chart->addSeries(upper_boundary_of_seventh_segment_memory);
    memory_pool_chart->addSeries(lower_boundary_of_seventh_segment_memory);
    memory_pool_chart->addSeries(upper_boundary_of_eighth_segment_memory);
    memory_pool_chart->addSeries(lower_boundary_of_eighth_segment_memory);
    memory_pool_chart->addSeries(upper_boundary_of_ninth_segment_memory);
    memory_pool_chart->addSeries(lower_boundary_of_ninth_segment_memory);
    memory_pool_chart->addSeries(upper_boundary_of_tenth_segment_memory);
    memory_pool_chart->addSeries(lower_boundary_of_tenth_segment_memory);

    //Build the ChartView container
    if(1 == paint_multi_map_flag)
    {
        paint_multi_map_flag = 0;
        chartView = new ChartView(memory_pool_chart);
    }
    else
    {
        ui->horizontalLayout_area->removeWidget(chartView);
        delete chartView;
        chartView = new ChartView(memory_pool_chart);
    }

    //Add the ChartView container to the UI interface
    ui->horizontalLayout_area->addWidget(chartView);

    //create start analyse
    QMessageBox *prompt_box = new QMessageBox(QMessageBox::Information,tr("提示"),tr("开始分析数据"));
    QTimer::singleShot(1000,prompt_box,SLOT(accept()));//

    prompt_box->exec();

}

/**************************************************************
 * Function Name : set_excel_column_name
 * Description   : According to parameter to set excel column name
 * Parameters    : excel_column_count -- need set column count
 * Returns       : null
 **************************************************************/
void MainWindow::set_excel_column_name(int excel_column_count)
{
    //Set the column name of the excel table
    xlsx.write("A1", "time");
    switch(excel_column_count)
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
}

/**************************************************************
 * Function Name : set_excel_column_name
 * Description   : According to parameter to set excel value
 * Parameters    : time_count -- need set excel postion
 *                 second_max_size -- need set excel value
 * Returns       : null
 **************************************************************/
void MainWindow::set_excel_value(int time_count, uint second_max_size)
{
    QString excel_postion;

    excel_postion = QString("A%1").arg(time_count+1);
    xlsx.write(excel_postion, time_count);

    switch(analyse_memory_count)
    {
    case 0:
        excel_postion = QString("B%1").arg(time_count+1);
        xlsx.write(excel_postion, second_max_size);
        break;
    case 1:
        excel_postion = QString("C%1").arg(time_count+1);
        xlsx.write(excel_postion, second_max_size);
        break;
    case 2:
        excel_postion = QString("D%1").arg(time_count+1);
        xlsx.write(excel_postion, second_max_size);
        break;

    case 3:
        excel_postion = QString("E%1").arg(time_count+1);
        xlsx.write(excel_postion, second_max_size);
        break;
    case 4:
        excel_postion = QString("F%1").arg(time_count+1);
        xlsx.write(excel_postion, second_max_size);
        break;
    case 5:
        excel_postion = QString("G%1").arg(time_count+1);
        xlsx.write(excel_postion, second_max_size);
        break;

    case 6:
        excel_postion = QString("H%1").arg(time_count+1);
        xlsx.write(excel_postion, second_max_size);
        break;
    case 7:
        excel_postion = QString("I%1").arg(time_count+1);
        xlsx.write(excel_postion, second_max_size);
        break;
    case 8:
        excel_postion = QString("J%1").arg(time_count+1);
        xlsx.write(excel_postion, second_max_size);
        break;

    case 9:
        excel_postion = QString("K%1").arg(time_count+1);
        xlsx.write(excel_postion, second_max_size);
        break;
    }
}

/**************************************************************
 * Function Name : detach_one_memory
 * Description   : Analyze the usage of the memory_pool
 * Parameters    : min_size
 *                 max_size
 * Returns       : null
 **************************************************************/
void MainWindow::detach_one_memory(uint min_malloc_size, uint max_malloc_size)
{
    QSqlQuery query;
    QSqlQuery query1;
    QSqlQuery query2;
    QSqlQuery query3;
    QSqlQuery query4;
    int id = 1;
    QDateTime start_time;
    QDateTime end_time;
    qint64 intervalTimeMS = 0;
    qint64 last_intervalTimeMS = 0;
    qint64 time_between_two_intervals = 0;
    QString date;
    QString type;
    QString malloc_address;
    uint malloc_size = 0;
    uint malloc_parent_id = 0;
    int exec_count = 0;
    uint sum_malloc_size = 0;
    uint sum_malloc_count = 0;
    uint sum_free_size = 0;
    uint sum_free_count = 0;
    int start_time_flag = 1;
    int time_count = 1;
    int search_finish_flag = 1;
    uint min_search_value = 0;
    uint max_search_value = 2500000;
    uint second_max_size = 0;
    int analyse_time_unit = 0;
    uint most_max_size = 0;
    uint most_max_id = 0;
    QString most_max_time;
    uint most_malloc_count = 0;
    uint most_free_count = 0;

    int sum_table_count = 0;
    int progress_percentage = 0;
    int detach_count = 0;

    QString analyse_memory_messages;

    uint memory_pool_start_memory_address = 0;
    uint memory_pool_end_memory_address = 0;

    //Parameter validity check
    if(min_malloc_size > max_malloc_size)
    {
        QMessageBox::about(NULL, "提示", "最小参数有误");
        return;
    }

    //Get how much memory needs to be analyzed
    detach_count = ui->detach_count_Box->currentText().toInt();

    //Get how many pieces of data each memory has
    query4.exec("select id from malloc_and_free order by id desc");

    if(query4.next())
    {
        sum_table_count = query4.value(0).toInt();
    }

    //Set the minimum time unit for analysis results
    analyse_time_unit = ui->separate_lineEdit->text().toInt();
    if(analyse_time_unit<=0)
    {
        QMessageBox::about(NULL, "提示", "分析时间单位有误");
        return;
    }

    //set excel column name
    set_excel_column_name(analyse_memory_count);

    //Create analysis table one_unfree to match malloc and free
    query3.exec("create table one_unfree_memory(id int primary key, "
                "date varchar, type varchar, malloc_address varchar, "
                "malloc_size varchar, malloc_and_free_id int)");

    //Always extract database data for analysis until the extracted data is empty
    while(1 == search_finish_flag)
    {
        //Extract 2.5 million pieces of data each time for analysis to prevent the query handle from crashing
        query.prepare("select * from malloc_and_free where id >= :min_search_value and id < :max_search_value");
        query.bindValue(":min_search_value", min_search_value);
        query.bindValue(":max_search_value", max_search_value);
        query.exec();

        //Set multiple statements to commit the same transaction to improve database insertion speed
        query3.exec("BEGIN");

        while(query.next())
        {
            //Set multiple statements to commit the same transaction to improve database insertion speed
            date = query.value(1).toString();
            type = query.value(2).toString();
            malloc_address = query.value(3).toString();
            malloc_size = query.value(4).toInt();
            malloc_parent_id = query.value(0).toInt();

            //Get the relative time of each memory application release
            if(1 == start_time_flag)
            {
                start_time = QDateTime::fromString(date, "yyyy.MM.dd hh:mm:ss.zzz");
                start_time_flag = 0;
            }

            end_time = QDateTime::fromString(date, "yyyy.MM.dd hh:mm:ss.zzz");

            intervalTimeMS = start_time.msecsTo(end_time);

            time_between_two_intervals = intervalTimeMS - last_intervalTimeMS;

            //Get the relative time of each memory application release
            progress_percentage =  double(sum_exec_count + malloc_parent_id)/double(sum_table_count)/double(detach_count)*100;

            //            qDebug()<< "progress_percentage" <<progress_percentage;
            ui->progressBar->setValue(progress_percentage);

            //Ensure that the memory usage value keeps changing when there is no memory request release in the current second
            second_max_size = (sum_malloc_size-sum_free_size);

            for(int i=0; i<(time_between_two_intervals)/1000/analyse_time_unit; i++)
            {
                set_excel_value(time_count, second_max_size);

                time_count++;
            }

            if(type == "malloc")
            {
                //Filter out memory information that malloc_size does not meet the boundary
                if(malloc_size>=min_malloc_size && malloc_size<max_malloc_size)
                {
                    query3.prepare("insert into one_unfree_memory values"
                                   "(:id, '"+date+"', '"+type+"', '"+malloc_address+"'"
                                   ", :malloc_size, :malloc_and_free_id)");
                    query3.bindValue(":id", id);
                    query3.bindValue(":malloc_size", malloc_size);
                    query3.bindValue(":malloc_and_free_id", malloc_parent_id);
                    query3.exec();

                    sum_malloc_size += malloc_size;

                    //Get the maximum memory usage per second
                    if(second_max_size<(sum_malloc_size-sum_free_size))
                    {
                        second_max_size = (sum_malloc_size-sum_free_size);
                    }

                    //Get the maximum total memory usage
                    if(most_max_size<(sum_malloc_size-sum_free_size))
                    {
                        most_max_size = (sum_malloc_size-sum_free_size);
                        most_max_id = id;
                        most_max_time = date;
                        most_malloc_count = sum_malloc_count + 1;
                        most_free_count = sum_free_count + 1;
                    }

                    id++;
                    sum_malloc_count++;

                }
            }

            //Analyze according to free_address matching malloc information
            if(type == "free")
            {
                query2.exec("select * from one_unfree_memory where malloc_address = '"+malloc_address+"' order by id desc limit 0,1");

                if(query2.next())
                {
                    malloc_size = query2.value(4).toInt();

                    sum_free_size += malloc_size;

                    //Get the maximum memory usage per second
                    if(second_max_size<(sum_malloc_size-sum_free_size))
                    {
                        second_max_size = (sum_malloc_size-sum_free_size);
                    }

                    //Get the maximum total memory usage
                    if(most_max_size<(sum_malloc_size-sum_free_size))
                    {
                        most_max_size = (sum_malloc_size-sum_free_size);
                        most_max_id = id;
                        most_max_time = date;
                        most_free_count = sum_free_count + 1;
                    }

                    id++;
                    sum_free_count++;

                    //Delete the matched malloc from the unfree memory table
                    query3.prepare("delete from one_unfree_memory where id = :delete_id");
                    query3.bindValue(":delete_id", query2.value(0).toInt());
                    query3.exec();

                }
            }

            //Store the data of every second obtained according to the time unit
            if(intervalTimeMS>time_count*analyse_time_unit*1000)
            {
                set_excel_value(time_count, second_max_size);

                time_count++;
            }

            //Every three thousand statements are committed as a transaction
            if(exec_count % 3000 == 0)
            {
                query3.exec("COMMIT");
                query3.exec("BEGIN");
            }

            last_intervalTimeMS = intervalTimeMS;
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
            search_finish_flag = 0;
            break;
        }
    }

    sum_exec_count += malloc_parent_id;

    //Ensure complete removal when clearing excel
    if(excel_row_count < time_count)
    {
        excel_row_count = time_count;
    }

    analyse_memory_count++;


    //Get the start address and end address of the memory pool
    query.exec("select start_and_end_address from malloc_and_free where id == 1");

    //Every time next is called, the data in the query table is moved to the next
    if(query.next())
    {
        start_memory_address = query.value(0).toString();
        memory_pool_start_memory_address = start_memory_address.toUInt(NULL,16);
    }

    query.exec("select start_and_end_address from malloc_and_free where id == 2");

    //Every time next is called, the data in the query table is moved to the next
    if(query.next())
    {
        end_memory_address = query.value(0).toString();
        memory_pool_end_memory_address = end_memory_address.toUInt(NULL,16);
    }



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
    ui->detach_textEdit->append("内存池开始地址");
    ui->detach_textEdit->append(start_memory_address);
    ui->detach_textEdit->append("内存池结束地址");
    ui->detach_textEdit->append(end_memory_address);


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

        //Check whether there is information printing outside the memory pool
        if((malloc_address.toUInt(NULL,16)>memory_pool_end_memory_address)
                || ((malloc_address.toUInt(NULL,16)+malloc_size)<memory_pool_start_memory_address))
        {
            ui->detach_textEdit->append("0000000000000000000000000000000000");
        }
        else if(((malloc_address.toUInt(NULL,16)+malloc_size)>memory_pool_end_memory_address)
                || (malloc_address.toUInt(NULL,16) < memory_pool_start_memory_address))
        {
            ui->detach_textEdit->append("1111111111111111111111111111111111");
        }
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

}

/**************************************************************
 * Function Name : update_detach_value
 * Description   : Update the default parameters of separated
 *                 memory according to the number of separated
 *                 segments
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::update_detach_value()
{
    int detach_count = 0;

    //init detach window
    ui->widget_one->hide();
    ui->widget_two->hide();
    ui->widget_three->hide();
    ui->widget_four->hide();
    ui->widget_five->hide();
    ui->widget_six->hide();
    ui->widget_seven->hide();
    ui->widget_eight->hide();
    ui->widget_nine->hide();
    ui->widget_ten->hide();

    ui->one_left_lineedit->clear();
    ui->one_right_lineedit->clear();
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
    ui->separate_lineEdit->clear();

    detach_count = ui->detach_count_Box->currentText().toInt();

    switch(detach_count)
    {
    case 1:
        ui->one_left_lineedit->setText("0");
        ui->one_right_lineedit->setText("4");
        ui->separate_lineEdit->setText("1");

        ui->widget_one->show();
        break;

    case 2:
        ui->one_left_lineedit->setText("0");
        ui->one_right_lineedit->setText("4");
        ui->two_left_lineedit->setText("4");
        ui->two_right_lineedit->setText("64");
        ui->separate_lineEdit->setText("1");

        ui->widget_one->show();
        ui->widget_two->show();
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
        break;

    case 4:
        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->show();
        ui->widget_four->show();
        break;

    case 5:
        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->show();
        ui->widget_four->show();
        ui->widget_five->show();
        break;

    case 6:
        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->show();
        ui->widget_four->show();
        ui->widget_five->show();
        ui->widget_six->show();
        break;

    case 7:
        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->show();
        ui->widget_four->show();
        ui->widget_five->show();
        ui->widget_six->show();
        ui->widget_seven->show();
        break;


    case 8:

        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->show();
        ui->widget_four->show();
        ui->widget_five->show();
        ui->widget_six->show();
        ui->widget_seven->show();
        ui->widget_eight->show();
        break;

    case 9:

        ui->widget_one->show();
        ui->widget_two->show();
        ui->widget_three->show();
        ui->widget_four->show();
        ui->widget_five->show();
        ui->widget_six->show();
        ui->widget_seven->show();
        ui->widget_eight->show();
        ui->widget_nine->show();
        break;

    case 10:
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
 * Function Name : paint_memory_pool_usage
 * Description   : Draw the memory usage of the memory pool
 * Parameters    : series -- Memory situation to be drawn
 *                 color -- Memory series color to be drawn
 *                 name -- series name
 *                 upper_boundary -- series's upper boundary
 *                 lower_boundary -- series"s lower boundary
 * Returns       : null
 **************************************************************/
void MainWindow::paint_memory_pool_usage(QAreaSeries *series , int color, QString name, QLineSeries* upper_boundary, QLineSeries* lower_boundary)
{
    QSqlQuery query;

    int unfree_count = 0;//The number of unreleased memory

    query.exec("select malloc_address, malloc_size from one_unfree_memory");
    while(query.next())
    {
        QString malloc_address = query.value(0).toString();
        uint malloc_size = query.value(1).toInt();

        uint series_start_malloc_address = malloc_address.toUInt(NULL, 16);
        uint series_end_malloc_address = series_start_malloc_address + malloc_size;

        //Analyze the graphics boundary used by the memory according to the zoom address
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

        //Add boundaries for memory usage
        upper_boundary->append(series_start_malloc_address, 0);
        upper_boundary->append(series_start_malloc_address, 100);
        upper_boundary->append(series_end_malloc_address, 100);
        upper_boundary->append(series_end_malloc_address, 0);

        lower_boundary->append(series_start_malloc_address, 0);
        lower_boundary->append(series_start_malloc_address, 0);
        lower_boundary->append(series_end_malloc_address, 0);
        lower_boundary->append(series_end_malloc_address, 0);

        unfree_count++;
    }

    query.exec("drop table one_unfree_memory");

    series = new QAreaSeries(upper_boundary, lower_boundary);

    //Add name and color for memory usage
    series->setName(name);

    QPen pen_busy_memory(color);
    pen_busy_memory.setWidth(0);
    series->setPen(pen_busy_memory);

    QLinearGradient gradient_busy_memory(QPointF(0, 0), QPointF(0, 1));
    gradient_busy_memory.setColorAt(0.0, color);
    gradient_busy_memory.setColorAt(1.0, color);
    gradient_busy_memory.setCoordinateMode(QGradient::ObjectBoundingMode);
    series->setBrush(gradient_busy_memory);

    //When no memory is occupied, the object is not drawn into the picture
    if(0 != unfree_count)
    {
        memory_pool_chart->addSeries(series);

        unfree_count = 0;
    }
}

/**************************************************************
 * Function Name : on_detachButton_clicked
 * Description   : Perform multi-segment memory analysis
 * Parameters    : null
 * Returns       : null
 **************************************************************///
void MainWindow::on_detachButton_clicked()
{
    QSqlQuery query;
    int detach_count = 0;
    uint min_malloc_size = 0;
    uint max_malloc_size = 0;
    QString series_name;
    int series_color = 0;

    //The button loses its function to ensure that the separation will not be interrupted
    ui->portNameBox->setEnabled(false);
    ui->baudrateBox->setEnabled(false);
    ui->dataBitsBox->setEnabled(false);
    ui->ParityBox->setEnabled(false);
    ui->stopBitsBox->setEnabled(false);
    ui->openButton->setEnabled(false);
    ui->searchButton->setEnabled(false);
    ui->clearButton->setEnabled(false);
    ui->detachButton->setEnabled(false);
    ui->saveButton->setEnabled(false);

    //Clear the last separation result
    on_clearButton_clicked();

    //Prepare for the separation progress bar
    progress_bar_prepartion();

    // Add series
    upper_boundary_of_free_memory = new QLineSeries();
    lower_boundary_of_free_memory = new QLineSeries();
    upper_boundary_of_first_segment_memory = new QLineSeries();
    lower_boundary_of_first_segment_memory = new QLineSeries();
    upper_boundary_of_second_segment_memory = new QLineSeries();
    lower_boundary_of_second_segment_memory = new QLineSeries();
    upper_boundary_of_third_segment_memory = new QLineSeries();
    lower_boundary_of_third_segment_memory = new QLineSeries();
    upper_boundary_of_fourth_segment_memory = new QLineSeries();
    lower_boundary_of_fourth_segment_memory = new QLineSeries();
    upper_boundary_of_fifth_segment_memory = new QLineSeries();
    lower_boundary_of_fifth_segment_memory = new QLineSeries();
    upper_boundary_of_sixth_segment_memory = new QLineSeries();
    lower_boundary_of_sixth_segment_memory = new QLineSeries();
    upper_boundary_of_seventh_segment_memory = new QLineSeries();
    lower_boundary_of_seventh_segment_memory = new QLineSeries();
    upper_boundary_of_eighth_segment_memory = new QLineSeries();
    lower_boundary_of_eighth_segment_memory = new QLineSeries();
    upper_boundary_of_ninth_segment_memory = new QLineSeries();
    lower_boundary_of_ninth_segment_memory = new QLineSeries();
    upper_boundary_of_tenth_segment_memory = new QLineSeries();
    lower_boundary_of_tenth_segment_memory = new QLineSeries();


    // Build chart objects
    memory_pool_chart = new QChart();


    /***********The memory pool initially shows the status***************/

    //Get memory pool point coordinates
    if((ui->start_address_lineEdit->text() == NULL && ui->end_address_lineEdit->text() == NULL))
    {
        new_start_memory_address = start_memory_address.toUInt(NULL, 16);
        new_end_memory_address = end_memory_address.toUInt(NULL, 16);

        *upper_boundary_of_free_memory << QPointF(start_memory_address.toUInt(NULL, 16), 100) << QPointF(end_memory_address.toUInt(NULL, 16), 100);
        *lower_boundary_of_free_memory<< QPointF(start_memory_address.toUInt(NULL, 16), 0) <<  QPointF(end_memory_address.toUInt(NULL, 16), 0);

    }
//    else if(ui->start_address_lineEdit->text().toUInt(NULL, 16)<start_memory_address.toUInt(NULL, 16) ||
//            ui->end_address_lineEdit->text().toUInt(NULL,16) > end_memory_address.toUInt(NULL, 16))
//    {
//        new_start_memory_address = start_memory_address.toUInt(NULL, 16);
//        new_end_memory_address = end_memory_address.toUInt(NULL, 16);

//        *upper_boundary_of_free_memory << QPointF(start_memory_address.toUInt(NULL, 16), 100) << QPointF(end_memory_address.toUInt(NULL, 16), 100);
//        *lower_boundary_of_free_memory<< QPointF(start_memory_address.toUInt(NULL, 16), 0) <<  QPointF(end_memory_address.toUInt(NULL, 16), 0);

//    }
    else
    {
        new_start_memory_address = ui->start_address_lineEdit->text().toUInt(NULL, 16);
        new_end_memory_address = ui->end_address_lineEdit->text().toUInt(NULL, 16);;
        qDebug() << "new_start_memory_address" << new_start_memory_address;
        qDebug() << "new_end_memory_address" << new_end_memory_address;

        // Assign a value to the series
        *upper_boundary_of_free_memory << QPointF(new_start_memory_address, 100) << QPointF(new_end_memory_address, 100);
        *lower_boundary_of_free_memory<< QPointF(new_start_memory_address, 0) <<  QPointF(new_end_memory_address, 0);

    }

    //Draw memory pool
    memory_free = new QAreaSeries(upper_boundary_of_free_memory, lower_boundary_of_free_memory);
    memory_free->setName("free memory");
    QPen pen_free_memmory(0x3D9140);
    pen_free_memmory.setWidth(0);
    memory_free->setPen(pen_free_memmory);

    QLinearGradient gradient_free_memory(QPointF(0, 0), QPointF(0, 1));
    gradient_free_memory.setColorAt(0.0, 0x3D9140);
    gradient_free_memory.setColorAt(1.0, 0x3D9140);
    gradient_free_memory.setCoordinateMode(QGradient::ObjectBoundingMode);
    memory_free->setBrush(gradient_free_memory);

    //Upload to legend
    memory_pool_chart->addSeries(memory_free);

    detach_count = ui->detach_count_Box->currentText().toInt();

    //first segment memory usage
    if(detach_count>=1)
    {
        min_malloc_size = ui->one_left_lineedit->text().toInt()*1024;
        max_malloc_size = ui->one_right_lineedit->text().toInt()*1024;

        detach_one_memory(min_malloc_size, max_malloc_size);

        series_name = "first_segment_memory";
        series_color = 0xFF0000;
        paint_memory_pool_usage(memory_first_segment, series_color, series_name, upper_boundary_of_first_segment_memory, lower_boundary_of_first_segment_memory);
    }

    //second segment memory usage
    if(detach_count>=2)
    {
        min_malloc_size = ui->two_left_lineedit->text().toInt()*1024;
        max_malloc_size = ui->two_right_lineedit->text().toInt()*1024;

        detach_one_memory(min_malloc_size, max_malloc_size);

        series_name = "second_segment_memory";
        series_color = 0x00FFFF;
        paint_memory_pool_usage(memory_second_segment, series_color, series_name, upper_boundary_of_second_segment_memory, lower_boundary_of_second_segment_memory);
    }

    //third segment memory usage
    if(detach_count>=3)
    {
        min_malloc_size = ui->three_left_lineedit->text().toInt()*1024;
        max_malloc_size = ui->three_right_lineedit->text().toInt()*1024;

        detach_one_memory(min_malloc_size, max_malloc_size);

        series_name = "third_segment_memory";
        series_color = 0xFFFF00;
        paint_memory_pool_usage(memory_third_segment, series_color, series_name, upper_boundary_of_third_segment_memory, lower_boundary_of_third_segment_memory);
    }

    //fourth segment memory usage
    if(detach_count>=4)
    {
        min_malloc_size = ui->four_left_lineedit->text().toInt()*1024;
        max_malloc_size = ui->four_right_lineedit->text().toInt()*1024;

        detach_one_memory(min_malloc_size, max_malloc_size);

        series_name = "fourth_segment_memory";
        series_color = 0x696969;
        paint_memory_pool_usage(memory_fourth_segment, series_color, series_name, upper_boundary_of_fourth_segment_memory, lower_boundary_of_fourth_segment_memory);
    }

    //fifth_segment memory usage
    if(detach_count>=5)
    {
        min_malloc_size = ui->five_left_lineedit->text().toInt()*1024;
        max_malloc_size = ui->five_right_lineedit->text().toInt()*1024;

        detach_one_memory(min_malloc_size, max_malloc_size);

        series_name = "fifth_segment_memory";
        series_color = 0x000080;
        paint_memory_pool_usage(memory_fifth_segment, series_color, series_name, upper_boundary_of_fifth_segment_memory, lower_boundary_of_fifth_segment_memory);
    }

    //sixth_segment memory usage
    if(detach_count>=6)
    {
        min_malloc_size = ui->six_left_lineedit->text().toInt()*1024;
        max_malloc_size = ui->six_right_lineedit->text().toInt()*1024;

        detach_one_memory(min_malloc_size, max_malloc_size);

        series_name = "sixth_segment_memory";
        series_color = 0xFF6A6A;
        paint_memory_pool_usage(memory_sixth_segment, series_color, series_name, upper_boundary_of_sixth_segment_memory, lower_boundary_of_sixth_segment_memory);
    }

    //seventh_segment memory usage
    if(detach_count>=7)
    {
        min_malloc_size = ui->seven_left_lineedit->text().toInt();
        max_malloc_size = ui->seven_right_lineedit->text().toInt();

        detach_one_memory(min_malloc_size, max_malloc_size);

        series_name = "seventh_segment_memory";
        series_color = 0xC71585;
        paint_memory_pool_usage(memory_seventh_segment, series_color, series_name, upper_boundary_of_seventh_segment_memory, lower_boundary_of_seventh_segment_memory);
    }

    //eighth_segment memory usage
    if(detach_count>=8)
    {
        min_malloc_size = ui->eight_left_lineedit->text().toInt()*1024;
        max_malloc_size = ui->eight_right_lineedit->text().toInt()*1024;

        detach_one_memory(min_malloc_size, max_malloc_size);

        series_name = "eighth_segment_memory";
        series_color = 0xFFA07A;
        paint_memory_pool_usage(memory_eighth_segment, series_color, series_name, upper_boundary_of_eighth_segment_memory, lower_boundary_of_eighth_segment_memory);
    }

    //ninth_segment memory usage
    if(detach_count>=9)
    {
        min_malloc_size = ui->nine_left_lineedit->text().toInt()*1024;
        max_malloc_size = ui->nine_right_lineedit->text().toInt()*1024;

        detach_one_memory(min_malloc_size, max_malloc_size);

        series_name = "ninth_segment_memory";
        series_color = 0x8B4500;
        paint_memory_pool_usage(memory_ninth_segment, series_color, series_name, upper_boundary_of_ninth_segment_memory, lower_boundary_of_ninth_segment_memory);
    }

    //tenth_segment memory usage
    if(detach_count>=10)
    {
        min_malloc_size = ui->ten_left_lineedit->text().toInt()*1024;
        max_malloc_size = ui->ten_right_lineedit->text().toInt()*1024;

        detach_one_memory(min_malloc_size, max_malloc_size);

        series_name = "tenth_segment_memory";
        series_color = 0x808080;
        paint_memory_pool_usage(memory_tenth_segment, series_color, series_name, upper_boundary_of_tenth_segment_memory, lower_boundary_of_tenth_segment_memory);
    }

    ui->progressBar->setValue(100);

    //Sixteenth transformation of coordinates
    memory_pool_chart->createDefaultAxes();

    QValueAxis *template_axisX = dynamic_cast<QValueAxis*>(memory_pool_chart->axisX());
    QValueAxis *hex_axisX = new QValueAxis();

    hex_axisX->setLabelFormat("%#X");
    hex_axisX->setMin(template_axisX->min());
    hex_axisX->setMax(template_axisX->max());

    memory_pool_chart->setAxisX(hex_axisX);

    memory_pool_chart->axisY()->setVisible(false);

    //If the chartview container already exists, there is no need to create a new one
    if(1 == paint_multi_map_flag)
    {
        paint_multi_map_flag = 0;
        chartView = new ChartView(memory_pool_chart);
    }
    else
    {
        ui->horizontalLayout_area->removeWidget(chartView);
        delete chartView;
        chartView = new ChartView(memory_pool_chart);
    }

    chartView->setRenderHint(QPainter::Antialiasing);

    //Add the chartview container to the UI interface
    ui->horizontalLayout_area->addWidget(chartView);

    //Reply button function
    ui->portNameBox->setEnabled(true);
    ui->baudrateBox->setEnabled(true);
    ui->dataBitsBox->setEnabled(true);
    ui->ParityBox->setEnabled(true);
    ui->stopBitsBox->setEnabled(true);
    ui->openButton->setEnabled(true);
    ui->searchButton->setEnabled(true);
    ui->clearButton->setEnabled(true);
    ui->detachButton->setEnabled(true);
    ui->saveButton->setEnabled(true);

    analyse_memory_count = 0;
    sum_exec_count = 0;

    QMessageBox::about(NULL, "提示", "数据分析完成");//
}

/**************************************************************
 * Function Name : on_saveButton_clicked
 * Description   : Save detach memory analysis results
 * Parameters    : null
 * Returns       : null
 **************************************************************/
void MainWindow::on_saveButton_clicked()
{
    QDir save_dir;
    QFile text_file;
    QString text_file_path;
    QString save_text_filename;
    QString save_excel_name;
    QString first_malloc_date;
    QString origin_db_name;
    QFile save_db_file;
    QString save_db_name;
    QSqlQuery query;

    //Set the path for saving analysis results
    query.exec("select date from malloc_and_free where id = 1");

    if(query.next())
    {
        first_malloc_date = query.value(0).toString();
    }

    first_malloc_date = first_malloc_date.mid(0, 10) + "-" + first_malloc_date.mid(11, 2) + "." +
            first_malloc_date.mid(14, 2) + "." + first_malloc_date.mid(17, 2) + "." + first_malloc_date.mid(20, 3);

    //New save folder
    if(!save_dir.exists(first_malloc_date)){
        save_dir.mkdir(first_malloc_date);
    }
    save_dir= first_malloc_date;

    //Save analysis results in text form
    save_text_filename = QString("单位为%1s的内存分析结果.txt")
            .arg(ui->separate_lineEdit->text().toInt());

    save_dir.remove(save_text_filename);
    text_file_path = save_dir.filePath(save_text_filename);
    text_file.setFileName(text_file_path);

    if(text_file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Append))
    {
        QTextStream out(&text_file);

        out << ui->detach_textEdit->toPlainText();
    }
    else
    {
        QMessageBox::about(NULL, "提示", "文件保存失败");
        return;
    }
    text_file.close();

    //Save the analysis results in excel form
    save_excel_name = QString("./"+ first_malloc_date + "/单位为%1s的内存分析表.xlsx")
            .arg(ui->separate_lineEdit->text().toInt());
    save_dir.remove(save_excel_name);
    xlsx.saveAs(save_excel_name);

    //Save the original database
    origin_db_name = "./Working_database.db";

    save_db_name = "./" + first_malloc_date+ "/" + first_malloc_date + ".db";
    save_db_file.setFileName(origin_db_name);

    save_db_file.copy(save_db_name);
    save_db_file.copy("./"+ first_malloc_date+"/" + origin_db_name);

    QMessageBox::about(NULL, "提示", "文件保存成功");

}

/**************************************************************
 * Function Name : MainWindow
 * Description   : Deconstructe Function MainWindow and remove
 *                 database
 * Parameters    : null
 * Returns       : null
 **************************************************************/
MainWindow::~MainWindow()
{

    //Disconnect the database and then delete the database
    QSqlDatabase database;
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("Working_database.db");
    database.close();

    QFile delete_file("./Working_database.db");

    if(delete_file.remove() != 1)
    {
        QMessageBox::about(NULL, "提示", "删除数据库失败");
    }

    delete ui;
}
