#ifndef CHARTPOPUP_H
#define CHARTPOPUP_H

#include <QDialog>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QSlider>
#include <QLabel>

class ChartPopup : public QDialog {
    Q_OBJECT
    
public:
    explicit ChartPopup(QChart* chart, const QString& title, QWidget* parent = nullptr);
    
protected:
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    
private slots:
    void onZoomChanged(int value);
    void resetZoom();
    
private:
    QChartView* m_chartView;
    QSlider* m_zoomSlider;
    QLabel* m_zoomLabel;
    qreal m_currentZoom;
};

#endif // CHARTPOPUP_H
