#ifndef WebEngine_HPP
#define WebEngine_HPP


#include <queue>
#include <Wt/WApplication>
#include "User.hpp"

namespace Wt {
    namespace Dbo {
        class SqlConnectionPool;
        namespace backend {
            class MySQL;
        }
    }
}

namespace boost {
    namespace system {
        class error_code;
    }
}

#ifdef _DEBUG
#include <iostream>

inline void printStr(const char* str) {
    std::cout<<std::endl<<str<<std::endl<<std::endl;
    return;
}

inline void printStr(std::string str) {
    std::cout<<std::endl<<str<<std::endl<<std::endl;
    return;
}

inline void printStr(Wt::WString str) {
    std::cout<<std::endl<<str<<std::endl<<std::endl;
    return;
}
#endif



class NewsWidget;


class WebEngine : public Wt::WApplication
{
public:
    WebEngine(const Wt::WEnvironment&, const dbo::SqlConnectionPool*);
    ~WebEngine();
    void setupHomePage();

private:
    void setupUserPage();
    void parseRecord();
    void generatePaymentCode();
    void updateDatabaseRecord(boost::system::error_code, Wt::Http::Message);

    void handleInternalPath(std::string);
    std::string getNetworkOperatorFromNumber(std::string);

    NewsWidget* fNews_Widget;
    dbo::SqlConnectionPool* fSQL_Connection_Pool;
    dbo::Session* fDatabase_Session;
    dbo::ptr<User> fUserPtr;
    Wt::WContainerWidget* fHome_Page;
    Wt::WContainerWidget* fUser_Page;
    Wt::WFileUpload* fFile_Upload_Widget;
    Wt::Signal<Wt::WString> fParse_Report;
    Wt::Http::Client* fPayment_Code_Server;
    Wt::WIOService* fCode_Generating_Service;
    std::string fUsername;
    std::string fPassword;

    struct Format {

        Format(){}
        Format(std::string firstname, std::string lastname, std::string mobilenumber, std::string networkoperator,
               std::string paymentcode) :
            fFirst_Name(firstname),
            fLast_Name(lastname),
            fMobile_Number(mobilenumber),
            fNetwork_Operator(networkoperator),
            fPayment_Code(paymentcode) {}

        bool operator==(const Format& other) {
            return ((fFirst_Name == other.fFirst_Name && fLast_Name == other.fLast_Name) || fMobile_Number == other.fMobile_Number);
        }

        std::string fFirst_Name;
        std::string fLast_Name;
        std::string fMobile_Number;
        std::string fNetwork_Operator;
        std::string fPayment_Code;
    };

    std::queue<int>* fRecord_Queue;
    bool fLoggedIn;
    std::string fPrevious_Path;
    int fPresent_Record;
};


#endif // WebEngine_HPP
