#ifndef NEWSWIDGET_HPP
#define NEWSWIDGET_HPP

#include <Wt/WContainerWidget>
#include <vector>

class IXMLDomParser;
namespace Wt {
    class WTimer;
    class WString;
    class WAnchor;
}

class NewsWidget : public Wt::WContainerWidget
{
public:
    NewsWidget(Wt::WContainerWidget* parent = 0);
    NewsWidget(const NewsWidget&);
    ~NewsWidget();
    inline bool isValid() const {return fValid;}
    inline Wt::WAnchor* header() const {return fHeader;}
    inline std::vector<Wt::WAnchor*>* newsLine() const {return fNews_Line;}

private:
    void getRssFeed();
    void renderNewsLine();
    void parseRssFeed(const char*);
    Wt::WTimer* fTimer;
    Wt::WAnchor* fHeader;
    std::vector<Wt::WAnchor*>* fNews_Line;
    bool fValid;
};

#endif // NEWSWIDGET_HPP
