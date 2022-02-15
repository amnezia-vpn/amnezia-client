#include "QrDecoderLogic.h"

#include "ui/uilogic.h"
#include "ui/pages_logic/StartPageLogic.h"

#if defined(Q_OS_ANDROID)
#include "android_controller.h"
#endif

using namespace amnezia;
using namespace PageEnumNS;

QrDecoderLogic::QrDecoderLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent)
{

}

void QrDecoderLogic::onUpdatePage()
{
    m_chunks.clear();
    set_detectingEnabled(true);
    set_totalChunksCount(0);
    set_receivedChunksCount(0);
    emit startDecode();
}

void QrDecoderLogic::onDetectedQrCode(const QString &code)
{
    //qDebug() << code;

    if (!detectingEnabled()) return;

    // check if chunk received
    QByteArray ba = QByteArray::fromBase64(code.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QDataStream s(&ba, QIODevice::ReadOnly);
    qint16 magic; s >> magic;


    if (magic == amnezia::qrMagicCode) {
        quint8 chunksCount; s >> chunksCount;
        if (totalChunksCount() != chunksCount) {
            m_chunks.clear();
        }
        set_totalChunksCount(chunksCount);

        quint8 chunkId; s >> chunkId;
        s >> m_chunks[chunkId];
        set_receivedChunksCount(m_chunks.size());

        if (m_chunks.size() == totalChunksCount()) {
            QByteArray data;
            for (int i = 0; i < totalChunksCount(); ++i) {
                data.append(m_chunks.value(i));
            }

            bool ok = uiLogic()->startPageLogic()->importConnectionFromQr(data);
            if (ok) {
                set_detectingEnabled(false);
                emit stopDecode();
            }
            else {
                m_chunks.clear();
                set_totalChunksCount(0);
                set_receivedChunksCount(0);
            }
        }
    }
    else {
        bool ok = uiLogic()->startPageLogic()->importConnectionFromQr(ba);
        if (ok) {
            set_detectingEnabled(false);
            emit stopDecode();
        }
    }
}

