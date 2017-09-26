TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    ../dir_list.c \
    ../dir_trans.c \
    ../file_trans.c \
    ../ftp_client.c

OTHER_FILES += \
    ../makefile

HEADERS += \
    ../dir_list.h \
    ../dir_trans.h \
    ../file_trans.h

