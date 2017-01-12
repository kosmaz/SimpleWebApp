#include "NewsWidget.hpp"
#include "IXMLParser.h"
#include <Wt/Http/Client>
#include <Wt/WAnchor>
#include <Wt/WString>
#include <Wt/WTimer>
#include <Wt/WImage>

using namespace Wt;
using namespace std;


NewsWidget::NewsWidget(WContainerWidget* parent) :
    WContainerWidget(parent),
    fTimer(new WTimer(this)),
    fHeader(nullptr),
    fNews_Line(nullptr),
    fValid(false)
{
    setStyleClass("news-widget");
    fTimer->setInterval(18000000);
    fTimer->timeout().connect([=](const WMouseEvent&)
    {
        fTimer->stop();
        getRssFeed();
    });
    getRssFeed();
}


NewsWidget::NewsWidget(const NewsWidget& other) :
    WContainerWidget(0),
    fTimer(new WTimer),
    fHeader(nullptr),
    fNews_Line(nullptr),
    fValid(false)
{
    setStyleClass(other.styleClass());
    fTimer->setInterval(18000000);
    fTimer->timeout().connect([=](const WMouseEvent&)
    {
        fTimer->stop();
        getRssFeed();
    });

    if(other.isValid())
    {
        fHeader = new WAnchor(other.header()->link(), other.header()->text(), this);
        fHeader->setImage(new WImage(other.header()->image()->imageLink(), other.header()->image()->alternateText()));
        fHeader->setTarget(Wt::TargetNewWindow);
        fNews_Line = new vector<WAnchor*>;
        for(unsigned i = 0; i < other.newsLine()->size(); ++i)
        {
            WAnchor* anchor = new WAnchor;
            anchor->setStyleClass("news-anchor");
            anchor->setTextFormat(Wt::XHTMLText);
            anchor->setText(other.newsLine()->at(i)->text());
            anchor->setTarget(Wt::TargetNewWindow);
            fNews_Line->push_back(anchor);
        }
        renderNewsLine();
    }
    else
        getRssFeed();

    return;
}


NewsWidget::~NewsWidget()
{
    if(fTimer->isActive())
        fTimer->stop();
    delete fTimer;
    if(fNews_Line)
    {
        for(unsigned i = 0; i < fNews_Line->size(); ++i)
            delete fNews_Line->at(i);
        delete fNews_Line;
    }
}


void NewsWidget::getRssFeed()
{
    Http::Client* rss = new Http::Client(this);
    rss->setTimeout(20);
    rss->done().connect([=](const boost::system::error_code& err, const Http::Message& msg, NoClass, NoClass, NoClass, NoClass)
    {
        if(!err)
        {
            if(msg.status() == 200)
                parseRssFeed(msg.body().c_str());
            else
            {
                log("warning") << "NewsWidget::getRssFeed -> Http::Client::get() -> Response status: " << msg.status();
                WTimer::singleShot(6000, this, &NewsWidget::getRssFeed);
            }
        }
        else
        {
            log("error") << "NewsWidget::getRssFeed -> Http::Client::get() -> boost::system::error_code: " << err.message();
            WTimer::singleShot(6000, this, &NewsWidget::getRssFeed);
        }
    });

    if(!rss->get("http://news.yahoo.com/rss/"))
    {
        log("error") << "NewsWidget::getRssFeed -> Http::Client::get() -> unable to schedule a GET request";
        WTimer::singleShot(6000, this, &NewsWidget::getRssFeed);
    }

    return;
}


void NewsWidget::renderNewsLine()
{
    for(unsigned i = 0; i < fNews_Line->size(); ++i)
        addWidget(fNews_Line->at(i));
    fTimer->start();
    fValid = true;
    WApplication::instance()->triggerUpdate();
    return;
}


void NewsWidget::parseRssFeed(const char* xml)
{
    vector<WAnchor*>* buffer = new vector<WAnchor*>;

    IXMLDomParser parser;
    ITCXMLNode rss_node = parser.parseString(xml, "rss");
    ITCXMLNode channel_node = rss_node.getChildNode("channel");

    if(fHeader == nullptr)
    {
        fHeader = new WAnchor(WLink(channel_node.getChildNode("link").getText()),
                                     WString(channel_node.getChildNode("title").getText()), this);
        fHeader->setImage(new WImage(WLink(channel_node.getChildNode("image").getChildNode("url").getText()), "YAHOO!!"));
        fHeader->setTarget(Wt::TargetNewWindow);
    }

    int length = channel_node.nChildNode("item");

    for(int i = 0; i < length; ++i)
    {
        ITCXMLNode item_node = channel_node.getChildNode("item", i);
        if(item_node.getChildNode("media:content").isEmpty())
            continue;

        WAnchor* anchor = new WAnchor;
        anchor->setStyleClass("news-anchor");
        anchor->setTextFormat(Wt::XHTMLText);
        anchor->setText(WString::fromUTF8(item_node.getChildNode("description").getText(), true));
        anchor->setTarget(Wt::TargetNewWindow);
        buffer->push_back(anchor);
    }

    if(fNews_Line == nullptr)
        fNews_Line = buffer;
    else
    {
        for(unsigned i = 0; i < fNews_Line->size(); ++i)
            delete fNews_Line->at(i);
        delete fNews_Line;
        fNews_Line = buffer;
    }
    renderNewsLine();

    return;
}

