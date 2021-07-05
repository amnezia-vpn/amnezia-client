import QtQuick 2.0
import QtQuick.Controls 2.0
import Ssh 1.0
Item {
    width: 1024
    height: 768

    TextField {
        id: input
        x: 104
        y: 44
        width: 482
        height: 40
        implicitWidth: 200
        selectByMouse: true
        text: "https://www.zhihu.com"
    }
    function get(url) {
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {
                console.log(xhr.responseXML, xhr.responseText.toString())
            } else if (xhr.readyState === XMLHttpRequest) {

            }
        }
        xhr.open('GET', url)
        xhr.send()
    }
    Button {
        x: 627
        y: 44
        text: "get"
        onClicked: {
            get(input.text)
        }
    }

    Button {
        id: button
        x: 104
        y: 170
        text: qsTr("Ssh")
        onClicked: ssh.connectToHost()
    }
    Ssh {
        id: ssh
    }


}
