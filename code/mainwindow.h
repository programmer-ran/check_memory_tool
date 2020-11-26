#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QtCharts>
#include <QSvgGenerator>
#include <QtXlsx/QtXlsx>

#include "chartview.h"


QT_CHARTS_USE_NAMESPACE


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    //接收串口数据进行解析函数
    void data_get();

    //分离出第零段内存信息
    void detach_zero_memory(int min_size, int max_size);

    //进度条前置准备
    void progress_bar_prepartion();

    //分离出第一段内存信息
    void detach_one_memory(int min_size, int max_size);

    //分离出第二段内存信息
    void detach_two_memory();

    //分离出第三段内存信息
    void detach_three_memory();

    //外部函数调用内部成员构造
    static MainWindow* getInstance();

private slots:

    //串口接收数据信号响应槽函数
    void serialPort_readyRead();

    //检测可用串口数
    void on_searchButton_clicked();

    //打开串口响应槽函数
    void on_openButton_clicked();

    //清除分离结果槽函数
    void on_clearButton_clicked();

    //进行内存malloc和free分离响应槽函数
    void on_analyseButton_clicked();

    //停止线程响应槽函数
    void stopThread();

    //进行分段内存分析响应槽函数
    void on_detachButton_clicked();

    //设置默认参数
    void update_detach_value();

    //保存内存分析结果响应槽函数
    void on_saveButton_clicked();


private:

    void wheelEvent(QWheelEvent *event);

    Ui::MainWindow *ui;
    QSerialPort serial;
    ChartView *chartView;
    QChart* m_chart_1;// 图表对象

    QLineSeries* series0 = NULL;
    QLineSeries* series1 = NULL;

    QLineSeries* series2 = NULL;
    QLineSeries* series3 = NULL;

    QLineSeries* series4 = NULL;
    QLineSeries* series5 = NULL;


    QLineSeries* series6 = NULL;
    QLineSeries* series7 = NULL;

    QLineSeries* series8 = NULL;
    QLineSeries* series9 = NULL;

    QLineSeries* series10 = NULL;
    QLineSeries* series11 = NULL;

    QLineSeries* series12 = NULL;
    QLineSeries* series13 = NULL;

    QLineSeries* series14 = NULL;
    QLineSeries* series15 = NULL;

    QLineSeries* series16 = NULL;
    QLineSeries* series17 = NULL;

    QLineSeries* series18 = NULL;
    QLineSeries* series19 = NULL;

    QLineSeries* series20 = NULL;
    QLineSeries* series21 = NULL;

    QXlsx::Document xlsx;

    bool isStopping;



};

#endif // MAINWINDOW_H
