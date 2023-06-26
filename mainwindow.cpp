#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include <QProcess>
#include <QMessageBox>
#include <QDirIterator>
#include <QTimer>



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->pbt_select_deb,&QPushButton::clicked,this,[=]{
        m_icon_path = "";
        ui->lineEdit_iconpath->clear();
        ui->lineEdit_deb_path->setText(GetDebPath());
    });
    connect(ui->pbt_output,&QPushButton::clicked,this,[=]{
        ui->lineEdit_output->setText(GetOutPutPath());
    });
    connect(ui->pbt_add_icon,&QPushButton::clicked,this,[=]{
        m_new_icon_path = GetIconPath();
        if(!m_new_icon_path.isEmpty()){
            QImage img_(m_new_icon_path);
            ui->label_icon_size->setText(QString("%1*%2px").arg(img_.size().width()).arg(img_.size().height()));
            if(img_.size().width() / img_.size().height() == 1
                    && img_.size().width() > 95
                    && img_.size().width() < 513){
                QFileInfo fi_(m_new_icon_path);
                m_icon_path = "/opt/apps/" + ui->lineEdit_pkgname->text() +"/entries/icons/" + fi_.fileName();
            }else {
                QMessageBox::information(this, "通知", QString("%1*%2px,图片大小不符").arg(img_.size().width()).arg(img_.size().height()), QMessageBox::Ok);
                return ;
            }

            ui->lineEdit_iconpath->setText(m_icon_path);
            AdjustIcon();
        }
    });

    //点击提取触发
    connect(ui->pbt_extra,&QPushButton::clicked,this,[=]{
        if(ui->lineEdit_deb_path->text().isEmpty()){
            return;
        }

        //解压deb包
        ExtractDeb(ui->lineEdit_deb_path->text());
        QMessageBox::information(this, "通知", "提取完成", QMessageBox::Ok);
        //读取control、desktop、info信息
        ReadDeb();

        //填写基本信息栏目的内容
        ui->lineEdit_appname->setText(m_appname);
        ui->lineEdit_pkgname->setText(m_pkgname);
        ui->lineEdit_version->setText(m_version);
        ui->lineEdit_arch->setText(m_arch);
        ui->lineEdit_iconpath->setText(m_icon_path);

        //生成deb包名

        ui->lineEdit_debname->setText(QString("%1_%2_%3.deb")
                                      .arg(m_pkgname)
                                      .arg(m_version)
                                      .arg(m_debpkg_arch));
    });

    //点击开始转化触发
    connect(ui->pbt_start,&QPushButton::clicked,this,[=]{
        if(ui->lineEdit_deb_path->text().isEmpty()){
            QMessageBox::warning(this, "通知", "请选择deb包", QMessageBox::Ok);

            return;
        }
        if(!ui->lineEdit_pkgname->text().contains(".")){
            QMessageBox::warning(this, "通知", "包名需为倒置域名格式", QMessageBox::Ok);

            return;
        }
        if(ui->lineEdit_output->text().isEmpty()){
            QMessageBox::warning(this, "通知", "请设置输出路径", QMessageBox::Ok);
            return;
        }
        if(ui->lineEdit_iconpath->text().isEmpty()){
            QMessageBox::warning(this, "通知", "请设置icon路径", QMessageBox::Ok);
            return;
        }

        BuildDebPkg();

    });

    //包名改变时触发
    connect(ui->lineEdit_pkgname,&QLineEdit::textChanged,this,[=]{
        qInfo() << "lineEdit_pkgname-textchange";
        m_pkgname = ui->lineEdit_pkgname->text();
        //同步修改输出deb名称
        if(!ui->lineEdit_pkgname->text().isEmpty() &&
                !ui->lineEdit_version->text().isEmpty()){
            ui->lineEdit_debname->setText(ui->lineEdit_pkgname->text() + "_" +
                                          ui->lineEdit_version->text() + "_" +
                                          m_debpkg_arch + ".deb");
        }

        //同步修改control文件
        QStringList controllist = ui->textEdit_control->toPlainText().split(QRegExp("[\n]"),QString::SkipEmptyParts);
        ui->textEdit_control->clear();
        for (int i = 0; i < controllist.size(); i++) {
            if (controllist[i].contains("Package")) {
                controllist[i] = QString("Package: %1").arg(ui->lineEdit_pkgname->text());
            }
            ui->textEdit_control->append(controllist[i]);
        }
        ui->textEdit_control->append("\n");

        //同步修改desktop文件
        QStringList desktoplist = ui->textEdit_desktop->toPlainText().split(QRegExp("[\n]"),QString::SkipEmptyParts);
        ui->textEdit_desktop->clear();
        for (int i = 0; i < desktoplist.size(); i++) {
            if (desktoplist[i].contains("Icon=")) {
                if (desktoplist[i].contains("/")) {
                    QFileInfo file(desktoplist[i].split("=")[1]);
                    desktoplist[i] = QString("Icon=/opt/apps/%1/entries/icons/%2").arg(ui->lineEdit_pkgname->text()).arg(file.fileName());
                }
            }

            if (desktoplist[i].contains("Exec=/")) {
                if (desktoplist[i].contains("/")) {
                    QFileInfo file(desktoplist[i].split("=")[1]);
                    desktoplist[i] = QString("Exec=/opt/apps/%1/files/%2").arg(ui->lineEdit_pkgname->text()).arg(desktoplist[i].split("/files/")[1]);
                }
            }
            ui->textEdit_desktop->append(desktoplist[i]);

        }

        //同步修改info文件
        QStringList infolist = ui->textEdit_info->toPlainText().split(QRegExp("[\n]"),QString::SkipEmptyParts);
        ui->textEdit_info->clear();
        for (int i = 0; i < infolist.size(); i++) {
            if (infolist[i].contains("appid")) {
                infolist[i] = QString(" \"appid\": \"%1\",").arg(ui->lineEdit_pkgname->text());
            }
            ui->textEdit_info->append(infolist[i]);
        }
    });

    //版本号改变时触发
    connect(ui->lineEdit_version,&QLineEdit::textChanged,this,[=]{
        qInfo() << "lineEdit_version-textchange";
        m_version = ui->lineEdit_version->text();

        //同步修改输出deb名称
        if(!ui->lineEdit_pkgname->text().isEmpty() &&
                !ui->lineEdit_version->text().isEmpty()){
            ui->lineEdit_debname->setText(ui->lineEdit_pkgname->text() + "_" +
                                          ui->lineEdit_version->text() + "_" +
                                          m_debpkg_arch + ".deb");
        }

        //同步修改control文件
        QStringList controllist = ui->textEdit_control->toPlainText().split(QRegExp("[\n]"),QString::SkipEmptyParts);
        ui->textEdit_control->clear();
        for (int i = 0; i < controllist.size(); i++) {
            if (controllist[i].contains("Version")) {
                controllist[i] = QString("Version: %1").arg(ui->lineEdit_version->text());
            }
            ui->textEdit_control->append(controllist[i]);
        }
        ui->textEdit_control->append("\n");

        //同步修改info文件
        QStringList infolist = ui->textEdit_info->toPlainText().split(QRegExp("[\n]"),QString::SkipEmptyParts);
        ui->textEdit_info->clear();
        for (int i = 0; i < infolist.size(); i++) {
            if (infolist[i].contains("version")) {
                infolist[i] = QString(" \"version\": \"%1\",").arg(ui->lineEdit_version->text());
            }
            ui->textEdit_info->append(infolist[i]);
        }
    });

    //应用名称改变时触发
    connect(ui->lineEdit_appname,&QLineEdit::textChanged,this,[=]{
        qInfo() << "lineEdit_appname-textchange";
        m_appname = ui->lineEdit_appname->text();

        //同步修改desktop文件
        QStringList desktoplist = ui->textEdit_desktop->toPlainText().split(QRegExp("[\n]"),QString::SkipEmptyParts);
        ui->textEdit_desktop->clear();
        for (int i = 0; i < desktoplist.size(); i++) {
            if (desktoplist[i].contains("Name=") && desktoplist[i].startsWith("Name")) {
                    desktoplist[i] = QString("Name=%1").arg(ui->lineEdit_appname->text());
            }
            ui->textEdit_desktop->append(desktoplist[i]);

        }

        //同步修改info文件
        QStringList infolist = ui->textEdit_info->toPlainText().split(QRegExp("[\n]"),QString::SkipEmptyParts);
        ui->textEdit_info->clear();
        for (int i = 0; i < infolist.size(); i++) {
            if (infolist[i].contains("name")) {
                infolist[i] = QString(" \"name\": \"%1\",").arg(ui->lineEdit_appname->text());
            }
            ui->textEdit_info->append(infolist[i]);
        }

    });

    connect(ui->pbt_run_app,&QPushButton::clicked,this,[=]{

        if(!ui->lineEdit_deb_path->text().isEmpty()){
            qInfo() << "run app";
            if(ui->checkBox_add_nosandbox->isChecked()
                    && !m_run_app_cmd.contains("--no-sandbox")){
                ui->textEdit_res->setText(CallCMD(m_run_app_cmd + " --no-sandbox"));
            }else {
                ui->textEdit_res->setText(CallCMD(m_run_app_cmd));
            }
        }
    });

    connect(ui->checkBox_add_nosandbox,&QCheckBox::stateChanged,this,[=]{
       AdjustExec();
    });




}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::Overdue()
{
    QMessageBox::warning(this, "通知", "已有新版本，请使用新版本", QMessageBox::Ok);
}

QString MainWindow::GetDebPath()
{

    return QFileDialog::getOpenFileName(this, tr("文件选取"),QDir::homePath(),tr("deb文件(*deb)"));

}

QString MainWindow::GetOutPutPath()
{
    return QFileDialog::getExistingDirectory(this,"选择一个目录",QDir::homePath(),QFileDialog::ShowDirsOnly);
}

QString MainWindow::GetIconPath()
{
    return QFileDialog::getOpenFileName(this, tr("文件选取"),QDir::homePath(),tr("图片文件(*.png *.jpg)"));
}

void MainWindow::ExtractDeb(QString path)
{
    qInfo() << "Extracting deb file...";
    const QString cmdstr_ = "rm -rf .debsrcfile && dpkg-deb -R " + path + " .debsrcfile";
    CallCMD(cmdstr_);
}

void MainWindow::ReadDeb()
{
    ReadControlFile();
    ReadDesktopFile();
    //通过control文件解析appname、pkgname、arch信息，通过desktop文件解析icon路径
    ResolveApp();
    AdjustDesktop();
    ReadInfoFile();
}

void MainWindow::ReadControlFile()
{
    QString control_path_ = SearchFile("control");
    if(control_path_.isEmpty()){
        return;
    }
    ui->textEdit_control->setText(ReadFile(control_path_));
}

void MainWindow::ResolveApp()
//TODO: 处理Exec和Icon没有绝对路径的情况
{
    if(!ui->textEdit_desktop->toPlainText().isEmpty()){
        QStringList desktoplist = ui->textEdit_desktop->toPlainText().split(QRegExp("[\n]"),QString::SkipEmptyParts);
        for (int i = 0; i < desktoplist.size(); i++) {
            if (desktoplist[i].contains("Icon=/")) {
                m_icon_path = desktoplist[i].split("=")[1];
                m_src_icon_path = desktoplist[i].split("=")[1];
            }

            if (desktoplist[i].contains("Name=") && desktoplist[i].startsWith("Name")) {
                m_appname = desktoplist[i].split("=")[1];
            }
        }
    }

    if(!ui->textEdit_control->toPlainText().isEmpty()){
        QStringList controllist = ui->textEdit_control->toPlainText().split(QRegExp("[\n]"),QString::SkipEmptyParts);
        for (int i = 0; i < controllist.size(); i++) {
            if (controllist[i].contains("Package")) {
                m_pkgname = controllist[i].split(": ")[1];
                if(m_appname.isEmpty()){
                    m_appname = m_pkgname;
                }
            }
            if(controllist[i].contains("Architecture")){
                m_arch = controllist[i].split(": ")[1];
                if(m_arch == "amd64"){
                    m_debpkg_arch = "X86";
                }else if (m_arch == "arm64") {
                    m_debpkg_arch = "ARM";
                }else {
                    m_debpkg_arch = "";
                }
            }
            if(controllist[i].contains("Version")){
                m_version = controllist[i].split(": ")[1];
            }
        }
    }

    if(!m_icon_path.isEmpty()){
        QFileInfo fi_(m_icon_path);
        QImage img_(QDir::currentPath() + "/" + src_path + m_icon_path);
        ui->label_icon_size->setText(QString("%1*%2px").arg(img_.size().width()).arg(img_.size().height()));
        if(img_.size().width() / img_.size().height() == 1
                && img_.size().width() > 95
                && img_.size().width() < 513){
            m_icon_path = "/opt/apps/" + m_pkgname +"/entries/icons/" + fi_.fileName();
        }else {
            QPalette red;
            red.setColor(QPalette::WindowText,Qt::red);
            ui->label_icon_size->setPalette(red); // 设置QLabel的颜色
            m_icon_path = "";
            ui->lineEdit_iconpath->clear();
        }

    }
}

void MainWindow::ReadDesktopFile()
{
    QString desktop_path_ = SearchFile("*.desktop");
    QString desktop_text_ = "";
    if(!desktop_path_.isEmpty()){
        desktop_text_ = ReadFile(desktop_path_);

    }else {
        desktop_text_.append("[Desktop Entry]\n");
        desktop_text_.append("Type=Application\n");
        desktop_text_.append(QString("Name=%1\n").arg(m_appname));
        desktop_text_.append("Exec=\n");
        desktop_text_.append("Icon=\n");
        desktop_text_.append("Terminal=false\n");
    }
    ui->textEdit_desktop->setText(desktop_text_);
}

void MainWindow::ReadInfoFile()
{
    QString info_path_ = SearchFile("info");
    QString info_text_ = "";
    if(!info_path_.isEmpty() && info_path_.contains("/opt/apps/")){
        info_text_ = ReadFile(info_path_);
    }else {
        info_text_.append("{\n");
        info_text_.append(QString(" \"appid\": \"%1\",\n").arg(m_pkgname));
        info_text_.append(QString(" \"name\": \"%1\",\n").arg(m_appname));
        info_text_.append(QString(" \"version\": \"%1\",\n").arg(m_version));
        info_text_.append(QString(" \"arch\": [\"%1\"],\n").arg(m_arch));
        info_text_.append(" \"permissions\": {\n");
        info_text_.append("  \"autostart\": false,\n");
        info_text_.append("  \"notification\": false,\n");
        info_text_.append("  \"trayicon\": false,\n");
        info_text_.append("  \"clipboard\": false,\n");
        info_text_.append("  \"account\": false,\n");
        info_text_.append("  \"bluetooth\": false,\n");
        info_text_.append("  \"camera\": false,\n");
        info_text_.append("  \"audio_record\": false,\n");
        info_text_.append("  \"installed_apps\": false\n");
        info_text_.append("  }\n");
        info_text_.append("}\n");
    }
    ui->textEdit_info->setText(info_text_);
}

void MainWindow::InitNewDir()
{
    //1. 新建目录
    QDir srcDir(src_path);
    srcDir.mkpath("/.opt/apps/" + m_pkgname);

    QDir destDir(src_path + "/.opt/apps/" + m_pkgname);
    destDir.mkpath("entries/icons");
    destDir.mkpath("entries/applications");
    destDir.mkpath("files");

    //2. 移动旧目录到新目录中

    //TODO: 调整图标和EXEC路径
    //移动图标
        QString cpiconStr = QString("cp %1/%2 %1/.opt/apps/%4/entries/icons/").arg(src_path).arg(m_src_icon_path).arg(m_pkgname);
        CallCMD(cpiconStr);

    //移动文件目录
    QString mvStr = QString("cd %1 && mv `ls |grep -v -e DEBIAN -e .opt` .opt/apps/%2/files/ && mv .opt opt").arg(src_path).arg(m_pkgname);
    qInfo() << mvStr;
    QString x = CallCMD(mvStr);
    qInfo() << x;
    QDir dir(src_path + "/DEBIAN");
    if(!dir.exists()){
        srcDir.mkpath("DEBIAN");
    }

}

void MainWindow::BuildDebPkg()
{
    //1. 若info目录不存在（不是UOS的包），则InitNewDir
    if(SearchFile("info").isEmpty()){
        qInfo() <<"无info文件";
        InitNewDir();
    }else {
        QMessageBox::information(this, "通知", "此包无需转换", QMessageBox::Ok);
        return;
    }
    //2. 写文件信息
        //control写到path_/DEBIAN
        Write2File(src_path+"/DEBIAN/control",ui->textEdit_control->toPlainText());
        //info写到path_ + "/opt/apps/" + m_map["pkg_name"]+"/info"
        Write2File(src_path + "/opt/apps/" + m_pkgname+"/info",ui->textEdit_info->toPlainText());

        //desktop写到path_ + "/opt/apps/" + m_map["pkg_name"]+"/entries/applications"
        QFileInfo fileInfo(SearchFile("*.desktop"));
        qInfo() << fileInfo.fileName();
        Write2File(src_path + "/opt/apps/" + m_pkgname+"/entries/applications/" + fileInfo.fileName(),ui->textEdit_desktop->toPlainText());

        //3. 上传新的Icon文件
        if(!m_new_icon_path.isEmpty()){
            QString cpiconStr = QString("cp %1 %2/%3").arg(m_new_icon_path).arg(QDir::currentPath()+"/"+src_path).arg(m_icon_path);
            CallCMD(cpiconStr);
        }
        //4. 打包dpkg-deb -b

        QString buildStr = QString("dpkg-deb -b %1 %2/%3").arg(src_path).arg(ui->lineEdit_output->text()).arg(ui->lineEdit_debname->text());
        CallCMD(buildStr);
        qInfo() << "over";
        QMessageBox::information(this, "通知", "转换完成", QMessageBox::Ok);


}

QString MainWindow::CallCMD(QString commands)
{
    QString res;
    qInfo() << "Running:" << commands;
    QProcess process;
    process.start("bash", QStringList() << "-c" << commands);
    process.waitForFinished();
    QString errmsg = process.readAllStandardError().data();
    if(!errmsg.isEmpty()){
        res = errmsg;
    }else {
        res = process.readAllStandardOutput();
    }
    qInfo() << res;
    qInfo() << "cmd run over!!";
    return res;
}

QString MainWindow::SearchFile(QString filename_)
{
    QString deb_srcfile_path = QDir::currentPath()+"/.debsrcfile";
    QString filepath = "";
    QDir dir;
    QStringList filters;
    filters << filename_ ;//过滤条件，可以添加多个选项，可以匹配文件后缀等。我这里只是找指定文件

    dir.setPath(deb_srcfile_path);
    dir.setNameFilters(filters);//添加过滤器

    //QDirIterator 此类可以很容易的返回指定目录的所有文件及文件夹，可以再递归遍历，也可以自动查找指定的文件
    QDirIterator iter(dir,QDirIterator::Subdirectories);

    while (iter.hasNext())
    {
        iter.next();
        QFileInfo info=iter.fileInfo();
        if (info.isFile())
        {
            filepath = info.absoluteFilePath();
            break;
        }
    }
    qInfo() << __FUNCTION__ << filename_ << ": " << filepath << __LINE__;
    return filepath;
}

QByteArray MainWindow::ReadFile(QString filepath)
{
    QFile file(filepath);
    QByteArray bytes;
    if(!file.exists()) //文件不存在
    {
        return "file not exist";
    }
    if(file.open(QFile::ReadOnly))
    {
        bytes = file.readAll();
    }

    file.close();
    return bytes;
}

void MainWindow::Write2File(QString filepath_, QString text_)
{
    QFile file(filepath_); // 文件路径
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qInfo() << "Failed to open file!";
        return;
    }

    QTextStream out(&file); // 新建一个输出流
    out << text_; // 写入文件
    out.flush(); // 刷新缓冲区
    file.close();
}

void MainWindow::AdjustDesktop()
{
    if(!ui->textEdit_desktop->toPlainText().isEmpty()){
        QStringList desktoplist = ui->textEdit_desktop->toPlainText().split(QRegExp("[\n]"),QString::SkipEmptyParts);
        ui->textEdit_desktop->clear();
        for (int i = 0; i < desktoplist.size(); i++) {
            if (desktoplist[i].contains("Exec=/")) {
                m_run_app_cmd = src_path + desktoplist[i].split("=")[1];
                desktoplist[i] = "Exec=/opt/apps/" + m_pkgname + "/files" + desktoplist[i].split("=")[1];
            }

            if (desktoplist[i].contains("Icon=/")) {
                QFileInfo fi(desktoplist[i].split("=")[1]);
//                desktoplist[i] = "Icon=/opt/apps/" + m_pkgname + "/entries/icons/" + fi.fileName();
            desktoplist[i] = "Icon=" + m_icon_path;
            }
            ui->textEdit_desktop->append(desktoplist[i]);
        }
    }
}

void MainWindow::AdjustIcon()
{
    if(!ui->textEdit_desktop->toPlainText().isEmpty()){
        QStringList desktoplist = ui->textEdit_desktop->toPlainText().split(QRegExp("[\n]"),QString::SkipEmptyParts);
        ui->textEdit_desktop->clear();
        for (int i = 0; i < desktoplist.size(); i++) {
            if (desktoplist[i].contains("Icon=")) {
            desktoplist[i] = "Icon=" + m_icon_path;
            }
            ui->textEdit_desktop->append(desktoplist[i]);
        }
    }
}

void MainWindow::AdjustExec()
{
    if(!ui->textEdit_desktop->toPlainText().isEmpty()){
        QStringList desktoplist = ui->textEdit_desktop->toPlainText().split(QRegExp("[\n]"),QString::SkipEmptyParts);
        ui->textEdit_desktop->clear();
        for (int i = 0; i < desktoplist.size(); i++) {
            QString op = "";
            if (desktoplist[i].contains("Exec=/")) {
                if(ui->checkBox_add_nosandbox->isChecked()
                        && !desktoplist[i].contains("--no-sandbox")){
                    desktoplist[i] += " --no-sandbox";
                }else {
                    desktoplist[i] = desktoplist[i].replace(" --no-sandbox","");
                }

            }
            ui->textEdit_desktop->append(desktoplist[i]);
        }
    }
}
