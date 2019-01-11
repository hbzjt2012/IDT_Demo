#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

#if 0
//    eco2_par = {
//           .min_co2 = 400,
//           .max_co2 = 5000,
//           .tvoc_to_eco2 = 800,
//           .hot_wine_rate = 0.3,
//           .open_window_rate = -0.05,
//    };

//    tvoc_par = {
//           .A = 680034,
//           .alpha = 0.7,
//    };

//    odor_par = {
//           .alpha = 0.8,
//           .stop_delay = 24,
//           .threshold = 1.3,
//           .tau = 720,
//           .stabilization_samples = 15,
//    };
#endif

    eco2_par.min_co2 =400;
    eco2_par.max_co2 = 5000;
    eco2_par.tvoc_to_eco2 = 800;
    eco2_par.hot_wine_rate = 0.3;
    eco2_par.open_window_rate = -0.05;

    tvoc_par.A = 680034;
    tvoc_par.alpha = 0.7;

    odor_par.alpha = 0.8;
    odor_par.stop_delay = 24;
    odor_par.threshold = 1.3;
    odor_par.tau = 720;
    odor_par.stabilization_samples =15;

    dev_hicom_status = STATUS_CLOSE;

    lblState = new QLabel(this);
    lblState->setText("");
    ui->statusBar->addWidget(lblState);

    lblTimer = new QLabel(this);
    lblTimer->setText("");
    ui->statusBar->addPermanentWidget(lblTimer);

    QTimer *timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(timerUpdate()));
    timer->start(1000);

    readTimer = new QTimer(this);
    connect(readTimer,SIGNAL(timeout()),this,SLOT(readSensor()));
    readTimer->setInterval(1000);

    /* 创建连接云平台的socket */
    sockfd = new QTcpSocket(this);
    connect(sockfd, SIGNAL(readyRead()), this, SLOT(recv_thread_func()));
    sockfd->connectToHost(ONENET_IP,ONENET_PORT);
    if(!sockfd->waitForConnected(3000))
    {
        ui->textEdit->append("连接ONENET服务器失败！");
    }else
    {
        ui->textEdit->append("连接ONENET服务器成功！");
    }

    /* 向设备云发送连接请求 */
    send_pkg = PacketConnect1(ONENET_DEV_ID, ONENET_API_KEY);
    printf("send connect to server, bytes: %d\n", send_pkg->_write_pos);
    ret=DoSend((const char*)send_pkg->_data, send_pkg->_write_pos);
    DeleteBuffer(&send_pkg);
}

MainWindow::~MainWindow()
{
    if(dev_hicom_status == STATUS_OPEN)
    {
        /* ****** BEGIN TARGET SPECIFIC DEINITIALIZATION ****** */
        hicom_status = hicom_power_off(hicom_handle);
        if (FTC_SUCCESS != hicom_status) {
            print_hicom_error(hicom_status);
            //return hicom_status;
        }

        /* disconnect HiCom board */
        hicom_status = hicom_close(hicom_handle);
        if (FTC_SUCCESS != hicom_status) {
            print_hicom_error(hicom_status);
            //return hicom_status;
        }
        /* ****** END TARGET SPECIFIC DEINITIALIZATION ****** */

        printf("Bye!\n");
    }

    delete sockfd;
    delete readTimer;
    delete ui;

}

void MainWindow::timerUpdate()
{
    QDateTime time = QDateTime::currentDateTime();
    QString str = time.toString("yyyy-MM-dd hh:mm:ss dddd");
    lblTimer->setText("系统时间: "+str);
}

void MainWindow::print_hicom_error(hicom_status_t status)
{
    char error_str[512];
    hicom_get_error_string(status, error_str, sizeof error_str);
    fprintf(stderr, "ERROR (HiCom): %s\n", error_str);
    QString str = "";
    str.sprintf("ERROR (HiCom): %s\n", error_str);
    ui->textEdit->append(str);
}

void MainWindow::readSensor(void)
{
    QString str = "";
//    qDebug("Bdddddddye!\n");
//     printf("Bdddddddye!\n");
//    ui->textEdit->append("Measurementing......!\n");

    /* INSTEAD OF POLLING THE INTERRUPT CAN BE USED FOR OTHER HW */
    /* wait until readout result is possible */
    while (LAST_SEQ_STEP != (zmod44xx_status & 0x07)) {
        dev.delay_ms(50);
        ret = zmod44xx_read_status(&dev, &zmod44xx_status);
        if(ret) {
            printf("Error %d, exiting program!\n", ret);
            str.sprintf("ERROR (HiCom): %d\n", ret);
            ui->textEdit->append(str);
            //goto exit;
        }
    }

    /* evaluate and show measurement results */
    ret = zmod44xx_read_rmox(&dev, &r_mox);
    if(ret) {
        printf("Error %d, exiting program!\n", ret);
        str.sprintf("ERROR (HiCom): %d\n", ret);
        ui->textEdit->append(str);
        //goto exit;
    }

    /* To work with the algorithms target specific libraries needs to be
     * downloaded from IDT webpage and included into the project */

    /* calculate clean dry air resistor */
    r_cda = r_cda_tracker(r_mox);

    /* calculate result to TVOC value */
    tvoc = calc_tvoc(r_mox, r_cda, &tvoc_par);

    /* calculate IAQ index */
    iaq = calc_iaq(r_mox, r_cda, &tvoc_par);

    /* calculate estimated CO2 */
    eco2 = calc_eco2(tvoc, 3, &eco2_par);

    /* get odor control signal */
    //cs_state = calc_odor(r_mox, &odor_par);

    printf("\n");
    printf("Measurement:\n");
    printf("  Rmox  = %5.0f kOhm\n", (r_mox / 1000.0));
    printf("  IAQ %d\n", iaq);
    printf("  TVOC %f mg/m^3\n", tvoc);
    printf("  eCO2 %f\n", eco2);
    //printf("  odor control state %d\n", cs_state);

    //ui->textEdit->append("\n");
    ui->textEdit->append("Measurement:\n");
    str.sprintf("  Rmox  = %3.2f kOhm", (r_mox / 1000.0));
    ui->textEdit->append(str);
    ui->lcdRmox->display((r_mox / 1000.0));
    str.sprintf("  IAQ %d", iaq);
    ui->textEdit->append(str);
    ui->lcdIAQ->display(iaq);
    str.sprintf("  TVOC %3.2f mg/m^3", tvoc);
    ui->textEdit->append(str);
    ui->lcdTVOC->display(tvoc);
    str.sprintf("  eCO2 %3.2f", eco2);
    ui->textEdit->append(str);
    ui->lcdeCO2->display(eco2);
    //str.sprintf("  odor control state %d\n", cs_state);
    //ui->textEdit->append(str);

    /* 向ONENET设备平台发送数据 */
    str.sprintf("Rmox,%5.0f;IAQ,%d;TVOC,%f;eCO2,%f",r_mox/1000.0,iaq,tvoc,eco2);
    //const char *send_str = str.toLatin1().data();
    char send_str[] = ",;temperature,2015-03-22 22:31:12,22.5;humidity,35%;pm2.5,89;1001";
    sprintf(send_str,",;Rmox,%3.2f;IAQ,%d;TVOC,%3.2f;eCO2,%3.2f",r_mox/1000.0,iaq,tvoc,eco2);
    send_pkg = PacketSavedataSimpleString(ONENET_DEV_ID, send_str, 0);
    DoSend((const char*)send_pkg->_data, send_pkg->_write_pos);
    DeleteBuffer(&send_pkg);

    /* INSTEAD OF POLLING THE INTERRUPT CAN BE USED FOR OTHER HW */
    /* waiting for sensor ready */
    while (FIRST_SEQ_STEP != (zmod44xx_status & 0x07)) {
        dev.delay_ms(50);
        ret = zmod44xx_read_status(&dev, &zmod44xx_status);
        if(ret) {
            printf("Error %d, exiting program!\n", ret);
            str.sprintf("ERROR (HiCom): %d\n", ret);
            ui->textEdit->append(str);
            //goto exit;
        }
    }
}

void MainWindow::on_btnStart_clicked()
{
    QString str = "";

    if(dev_hicom_status == STATUS_CLOSE)
    {
        /* ****** BEGIN TARGET SPECIFIC INITIALIZATION ************************** */
        /* This part automatically resets the sensor. */
        /* connect to HiCom board */
        hicom_status = hicom_open(&hicom_handle);
        if (FTC_SUCCESS != hicom_status) {
            print_hicom_error(hicom_status);
            //return hicom_status;
        }

        /* switch supply on */
        hicom_status = hicom_power_on(hicom_handle);
        if (FTC_SUCCESS != hicom_status) {
            print_hicom_error(hicom_status);
            //return hicom_status;
        }

        set_hicom_handle(&hicom_handle);

        qDebug("hicom_handle:%d",(int)hicom_handle);

        dev_hicom_status = STATUS_OPEN;
    }

    /* ****** END TARGET SPECIFIC INITIALIZATION **************************** */

    /* Set initial hardware parameter */
    dev.read = hicom_i2c_read;
    dev.write = hicom_i2c_write;
    dev.delay_ms = hicom_sleep;
    dev.i2c_addr = ZMOD4410_I2C_ADDRESS;

    /* initialize and start sensor */
    ret = zmod44xx_read_sensor_info(&dev);
    if(ret) {
        printf("Error %d, exiting program!\n", ret);
        str.sprintf("ERROR (HiCom): %d\n", ret);
        ui->textEdit->append(str);
        //goto exit;
    }

    ret = zmod44xx_init_sensor(&dev);
    if(ret) {
        printf("Error %d, exiting program!\n", ret);
        str.sprintf("ERROR (HiCom): %d\n", ret);
        ui->textEdit->append(str);
        //goto exit;
    }

    ret = zmod44xx_init_measurement(&dev);
    if(ret) {
        printf("Error %d, exiting program!\n", ret);
        str.sprintf("ERROR (HiCom): %d\n", ret);
        ui->textEdit->append(str);
        //goto exit;
    }

    ret = zmod44xx_start_measurement(&dev);
    if(ret) {
        printf("Error %d, exiting program!\n", ret);
        str.sprintf("ERROR (HiCom): %d\n", ret);
        ui->textEdit->append(str);
        //goto exit;
    }
   #if 1
    /* Wait for initialization finished. */
    do {
        dev.delay_ms(50);
        ret = zmod44xx_read_status(&dev, &zmod44xx_status);
        if(ret) {
            printf("Error %d, exiting program!\n", ret);
            str.sprintf("ERROR (HiCom): %d\n", ret);
            ui->textEdit->append(str);
            //ui->textEdit->append("Initialization...!\n");
            //goto exit;
        }
    } while (FIRST_SEQ_STEP != (zmod44xx_status & 0x07));

    readTimer->start();
#endif
    printf("Evaluate measurements in a loop. Press any key to quit.\n");
    ui->textEdit->append("Evaluate measurements in a loop!\n");
    lblState->setText("开始检测");

}

void MainWindow::on_btnStop_clicked()
{
    readTimer->stop();

    if(dev_hicom_status == STATUS_OPEN)
    {
        /* ****** BEGIN TARGET SPECIFIC DEINITIALIZATION ****** */
        hicom_status = hicom_power_off(hicom_handle);
        if (FTC_SUCCESS != hicom_status) {
            print_hicom_error(hicom_status);
            //return hicom_status;
        }

        /* disconnect HiCom board */
        hicom_status = hicom_close(hicom_handle);
        if (FTC_SUCCESS != hicom_status) {
            print_hicom_error(hicom_status);
            //return hicom_status;
        }

        /* ****** END TARGET SPECIFIC DEINITIALIZATION ****** */

        printf("Bye!\n");

        dev_hicom_status = STATUS_CLOSE;
    }

    ui->textEdit->append("Stop measurement!\n");
    lblState->setText("停止检测");
}

void MainWindow::recv_thread_func()
{
    int error = 0;
    int n, rtn;
    char buffer[1024];
    RecvBuffer* recv_buf = NewBuffer();
    EdpPacket* pkg;
    uint8 mtype, jsonorbin;

    char* src_devid;
    char* push_data;
    uint32 push_datalen;

    struct UpdateInfoList* up_info = NULL;

    cJSON* save_json;
    char* save_json_str;

    cJSON* desc_json;
    char* desc_json_str;
    char* save_bin;
    uint32 save_binlen;
    unsigned short msg_id;
    unsigned char save_date_ret;

    char* cmdid;
    uint16 cmdid_len;
    char*  cmd_req;
    uint32 cmd_req_len;
    EdpPacket* send_pkg;
    char* ds_id;
    double dValue = 0;
    int iValue = 0;
    char* cValue = NULL;

    char* simple_str = NULL;
    char cmd_resp[] = "ok";
    unsigned cmd_resp_len = 0;

    DataTime stTime = {0};

    FloatDPS* float_data = NULL;
    int count = 0;
    int i = 0;

    printf("[%s] recv thread start ...\n", __func__);
    while (error == 0)
    {
        /* 试着接收1024个字节的数据 */
        n = sockfd->read(buffer, 1024);
        if (n <= 0)
            break;
        printf("recv from server, bytes: %d\n", n);
        /* wululu test print send bytes */
        hexdump((const unsigned char *)buffer, n);
        /* 成功接收了n个字节的数据 */
        WriteBytes(recv_buf, buffer, n);
        while(1)
        {
            /* 获取一个完成的EDP包 */
            if ((pkg = GetEdpPacket(recv_buf)) == 0)
            {
                printf("need more bytes...\n");
                break;
            }
            /* 获取这个EDP包的消息类型 */
            mtype = EdpPacketType(pkg);
    #ifdef _ENCRYPT
            if (mtype != ENCRYPTRESP){
                if (g_is_encrypt){
                    SymmDecrypt(pkg);
                }
            }
    #endif
            /* 根据这个EDP包的消息类型, 分别做EDP包解析 */
            switch(mtype)
            {
    #ifdef _ENCRYPT
            case ENCRYPTRESP:
                UnpackEncryptResp(pkg);
                break;
    #endif
            case CONNRESP:
                /* 解析EDP包 - 连接响应 */
                rtn = UnpackConnectResp(pkg);
                printf("recv connect resp, rtn: %d\n", rtn);
                break;
            case PUSHDATA:
                /* 解析EDP包 - 数据转发 */
                UnpackPushdata(pkg, &src_devid, &push_data, &push_datalen);
                printf("recv push data, src_devid: %s, push_data: %s, len: %d\n",
                       src_devid, push_data, push_datalen);
                free(src_devid);
                free(push_data);
                break;
            case UPDATERESP:
                UnpackUpdateResp(pkg, &up_info);
                while (up_info){
                    printf("name = %s\n", up_info->name);
                    printf("version = %s\n", up_info->version);
                    printf("url = %s\nmd5 = ", up_info->url);
                    for (i=0; i<32; ++i){
                        printf("%c", (char)up_info->md5[i]);
                    }
                    printf("\n");
                    up_info = up_info->next;
                }
                FreeUpdateInfolist(up_info);
                break;

            case SAVEDATA:
                /* 解析EDP包 - 数据存储 */
                if (UnpackSavedata(pkg, &src_devid, &jsonorbin) == 0)
                {
                    if (jsonorbin == kTypeFullJson
                        || jsonorbin == kTypeSimpleJsonWithoutTime
                        || jsonorbin == kTypeSimpleJsonWithTime)
                    {
                        printf("json type is %d\n", jsonorbin);
                        /* 解析EDP包 - json数据存储 */
                        /* UnpackSavedataJson(pkg, &save_json); */
                        /* save_json_str=cJSON_Print(save_json); */
                        /* printf("recv save data json, src_devid: %s, json: %s\n", */
                        /*     src_devid, save_json_str); */
                        /* free(save_json_str); */
                        /* cJSON_Delete(save_json); */

                        /* UnpackSavedataInt(jsonorbin, pkg, &ds_id, &iValue); */
                        /* printf("ds_id = %s\nvalue= %d\n", ds_id, iValue); */

                        UnpackSavedataDouble((SaveDataType)jsonorbin, pkg, &ds_id, &dValue);
                        printf("ds_id = %s\nvalue = %f\n", ds_id, dValue);

                        /* UnpackSavedataString(jsonorbin, pkg, &ds_id, &cValue); */
                        /* printf("ds_id = %s\nvalue = %s\n", ds_id, cValue); */
                        /* free(cValue); */

                        free(ds_id);

                    }
                    else if (jsonorbin == kTypeBin)
                    {/* 解析EDP包 - bin数据存储 */
                        UnpackSavedataBin(pkg, &desc_json, (uint8**)&save_bin, &save_binlen);
                        desc_json_str=cJSON_Print(desc_json);
                        printf("recv save data bin, src_devid: %s, desc json: %s, bin: %s, binlen: %d\n",
                               src_devid, desc_json_str, save_bin, save_binlen);
                        free(desc_json_str);
                        cJSON_Delete(desc_json);
                        free(save_bin);
                    }
                    else if (jsonorbin == kTypeString ){
                        UnpackSavedataSimpleString(pkg, &simple_str);

                        printf("%s\n", simple_str);
                        free(simple_str);
                    }else if (jsonorbin == kTypeStringWithTime){
                        UnpackSavedataSimpleStringWithTime(pkg, &simple_str, &stTime);

                        printf("time:%u-%02d-%02d %02d-%02d-%02d\nstr val:%s\n",
                            stTime.year, stTime.month, stTime.day, stTime.hour, stTime.minute, stTime.second, simple_str);
                        free(simple_str);
                    }else if (jsonorbin == kTypeFloatWithTime){
                        if(UnpackSavedataFloatWithTime(pkg, &float_data, &count, &stTime)){
                            printf("UnpackSavedataFloatWithTime failed!\n");
                        }

                        printf("read time:%u-%02d-%02d %02d-%02d-%02d\n",
                            stTime.year, stTime.month, stTime.day, stTime.hour, stTime.minute, stTime.second);
                        printf("read float data count:%d, ptr:[%p]\n", count, float_data);

                        for(i = 0; i < count; ++i){
                            printf("ds_id=%u,value=%f\n", float_data[i].ds_id, float_data[i].f_data);
                        }

                        free(float_data);
                        float_data = NULL;
                    }
                    free(src_devid);
                }else{
                    printf("error\n");
                }
                break;
            case SAVEACK:
                UnpackSavedataAck(pkg, &msg_id, &save_date_ret);
                printf("save ack, msg_id = %d, ret = %d\n", msg_id, save_date_ret);
                break;
            case CMDREQ:
                if (UnpackCmdReq(pkg, &cmdid, &cmdid_len,
                                 &cmd_req, &cmd_req_len) == 0){
                    /*
                     * 用户按照自己的需求处理并返回，响应消息体可以为空，此处假设返回2个字符"ok"。
                     * 处理完后需要释放
                     */
                    cmd_resp_len = strlen(cmd_resp);
                    send_pkg = PacketCmdResp(cmdid, cmdid_len,
                                             cmd_resp, cmd_resp_len);
    #ifdef _ENCRYPT
                    if (g_is_encrypt){
                        SymmEncrypt(send_pkg);
                    }
    #endif
                    DoSend((const char*)send_pkg->_data, send_pkg->_write_pos);
                    DeleteBuffer(&send_pkg);

                    free(cmdid);
                    free(cmd_req);
                }
                break;
            case PINGRESP:
                /* 解析EDP包 - 心跳响应 */
                UnpackPingResp(pkg);
                printf("recv ping resp\n");
                qDebug("recv ping resp\n");
                break;

            default:
                /* 未知消息类型 */
                error = 1;
                printf("recv failed...\n");
                break;
            }
            DeleteBuffer(&pkg);
        }
    }

    DeleteBuffer(&recv_buf);
    printf("[%s] recv thread end ...\n", __func__);

}

/*
 * 函数名:  DoSend
 * 功能:    将buffer中的len字节内容写入(发送)socket描述符sockfd, 成功时返回写的(发送的)字节数.
 * 参数:    sockfd  socket描述符
 *          buffer  需发送的字节
 *          len     需发送的长度
 * 说明:    这里只是给出了一个发送数据的例子, 其他方式请查询相关socket api
 *          一般来说, 发送都需要循环发送, 是因为需要发送的字节数 > socket的写缓存区时, 一次send是发送不完的.
 * 相关socket api:
 *          send
 * 返回值:  类型 (int32)
 *          <=0     发送失败
 *          >0      成功发送的字节数
 */
int32 MainWindow::DoSend(const char* buffer, uint32 len)
{
    int32 total  = 0;
    int32 n = 0;
    while (len != total)
    {
        /* 试着发送len - total个字节的数据 */
        //n = Send(sockfd,buffer + total,len - total,MSG_NOSIGNAL);
        n = sockfd->write(buffer + total,len - total);
        if (n <= 0)
        {
            fprintf(stderr, "ERROR writing to socket\n");
            return n;
        }
        /* 成功发送了n个字节的数据 */
        total += n;
    }
    /* wululu test print send bytes */
    hexdump((const unsigned char *)buffer, len);
    return total;
}

/*
 * buffer按十六进制输出
 */
void MainWindow::hexdump(const unsigned char *buf, uint32 num)
{
    uint32 i = 0;
    for (; i < num; i++)
    {
        printf("%02X ", buf[i]);
        if ((i+1)%8 == 0)
            printf("\n");
    }
    printf("\n");
}
