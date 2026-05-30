#ifndef MACTRAYTHEME_H
#define MACTRAYTHEME_H

#include <functional>

class QObject;

namespace MacTrayTheme
{

    void installThemeObserver(const std::function<void()> &onThemeChanged, QObject *parent);

} // namespace MacTrayTheme

#endif // MACTRAYTHEME_H
