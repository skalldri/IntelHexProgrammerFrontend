#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QtCore>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
            ui->portList->addItem(info.portName());
    }
    port = new QSerialPort();
    hexFile = new QFile();

    QObject::connect(port, SIGNAL(readyRead()), this, SLOT(serialDataReceived()));
    //QObject::connect(port, SIGNAL, this, SLOT(serialDataReceived()));
    QObject::connect(port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(serialError()));
    QObject::connect(this, SIGNAL(hardwareResponseFinished()), this, SLOT(serialReady()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openFileDialog()
{
    QFileDialog openDialog;
    ui->fileEdit->setText(openDialog.getOpenFileName());
}

void MainWindow::program()
{
    hexFile->setFileName(ui->fileEdit->text());

    if(!hexFile->exists())
    {
        ui->statusBar->showMessage("Aborted: file does not exist.");
        return;
    }

    if(!hexFile->open(QFile::ReadOnly))
    {
        ui->statusBar->showMessage("Aborted: unable to open file for reading.");
        return;
    }

    if(port->isOpen())
        port->close();

    QSerialPortInfo info;

    foreach(info, QSerialPortInfo::availablePorts())
    {
        if(info.portName() == ui->portList->currentText())
        {
            port->setPort(info);
            break;
        }
    }

    port->setBaudRate(QSerialPort::Baud9600);
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);

    if(port->open(QIODevice::ReadWrite))
    {
        qDebug("Port open");
    }

    port->waitForReadyRead(10000);
    ui->statusBar->showMessage("Programming... (this may take a while)");
}

void MainWindow::serialDataReceived()
{
    QString msg = QString(port->readAll());
    message += msg;

    //qDebug() << "Message partial received: " << msg;

    if(msg.contains('!'))
    {
        if(message.contains("ERR"))
        {
            port->close();
            qDebug() << "WARNING: PROGRAMMER THREW ERROR DURING PROGRAMMING. HALTING.";
            qDebug() << message;
            return;
        }

        messageFinished = true;
        qDebug() << message;
        emit hardwareResponseFinished();
    }
}

void MainWindow::serialError()
{
    qDebug() << port->errorString();
}

void MainWindow::serialReady()
{
    if(!hexFile->isOpen())
    {
        ui->statusBar->showMessage("Error: file is closed while trying to program.");
        port->close();
        return;
    }

    messageFinished = false;
    message = "";

    if(!hexFile->atEnd())
    {
        //TODO: remove trailing endline char from line
        QByteArray line = hexFile->readLine();
        qDebug() << "Writing " << line;
        port->write(line);
        port->write("*");
    }
    else
    {
        ui->statusBar->showMessage("Programming complete");
    }
}
