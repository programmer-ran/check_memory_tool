#include "chartview.h"

/**************************************************************
 * Function Name : ChartView
 * Description   : Construct function ChartView
 * Parameters    : chart
 *                 parent
 * Returns       : ChartView
 **************************************************************/
ChartView::ChartView(QChart *chart, QWidget *parent) :
    QChartView(chart, parent),
    isClicking(false),
    xOld(0), yOld(0)
{
    setRubberBand(QChartView::RectangleRubberBand);
}


/**************************************************************
 * Function Name : mousePressEvent
 * Description   : Create mouse click control waveform events
 * Parameters    : event
 * Returns       : null
 **************************************************************/
void ChartView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() & Qt::LeftButton) {
        isClicking = true;
    } else if (event->button() & Qt::RightButton) {
        chart()->zoomReset();
    }

    QChartView::mousePressEvent(event);
}


/**************************************************************
 * Function Name : mouseMoveEvent
 * Description   : Create keyboard movement control waveform events
 * Parameters    : event
 * Returns       : null
 **************************************************************/
void ChartView::mouseMoveEvent(QMouseEvent *event)
{
    int x, y;

    if (isClicking) {
        if (xOld == 0 && yOld == 0) {

        } else {
            x = event->x() - xOld;
            y = event->y() - yOld;
            chart()->scroll(-x, y);
        }

        xOld = event->x();
        yOld = event->y();

        return;
    }

    QChartView::mouseMoveEvent(event);
}

/**************************************************************
 * Function Name : mouseReleaseEvent
 * Description   : Create keyboard right click to restore waveform event
 * Parameters    : event
 * Returns       : null
 **************************************************************/
void ChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if (isClicking) {
        xOld = yOld = 0;
        isClicking = false;
    }

    /* Disable original right click event */
    if (!(event->button() & Qt::RightButton)) {
        QChartView::mouseReleaseEvent(event);
    }
}


/**************************************************************
 * Function Name : keyPressEvent
 * Description   : Create keyboard control waveform events
 * Parameters    : event
 * Returns       : null
 **************************************************************/
void ChartView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        chart()->scroll(-10, 0);
        break;
    case Qt::Key_Right:
        chart()->scroll(10, 0);
        break;
    case Qt::Key_Up:
        chart()->scroll(0, 10);
        break;
    case Qt::Key_Down:
        chart()->scroll(0, -10);
        break;
    default:
        keyPressEvent(event);
        break;
    }
}
