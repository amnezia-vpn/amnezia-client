#ifndef WINTRAYTHEME_H
#define WINTRAYTHEME_H

#include <functional>

class QObject;

namespace WinTrayTheme
{

    void installThemeObserver(const std::function<void()> &onThemeChanged, QObject *parent);

} // namespace WinTrayTheme

#endif // WINTRAYTHEME_H
