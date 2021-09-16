SortFilterProxyModel
====================

SortFilterProxyModel is an implementation of `QSortFilterProxyModel` conveniently exposed for QML.

Install
-------
##### With [qpm](https://qpm.io) :
1. `qpm install fr.grecko.sortfilterproxymodel`
2. add `include(vendor/vendor.pri)` in your .pro if it is not already done
3. `import SortFilterProxyModel 0.2` to use this library in your QML files

##### Without qpm :
1. clone or download this repository
2. * `qmake` add `include  (<path/to/SortFilterProxyModel>/SortFilterProxyModel.pri)` in your `.pro`
   * `CMake` add $<TARGET_OBJECTS:SortFilterProxyModel> to the sources of your executable target in your cmake project
3. `import SortFilterProxyModel 0.2` to use this library in your QML files

Sample Usage
------------

- You can do simple filtering and sorting with SortFilterProxyModel:
```qml
import QtQuick 2.2
import QtQuick.Controls 1.2
import SortFilterProxyModel 0.2

ApplicationWindow {
    visible: true
    width: 640
    height: 480

    ListModel {
        id: personModel
        ListElement {
            firstName: "Erwan"
            lastName: "Castex"
            favorite: true
        }
        // ...
    }

    TextField {
        id: textField
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: implicitHeight
    }

    SortFilterProxyModel {
        id: personProxyModel
        sourceModel: personModel
        filters: RegExpFilter {
            roleName: "lastName"
            pattern: textField.text
            caseSensitivity: Qt.CaseInsensitive
        }
        sorters: StringSorter { roleName: "firstName" }
    }

    ListView {
        anchors { top: textField.bottom; bottom: parent.bottom; left: parent.left; right: parent.right }
        model: personProxyModel
        delegate: Text { text: model.firstName + " " + model.lastName}
    }
}
```
Here the `ListView` will only show elements that contains the content of the `TextField` in their `lastName` role.

- But you can also achieve more complex filtering or sorting with multiple `filters` and `sorters`:
```qml
    SortFilterProxyModel {
        id: personProxyModel
        sourceModel: personModel
        filters: [
            ValueFilter {
                enabled: onlyShowFavoritesCheckbox.checked
                roleName: "favorite"
                value: true
            },
            AnyOf {
                RegExpFilter {
                    roleName: "lastName"
                    pattern: textField.text
                    caseSensitivity: Qt.CaseInsensitive
                }
                RegExpFilter {
                    roleName: "firstName"
                    pattern: textField.text
                    caseSensitivity: Qt.CaseInsensitive
                }
            }
        ]
        sorters: [
            RoleSorter { roleName: "favorite"; sortOrder: Qt.DescendingOrder },
            StringSorter { roleName: "firstName" },
            StringSorter { roleName: "lastName" }
        ]
    }

    CheckBox {
        id:onlyShowFavoritesCheckbox
    }
```
This will show in the corresponding `ListView` only the elements where the `firstName` or the `lastName` match the text entered in the `textField`, and if the `onlyShowFavoritesCheckbox` is checked it will aditionnally filter the elements where `favorite` is `true`.
The favorited elements will be shown first and all the elements are sorted by `firstName` and then `lastName`.

Showcase Application
--------------------
You can find an application showcasing this library here: https://github.com/oKcerG/SFPMShowcase

License
-------
This library is licensed under the MIT License.

Documentation
-------------
This component is a subclass of [`QSortFilterProxyModel`](http://doc.qt.io/qt-5/qsortfilterproxymodel.html), to use it, you need to set the `sourceModel` property to a [`QAbstractItemModel*`](http://doc.qt.io/qt-5/qabstractitemmodel.html) with correct role names.
This means you can use it with custom c++ models or `ListModel`, but not with JavaScript models like arrays, integers or object instances.

The complete documentation reference is available here: https://okcerg.github.io/SortFilterProxyModel/

Contributing
------------
Don't hesitate to open an issue about a suggestion, a bug, a lack of clarity in the documentation, etc.

Pull requests are also welcome, if it's a important change you should open an issue first though.
