QT -= gui
QT += widgets
QT += sql
QT += network

CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        QtService/qtservice.cpp \
        QtService/qtservice_win.cpp \
        QtTelegramBot/networking.cpp \
        QtTelegramBot/qttelegrambot.cpp \
        QtTelegramBot/types/audio.cpp \
        QtTelegramBot/types/callbackquery.cpp \
        QtTelegramBot/types/chat.cpp \
        QtTelegramBot/types/contact.cpp \
        QtTelegramBot/types/document.cpp \
        QtTelegramBot/types/location.cpp \
        QtTelegramBot/types/message.cpp \
        QtTelegramBot/types/photosize.cpp \
        QtTelegramBot/types/sticker.cpp \
        QtTelegramBot/types/update.cpp \
        QtTelegramBot/types/user.cpp \
        QtTelegramBot/types/video.cpp \
        QtTelegramBot/types/voice.cpp \
        main.cpp \
        service.cpp \
        ttelegrambot.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    QtService/QtService \
    QtService/QtServiceBase \
    QtService/QtServiceController \
    QtService/qtservice.h \
    QtService/qtservice_p.h \
    QtTelegramBot/networking.h \
    QtTelegramBot/qttelegrambot.h \
    QtTelegramBot/types/audio.h \
    QtTelegramBot/types/callbackquery.h \
    QtTelegramBot/types/chat.h \
    QtTelegramBot/types/contact.h \
    QtTelegramBot/types/document.h \
    QtTelegramBot/types/file.h \
    QtTelegramBot/types/location.h \
    QtTelegramBot/types/message.h \
    QtTelegramBot/types/photosize.h \
    QtTelegramBot/types/reply/forcereply.h \
    QtTelegramBot/types/reply/genericreply.h \
    QtTelegramBot/types/reply/inlinekeyboardmarkup.h \
    QtTelegramBot/types/reply/replykeyboardhide.h \
    QtTelegramBot/types/reply/replykeyboardmarkup.h \
    QtTelegramBot/types/reply/replykeyboardremove.h \
    QtTelegramBot/types/sticker.h \
    QtTelegramBot/types/update.h \
    QtTelegramBot/types/user.h \
    QtTelegramBot/types/video.h \
    QtTelegramBot/types/voice.h \
    service.h \
    ttelegrambot.h

DISTFILES +=
