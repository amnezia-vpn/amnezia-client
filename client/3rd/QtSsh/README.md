# QSsh

this project is base on Qt-creator-open-source-4.3.1
project is at 

`http://code.qt.io/cgit/qt-creator/qt-creator.git/`

you can download code zip at 

`http://download.qt.io/official_releases/qtcreator/4.3/4.3.1/`

## Getting Started

> * For linux user, if your Qt is installed through package manager tools such "apt-get", make sure that you have installed the Qt5 develop package *qtbase5-private-dev*

### Usage(1): Use QtSsh as Qt5's addon module

#### Building the module

> **Note**: Perl is needed in this step.

* Download the source code.

* Put the source code in any directory you like

* Go to top directory of the project in a terminal and run

```
    qmake
    make
    make install
```

The library, the header files, and others will be installed to your system.

#### Import the module 

`   QT += ssh`

#### Include Headerfile

`   #include<QtSsh/sshconnection.h>`

#### Close qtc.ssh log

`   QLoggingCategory::setFilterRules(QStringLiteral("qtc.ssh=false")`