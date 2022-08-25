#ifndef QCRYPTOERROR_H
#define QCRYPTOERROR_H

#include <QObject>

#include "QSimpleCrypto_global.h"

/// TODO: Add Special error code for each error.

// clang-format off
namespace QSimpleCrypto
{
    class QSIMPLECRYPTO_EXPORT QCryptoError : public QObject {
        Q_OBJECT

    public:
        explicit QCryptoError(QObject* parent = nullptr);

        ///
        /// \brief setError - Sets error information
        /// \param errorCode - Error code.
        /// \param errorSummary - Error summary.
        ///
        inline void setError(const quint8 errorCode, const QString& errorSummary)
        {
            m_currentErrorCode = errorCode;
            m_errorSummary = errorSummary;
        }

        ///
        /// \brief lastError - Returns last error.
        /// \return Returns eror ID and error Text.
        ///
        inline QPair<quint8, QString> lastError() const
        {
            return QPair<quint8, QString>(m_currentErrorCode, m_errorSummary);
        }

    private:
        quint8 m_currentErrorCode;
        QString m_errorSummary;
    };
}

#endif // QCRYPTOERROR_H
