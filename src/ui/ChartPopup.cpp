#include "ChartPopup.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QWheelEvent>
#include <QKeyEvent>

ChartPopup::ChartPopup(QChart* chart, const QString& title, QWidget* parent)
    : QDialog(parent)
    , m_currentZoom(1.0)
{
    setWindowTitle(title);
    setMinimumSize(800, 600);
    resize(1000, 700);
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Chart view with rubber band zoom
    m_chartView = new QChartView(chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setRubberBand(QChartView::RectangleRubberBand);
    
    // Zoom controls
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    
    QLabel* zoomIcon = new QLabel("🔍");
    controlsLayout->addWidget(zoomIcon);
    
    m_zoomSlider = new QSlider(Qt::Horizontal);
    m_zoomSlider->setRange(50, 300);
    m_zoomSlider->setValue(100);
    m_zoomSlider->setFixedWidth(200);
    connect(m_zoomSlider, &QSlider::valueChanged, this, &ChartPopup::onZoomChanged);
    controlsLayout->addWidget(m_zoomSlider);
    
    m_zoomLabel = new QLabel("100%");
    m_zoomLabel->setFixedWidth(50);
    controlsLayout->addWidget(m_zoomLabel);
    
    QPushButton* resetBtn = new QPushButton("Reset View");
    connect(resetBtn, &QPushButton::clicked, this, &ChartPopup::resetZoom);
    controlsLayout->addWidget(resetBtn);
    
    controlsLayout->addStretch();
    
    // Help text
    QLabel* helpLabel = new QLabel("Scroll to zoom • Drag to pan • Drag rectangle to zoom area");
    helpLabel->setStyleSheet("color: #a6adc8; font-size: 11px;");
    controlsLayout->addWidget(helpLabel);
    
    QPushButton* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    controlsLayout->addWidget(closeBtn);
    
    mainLayout->addWidget(m_chartView);
    mainLayout->addLayout(controlsLayout);
    
    // Styling
    setStyleSheet(R"(
        QDialog {
            background-color: #1e1e2e;
        }
        QSlider::groove:horizontal {
            background: #45475a;
            height: 6px;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #89b4fa;
            width: 16px;
            margin: -5px 0;
            border-radius: 8px;
        }
        QPushButton {
            background-color: #45475a;
            color: #cdd6f4;
            border: none;
            padding: 8px 16px;
            border-radius: 6px;
        }
        QPushButton:hover {
            background-color: #585b70;
        }
        QLabel {
            color: #cdd6f4;
        }
    )");
}

void ChartPopup::wheelEvent(QWheelEvent* event) {
    // Zoom with mouse wheel
    if (event->angleDelta().y() > 0) {
        m_chartView->chart()->zoomIn();
        m_currentZoom *= 1.1;
    } else {
        m_chartView->chart()->zoomOut();
        m_currentZoom *= 0.9;
    }
    
    int sliderValue = qBound(50, (int)(m_currentZoom * 100), 300);
    m_zoomSlider->blockSignals(true);
    m_zoomSlider->setValue(sliderValue);
    m_zoomSlider->blockSignals(false);
    m_zoomLabel->setText(QString("%1%").arg(sliderValue));
    
    event->accept();
}

void ChartPopup::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        accept();
    } else if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
        m_chartView->chart()->zoomIn();
        m_currentZoom *= 1.2;
    } else if (event->key() == Qt::Key_Minus) {
        m_chartView->chart()->zoomOut();
        m_currentZoom *= 0.8;
    } else if (event->key() == Qt::Key_0) {
        resetZoom();
    }
    QDialog::keyPressEvent(event);
}

void ChartPopup::onZoomChanged(int value) {
    qreal targetZoom = value / 100.0;
    qreal factor = targetZoom / m_currentZoom;
    
    if (factor > 1) {
        m_chartView->chart()->zoom(factor);
    } else {
        m_chartView->chart()->zoom(factor);
    }
    
    m_currentZoom = targetZoom;
    m_zoomLabel->setText(QString("%1%").arg(value));
}

void ChartPopup::resetZoom() {
    m_chartView->chart()->zoomReset();
    m_currentZoom = 1.0;
    m_zoomSlider->setValue(100);
    m_zoomLabel->setText("100%");
}
