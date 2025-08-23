#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_dxf_triggered()
{
	
}

void MainWindow::on_actionExport_xModel_triggered()
{
	
}

void MainWindow::on_actionExit_triggered()
{
	
}

void MainWindow::on_actionAutoWire_triggered()
{
	
}

void MainWindow::on_actionView_Logs_triggered()
{
	
}
