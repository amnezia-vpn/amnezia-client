#ifndef TRAYTHEMECHANGEFILTER_H
#define TRAYTHEMECHANGEFILTER_H

#include <functional>

#include <QObject>

class TrayThemeChangeFilter final : public QObject {
    Q_OBJECT

public:
    explicit TrayThemeChangeFilter(std::function<void()> onThemeChanged, QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    std::function<void()> m_onThemeChanged;
};

#endif // TRAYTHEMECHANGEFILTER_H
