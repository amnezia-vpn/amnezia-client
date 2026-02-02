#include <QQmlExtensionPlugin>

QT_BEGIN_NAMESPACE

class WebViewPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface" FILE "webview.json")
    Q_INTERFACES(QQmlExtensionInterface)
    
public:
    void registerTypes(const char* uri) override;
};

QT_END_NAMESPACE
