#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //软件过期提醒
    void Overdue();

    //获取信息
    QString GetDebPath();
    QString GetOutPutPath();
    QString GetIconPath();

    //转换包相关操作
    void ExtractDeb(QString path);
    void ReadDeb();
    void ReadControlFile();
    //解析appname、pkgname、arch信息
    void ResolveApp();
    void ReadDesktopFile();
    void ReadInfoFile();

    //初始化目录结构
    void InitNewDir();
    //打包
    void BuildDebPkg();


    //工具类方法
    QString CallCMD(QString commands);
    QString SearchFile(QString filename);
    QByteArray ReadFile(QString filepath);
    void Write2File(QString filepath_, QString text_);
    //重新调整desktop文件的Exec和Icon的路径
    void AdjustDesktop();
    //重新调整desktop文件的Icon的路径
    void AdjustIcon();
    void AdjustExec();



private:
    Ui::MainWindow *ui;

    QString m_pkgname;
    QString m_appname;
    QString m_version;
    QString m_arch;
    QString m_debpkg_arch;

    const QString src_path = ".debsrcfile";
    //调整后icon的路径
    QString m_icon_path;
    //原始icon路径
    QString m_src_icon_path;
    //自己上传的icon路径
    QString m_new_icon_path;
    QString m_run_app_cmd = "";
    QString m_sand_str = "";

};

#endif // MAINWINDOW_H
