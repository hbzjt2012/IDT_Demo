#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QDateTime>
#include <QTcpSocket>

#include <conio.h> /* This include is just for Windows PCs. */
#include <stdio.h>

/* Algorithm library header files, download the target specific library
 * from the IDT webpage. */
#include "libs/eco2.h"
#include "libs/iaq.h"
#include "libs/odor.h"
#include "libs/tvoc.h"
#include "libs/r_cda.h"

/* Files needed for hardware access, needs to be adjusted to target. */
#include "hicom/hicom.h"
#include "hicom/hicom_i2c.h"

/* files to control the sensor */
#include "zmod44xx.h"

/* EDP头文件 */
#include "edp/cJSON.h"
#include "edp/Common.h"
#include "edp/EdpKit.h"

// start sequencer defines

#define FIRST_SEQ_STEP      0
#define LAST_SEQ_STEP       1

#define STATUS_LAST_SEQ_STEP_MASK   0x0F

#define STATUS_OPEN  1
#define STATUS_CLOSE 0

#define ONENET_IP   "183.230.40.39"
#define ONENET_PORT 876

#define ONENET_DEV_ID  "512322645"
#define ONENET_API_KEY "g9dAyrBB=ucYTGYejL2GlAp0zHo="

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QLabel *lblTimer;
    QLabel *lblState;
    QTimer *readTimer;
    QTcpSocket *sockfd;

    /* These are the hardware handles which needs to be adjusted to specific HW */
    hicom_handle_t hicom_handle;
    hicom_status_t hicom_status;

    zmod44xx_dev_t dev;
    int8_t ret;
    uint8_t zmod44xx_status;
    float r_mox;

    /* To work with the algorithms target specific libraries needs to be
    * downloaded from IDT webpage and included into the project. */
    uint8_t iaq;
    float eco2;
    float r_cda;
    float tvoc;
    control_signal_state_t cs_state;

    eco2_params eco2_par;
    tvoc_params tvoc_par;
    odor_params odor_par;

    uint8_t dev_hicom_status;

    /*连接ONENET用参数变量*/
    EdpPacket* send_pkg;
    /********************/

    void print_hicom_error(hicom_status_t status);

    int32 DoSend(const char *buffer, uint32 len);
    void hexdump(const unsigned char *buf, uint32 num);
private slots:
    void timerUpdate(void);
    void readSensor(void);
    void on_btnStart_clicked();
    void on_btnStop_clicked();
    void recv_thread_func();
};

#endif // MAINWINDOW_H
