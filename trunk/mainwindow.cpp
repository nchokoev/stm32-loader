#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ftHandle = NULL;
    ui->setupUi(this);
    tmr = new QTimer(this);

    connect(ui->cbDevices, SIGNAL(currentIndexChanged(const QString &)),
        this, SLOT(currentIndexChanged(const QString &)));
    connect(tmr, SIGNAL(timeout()),
        this, SLOT(tmrout()));
    bitmask=0;
    Refresh();
    ui->textEdit_Status->clear();
}

MainWindow::~MainWindow()
{
    delete ui;
}

int MainWindow::fLoad(void)
{
    int err=-1;
    int cntr = 3;
    quint8 ver;
    QString fname;
    QByteArray sBuffer;
    QByteArray vBuffer;
    fname = ui->lnPath->text();
    ui->textEdit_Status->clear();
    if(fname.isEmpty())
    {
        ui->textEdit_Status->append(tr("No File selected."));
        return -1;
    }

    QFile fileB(fname);
    if (!fileB.open(QIODevice::ReadOnly))
    {
        ui->textEdit_Status->append(tr("Cannot open the file."));;
        return -1;
    }
    sBuffer=fileB.readAll();
    fileB.close();

    configSerial();

    while((cntr-- > 0) && (err != 0))
    {
        BootMode0HI();
        Thrd::msleep(100);
        ST_Reset();
        Thrd::msleep(5);
        err = ST_Connect();

        if(err != 0)
        {
            BootMode0LO();
            Thrd::msleep(100);
            ST_Reset();
        }
    }
    if((cntr > 0) || (err == 0))
    {
        ui->textEdit_Status->append(tr("Connected."));;
        err = ST_Get(&ver);
        if((err != 0) || (ver < 0x20))
        {
            ui->textEdit_Status->append(tr("ST Loader not found or version < 0x20."));;
            return -1;
        }
    }
    else
    {
        ui->textEdit_Status->append(tr("Cannot connect."));
        return -1;
    }

    int addr = 0x08000000;
    vBuffer.resize(sBuffer.size());
    ST_EraseMemory();
    err = ST_WriteMemory(addr, sBuffer.size(), (unsigned char *)sBuffer.data());
    if(ui->cbVerify->isChecked())
    {
        ui->textEdit_Status->append(tr("Verify!"));
        Thrd::msleep(100);
        err = ST_ReadMemory(addr, vBuffer.size(), (unsigned char *)vBuffer.data());
        for(int i=0; i<vBuffer.size(); i++)
        {
            if(vBuffer.at(i) != sBuffer.at(i))
            {
                ui->textEdit_Status->append(QString(tr("Verify Error at %1")).arg(i+addr));
                return -1;
            }
        }
    }
    ui->textEdit_Status->append(tr("Loaded!"));
    return 0;
}

void MainWindow::tmrout(void)
{
    tmout = 1;
}

void MainWindow::ST_Reset(void)
{
    ui->textEdit_Status->append("Board reset.");;
    ResetLO();
    Thrd::msleep(100);
    ResetHI();
    Thrd::msleep(100);
}

void MainWindow::ST_BMode0(bool mode)
{
    switch(mode)
    {
        case 0:
            BootMode0LO();
            break;
        case 1:
            BootMode0HI();
            break;
    }
}

void MainWindow::BootMode0LO(void)
{
    ui->textEdit_Status->append(tr("Set Boot mode 0"));
    // Initialise mask value
    bitmask |= (ST_BOOT0_PIN<<4);//0x40;
    bitmask &= ~ST_BOOT0_PIN;//~0x04;
    setBitMode(bitmask);
}

void MainWindow::BootMode0HI(void)
{
    ui->textEdit_Status->append(tr("Set Boot Mode 1"));
    // Initialise mask value
    bitmask |= ((ST_BOOT0_PIN<<4)|ST_BOOT0_PIN);//0x44;
    setBitMode(bitmask);
}

void MainWindow::ResetLO(void)
{
    // Initialise mask value
    bitmask |= (ST_RESET_PIN<<4);//0x80;
    bitmask &= ~ST_RESET_PIN;//~0x08;
    setBitMode(bitmask);
}

void MainWindow::ResetHI(void)
{
    // Initialise mask value
    bitmask &= ~((ST_RESET_PIN<<4) | ST_RESET_PIN);//0x88;
    setBitMode(bitmask);
}

void MainWindow::setBitMode(quint8 bits)
{
    FT_STATUS ftStatus;
    QString dev;
    dev = ui->cbDevices->currentText();

    // Open device
    ftStatus = FT_OpenEx((void*)dev.toLatin1().constData(), FT_OPEN_BY_DESCRIPTION, &ftHandle);
//    ftStatus = FT_Open(0, &ftHandle);
    if (ftStatus != FT_OK)
    {
        qDebug("FT_OpenEx Failed.");
        return;
    }

    // Set CBUS pins states
    ftStatus = FT_SetBitMode(ftHandle,bits,0x20);
    if (ftStatus != FT_OK)
    {
        qDebug("Failed to write data to CBUS pins.");
    }

    // Close device handle
    ftStatus = FT_Close(ftHandle);
}

int MainWindow::configSerial(void)
{
    FT_STATUS ftStatus;
    // Open device
    QString dev;
    dev = ui->cbDevices->currentText();
    ftStatus = FT_OpenEx((void*)dev.toLatin1().constData(), FT_OPEN_BY_DESCRIPTION, &ftHandle);
    if (ftStatus != FT_OK)
    {
        qDebug("FT_OpenEx Failed.");
        return -1;
    }
    ftStatus = FT_SetBaudRate(ftHandle, FT_BAUD_115200);
    if (ftStatus != FT_OK)
    {
        qDebug("FT_SetBaudRate Failed.");
        return -1;
    }
    ftStatus = FT_SetDataCharacteristics(ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_EVEN);
    if (ftStatus != FT_OK)
    {
        qDebug("FT_SetDataCharacteristics Failed.");
        return -1;
    }
    ftStatus = FT_SetTimeouts(ftHandle,300,300);
    if (ftStatus != FT_OK)
    {
        qDebug("FT_SetTimeouts Failed.");
        return -1;
    }
    // Close device handle
    ftStatus = FT_Close(ftHandle);
    return 0;
}

void MainWindow::Refresh(void)
{
    ui->textEdit_Status->append(tr("Refresh."));
    FT_STATUS ftStatus;
    quint32 NumDevs;
    quint32 RCount;
    quint32 count;

    FT_DEVICE_LIST_INFO_NODE *DevInfo;
    FT_DEVICE_LIST_INFO_NODE *RDevsInfo;

    ui->cbDevices->clear();
    // Get number of devices connected
    ftStatus = FT_CreateDeviceInfoList((LPDWORD)&NumDevs);
    // Allocate storage
    DevInfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*NumDevs);
    // Get device list
    ftStatus = FT_GetDeviceInfoList(DevInfo,(LPDWORD)&NumDevs);
    // Get number of FT232R devices
    RCount = 0;
    for (count = 0; count < NumDevs; count++ )
    {
        if (DevInfo[count].Type == 5)
        {
            RCount = RCount + 1;
        }
    }
    // Ensure at least 1 R chip available.  If not, show dialog
    if (RCount == 0)
    {
        ui->cbDevices->addItem(QString("No FT232R or FT245R devices found."));
    }
    else {
        // Allocate storage for R chips
        RDevsInfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*RCount);
        // Get R chip device list
        RCount = 0;
        for (count = 0; count < NumDevs; count++ )
        {
            if (DevInfo[count].Type == 6)
            {
                RDevsInfo[RCount] = DevInfo[count];
                RCount = RCount + 1;
            }
        }
        // Populate combobox
        QStringList devices;
        devices.clear();
        for (count = 0; count < NumDevs; count++ )
        {
            devices.append(DevInfo[count].Description);
        }
        ui->cbDevices->addItems(devices);
        // Free storage
        free(DevInfo);
        free(RDevsInfo);
    }
}

void MainWindow::currentIndexChanged(const QString &text)
{
    qDebug() << "currentIndexChanged->" << text.toLatin1();
}

int MainWindow::ST_Connect(void)
{
    FT_STATUS ftStatus;
    quint8 buf[1];
    quint32 bw;
    int err = -1;
//    configSerial();
    // Open device
    QString dev;
    dev = ui->cbDevices->currentText();
    ftStatus = FT_OpenEx((void*)dev.toLatin1().constData(), FT_OPEN_BY_DESCRIPTION, &ftHandle);
    if (ftStatus != FT_OK)
    {
        qDebug("FT_OpenEx Failed.");
        return -1;
    }
    buf[0]=0x7F;
    FT_Write(ftHandle, buf, 1, (LPDWORD)&bw);

    FT_Read(ftHandle, buf,1, (LPDWORD)&bw);
    if (buf[0] == ST_ACK)
    {
        qDebug("ST OK!");
        err = 0;
    }
    // Close device handle
    ftStatus = FT_Close(ftHandle);
    return err;
}

int MainWindow::ST_Get(quint8 *ver)
{
    FT_STATUS ftStatus;
    quint8 buf[128];
    quint8 nrbytes;
    quint32 bw;
    int err = -1;
//    configSerial();
    // Open device
    QString dev;
    dev = ui->cbDevices->currentText();
    ftStatus = FT_OpenEx((void*)dev.toLatin1().constData(), FT_OPEN_BY_DESCRIPTION, &ftHandle);
    if (ftStatus != FT_OK)
    {
        qDebug("FT_OpenEx Failed.");
        return err;
    }
    buf[0]=ST_GET_CMD;
    buf[1]=0xFF-ST_GET_CMD;
    FT_Write(ftHandle, buf, 2, (LPDWORD)&bw);

    FT_Read(ftHandle, buf,1, (LPDWORD)&bw);
    if (buf[0] == ST_ACK)
    {
        //Receive Nr bytes
        FT_Read(ftHandle, buf, 1, (LPDWORD)&bw);
        nrbytes = buf[0]+2; //Version + ack
        //Receive all
        FT_Read(ftHandle, buf, nrbytes, (LPDWORD)&bw);
        if(ver != NULL) *ver = buf[0];
        err = 0;
    }
    // Close device handle
    ftStatus = FT_Close(ftHandle);
    return err;
}

void MainWindow::ST_GetVersion(void)
{
}

void MainWindow::ST_GetID(void)
{
}

int MainWindow::ST_ReadMemory(unsigned int startAddress, unsigned short count, unsigned char *data)
{
    FT_STATUS ftStatus;
    quint8 buf[5];
    quint32 bw;
    int err = -1;
    unsigned short loccount;
    unsigned short locbuffsize;
    unsigned char *plocbuff;
    unsigned int locaddr;
//    configSerial();
    // Open device
    QString dev;
    dev = ui->cbDevices->currentText();
    ftStatus = FT_OpenEx((void*)dev.toLatin1().constData(), FT_OPEN_BY_DESCRIPTION, &ftHandle);
    if (ftStatus != FT_OK)
    {
        qDebug("FT_OpenEx Failed.");
        return err;
    }
    plocbuff = data;
    loccount = count;
    locaddr = startAddress;

    while (loccount)
    {
        locbuffsize=(loccount > 256) ? 256 : loccount;
        //Send Command
        buf[0]=ST_READMEMORY_CMD;
        buf[1]=0xFF-ST_READMEMORY_CMD;
        FT_Write(ftHandle, buf, 2, (LPDWORD)&bw);
        FT_Read(ftHandle, buf,1, (LPDWORD)&bw);
        if (buf[0] == ST_ACK)
        {
            //Send Start Address
            buf[0]=(locaddr>>24)&0xFF;
            buf[1]=(locaddr>>16)&0xFF;
            buf[2]=(locaddr>>8)&0xFF;
            buf[3]=(locaddr>>0)&0xFF;
            buf[4]=buf[0]^buf[1]^buf[2]^buf[3];
            FT_Write(ftHandle, buf, 5, (LPDWORD)&bw);
            FT_Read(ftHandle, buf,1, (LPDWORD)&bw);
            if (buf[0] == ST_ACK)
            {
                //Send data count
                buf[0]=locbuffsize-1;
                buf[1]=0xFF-buf[0];
                FT_Write(ftHandle, buf, 2, (LPDWORD)&bw);
                FT_Read(ftHandle, buf,1, (LPDWORD)&bw);
                if (buf[0] == ST_ACK)
                {
                    //Receive the data
                    FT_Read(ftHandle, plocbuff, loccount, (LPDWORD)&bw);
                    err = 0;
                }
            }
        }
        loccount -= locbuffsize;
        locaddr += locbuffsize;
        plocbuff += locbuffsize;
    }
    // Close device handle
    ftStatus = FT_Close(ftHandle);
    return err;
}

void MainWindow::ST_Go(void)
{
}

int MainWindow::ST_WriteMemory(unsigned int startAddress, unsigned short count, unsigned char *data)
{
    FT_STATUS ftStatus;
    quint8 buf[5];
    quint32 bw;
    quint8 csum=0;
    int err = -1;
    unsigned short loccount;
    unsigned short locbuffsize;
    unsigned char *plocbuff;
    unsigned int locaddr;
//    configSerial();
    // Open device
    QString dev;
    dev = ui->cbDevices->currentText();
    ftStatus = FT_OpenEx((void*)dev.toLatin1().constData(), FT_OPEN_BY_DESCRIPTION, &ftHandle);
    if (ftStatus != FT_OK)
    {
        qDebug("FT_OpenEx Failed.");
        return err;
    }
    plocbuff = data;
    loccount = count;
    locaddr = startAddress;

    while (loccount)
    {
        locbuffsize=(loccount > 256) ? 256 : loccount;
        csum = locbuffsize-1;
        for (int i=0; i < locbuffsize; i++)
        {
            csum ^= plocbuff[i];
        }
        //Send Command
        buf[0]=ST_WRITEMEMORY_CMD;
        buf[1]=0xFF-ST_WRITEMEMORY_CMD;
        FT_Write(ftHandle, buf, 2, (LPDWORD)&bw);
        FT_Read(ftHandle, buf,1, (LPDWORD)&bw);
        if (buf[0] == ST_ACK)
        {
            //Send Start Address
            buf[0]=(locaddr>>24)&0xFF;
            buf[1]=(locaddr>>16)&0xFF;
            buf[2]=(locaddr>>8)&0xFF;
            buf[3]=(locaddr>>0)&0xFF;
            buf[4]=buf[0]^buf[1]^buf[2]^buf[3];
            FT_Write(ftHandle, buf, 5, (LPDWORD)&bw);
            FT_Read(ftHandle, buf,1, (LPDWORD)&bw);
            if (buf[0] == ST_ACK)
            {
                //Send data count
                buf[0]=locbuffsize-1;
                FT_Write(ftHandle, buf, 1, (LPDWORD)&bw);
                //Send the data
                FT_Write(ftHandle, plocbuff, locbuffsize, (LPDWORD)&bw);
                //Send checksum 
                buf[0]=csum;
                FT_Write(ftHandle, buf, 1, (LPDWORD)&bw);
                FT_Read(ftHandle, buf,1, (LPDWORD)&bw);
                if (buf[0] == ST_ACK)
                {
                    err = 0;
                }
            }
        }
        loccount -= locbuffsize;
        locaddr += locbuffsize;
        plocbuff += locbuffsize;
    }
    // Close device handle
    ftStatus = FT_Close(ftHandle);
    return err;
}

int MainWindow::ST_EraseMemory(void)
{
    FT_STATUS ftStatus;
    quint8 buf[5];
    quint32 bw;
    int err = -1;
//    configSerial();
    // Open device
    QString dev;
    dev = ui->cbDevices->currentText();
    ftStatus = FT_OpenEx((void*)dev.toLatin1().constData(), FT_OPEN_BY_DESCRIPTION, &ftHandle);
    if (ftStatus != FT_OK)
    {
        qDebug("FT_OpenEx Failed.");
        return err;
    }
    //Send Command
    buf[0]=ST_ERASEMEMORY_CMD;
    buf[1]=0xFF-ST_ERASEMEMORY_CMD;
    FT_Write(ftHandle, buf, 2, (LPDWORD)&bw);
    FT_Read(ftHandle, buf,1, (LPDWORD)&bw);
    if (buf[0] == ST_ACK)
    {
        //Erase All Command
        buf[0]=0xFF;
        buf[1]=0x00;
        FT_Write(ftHandle, buf, 2, (LPDWORD)&bw);
        FT_Read(ftHandle, buf,1, (LPDWORD)&bw);
        if (buf[0] == ST_ACK)
        {
            err=0;
        }
    }
    // Close device handle
    ftStatus = FT_Close(ftHandle);
    return err;
}

void MainWindow::ST_WriteProtect(void)
{
}

void MainWindow::ST_WriteUnprotect(void)
{
}

void MainWindow::ST_ReadoutProtect(void)
{
}

void MainWindow::ST_ReadoutUnprotect(void)
{
}

void MainWindow::on_pbReset_clicked()
{
    ST_Reset();
}

void MainWindow::on_pbBrowse_clicked()
{
    QString fname;
    fname = QFileDialog::getOpenFileName(this, tr("Open File"), ui->lnPath->text(), tr("Binary Files (*.bin)"));
    if (!fname.isEmpty())
    {
        ui->lnPath->setText(fname);
    }
}

void MainWindow::on_pbLoad_clicked()
{
    int err;
    err = fLoad();
    if(err)
    {
        qDebug("Load Error.");
        return;
    }
    BootMode0LO();
    Thrd::msleep(100);
    ST_Reset();
}

void MainWindow::on_pbRefresh_clicked()
{
    Refresh();
}
