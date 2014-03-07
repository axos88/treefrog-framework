/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <THttpRequest>
#include <TMultipartFormData>
#include <THttpUtility>
#include "tsystemglobal.h"
#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#endif

#include <QDebug>

class MethodHash : public QHash<QString, Tf::HttpMethod>
{
public:
    MethodHash() : QHash<QString, Tf::HttpMethod>()
    {
        insert("get",     Tf::Get);
        insert("head",    Tf::Head);
        insert("post",    Tf::Post);
        insert("options", Tf::Options);
        insert("put",     Tf::Put);
        insert("delete",  Tf::Delete);
        insert("trace",   Tf::Trace);
        insert("patch",   Tf::Patch);
    }
};
Q_GLOBAL_STATIC(MethodHash, methodHash)


/*!
  \class THttpRequestData
  \brief The THttpRequestData class is for shared THttpRequest data objects.
*/

THttpRequestData::THttpRequestData(const THttpRequestData &other)
    : QSharedData(other),
      header(other.header),
      queryItems(other.queryItems),
      formItems(other.formItems),
      multipartFormData(other.multipartFormData),
#if QT_VERSION >= 0x050000
      jsonData(other.jsonData),
#endif
      clientAddress(other.clientAddress)
{ }

/*!
  \class THttpRequest
  \brief The THttpRequest class contains request information for HTTP.
*/

/*!
  \fn THttpRequest::THttpRequest()
  Constructor.
*/
THttpRequest::THttpRequest()
    : d(new THttpRequestData)
{ }

/*!
  \fn THttpRequest::THttpRequest(const THttpRequest &other)
  Copy constructor.
*/
THttpRequest::THttpRequest(const THttpRequest &other)
    : d(other.d)
{ }

/*!
  Constructor with the header \a header and the body \a body.
*/
THttpRequest::THttpRequest(const THttpRequestHeader &header, const QByteArray &body, const QHostAddress &clientAddress)
    : d(new THttpRequestData)
{
    d->header = header;
    d->clientAddress = clientAddress;
    parseBody(body, header);
}

/*!
  Constructor with the header \a header and a body generated by
  reading the file \a filePath.
*/
THttpRequest::THttpRequest(const QByteArray &header, const QString &filePath, const QHostAddress &clientAddress)
    : d(new THttpRequestData)
{
    d->header = THttpRequestHeader(header);
    d->clientAddress = clientAddress;
    d->multipartFormData = TMultipartFormData(filePath, boundary());
    d->formItems = d->multipartFormData.formItems();
}

/*!
  Destructor.
*/
THttpRequest::~THttpRequest()
{ }

/*!
  Assignment operator.
*/
THttpRequest &THttpRequest::operator=(const THttpRequest &other)
{
    d = other.d;
    return *this;
}

/*!
  Sets the request to \a header and \a body. This function is for internal use only.
*/
// void THttpRequest::setRequest(const THttpRequestHeader &header, const QByteArray &body)
// {
//     d->header = THttpRequestHeader(header);
//     d->queryItems.clear();
//     d->formItems.clear();
//     d->multipartFormData.clear();
// #if QT_VERSION >= 0x050000
//     d->jsonData = QJsonDocument();
// #endif
//     parseBody(body, header);
// }

/*!
  Sets the request to \a header and \a body. This function is for internal use only.
*/
// void THttpRequest::setRequest(const QByteArray &header, const QByteArray &body)
// {
//     d->header = THttpRequestHeader(header);
//     d->queryItems.clear();
//     d->formItems.clear();
//     d->multipartFormData.clear();
// #if QT_VERSION >= 0x050000
//     d->jsonData = QJsonDocument();
// #endif
//     parseBody(body, header);
// }

/*!
  Sets the request to \a header and \a filePath. This function is for internal use only.
*/
// void THttpRequest::setRequest(const QByteArray &header, const QString &filePath)
// {
//     d->header = THttpRequestHeader(header);
//     d->queryItems.clear();
//     d->formItems.clear();
//     d->multipartFormData = TMultipartFormData(filePath, boundary());
//     d->formItems.unite(d->multipartFormData.formItems());
// #if QT_VERSION >= 0x050000
//     d->jsonData = QJsonDocument();
// #endif
// }

/*!
  Returns the method.
 */
Tf::HttpMethod THttpRequest::method() const
{
    Tf::HttpMethod ret = Tf::Invalid;

    QString s = d->header.method().toLower();
    if (!methodHash()->contains(s)) {
        return Tf::Invalid;
    }

    ret = methodHash()->value(s);

    //If this is a Post and we have a query parameter named 'method', override.
    if ((ret == Tf::Post) && (hasQueryItem("_method")))
    {
      QString queryMethod = queryItemValue("_method");
      if (methodHash()->contains(queryMethod))
        ret = methodHash()->value(queryMethod);
    }

    return ret;
}

/*!
  Returns the string value whose name is equal to \a name from the URL or the
  form data.
 */
QString THttpRequest::parameter(const QString &name) const
{
    return allParameters()[name].toString();
}

/*!
  \fn bool THttpRequest::hasQuery() const

  Returns true if the URL contains a Query.
 */

/*!
  Returns true if there is a query string pair whose name is equal to \a name
  from the URL.
 */
bool THttpRequest::hasQueryItem(const QString &name) const
{
    return d->queryItems.contains(name);
}

/*!
  Returns the query string value whose name is equal to \a name from the URL.
 */
QString THttpRequest::queryItemValue(const QString &name) const
{
    return d->queryItems.value(name).toString();
}

/*!
  This is an overloaded function.
  Returns the query string value whose name is equal to \a name from the URL.
  If the query string contains no item with the given \a name, the function
  returns \a defaultValue.
 */
QString THttpRequest::queryItemValue(const QString &name, const QString &defaultValue) const
{
    return d->queryItems.value(name, QVariant(defaultValue)).toString();
}

/*!
  Returns the list of query string values whose name is equal to \a name from
  the URL.
 */
QStringList THttpRequest::allQueryItemValues(const QString &name) const
{
    QStringList ret;
    QVariantList values = d->queryItems.values(name);
    for (QListIterator<QVariant> it(values); it.hasNext(); ) {
        ret << it.next().toString();
    }
    return ret;
}

/*!
  \fn QVariantMap THttpRequest::queryItems() const
  Returns the query string of the URL, as a map of keys and values.
 */


/*!
  \fn bool THttpRequest::hasForm() const

  Returns true if the request contains form data.
 */


/*!
  Returns true if there is a string pair whose name is equal to \a name from
  the form data.
 */
bool THttpRequest::hasFormItem(const QString &name) const
{
    return d->formItems.contains(name);
}

/*!
  Returns the string value whose name is equal to \a name from the form data.
 */
QString THttpRequest::formItemValue(const QString &name) const
{
    return d->formItems.value(name).toString();
}

/*!
  This is an overloaded function.
  Returns the string value whose name is equal to \a name from the form data.
  If the form data contains no item with the given \a name, the function
  returns \a defaultValue.
 */
QString THttpRequest::formItemValue(const QString &name, const QString &defaultValue) const
{
    return d->formItems.value(name, QVariant(defaultValue)).toString();
}

/*!
  Returns the list of string value whose name is equal to \a name from the
  form data.
 */
QStringList THttpRequest::allFormItemValues(const QString &name) const
{
    QStringList ret;
    QVariantList values = d->formItems.values(name);
    for (QListIterator<QVariant> it(values); it.hasNext(); ) {
        ret << it.next().toString();
    }
    return ret;
}

/*!
  Returns the list of string value whose key is equal to \a key, such as
  "foo[]", from the form data.
 */
QStringList THttpRequest::formItemList(const QString &key) const
{
    QString k = key;
    if (!k.endsWith("[]")) {
        k += QLatin1String("[]");
    }
    return allFormItemValues(k);
}

/*!
  Returns the map of variant value whose key is equal to \a key from
  the form data.
 */
QVariantMap THttpRequest::formItems(const QString &key) const
{
    QVariantMap map;
    QRegExp rx(key + "\\[([^\\[\\]]+)\\]");
    for (QMapIterator<QString, QVariant> i(d->formItems); i.hasNext(); ) {
        i.next();
        if (rx.exactMatch(i.key())) {
            map.insert(rx.cap(1), i.value());
        }
    }
    return map;
}

void THttpRequest::parseBody(const QByteArray &body, const THttpRequestHeader &header)
{
    switch (method()) {
    case Tf::Post: {
        QString ctype = QString::fromLatin1(header.contentType().trimmed());
        if (ctype.startsWith("multipart/form-data", Qt::CaseInsensitive)) {
            // multipart/form-data
            d->multipartFormData = TMultipartFormData(body, boundary());
            d->formItems = d->multipartFormData.formItems();

        } else if (ctype.startsWith("application/json", Qt::CaseInsensitive)) {
#if QT_VERSION >= 0x050000
            d->jsonData = QJsonDocument::fromJson(body);
#else
            tSystemWarn("unsupported content-type: %s", qPrintable(ctype));
#endif
        } else {
            // 'application/x-www-form-urlencoded'
            if (!body.isEmpty()) {
                QList<QByteArray> formdata = body.split('&');
                for (QListIterator<QByteArray> i(formdata); i.hasNext(); ) {
                    QList<QByteArray> nameval = i.next().split('=');
                    if (!nameval.value(0).isEmpty()) {
                        // URL decode
                        QString key = THttpUtility::fromUrlEncoding(nameval.value(0));
                        QString val = THttpUtility::fromUrlEncoding(nameval.value(1));
                        d->formItems.insertMulti(key, val);
                        tSystemDebug("POST Hash << %s : %s", qPrintable(key), qPrintable(val));
                    }
                }
            }
        }
        /* FALL THROUGH */ }

    case Tf::Get: {
        // query parameter
        QList<QByteArray> data = d->header.path().split('?');
        QString getdata = data.value(1);
        if (!getdata.isEmpty()) {
            QStringList pairs = getdata.split('&', QString::SkipEmptyParts);
            for (QStringListIterator i(pairs); i.hasNext(); ) {
                QStringList s = i.next().split('=');
                if (!s.value(0).isEmpty()) {
                    QString key = THttpUtility::fromUrlEncoding(s.value(0).toLatin1());
                    QString val = THttpUtility::fromUrlEncoding(s.value(1).toLatin1());
                    d->queryItems.insertMulti(key, val);
                    tSystemDebug("GET Hash << %s : %s", qPrintable(key), qPrintable(val));
                }
            }
        }
        break; }

    default:
        // do nothing
        break;
    }
}

/*!
  Returns the boundary of multipart/form-data.
*/
QByteArray THttpRequest::boundary() const
{
    QByteArray boundary;
    QString contentType = d->header.rawHeader("content-type").trimmed();

    if (contentType.startsWith("multipart/form-data", Qt::CaseInsensitive)) {
        QStringList lst = contentType.split(QChar(';'), QString::SkipEmptyParts, Qt::CaseSensitive);
        for (QStringListIterator it(lst); it.hasNext(); ) {
            QString string = it.next().trimmed();
            if (string.startsWith("boundary=", Qt::CaseInsensitive)) {
                boundary  = "--";
                boundary += string.mid(9).toLatin1();
                break;
            }
        }
    }
    return boundary;
}

/*!
  Returns the cookie associated with the name.
 */
QByteArray THttpRequest::cookie(const QString &name) const
{
    QList<TCookie> list = cookies();
    for (QListIterator<TCookie> i(list); i.hasNext(); ) {
        const TCookie &c = i.next();
        if (c.name() == name) {
            return c.value();
        }
    }
    return QByteArray();
}

/*!
  Returns the all cookies.
 */
QList<TCookie> THttpRequest::cookies() const
{
    QList<TCookie> result;
    QList<QByteArray> cookieStrings = d->header.rawHeader("Cookie").split(';');
    for (QListIterator<QByteArray> i(cookieStrings); i.hasNext(); ) {
        QByteArray ba = i.next().trimmed();
        if (!ba.isEmpty())
            result += TCookie::parseCookies(ba);
    }
    return result;
}

/*!
  Returns a map of all form data.
*/
QVariantMap THttpRequest::allParameters() const
{
    QVariantMap params = d->queryItems;
    return params.unite(d->formItems);
}


QList<THttpRequest> THttpRequest::generate(const QByteArray &byteArray, const QHostAddress &address)
{
    QList<THttpRequest> reqList;
    int from = 0;
    int headidx;

    while ((headidx = byteArray.indexOf("\r\n\r\n", from)) > 0) {
        headidx += 4;
        THttpRequestHeader header(byteArray.mid(from));

        int contlen = header.contentLength();
        if (contlen <= 0) {
            reqList << THttpRequest(header, QByteArray(), address);
        } else {
            reqList << THttpRequest(header, byteArray.mid(headidx, contlen), address);
        }
        from = headidx + contlen;
    }

    return reqList;
}

/*!
  \fn const THttpRequestHeader &THttpRequest::header() const
  Returns the HTTP header of the request.
*/

/*!
  \fn TMultipartFormData &THttpRequest::multipartFormData()
  Returns a object of multipart/form-data.
*/

/*!
  \fn const QVariantMap &THttpRequest::formItems() const
  Returns the map of all form data.
*/

/*!
  \fn QHostAddress THttpRequest::clientAddress() const
  Returns the address of the client host.
*/

/*!
  \fn bool THttpRequest::hasJson() const
  Returns true if the request contains JSON data.
*/

/*!
  \fn const QJsonDocument &THttpRequest::jsonData() const
  Return the JSON data contained in the request.
*/
