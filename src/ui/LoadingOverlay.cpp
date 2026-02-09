#include "LoadingOverlay.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QEvent>
#include <QResizeEvent>

LoadingOverlay::LoadingOverlay(QWidget* parent)
    : QWidget(parent)
    , m_animationFrame(0)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    
    m_spinnerLabel = new QLabel();
    m_spinnerLabel->setAlignment(Qt::AlignCenter);
    m_spinnerLabel->setStyleSheet("font-size: 48px;");
    
    m_messageLabel = new QLabel();
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setStyleSheet("font-size: 16px; color: #cdd6f4; font-weight: bold;");
    
    layout->addWidget(m_spinnerLabel);
    layout->addWidget(m_messageLabel);
    
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &LoadingOverlay::updateSpinner);
    
    if (parent) {
        parent->installEventFilter(this);
        resize(parent->size());
    }
    
    QWidget::hide();
}

void LoadingOverlay::show(const QString& message) {
    if (parentWidget()) {
        resize(parentWidget()->size());
        raise();
    }
    m_messageLabel->setText(message);
    m_animationFrame = 0;
    updateSpinner();
    m_animationTimer->start(100);
    QWidget::show();
}

void LoadingOverlay::hide() {
    m_animationTimer->stop();
    QWidget::hide();
}

void LoadingOverlay::setMessage(const QString& message) {
    m_messageLabel->setText(message);
}

void LoadingOverlay::updateSpinner() {
    static const QStringList frames = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    m_spinnerLabel->setText(frames[m_animationFrame % frames.size()]);
    m_animationFrame++;
}

void LoadingOverlay::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 180));
}

bool LoadingOverlay::eventFilter(QObject* watched, QEvent* event) {
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        resize(parentWidget()->size());
    }
    return QWidget::eventFilter(watched, event);
}
