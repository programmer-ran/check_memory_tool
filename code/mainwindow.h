/***************************************************************
* Filename     : mainwindow.h
* Description   : Add UI and functions to application window objects
* Version      : 1.0
* History       :
* penghongran 2020-11-30  finished
**************************************************************/
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

    //Receive serial port data for analysis function
    void analyse_receive_window_data();

    //prepare for progress_bar_prepartion
    void progress_bar_prepartion();

    //set excel column name
    void set_excel_column_name(int excel_column_count);

    //set excel column value
    void set_excel_value(int time_count, uint second_max_size);

    //Separate memory information from the database
    void detach_one_memory(uint min_malloc_size, uint max_malloc_size);

private slots:

    //Serial port receiving data signal response slot function
    void serialPort_readyRead();

    //Check the number of available serial ports
    void on_searchButton_clicked();

    //Open serial port response slot function
    void on_openButton_clicked();

    //Clear separation result slot function
    void on_clearButton_clicked();

//    //Separate memory malloc and free response slot functions
//    void on_analyseButton_clicked();

    //Perform segmented memory analysis response slot function
    void on_detachButton_clicked();

    //According to detach count to set default parameters
    void update_detach_value();

    //Save memory analysis result response slot function
    void on_saveButton_clicked();

    //Plot the memory usage of the memory pool
    void paint_memory_pool_usage(QAreaSeries *series , int color, QString name,
                                 QLineSeries* upper_boundary, QLineSeries* lower_boundary);

private:

    Ui::MainWindow *ui;
    QSerialPort serial;
    ChartView *chartView;
    QChart* memory_pool_chart;//Simulate memory pool container

    //Record and the number of lines analyzed by recvlineEdit
    int recvEdit_analyzed_number = 0;

    //Clear excel parameters
    int excel_row_count = 0;
    int excel_column_count = 12;

    //how many pieces of memory information were analyzed
    uint sum_exec_count = 0;

    int analyse_memory_count = 0;

    //Need to create a new chartview container flag
    int paint_multi_map_flag = 1;

    /***The memory_pool's usage***/

    //memory_pool_start_and_end_address
    QString start_memory_address = "";
    QString end_memory_address = "";

    //Start address and end address of the memory pool after scaling
    uint new_start_memory_address = 0;
    uint new_end_memory_address = 0;

    //free_memory
    QAreaSeries* memory_free;
    QLineSeries* upper_boundary_of_free_memory;
    QLineSeries* lower_boundary_of_free_memory= NULL;

    //first_segment_memory
    QAreaSeries* memory_first_segment;
    QLineSeries* upper_boundary_of_first_segment_memory = NULL;
    QLineSeries* lower_boundary_of_first_segment_memory = NULL;

    //second_segment_memory
    QAreaSeries* memory_second_segment;
    QLineSeries* upper_boundary_of_second_segment_memory = NULL;
    QLineSeries* lower_boundary_of_second_segment_memory = NULL;

    //third_segment_memory
    QAreaSeries* memory_third_segment;
    QLineSeries* upper_boundary_of_third_segment_memory = NULL;
    QLineSeries* lower_boundary_of_third_segment_memory = NULL;

    //fourth_segment_memory
    QAreaSeries* memory_fourth_segment;
    QLineSeries* upper_boundary_of_fourth_segment_memory = NULL;
    QLineSeries* lower_boundary_of_fourth_segment_memory = NULL;

    //fifth_segment_memory
    QAreaSeries* memory_fifth_segment;
    QLineSeries* upper_boundary_of_fifth_segment_memory = NULL;
    QLineSeries* lower_boundary_of_fifth_segment_memory = NULL;

    //sixth_segment_memory
    QAreaSeries* memory_sixth_segment;
    QLineSeries* upper_boundary_of_sixth_segment_memory = NULL;
    QLineSeries* lower_boundary_of_sixth_segment_memory = NULL;

    //seventh_segment_memory
    QAreaSeries* memory_seventh_segment;
    QLineSeries* upper_boundary_of_seventh_segment_memory = NULL;
    QLineSeries* lower_boundary_of_seventh_segment_memory = NULL;

    //eighth_segment_memory
    QAreaSeries* memory_eighth_segment;
    QLineSeries* upper_boundary_of_eighth_segment_memory = NULL;
    QLineSeries* lower_boundary_of_eighth_segment_memory = NULL;

    //ninth_segment_memory
    QAreaSeries* memory_ninth_segment;
    QLineSeries* upper_boundary_of_ninth_segment_memory = NULL;
    QLineSeries* lower_boundary_of_ninth_segment_memory = NULL;

    //ten_segment_memory
    QAreaSeries* memory_tenth_segment;
    QLineSeries* upper_boundary_of_tenth_segment_memory = NULL;
    QLineSeries* lower_boundary_of_tenth_segment_memory = NULL;

    QXlsx::Document xlsx;//save excel object

};

#endif // MAINWINDOW_H
