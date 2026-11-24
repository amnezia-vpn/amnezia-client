#include "jsonSyntaxHighlighter.h"

#include <QRegularExpression>
#include <QTextBlock>
#include <QTextDocument>

#include "logger.h"

namespace
{
    Logger logger("JsonSyntaxHighlighter");
}

JsonSyntaxHighlighter::JsonSyntaxHighlighter(QObject *parent) : QSyntaxHighlighter(parent)
{
}

QQuickTextDocument *JsonSyntaxHighlighter::textDocument() const
{
    return m_textDocument;
}

void JsonSyntaxHighlighter::setTextDocument(QQuickTextDocument *document)
{
    if (m_textDocument == document) {
        return;
    }
    m_textDocument = document;
    setDocument(document ? document->textDocument() : nullptr);
    logger.info() << "setTextDocument:" << (document ? "attached" : "detached");
    emit textDocumentChanged();
}

QColor JsonSyntaxHighlighter::valueColor() const
{
    return m_valueColor;
}

void JsonSyntaxHighlighter::setValueColor(const QColor &color)
{
    if (m_valueColor == color) {
        return;
    }
    m_valueColor = color;
    emit styleChanged();
    rehighlight();
}

QColor JsonSyntaxHighlighter::errorColor() const
{
    return m_errorColor;
}

void JsonSyntaxHighlighter::setErrorColor(const QColor &color)
{
    if (m_errorColor == color) {
        return;
    }
    m_errorColor = color;
    emit styleChanged();
    rehighlight();
}

int JsonSyntaxHighlighter::errorLine() const
{
    return m_errorLine;
}

void JsonSyntaxHighlighter::setErrorLine(int line)
{
    if (m_errorLine == line) {
        return;
    }
    logger.info() << "setErrorLine:" << QString::number(m_errorLine) << "->" << QString::number(line);
    const int previous = m_errorLine;
    m_errorLine = line;
    emit styleChanged();
    if (!document()) {
        return;
    }
    for (const int number : { previous, line }) {
        const QTextBlock block = document()->findBlockByNumber(number - 1);
        if (block.isValid()) {
            rehighlightBlock(block);
        }
    }
}

void JsonSyntaxHighlighter::highlightBlock(const QString &text)
{
    if (m_errorLine > 0 && currentBlock().blockNumber() + 1 == m_errorLine) {
        QTextCharFormat error;
        error.setForeground(m_errorColor);
        setFormat(0, text.size(), error);
        return;
    }

    QTextCharFormat value;
    value.setForeground(m_valueColor);

    static const QRegularExpression token(QStringLiteral(
            R"(("(?:\\.|[^"\\])*")(\s*:)?|(-?\b\d+(?:\.\d+)?(?:[eE][+-]?\d+)?\b)|\b(true|false|null)\b)"));
    QRegularExpressionMatchIterator it = token.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        if (match.capturedLength(1) > 0 && match.capturedLength(2) > 0) {
            continue;
        }
        setFormat(match.capturedStart(), match.capturedLength(), value);
    }
}
