/***************************************************************
* Filename     : chartview.cpp
* Description   : Rewrite Qchartview class, add scroll wheel
*                 and mouse to control zoom memory chart events
* Version      : 1.0
* History       :
* penghongran 2020-11-30  finished
**************************************************************/
#include "chartview.h"

#include <QValueAxis>

/**************************************************************
 * Function Name : ChartView
 * Description   : Construct function ChartView
 * Parameters    : chart
 *                 parent
 * Returns       : ChartView
 **************************************************************/
ChartView::ChartView(QChart *chart, QWidget *parent) :
    QChartView(chart, parent),
    is_clicking(false),
    old_x(0)
{
    setRubberBand(QChartView::RectangleRubberBand);
}

/**************************************************************
 * Function Name : mousePressEvent
 * Description   : Create mouse click control waveform events
 * Parameters    : event -- operate object
 * Returns       : null
 **************************************************************/
void ChartView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() & Qt::LeftButton) {
        is_clicking = true;
    } else if (event->button() & Qt::RightButton) {
        chart()->createDefaultAxes();
        chart()->zoomReset();

        //Sixteenth transformation of coordinates
        QValueAxis *template_axisX = dynamic_cast<QValueAxis*>(chart()->axisX());//
        QValueAxis *hex_axisX = new QValueAxis();

        hex_axisX->setLabelFormat("%#X");
        hex_axisX->setMin(template_axisX->min());
        hex_axisX->setMax(template_axisX->max());

        chart()->setAxisX(hex_axisX);
        chart()->axisY()->setVisible(false);
        chart()->update();
    }

    QChartView::mousePressEvent(event);
}

/**************************************************************
 * Function Name : mouseMoveEvent
 * Description   : Create keyboard movement control waveform events
 * Parameters    : event -- operate object
 * Returns       : null
 **************************************************************/
void ChartView::mouseMoveEvent(QMouseEvent *event)
{
    int x = 0;

    if (is_clicking) {
        if (old_x == 0) {

        } else {
            chart()->createDefaultAxes();

            x = event->x() - old_x;
            chart()->scroll(-x, 0);

            //Sixteenth transformation of coordinates
            QValueAxis *template_axisX = dynamic_cast<QValueAxis*>(chart()->axisX());//
            QValueAxis *hex_axisX = new QValueAxis();

            hex_axisX->setLabelFormat("%#X");
            hex_axisX->setMin(template_axisX->min());
            hex_axisX->setMax(template_axisX->max());

            chart()->setAxisX(hex_axisX);
            chart()->axisY()->setVisible(false);
        }

        old_x = event->x();

        chart()->update();

        return;
    }

    QChartView::mouseMoveEvent(event);
}

/**************************************************************
 * Function Name : mouseReleaseEvent
 * Description   : Create keyboard right click to restore waveform event
 * Parameters    : event -- operate object
 * Returns       : null
 **************************************************************/
void ChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if (is_clicking) {
        old_x = 0;
        is_clicking = false;
    }

    /* Disable original right click event */
    if (!(event->button() & Qt::RightButton)) {
        QChartView::mouseReleaseEvent(event);
    }
}


/**************************************************************
 * Function Name : wheelEvent
 * Description   : Zoom in and out of the waveform
 * Parameters    : event
 * Returns       : null
 **************************************************************/
void ChartView::wheelEvent(QWheelEvent *event)
{
    //Scaling factor
    double factor = 0.0;

    if (event->delta() > 0)
    {
        factor = 1.1;
    }
    else
    {
        factor = double(10.0/11);
    }

    chart()->createDefaultAxes();

    //The current position of the mouse
    QPointF mousePos = mapFromGlobal(QCursor::pos());

    QRectF rect;

    rect.setLeft(chart()->plotArea().left());
    rect.setTop(chart()->plotArea().top());
    rect.setWidth(chart()->plotArea().width()/factor);
    rect.setHeight(chart()->plotArea().height());

    mousePos.setX(chart()->plotArea().center().x());
    mousePos.setY(rect.y());
    rect.moveCenter(mousePos);
    chart()->zoomIn(rect);

    //Sixteenth transformation of coordinates
    QValueAxis *template_axisX = dynamic_cast<QValueAxis*>(chart()->axisX());//

    QValueAxis *hex_axisX = new QValueAxis();

    hex_axisX->setLabelFormat("%#X");
    hex_axisX->setMin(template_axisX->min());
    hex_axisX->setMax(template_axisX->max());

    chart()->setAxisX(hex_axisX);
    chart()->axisY()->setVisible(false);
    chart()->update();

    //Recursive call
    QWidget::wheelEvent(event);
}
