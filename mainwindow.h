#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
//#include "fttypedefs.h"
#include <windows.h>
#include "ftd2xx.h"

#define DLL_NAME "ftd2xx.dll"

#define FT_CBUS2    (1<<2)
#define FT_CBUS3    (1<<3) 

#define ST_RESET_PIN    FT_CBUS3
#define ST_BOOT0_PIN    FT_CBUS2

#define ST_GET_CMD              0x00
#define ST_GETVERSION_CMD       0x01
#define ST_GETID_CMD            0x02
#define ST_READMEMORY_CMD       0x11
#define ST_GO_CMD               0x21
#define ST_WRITEMEMORY_CMD      0x31
#define ST_ERASEMEMORY_CMD      0x43
#define ST_WRITEPROTECT_CMD     0x63
#define ST_WRITEUNPROTECT_CMD   0x73
#define ST_READOUTPROTECT_CMD   0x82
#define ST_READOUTUNPROTECT_CMD 0x92

#define ST_ACK                  0x79
#define ST_NACK                 0x1F

namespace Ui
{
    class MainWindow;
}

class Thrd : public QThread
{
public:
    static void sleep(unsigned long secs) {
        QThread::sleep(secs);
    }
    static void msleep(unsigned long msecs) {
        QThread::msleep(msecs);
    }
    static void usleep(unsigned long usecs) {
        QThread::usleep(usecs);
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    FT_HANDLE ftHandle;
    quint8 bitmask;
    quint8 tmout;
    QTimer *tmr;
    QString filepath;
    Ui::MainWindow *ui;
    void setBitMode(quint8 bits);
    int configSerial(void);
    void ST_BMode0(bool mode);
    //void ST_Reset(void);
    void BootMode0LO(void);
    void BootMode0HI(void);
    int ST_Connect(void);
    int ST_Get(quint8 *ver);
    void ST_GetVersion(void);
    void ST_GetID(void);
    int ST_ReadMemory(unsigned int startAddress, unsigned short count, unsigned char *data);
    void ST_Go(void);
    int ST_WriteMemory(unsigned int startAddress, unsigned short count, unsigned char *data);
    int ST_EraseMemory(void);
    void ST_WriteProtect(void);
    void ST_WriteUnprotect(void);
    void ST_ReadoutProtect(void);
    void ST_ReadoutUnprotect(void);
    void ResetHI(void);
    void ResetLO(void);
    int fLoad(void);
    void ST_Reset(void);
    void Refresh(void);

private slots:
    void on_pbRefresh_clicked();
    void on_pbLoad_clicked();
    void on_pbBrowse_clicked();
    void on_pbReset_clicked();
    void currentIndexChanged(const QString &text);
    void tmrout(void);
};

#endif // MAINWINDOW_H
