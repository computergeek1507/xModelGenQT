/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionOpen_DXF;
    QAction *actionExport_xModel;
    QAction *actionExit;
    QAction *actionAutoWire;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    QSplitter *splitter;
    QListWidget *listWidgetNodes;
    QTableView *tableViewDraw;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QSpinBox *spinBox_wireSize;
    QPushButton *pushButton_autoWire;
    QSpacerItem *horizontalSpacer;
    QMenuBar *menuBar;
    QMenu *menuFile;
    QMenu *menuWire;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(662, 436);
        actionOpen_DXF = new QAction(MainWindow);
        actionOpen_DXF->setObjectName(QString::fromUtf8("actionOpen_DXF"));
        actionExport_xModel = new QAction(MainWindow);
        actionExport_xModel->setObjectName(QString::fromUtf8("actionExport_xModel"));
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName(QString::fromUtf8("actionExit"));
        actionAutoWire = new QAction(MainWindow);
        actionAutoWire->setObjectName(QString::fromUtf8("actionAutoWire"));
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        verticalLayout = new QVBoxLayout(centralWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        splitter = new QSplitter(centralWidget);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(splitter->sizePolicy().hasHeightForWidth());
        splitter->setSizePolicy(sizePolicy);
        splitter->setOrientation(Qt::Horizontal);
        splitter->setOpaqueResize(true);
        splitter->setHandleWidth(5);
        splitter->setChildrenCollapsible(false);
        listWidgetNodes = new QListWidget(splitter);
        listWidgetNodes->setObjectName(QString::fromUtf8("listWidgetNodes"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(listWidgetNodes->sizePolicy().hasHeightForWidth());
        listWidgetNodes->setSizePolicy(sizePolicy1);
        splitter->addWidget(listWidgetNodes);
        tableViewDraw = new QTableView(splitter);
        tableViewDraw->setObjectName(QString::fromUtf8("tableViewDraw"));
        tableViewDraw->setShowGrid(false);
        splitter->addWidget(tableViewDraw);

        verticalLayout->addWidget(splitter);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(centralWidget);
        label->setObjectName(QString::fromUtf8("label"));
        QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy2);

        horizontalLayout->addWidget(label);

        spinBox_wireSize = new QSpinBox(centralWidget);
        spinBox_wireSize->setObjectName(QString::fromUtf8("spinBox_wireSize"));
        sizePolicy2.setHeightForWidth(spinBox_wireSize->sizePolicy().hasHeightForWidth());
        spinBox_wireSize->setSizePolicy(sizePolicy2);
        spinBox_wireSize->setMinimum(1);
        spinBox_wireSize->setSingleStep(5);
        spinBox_wireSize->setValue(5);

        horizontalLayout->addWidget(spinBox_wireSize);

        pushButton_autoWire = new QPushButton(centralWidget);
        pushButton_autoWire->setObjectName(QString::fromUtf8("pushButton_autoWire"));
        sizePolicy2.setHeightForWidth(pushButton_autoWire->sizePolicy().hasHeightForWidth());
        pushButton_autoWire->setSizePolicy(sizePolicy2);

        horizontalLayout->addWidget(pushButton_autoWire);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout->addLayout(horizontalLayout);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 662, 21));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuWire = new QMenu(menuBar);
        menuWire->setObjectName(QString::fromUtf8("menuWire"));
        MainWindow->setMenuBar(menuBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        MainWindow->setStatusBar(statusBar);

        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuWire->menuAction());
        menuFile->addAction(actionOpen_DXF);
        menuFile->addAction(actionExport_xModel);
        menuFile->addSeparator();
        menuFile->addAction(actionExit);
        menuWire->addAction(actionAutoWire);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", nullptr));
        actionOpen_DXF->setText(QApplication::translate("MainWindow", "Open DXF...", nullptr));
        actionExport_xModel->setText(QApplication::translate("MainWindow", "Export xModel", nullptr));
        actionExit->setText(QApplication::translate("MainWindow", "Exit", nullptr));
        actionAutoWire->setText(QApplication::translate("MainWindow", "AutoWire", nullptr));
        label->setText(QApplication::translate("MainWindow", "Wire Length:", nullptr));
        pushButton_autoWire->setText(QApplication::translate("MainWindow", "Auto Wire", nullptr));
        menuFile->setTitle(QApplication::translate("MainWindow", "File", nullptr));
        menuWire->setTitle(QApplication::translate("MainWindow", "Wire", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
