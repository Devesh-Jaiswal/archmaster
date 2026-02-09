#ifndef LOADINGOVERLAY_H
#define LOADINGOVERLAY_H

#include <QWidget>
#include <QLabel>
#include <QTimer>

class LoadingOverlay : public QWidget {
    Q_OBJECT
    
public:
    explicit LoadingOverlay(QWidget* parent = nullptr);
    
    void show(const QString& message = "Loading...");
    void hide();
    void setMessage(const QString& message);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    
private:
    QLabel* m_messageLabel;
    QLabel* m_spinnerLabel;
    QTimer* m_animationTimer;
    int m_animationFrame;
    
    void updateSpinner();
};

#endif // LOADINGOVERLAY_H
