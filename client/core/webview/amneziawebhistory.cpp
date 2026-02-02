#include "amneziawebhistory.h"
#include "amneziawebhistory_p.h"
#include "amneziawebview.h"
#include "amneziawebview_p.h"

#include <QSharedData>
#include <QDebug>


/*!
  Constructs a history item from \a other. The new item and \a other
  will share their data, and modifying either this item or \a other will
  modify both instances.
*/
AmneziaWebHistoryItem::AmneziaWebHistoryItem(const AmneziaWebHistoryItem &other)
    : d_ptr(other.d_ptr)
{
}

/*!
  Assigns the \a other history item to this. This item and \a other
  will share their data, and modifying either this item or \a other will
  modify both instances.
*/
AmneziaWebHistoryItem &AmneziaWebHistoryItem::operator=(const AmneziaWebHistoryItem &other)
{
    d_ptr = other.d_ptr;
    return *this;
}

/*!
  Destroys the history item.
*/
AmneziaWebHistoryItem::~AmneziaWebHistoryItem()
{
}

/*!
 Returns the URL associated with the history item.

 \sa originalUrl(), title(), lastVisited(), data(), mimeType()
*/
QUrl AmneziaWebHistoryItem::url() const
{
    Q_D(const AmneziaWebHistoryItem);
    if (d)
        return d->url();
    return QUrl();
}


/*!
 Returns the title of the page associated with the history item.

 \sa icon(), url(), lastVisited(), data(), mimeType()
*/
QString AmneziaWebHistoryItem::title() const
{
    Q_D(const AmneziaWebHistoryItem);
    if (d)
        return d->title();
    return QString();
}

/*!
 Returns the icon associated with the history item.

 \sa title(), url(), lastVisited(), data(), mimeType()
*/
QIcon AmneziaWebHistoryItem::icon() const
{
    Q_D(const AmneziaWebHistoryItem);
    if (d)
        return d->icon();

    return QIcon();
}

/*!
 Returns the data associated with the history item.

 \sa icon(), title(), url(), lastVisited(), mimeType()
*/
QByteArray AmneziaWebHistoryItem::data() const
{
    Q_D(const AmneziaWebHistoryItem);
    if(d) return d->data();
    return QByteArray();
}

/*!
 Returns the mimeType associated with the history item.

 \sa icon(), title(), url(), lastVisited(), data()
*/
QString AmneziaWebHistoryItem::mimeType() const
{
    Q_D(const AmneziaWebHistoryItem);
    if(d) return d->mimeType();
    return QString("text/html");
}

/*!*
  \internal
*/
AmneziaWebHistoryItem::AmneziaWebHistoryItem(AmneziaWebHistoryItemPrivate *priv) : d_ptr(priv)
{
}

/*!
    \since 4.5
    Returns whether this is a valid history item.
*/
bool AmneziaWebHistoryItem::isValid() const
{
    Q_D(const AmneziaWebHistoryItem);
    bool valid = (d);
    return valid;
}

AmneziaWebHistory::AmneziaWebHistory(AmneziaWebView *parent) : QObject(parent)
    , d_ptr(new AmneziaWebHistoryPrivate())
{
    Q_D(AmneziaWebHistory);
    d->q_ptr = this;
}

AmneziaWebHistory::~AmneziaWebHistory()
{
    clear();
}

/*!
  Clears the history.

  \sa count(), items()
*/
void AmneziaWebHistory::clear()
{
    Q_D(AmneziaWebHistory);
    while (d->items.count()) {
        d->items.removeFirst();
    }
}

/*!
  Returns a list of all items currently in the history.

  \sa count(), clear()
*/
QList<AmneziaWebHistoryItem> AmneziaWebHistory::items() const
{
    Q_D(const AmneziaWebHistory);
    QList<AmneziaWebHistoryItem> ret;

    for (int i = 0; i < d->items.size(); ++i) {
        AmneziaWebHistoryItem item(d->items[i]);
        ret.append(item);
    }
    return ret;
}

/*!
  Returns the list of items in the backwards history list.
  At most \a maxItems entries are returned.

  \sa forwardItems()
*/
QList<AmneziaWebHistoryItem> AmneziaWebHistory::backItems(int maxItems) const
{
    Q_D(const AmneziaWebHistory);

    int count = d->currentIndex;
    if (maxItems >= 0) {
        count = qMin(count, maxItems);
    }

    QList<AmneziaWebHistoryItem> ret;
    for (int i = (d->currentIndex - count); i < d->currentIndex; i++) {
        ret.append(d->items[i]);
    }
    return ret;
}

/*!
  Returns the list of items in the forward history list.
  At most \a maxItems entries are returned.

  \sa backItems()
*/
QList<AmneziaWebHistoryItem> AmneziaWebHistory::forwardItems(int maxItems) const
{
    Q_D(const AmneziaWebHistory);

    int count = d->items.count() - d->currentIndex - 1;
    if (maxItems >= 0) {
        count = qMin(count, maxItems);
    }

    QList<AmneziaWebHistoryItem> ret;
    for (int i = (d->currentIndex + 1); i <= d->currentIndex + count; i++) {
        ret.append(d->items[i]);
    }
    return ret;
}


/*!
  Returns true if there is an item preceding the current item in the history;
  otherwise returns false.

  \sa canGoForward()
*/
bool AmneziaWebHistory::canGoBack() const
{
    const AmneziaWebHistoryItem current = currentItem();
    bool can = (current.isValid() && current.d_ptr->backItem() != nullptr);
    return can;
}

/*!
  Returns true if we have an item to go forward to; otherwise returns false.

  \sa canGoBack()
*/
bool AmneziaWebHistory::canGoForward() const
{
    const AmneziaWebHistoryItem current = currentItem();
    bool can = (current.isValid() && current.d_ptr->forwardItem() != nullptr);
    return can;
}

/*!
  Set the current item to be the previous item in the history and goes to the
  corresponding page; i.e., goes back one history item.

  \sa forward(), goToItem()
*/
void AmneziaWebHistory::back()
{
    Q_D(AmneziaWebHistory);
    if(!canGoBack()) return;
    AmneziaWebView *view = qobject_cast<AmneziaWebView*>(parent());
    AmneziaWebHistoryItem item = backItem();

    d->currentIndex--;
    if (view) {
        if (item.data().length() > 0) {
            view->setContent(item.data(), item.mimeType(), item.url());
        }
        else {

            view->setUrl(item.url());
        }
    }
}

/*!
  Sets the current item to be the next item in the history and goes to the
  corresponding page; i.e., goes forward one history item.

  \sa back(), goToItem()
*/
void AmneziaWebHistory::forward()
{
    Q_D(AmneziaWebHistory);
    if(!canGoForward()) return;
    AmneziaWebView *view = qobject_cast<AmneziaWebView*>(parent());
    AmneziaWebHistoryItem item = backItem();

    d->currentIndex++;
    if (view) {
        if (item.data().length() > 0) {
            view->setContent(item.data(), item.mimeType(), item.url());
        }
        else {
            view->setUrl(item.url());
        }
    }
}

/*!
  Sets the current item to be the specified \a item in the history and goes to the page.

  \sa back(), forward()
*/
void AmneziaWebHistory::goToItem(const AmneziaWebHistoryItem &item)
{
    Q_D(AmneziaWebHistory);
    if(!item.isValid()) return;
    AmneziaWebView *view = qobject_cast<AmneziaWebView*>(parent());
    if (!view) return; //There is no view to go.
    if (item.url().isEmpty()) return; //

    int index = -1;
    for(int i= 0; i < d->items.count(); ++i) {
        if(d->items[i].d_ptr.data() == item.d_ptr.data()) {
            index = i;
            break;
        }
    }

    if (index >= 0) {

        d->currentIndex = index;
        if (item.data().length() > 0) {
            view->setContent(item.data(), item.mimeType(), item.url());
        }
        else {
            view->setUrl(item.url());
        }
    }
}

/*!
 Returns the current item in the history.
 */
AmneziaWebHistoryItem AmneziaWebHistory::currentItem() const
{
    Q_D(const AmneziaWebHistory);

    if ((d->currentIndex >= 0) && (d->currentIndex < d->items.count())) {
        return AmneziaWebHistoryItem(d->items.at(d->currentIndex));
    }
    return AmneziaWebHistoryItem(nullptr);
}

void AmneziaWebHistory::append(const QUrl& url, const QByteArray& data, const QString& mimeType)
{
    Q_D(AmneziaWebHistory);

    const AmneziaWebHistoryItem current = currentItem();
    // Check if url is same as current, and do not add it second time.
    if (current.url() == url) return;

    AmneziaWebHistoryItemPrivate *priv = new AmneziaWebHistoryItemPrivate();
    if(current.isValid()) {
        current.d_ptr->_forwardItem = priv;
        priv->_backItem = current.d_ptr.data();
    }
    priv->_data = data;
    priv->_url = url;
    priv->_mimeType = mimeType;

    //Remove last items till current
    while (d->items.count() > (d->currentIndex + 1)) {
        d->items.removeLast();
    }

    //No more then maximum
    while (d->items.count() >= d->maximumCount) {
        d->items.removeFirst();
    }

    d->items.append(AmneziaWebHistoryItem(priv));
    d->currentIndex = (d->items.count() - 1);

}

/*!
  Returns the item before the current item in the history.
*/
AmneziaWebHistoryItem AmneziaWebHistory::backItem() const
{
    AmneziaWebHistoryItem current = currentItem();
    return AmneziaWebHistoryItem(current.d_ptr->backItem());
}


/*!
  Returns the item after the current item in the history.
*/
AmneziaWebHistoryItem AmneziaWebHistory::forwardItem() const
{
    AmneziaWebHistoryItem current = currentItem();
    return AmneziaWebHistoryItem(current.d_ptr->forwardItem());
}

/*!
  \since 4.5
  Returns the index of the current item in history.
*/
int AmneziaWebHistory::currentItemIndex() const
{
    Q_D(const AmneziaWebHistory);
    return d->currentIndex;
}

/*!
  Returns the item at index \a i in the history.
*/
AmneziaWebHistoryItem AmneziaWebHistory::itemAt(int i) const
{
    Q_D(const AmneziaWebHistory);
    int index = (i < 0) ? 0 : i;
    index = (index >= count()) ? (count() -1) : index;
    if (index >= 0) {
        return AmneziaWebHistoryItem(d->items.at(index));
    }
    return AmneziaWebHistoryItem(nullptr);
}

/*!
    Returns the total number of items in the history.
*/
int AmneziaWebHistory::count() const
{
    Q_D(const AmneziaWebHistory);
    return d->items.count();
}

/*!
  \since 4.5
  Returns the maximum number of items in the history.

  \sa setMaximumItemCount()
*/
int AmneziaWebHistory::maximumItemCount() const
{
    Q_D(const AmneziaWebHistory);
    return d->maximumCount;
}

/*!
  \since 4.5
  Sets the maximum number of items in the history to \a count.

  \sa maximumItemCount()
*/
void AmneziaWebHistory::setMaximumItemCount(int count)
{
    Q_D(AmneziaWebHistory);
    d->maximumCount = count;
}
