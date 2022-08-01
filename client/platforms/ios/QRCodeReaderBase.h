#ifndef QRCODEREADERBASE_H
#define QRCODEREADERBASE_H

#include <QObject>
#include <QRect>

class QRCodeReader: public QObject {
    Q_OBJECT

public:
    QRCodeReader();

    QRect cameraSize();

public slots:
    void startReading();
    void stopReading();
    void setCameraSize(QRect value);

signals:
    void codeReaded(QString code);
    
private:
    void* m_qrCodeReader;
    QRect m_cameraSize;
};

#endif // QRCODEREADERBASE_H
