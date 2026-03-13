#include "logindialog.h"
#include "ui_logindialog.h"
#include "HttpMgr.h"
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include "tcpmgr.h"
LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    connect(ui->reg_btn,&QPushButton::clicked, this, &LoginDialog::switchRegister);
    ui->forget_label->SetState("normal","hover","","selected","selected_hover","");

    connect(ui->forget_label,&ClickedLabel::clicked,this,&LoginDialog::slot_forget_pwd);
    initHead();

    //连接登录回包信号
    connect(HttpMgr::GetInstance().get(),&HttpMgr::sig_login_mod_finish,this,
            &LoginDialog::slot_logion_mod_finish);
    initHttpHandlers();

    //连接tcp连接请求的信号和槽函数
    connect(this, &LoginDialog::sig_connect_tcp, TcpMgr::GetInstance().get(), &TcpMgr::slot_tcp_connect);
    //连接tcp管理者发出的连接成功信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_con_success, this, &LoginDialog::slot_tcp_con_finish);
    //连接tcp管理者发出的连接失败信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_login_failed, this, &LoginDialog::slot_login_failed);

}

LoginDialog::~LoginDialog()
{
    qDebug()<<"logindialog destructed";
    delete ui;
}

void LoginDialog::initHttpHandlers()
{
    //注册获取登录回包逻辑
    _handlers.insert(ReqId::ID_LOGIN_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            enableBtn(true);
            return;
        }
        auto email = jsonObj["email"].toString();

        //发送信号通知tcpMgr发送长连接
        ServerInfo si;
        si.Uid = jsonObj["uid"].toInt();
        si.Host = jsonObj["host"].toString();
        si.Port = jsonObj["port"].toString();
        si.Token = jsonObj["token"].toString();

        _uid = si.Uid;
        _token = si.Token;
        qDebug()<<"email is "<<email<<" uid is "<<si.Uid<<" host is "
                 <<si.Host<<" Port is "<<si.Port<<" Token is "<<si.Token;
        emit sig_connect_tcp(si);

        auto user = jsonObj["user"].toString();
        showTip(tr("登录成功"), true);
        qDebug()<< "user is " << user ;
    });
}

void LoginDialog::initHead()
{
    //加载图片
    QPixmap originalPixmap(":/res/head_1.jpg");
    //设置图片自动缩放
    qDebug()<<originalPixmap.size()<<ui->head_label->size();
    originalPixmap = originalPixmap.scaled(ui->head_label->size(),
                                           Qt::KeepAspectRatio,Qt::SmoothTransformation);
    //创建一个和原始图片相同大小的QPixmap,用于绘制圆角图片
    QPixmap roundedPixmap(originalPixmap.size());
    roundedPixmap.fill(Qt::transparent);


    QPainter painter(&roundedPixmap);
    painter.setRenderHint(QPainter::Antialiasing);// 设置抗锯齿，使圆角更圆滑
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    //使用QPainterPath设置圆角
    QPainterPath path;
    path.addRoundedRect(0,0,originalPixmap.width(),originalPixmap.height(),10,10);
    painter.setClipPath(path);

    //将原始按图片绘制到roundedPixmap上
    painter.drawPixmap(0,0,originalPixmap);

    //设置绘制好的 圆角图片到QLabel上
    ui->head_label->setPixmap(roundedPixmap);
}

void LoginDialog::slot_forget_pwd()
{
    qDebug()<<"slot forget pwd";
    emit switchReset();
}

void LoginDialog::on_login_btn_clicked()
{
    qDebug()<<"login btn clicked";
    if(checkUserValid() == false){
        return;
    }
    if(checkPwdValid() == false){
        return ;
    }
    auto user = ui->email_edit->text();
    auto pwd = ui->pass_edit->text();
    //发送http请求登录
    QJsonObject json_obj;
    json_obj["user"] = user;
    json_obj["passwd"] = xorString(pwd);
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_login"),
                                        json_obj, ReqId::ID_LOGIN_USER,Modules::LOGINMOD);
}

void LoginDialog::slot_logion_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err!=ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"),false);
        return;
    }

    //解析JSON 字符串，res 转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if(jsonDoc.isNull()){
        showTip(tr("json解析失败"),false);
        return ;
    }
    //json 解析错误
    if(!jsonDoc.isObject()){
        showTip(tr("json解析失败"),false);
        return ;
    }

    //
    _handlers[id](jsonDoc.object());
    return ;
}




bool LoginDialog::checkUserValid(){
    auto user = ui->email_edit->text();
    if(user.isEmpty()){
        qDebug() << "User empty " ;
        return false;
    }
    return true;
}
bool LoginDialog::checkPwdValid(){
    auto pwd = ui->pass_edit->text();
    if(pwd.length() < 6 || pwd.length() > 15){
        qDebug() << "Pass length invalid";
        return false;
    }
    return true;
}



void LoginDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}
void LoginDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
        ui->err_tip->clear();
        return;
    }
    showTip(_tip_errs.first(), false);
}

void LoginDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
        ui->err_tip->setProperty("state","normal");
    }else{
        ui->err_tip->setProperty("state","err");
    }
    ui->err_tip->setText(str);
    repolish( ui->err_tip);
}

void LoginDialog::enableBtn(bool enabled){
    ui->login_btn->setEnabled(enabled);
    ui->reg_btn->setEnabled(enabled);
}

void LoginDialog::slot_tcp_con_finish(bool bsuccess)
{
    if(bsuccess){
        showTip(tr("聊天服务连接成功，正在登录..."),true);
        QJsonObject jsonObj;
        jsonObj["uid"] = _uid;
        jsonObj["token"] = _token;
        QJsonDocument doc(jsonObj);
        QString jsonString = doc.toJson(QJsonDocument::Indented);

        //发送tcp请求给chat server
        emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_CHAT_LOGIN, jsonString.toUtf8());
    }else{
        showTip(tr("网络异常"),false);
        enableBtn(true);
    }
}

void LoginDialog::slot_login_failed(int err)
{
    QString result = QString("登录失败, err is %1")
                         .arg(err);
    showTip(result,false);
    enableBtn(true);
}
