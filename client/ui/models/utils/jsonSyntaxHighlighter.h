#ifndef JSONSYNTAXHIGHLIGHTER_H
#define JSONSYNTAXHIGHLIGHTER_H

#include <QColor>
#include <QPointer>
#include <QQuickTextDocument>
#include <QSyntaxHighlighter>

class JsonSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument *textDocument READ textDocument WRITE setTextDocument NOTIFY textDocumentChanged)
    Q_PROPERTY(QColor valueColor READ valueColor WRITE setValueColor NOTIFY styleChanged)
    Q_PROPERTY(QColor errorColor READ errorColor WRITE setErrorColor NOTIFY styleChanged)
    Q_PROPERTY(int errorLine READ errorLine WRITE setErrorLine NOTIFY styleChanged)

public:
    explicit JsonSyntaxHighlighter(QObject *parent = nullptr);

    QQuickTextDocument *textDocument() const;
    void setTextDocument(QQuickTextDocument *document);

    QColor valueColor() const;
    void setValueColor(const QColor &color);

    QColor errorColor() const;
    void setErrorColor(const QColor &color);

    int errorLine() const;
    void setErrorLine(int line);

signals:
    void textDocumentChanged();
    void styleChanged();

protected:
    void highlightBlock(const QString &text) override;

private:
    QPointer<QQuickTextDocument> m_textDocument;
    QColor m_valueColor;
    QColor m_errorColor;
    int m_errorLine = 0;
};

#endif // JSONSYNTAXHIGHLIGHTER_H
