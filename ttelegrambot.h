#ifndef TTELEGRAMBOT_H
#define TTELEGRAMBOT_H

#include <QObject>
#include <QSettings>
#include <QSqlDatabase>
#include <QTimer>
#include <QDir>
#include <QStringList>
#include <QRandomGenerator>
#include <QMap>
#include "QtTelegramBot/qttelegrambot.h"
#include "QtTelegramBot/types/message.h"

using namespace Telegram;

class TTelegramBot : public QObject
{
    Q_OBJECT
public:
    typedef enum {CODE_OK, CODE_ERROR, CODE_INFORMATION} MSG_CODE;
private:
    typedef enum {CHAT_OK, CHAT_REQUEST_COUPON_STATUS, CHAT_REQUEST_COUPON_STATUS_ON_AZS} CHAT_MODE;
    typedef struct {
        CHAT_MODE ChatMode;
        QString Data;
    } TChatMode;

private:
     QSqlDatabase DB;
     QSqlDatabase LogDB;
     QSettings& Config;
     QString Token;
     QString TmpPath;
     QRandomGenerator *rg;
     QMap<QString, TChatMode> ChatMode;

     Telegram::Bot *bot;


public:
    explicit TTelegramBot(QSettings& Config, QObject *parent = nullptr);
    ~TTelegramBot();

    void Start();
    void Stop();

private:
   void SendLogMsg(uint16_t Category, const QString& Msg);
   void SendCouponsInfo(const QString& ChatID);
   void SendCouponStatus(const QString& ChatID, const QString& Code);
   void SendCouponStatusOnAZS(const QString& ChatID, const QString& AZSCode, const QString& Code);
   void SendCouponStatusOnAllAZS(const QString& ChatID, const QString& Code);
   void SendFilesInfo(const QString& ChatID);
   void SendFile(const QString& ChatID, const QString& FileID);
   void SendLevelGaugeInfo(const QString& ChatID);
   void SendStatusLevelGauge(const QString& ChatID, const QString& AZSCode);
   void SendConfigLevelGauge(const QString& ChatID, const QString& AZSCode);


   void SelectAZSButton(const QString& ChatID, const QString& AddonsStr);
   void SelectFilesButton(const QString& ChatID, const QString& AZSCode);

public slots:
    void onNewMessage(Message message);
    void onUpdate(Update update);
};

#endif // TTELEGRAMBOT_H
