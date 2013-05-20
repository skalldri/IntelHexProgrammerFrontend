#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QFileDialog>
#include <QtCore>
#include <QFile>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void openFileDialog();
    void program();
    void serialDataReceived();
    void serialError();
    void serialReady();

signals:
    void hardwareResponseFinished();
    
private:
    Ui::MainWindow *ui;
    QSerialPort* port;
    bool messageFinished;
    QString message;
    QFile* hexFile;
};

#endif // MAINWINDOW_H
