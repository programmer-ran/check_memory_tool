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

    //创建键盘控制波形图事件
    void keyPressEvent(QKeyEvent *event);

    //创建鼠标点击控制波形图事件
    void mousePressEvent(QMouseEvent *event);

    //创建键盘移动控制波形图事件
    void mouseMoveEvent(QMouseEvent *event);

    //创建键盘右键恢复波形图事件
    void mouseReleaseEvent(QMouseEvent *event);

private:
    bool isClicking;

    int xOld;
    int yOld;
};




#endif /* __CHARTVIEW_H__ */
