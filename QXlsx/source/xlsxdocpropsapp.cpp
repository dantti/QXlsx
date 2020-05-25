/****************************************************************************
** Copyright (c) 2013-2014 Debao Zhang <hello@debao.me>
** All right reserved.
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
****************************************************************************/
#include "xlsxdocpropsapp_p.h"

#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QVariant>
#include <QBuffer>

QT_BEGIN_NAMESPACE_XLSX

DocPropsApp::DocPropsApp(CreateFlag flag)
    :AbstractOOXmlFile(flag)
{
}

void DocPropsApp::addPartTitle(const QString &title)
{
    m_titlesOfPartsList.append(title);
}

void DocPropsApp::addHeadingPair(const QString &name, int value)
{
    m_headingPairsList.append({ name, value });
}

bool DocPropsApp::setProperty(const QString &name, const QString &value)
{
    static const QStringList validKeys = {
        QStringLiteral("manager"), QStringLiteral("company")
    };

    if (!validKeys.contains(name))
        return false;

    if (value.isEmpty())
        m_properties.remove(name);
    else
        m_properties[name] = value;

    return true;
}

QString DocPropsApp::property(const QString &name) const
{
    auto it = m_properties.constFind(name);
    if (it != m_properties.constEnd())
        return it.value();

    return QString();
}

QStringList DocPropsApp::propertyNames() const
{
    return m_properties.keys();
}

void DocPropsApp::saveToXmlFile(QIODevice *device) const
{
    QXmlStreamWriter writer(device);
    QString vt = QStringLiteral("http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes");

    writer.writeStartDocument(QStringLiteral("1.0"), true);
    writer.writeStartElement(QStringLiteral("Properties"));
    writer.writeDefaultNamespace(QStringLiteral("http://schemas.openxmlformats.org/officeDocument/2006/extended-properties"));
    writer.writeNamespace(vt, QStringLiteral("vt"));
    writer.writeTextElement(QStringLiteral("Application"), QStringLiteral("Microsoft Excel"));
    writer.writeTextElement(QStringLiteral("DocSecurity"), QStringLiteral("0"));
    writer.writeTextElement(QStringLiteral("ScaleCrop"), QStringLiteral("false"));

    writer.writeStartElement(QStringLiteral("HeadingPairs"));
    writer.writeStartElement(vt, QStringLiteral("vector"));
    writer.writeAttribute(QStringLiteral("size"), QString::number(m_headingPairsList.size()*2));
    writer.writeAttribute(QStringLiteral("baseType"), QStringLiteral("variant"));

    for (const auto &pair : m_headingPairsList) {
        writer.writeStartElement(vt, QStringLiteral("variant"));
        writer.writeTextElement(vt, QStringLiteral("lpstr"), pair.first);
        writer.writeEndElement(); //vt:variant
        writer.writeStartElement(vt, QStringLiteral("variant"));
        writer.writeTextElement(vt, QStringLiteral("i4"), QString::number(pair.second));
        writer.writeEndElement(); //vt:variant
    }
    writer.writeEndElement();//vt:vector
    writer.writeEndElement();//HeadingPairs

    writer.writeStartElement(QStringLiteral("TitlesOfParts"));
    writer.writeStartElement(vt, QStringLiteral("vector"));
    writer.writeAttribute(QStringLiteral("size"), QString::number(m_titlesOfPartsList.size()));
    writer.writeAttribute(QStringLiteral("baseType"), QStringLiteral("lpstr"));
    for (const QString &title : m_titlesOfPartsList)
        writer.writeTextElement(vt, QStringLiteral("lpstr"), title);
    writer.writeEndElement();//vt:vector
    writer.writeEndElement();//TitlesOfParts

    auto it = m_properties.constFind(QStringLiteral("manager"));
    if (it != m_properties.constEnd())
        writer.writeTextElement(QStringLiteral("Manager"), it.value());
    //Not like "manager", "company" always exists for Excel generated file.

    it = m_properties.constFind(QStringLiteral("company"));
    writer.writeTextElement(QStringLiteral("Company"), it != m_properties.constEnd() ? it.value() : QString());
    writer.writeTextElement(QStringLiteral("LinksUpToDate"), QStringLiteral("false"));
    writer.writeTextElement(QStringLiteral("SharedDoc"), QStringLiteral("false"));
    writer.writeTextElement(QStringLiteral("HyperlinksChanged"), QStringLiteral("false"));
    writer.writeTextElement(QStringLiteral("AppVersion"), QStringLiteral("12.0000"));

    writer.writeEndElement(); //Properties
    writer.writeEndDocument();
}

bool DocPropsApp::loadFromXmlFile(QIODevice *device)
{
    QXmlStreamReader reader(device);
    while (!reader.atEnd()) {
         QXmlStreamReader::TokenType token = reader.readNext();
         if (token == QXmlStreamReader::StartElement) {
             if (reader.name() == QLatin1String("Properties"))
                 continue;

             if (reader.name() == QStringLiteral("Manager")) {
                 setProperty(QStringLiteral("manager"), reader.readElementText());
             } else if (reader.name() == QStringLiteral("Company")) {
                 setProperty(QStringLiteral("company"), reader.readElementText());
             }
         }

         if (reader.hasError()) {
             qDebug("Error when read doc props app file.");
         }
    }
    return true;
}

QT_END_NAMESPACE_XLSX
