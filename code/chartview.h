/***************************************************************
* Filename     : chartview.h
* Description   : Rewrite Qchartview class, add scroll wheel
*                 and mouse to control zoom memory chart events
* Version      : 1.0
* History       :
* penghongran 2020-11-30  finished
**************************************************************/
#ifndef __CHARTVIEW_H__
#define __CHARTVIEW_H__

#include <QChartView>
#include <QRubberBand>

QT_CHARTS_USE_NAMESPACE

class ChartView : public QChartView
{
public:
    ChartView(QChart *chart, QWidget *parent = 0);

protected:

    //Create mouse click control waveform events
    void mousePressEvent(QMouseEvent *event);

    //Create keyboard movement control waveform events
    void mouseMoveEvent(QMouseEvent *event);

    //Create keyboard right click to restore waveform events
    void mouseReleaseEvent(QMouseEvent *event);

    //Memory pool zoom operation function
    void wheelEvent(QWheelEvent *event);

private:

    bool is_clicking;
    int old_x;
};


#endif /* __CHARTVIEW_H__ */
