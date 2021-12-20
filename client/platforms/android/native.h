#ifndef NATIVE_H
#define NATIVE_H

#include <QObject>

class NativeHelpers {
public:
    static void registerApplicationInstance(QObject *app_p) {
        application_p_ = app_p;
    }

    static QObject* getApplicationInstance() {
        return application_p_;
    }

private:
    static QObject *application_p_;
};

#endif // NATIVE_H
