import QtQuick

import Qt.labs.folderlistmodel

import PageType 1.0

Item {
    property var pages: ({})

    signal finished()

    FolderListModel {
        id: folderModelPages
        folder: "qrc:/ui/qml/Pages2/"
        nameFilters: ["*.qml"]
        showDirs: false

        onStatusChanged: {
            if (status == FolderListModel.Ready) {
                for (var i = 0; i < folderModelPages.count; i++) {
                    createPagesObjects(folderModelPages.get(i, "filePath"), PageType.Basic);
                }
                finished()
            }
        }

        function createPagesObjects(file, type) {
            if (file.indexOf("Base") !== -1) {
                return; // skip Base Pages
            }

            var c = Qt.createComponent("qrc" + file);

            var finishCreation = function(component) {
                if (component.status === Component.Ready) {
                    var obj = component.createObject(root);
                    if (obj === null) {
                        console.debug("Error creating object " + component.url);
                    } else {
                        obj.visible = false
                        if (type === PageType.Basic) {
                            pages[obj.page] = obj
                        }
                    }
                } else if (component.status === Component.Error) {
                    console.debug("Error loading component:", component.errorString());
                }
            }

            if (c.status === Component.Ready) {
                finishCreation(c);
            } else {
                console.debug("Warning: " + file + " page components are not ready " + c.errorString());
            }
        }
    }
}
