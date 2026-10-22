#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#import "AmneziaOpenUrlImport.h"

using SceneOpenURLContexts = void (*)(id, SEL, UIScene *, NSSet<UIOpenURLContext *> *);

static SceneOpenURLContexts g_originalSceneOpenURLContexts = nullptr;

static void amnezia_handleURL(NSURL *url)
{
    AmneziaHandleOpenUrl(url);
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
