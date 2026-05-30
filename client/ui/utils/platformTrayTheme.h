#ifndef PLATFORMTRAYTHEME_H
#define PLATFORMTRAYTHEME_H

#include <functional>

class QObject;

void installTrayThemeObserver(const std::function<void()> &onThemeChanged, QObject *parent);

#endif // PLATFORMTRAYTHEME_H
