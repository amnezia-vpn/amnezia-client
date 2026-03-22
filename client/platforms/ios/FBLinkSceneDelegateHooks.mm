#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#include <dispatch/dispatch.h>

#include <QByteArray>
#include <QFile>
#include <QString>

#include "ios_controller.h"

using SceneOpenURLContexts = void (*)(id, SEL, UIScene *, NSSet<UIOpenURLContext *> *);

static SceneOpenURLContexts g_originalSceneOpenURLContexts = nullptr;

static void amnezia_handleURL(NSURL *url)
{
    if (!url || !url.isFileURL) {
        return;
    }

    QString filePath(url.path.UTF8String);
    if (filePath.isEmpty()) {
        return;
    }

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        if (filePath.contains("backup")) {
            IosController::Instance()->importBackupFromOutside(filePath);
            return;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return;
        }

        const QByteArray data = file.readAll();
        IosController::Instance()->importConfigFromOutside(QString::fromUtf8(data));
    });
}

static void amnezia_scene_openURLContexts(id self, SEL _cmd, UIScene *scene, NSSet<UIOpenURLContext *> *contexts)
{
    if (g_originalSceneOpenURLContexts) {
        g_originalSceneOpenURLContexts(self, _cmd, scene, contexts);
    }

    if (!contexts || contexts.count == 0) {
        return;
    }

    if (@available(iOS 13.0, *)) {
        for (UIOpenURLContext *context in contexts) {
            amnezia_handleURL(context.URL);
        }
    }
}

@interface AmneziaSceneDelegateHooks : NSObject
@end

@implementation AmneziaSceneDelegateHooks

+ (void)load
{
    Class cls = objc_getClass("QIOSWindowSceneDelegate");
    if (!cls) {
        return;
    }

    SEL selector = @selector(scene:openURLContexts:);
    Method method = class_getInstanceMethod(cls, selector);
    if (method) {
        g_originalSceneOpenURLContexts = reinterpret_cast<SceneOpenURLContexts>(method_getImplementation(method));
        method_setImplementation(method, reinterpret_cast<IMP>(amnezia_scene_openURLContexts));
    } else {
        const char *types = "v@:@@";
        class_addMethod(cls, selector, reinterpret_cast<IMP>(amnezia_scene_openURLContexts), types);
    }
}

@end
