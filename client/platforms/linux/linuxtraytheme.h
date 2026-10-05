#ifndef LINUXTRAYTHEME_H
#define LINUXTRAYTHEME_H

#include <functional>

class QObject;

namespace LinuxTrayTheme
{

    void installThemeObserver(const std::function<void()> &onThemeChanged, QObject *parent);

} // namespace LinuxTrayTheme

#endif // LINUXTRAYTHEME_H
