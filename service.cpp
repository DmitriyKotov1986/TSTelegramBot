#include <QFileInfo>
#include <QDir>
#include "service.h"
#include "ttelegrambot.h"

Service::Service(int argc, char **argv)
    : QtService<QApplication>(argc, argv, "TS Telegram Bot Service")

{
    setServiceDescription("Telegram Bot Service");
    setServiceFlags(QtServiceBase::CanBeSuspended);
}

Service::~Service()
{
}

void Service::start()
{
    try {
        QString ConfigFileName = application()->applicationDirPath() + "/TSTelegramBotService.ini";
        QDir::setCurrent(application()->applicationDirPath()); //меняем текущую директорию

        qDebug() << "Start service";
        qDebug() << "Reading configuration from :" + ConfigFileName;

        QFileInfo FileInfo(ConfigFileName);
        if (!FileInfo.exists()) {
            throw std::runtime_error("Configuration file not found.");
        }

        Config = new QSettings(ConfigFileName, QSettings::IniFormat);

        TelegramBot = new TTelegramBot(*Config);

        TelegramBot->Start();

    }  catch (const std::exception &e) {
        qCritical() << "Critical error on start service. Message:" << e.what();
        exit(-1);
    }
}

void Service::pause()
{
    try {
        TelegramBot->Stop();
    }  catch (const std::exception &e) {
        qCritical() << "Critical error on pause service. Message:" << e.what();
        exit(-1);
    }
}

void Service::resume()
{
    try {
         TelegramBot->Start();
    }  catch (const std::exception &e) {
        qCritical() << "Critical error on resume service. Message:" << e.what();
        exit(-1);
    }
}

void Service::stop()
{
    try {
        TelegramBot->Stop();
        TelegramBot->deleteLater();
    }  catch (const std::exception &e) {
        qCritical() << "Critical error on stop service. Message:" << e.what();
        exit(-1);
    }
}



