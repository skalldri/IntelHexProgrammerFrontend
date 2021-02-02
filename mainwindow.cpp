#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QtCore>

QByteArray lastLine;

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
    {
        port->close();
    }

    QSerialPortInfo info;

    foreach(info, QSerialPortInfo::availablePorts())
    {
        if(info.portName() == ui->portList->currentText())
        {
            qDebug() << "Port selected";
            port->setPort(info);
            break;
        }
    }

    port->setBaudRate(QSerialPort::Baud9600);
    port->setDataBits(QSerialPort::Data8);
    port->setStopBits(QSerialPort::OneStop);
    port->setParity(QSerialPort::NoParity);
    port->setFlowControl(QSerialPort::NoFlowControl);

    if(port->open(QIODevice::ReadWrite))
    {
        qDebug("Port open");
    }

    ui->statusBar->showMessage("Programming... (this may take a while)");

    serialReady();
}

void MainWindow::serialDataReceived()
{
    QString msg = QString(port->readAll());
    message += msg;

    if(msg.contains('>') && !messageFinished)
    {
        messageFinished = true;

        if(message.contains("ERR") || message.contains("WARN"))
        {
            qDebug() << "WARNING: PROGRAMMER THREW ERROR DURING PROGRAMMING.";
            qDebug() << message;

            ui->statusBar->showMessage("WARNING: Programming error detected");

            QObject::disconnect(port, SIGNAL(readyRead()), this, SLOT(serialDataReceived()));

            port->write(QByteArray(1, '\n'));
            QThread::msleep(500);
            port->write(QByteArray(1, '\n'));
            QThread::msleep(500);

            qDebug() << port->readAll();

            QObject::connect(port, SIGNAL(readyRead()), this, SLOT(serialDataReceived()));

            qDebug() << "Writing " << lastLine;

            message = "";
            messageFinished = false;
            port->write(lastLine);
            return;
        }

        ui->statusBar->showMessage("Programming... (this may take a while)");
        qDebug() << message;
        QThread::msleep(200);
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
        line.truncate(line.length() - 2);
        line.append('\n');
        lastLine = line;
        qDebug() << "Writing " << line;
        port->write(line);
    }
    else
    {
        ui->statusBar->showMessage("Programming complete");
    }
}
