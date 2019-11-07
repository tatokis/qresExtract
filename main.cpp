/*
    Copyright (C) 2019  Tasos Sahanidis <code@tasossah.com>
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <QCoreApplication>
#include <QResource>
#include <QDir>
#include <QDebug>
#include <QTextStream>

#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
#define qInfo       qDebug
#endif

QTextStream cout(stdout);

void copyFiles(QString dest, QTextStream &qrc, const QString &source = ":/mnt")
{
    if(!dest.endsWith('/'))
        dest.append('/');

    QDir dir(source);
    QFileInfoList list = dir.entryInfoList();
    int listcount = list.count();

    for(int i = 0; i < listcount; i++)
    {
        QFileInfo file = list.at(i);

        // Strip out ":/mnt"
        cout << file.absoluteFilePath().midRef(5) << endl;
        QString destFileName = dest + file.fileName();

        if(file.isDir())
        {
            {
                QDir targetDir(dest);
                targetDir.mkpath(file.fileName());
            }

            copyFiles(dest + file.fileName(), qrc, file.absoluteFilePath());
        }
        else
        {
            qrc << "<file>" << file.absoluteFilePath().midRef(6) << "</file>" << endl;

            if(!QFile::copy(file.absoluteFilePath(), destFileName))
            {
                qInfo() << "Could not write" << destFileName;
                continue;
            }

            // We need to set the permissions (644) as they are read only by default
            if(!QFile::setPermissions(destFileName, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther))
                qInfo() << "Couldn't set permissions for" << destFileName;
        }
    }
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        qInfo() << "Usage: ./qresExtract <resource.bin> [output path]";
        return 1;
    }

    QString fileStr = QString::fromLocal8Bit(argv[1]);
    QString fileName;
    {
        QFile file(fileStr);
        if(!file.exists())
        {
            qInfo() << "No such file exists.";
            return 1;
        }
        fileName = QFileInfo(file).baseName();

        if(!QResource::registerResource(fileStr, "/mnt"))
        {
            qInfo() << "Could not load" << fileStr;
            qInfo() << "This may be because your version of Qt doesn't support this qres version";
            if(file.open(QIODevice::ReadOnly))
            {
                file.seek(0x7);
                uchar ver = file.read(1)[0];
                qInfo() << "qres version of this file is" << ver;
                if(ver == 3)
                    qInfo() << "Version 3 requires at least Qt 5.13. You are using Qt" << QT_VERSION_STR;
            }
            return 1;
        }
    }

    QString outPath(".");
    if(argc > 2)
        outPath = QString::fromLocal8Bit(argv[2]);

    if(!outPath.endsWith('/'))
        outPath.append('/');

    outPath.append(fileName);

    // Needed so that we can open the qrc
    {
        QDir().mkpath(outPath);
    }

    QFile qrcFile(outPath + "/" + fileName + ".qrc");
    if(!qrcFile.open(QIODevice::WriteOnly))
    {
        qInfo() << "Unable to open" << qrcFile.fileName();
        return 1;
    }

    QTextStream qrc(&qrcFile);
    qrc << "<!DOCTYPE RCC><RCC version=\"1.0\">" << endl;
    qrc << "<qresource>" << endl;

    copyFiles(outPath, qrc);

    qrc << "</qresource>" << endl;
    qrc << "</RCC>" << endl;
    qrcFile.close();

    return 0;
}

