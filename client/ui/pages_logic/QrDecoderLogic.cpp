#include "QrDecoderLogic.h"

#include "ui/uilogic.h"
#include "ui/pages_logic/StartPageLogic.h"

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>
#include "../../platforms/android/androidutils.h"
#endif

using namespace amnezia;
using namespace PageEnumNS;

namespace {
    QrDecoderLogic* mInstance = nullptr;
    constexpr auto CLASSNAME = "org.amnezia.vpn.qt.CameraActivity";
}

QrDecoderLogic::QrDecoderLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent)
{
    mInstance = this;

    #if (defined(Q_OS_ANDROID))
        AndroidUtils::runOnAndroidThreadAsync([]() {
            JNINativeMethod methods[]{
                {"passDataToDecoder", "(Ljava/lang/String;)V", reinterpret_cast<void*>(onNewDataChunk)},
            };

            QJniObject javaClass(CLASSNAME);
            QJniEnvironment env;
            jclass objectClass = env->GetObjectClass(javaClass.object<jobject>());
            env->RegisterNatives(objectClass, methods, sizeof(methods) / sizeof(methods[0]));
            env->DeleteLocalRef(objectClass);
        });
    #endif
}

void QrDecoderLogic::stopDecodingQr()
{
    #if (defined(Q_OS_ANDROID))
        QJniObject::callStaticMethod<void>(CLASSNAME, "stopQrCodeReader", "()V");
    #endif

    emit stopDecode();
}

#ifdef Q_OS_ANDROID
void QrDecoderLogic::onNewDataChunk(JNIEnv *env, jobject thiz, jstring data)
{
    Q_UNUSED(thiz);
    const char* buffer = env->GetStringUTFChars(data, nullptr);
    if (!buffer) {
        return;
    }

    QString parcelBody(buffer);
    env->ReleaseStringUTFChars(data, buffer);

    if (mInstance != nullptr) {
        if (!mInstance->m_detectingEnabled) {
            mInstance->onUpdatePage();
        }
        mInstance->onDetectedQrCode(parcelBody);
    }
}
#endif

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

            bool ok = uiLogic()->pageLogic<StartPageLogic>()->importConnectionFromQr(data);
            if (ok) {
                set_detectingEnabled(false);
                stopDecodingQr();
            } else {
                m_chunks.clear();
                set_totalChunksCount(0);
                set_receivedChunksCount(0);
            }
        }
    } else {
        bool ok = uiLogic()->pageLogic<StartPageLogic>()->importConnectionFromQr(ba);
        if (ok) {
            set_detectingEnabled(false);
            stopDecodingQr();
        }
    }
}
