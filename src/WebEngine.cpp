#include <boost/system/error_code.hpp>
#include <Wt/WOverlayLoadingIndicator>
#include <Wt/Auth/HashFunction>
#include <Wt/WBootstrapTheme>
#include <Wt/Dbo/backend/MySQL>
#include <Wt/Dbo/Transaction>
#include <Wt/Dbo/Session>
#include <Wt/Http/Client>
#include <Wt/WMessageBox>
#include <Wt/WProgressBar>
#include <Wt/WPushButton>
#include <Wt/WFileUpload>
#include <Wt/WPopupMenu>
#include <Wt/WIOService>
#include <Wt/WLineEdit>
#include <Wt/WString>
#include <Wt/WLabel>
#include <Wt/WImage>
#include <Wt/WText>
#include <thread>
#include <stdio.h>
#include <ctype.h>

#include "WebEngine.hpp"
#include "NewsWidget.hpp"
using namespace Wt;
using namespace std;


WebEngine::WebEngine(const WEnvironment& env, const dbo::SqlConnectionPool* dbconnectionpool) :
    WApplication(env),
    fNews_Widget(nullptr),
    fUser_Page(nullptr),
    fFile_Upload_Widget(nullptr),
    fPayment_Code_Server(nullptr),
    fCode_Generating_Service(nullptr),
    fRecord_Queue(nullptr),
    fLoggedIn(false)
{
    setTitle("NESTOR");
    addMetaHeader(Wt::MetaName, "viewport", "width=device-width, initial-scale=1.0");
    WBootstrapTheme* theme = new WBootstrapTheme;
    theme->setVersion(WBootstrapTheme::Version3);
    setTheme(theme);
    useStyleSheet(WLink("css/MyStyle.css"));
    WOverlayLoadingIndicator* indicator = new WOverlayLoadingIndicator;
    indicator->setMessage("Loading...");
    setLoadingIndicator(indicator);
    log("info") << "Started new session with id "<<sessionId();
    enableUpdates();
    enableInternalPaths();
    internalPathChanged().connect(this, &WebEngine::handleInternalPath);

    fSQL_Connection_Pool = const_cast<dbo::SqlConnectionPool*>(dbconnectionpool);
    fDatabase_Session = new dbo::Session;
    fDatabase_Session->setConnectionPool(*fSQL_Connection_Pool);
    fDatabase_Session->mapClass<User>("user");

#ifdef _DEBUG
    try
    {
        dbo::Transaction transaction(*fDatabase_Session);
        fDatabase_Session->execute("create table user ("
                                   "id BIGINT primary key AUTO_INCREMENT,"
                                   "version integer not null,"
                                   "serialno integer not null,"
                                   "username text not null,"
                                   "password text not null,"
                                   "firstname text not null,"
                                   "lastname varchar(5) not null,"
                                   "number varchar(11) not null,"
                                   "network varchar(16) not null,"
                                   "code varchar(7) not null,"
                                   "role integer not null)");

        fDatabase_Session->execute("create unique index user_index on user (serialno, number)");
        Auth::BCryptHashFunction hasher(5);
        User* user = new User(-1, "admin", hasher.compute("admin", "admin"), "admin", "admin", "", "", "", User::ADMIN);
        fDatabase_Session->add<User>(user);
        transaction.commit();
    }
    catch(const dbo::Exception& ex) {
        log("error") << "WebEngine::WebEngine -> dbo::Session.createTables() -> " << ex.what();
    }
#endif

}


WebEngine::~WebEngine()
{
    delete fNews_Widget;
    delete fDatabase_Session;
    delete fHome_Page;
    if(fUser_Page != nullptr)
        delete fUser_Page;
}


void WebEngine::setupHomePage()
{
    setInternalPath("/Home");
    fPrevious_Path = "/Home";
    fHome_Page = new WContainerWidget(root());
    WContainerWidget* header = new WContainerWidget(fHome_Page);
    header->setStyleClass("header");
    WImage* company_image = new WImage(WLink("images/Wt-Logo.png"), header);
    company_image->resize(64, 64);
    company_image->setStyleClass("image");

    fNews_Widget = new NewsWidget(fHome_Page);
    WContainerWidget* login_widget = new WContainerWidget(fHome_Page);
    login_widget->setStyleClass("login-widget");

    WLabel* login_label = new WLabel("Login", login_widget);
    login_label->setInline(false);
    login_label->setStyleClass("login-label");

    WLabel* username_label = new WLabel("Username", login_widget);
    username_label->setStyleClass("username-label");
    WLineEdit* username = new WLineEdit(login_widget);
    username->setPlaceholderText("first name.last name");
    username_label->setBuddy(username);
    username->setStyleClass("username");

    WLabel* password_label = new WLabel("Password", login_widget);
    password_label->setStyleClass("password-label");
    WLineEdit* password = new WLineEdit(login_widget);
    password->setPlaceholderText("mobile number");
    password->setEchoMode(WLineEdit::Password);
    password_label->setBuddy(password);
    password->setStyleClass("password");

    WPushButton* login_button = new WPushButton("login", login_widget);
    login_button->setStyleClass("login-button");

    WLabel* incorrect_label = new WLabel("Incorrect username or password", login_widget);
    incorrect_label->setStyleClass("incorrect_label");
    incorrect_label->hide();

    login_button->clicked().connect([=](const WMouseEvent&)
    {
        if(!incorrect_label->isHidden())
            incorrect_label->hide();
        fUsername = username->text().toUTF8();
        fPassword = password->text().toUTF8();
        dbo::ptr<User> user;
        try
        {
            dbo::Transaction transaction(*fDatabase_Session);
            user = fDatabase_Session->find<User>().where("username = ?").bind(fUsername).resultValue();
            Auth::BCryptHashFunction hasher(5); //use default value 0 or values ranging from 4 - 31
            if(hasher.compute(fPassword, user->fLast_Name) == user->fPassword)
            {
                fUserPtr = user;
                setupUserPage();
            }
            else
                incorrect_label->show();
        }
        catch(const dbo::Exception& ex) {
            incorrect_label->show();
            log("error") << "WebEngine::setupHomePage -> dbo::Session.find() -> " << ex.what();
        }

    });

    WText* footer = new WText(fHome_Page);
    footer->setStyleClass("footer");
    footer->setText("This website was developed by -k0$m@3- Inc."
                    " All Rights Reserved."
                    "<p><i>Powered by Wt.</i></p>");

    return;
}


void WebEngine::setupUserPage()
{
    setInternalPath("/user");
    fPrevious_Path = "/user";
    fLoggedIn = true;
    root()->removeWidget(fHome_Page);
    fUser_Page = new WContainerWidget(root());
    fUser_Page->setStyleClass("background");

    WText* welcome_widget = new WText(fUser_Page);
    welcome_widget->setTextFormat(Wt::XHTMLUnsafeText);
    welcome_widget->setStyleClass("welcome-widget");
    welcome_widget->setText("Welcome<br /><br/ ><br /><br/ ><br /><br/ >" + fUserPtr->fFirst_Name + " " + fUserPtr->fLast_Name);
    WPushButton* continue_button = new WPushButton("Continue >>", fUser_Page);
    continue_button->setStyleClass("continue-button");
    continue_button->clicked().connect([=](const WMouseEvent&)
    {
        delete welcome_widget;
        delete continue_button;
        fUser_Page->removeStyleClass("background");
        WContainerWidget* header = new WContainerWidget(fUser_Page);
        header->setStyleClass("header");
        WImage* company_image = new WImage(WLink("images/Wt-Logo.png"), header);
        company_image->resize(64, 64);
        company_image->setStyleClass("image");
        company_image->clicked().connect([=](const WMouseEvent&)
        {
            root()->removeWidget(fUser_Page);
            root()->addWidget(fHome_Page);
            setInternalPath("/Home");
            fPrevious_Path = "/Home";
        });

        WPushButton* logout_button = new WPushButton;
        WPushButton* lastname_label = new WPushButton(fUserPtr->fFirst_Name, header);
        lastname_label->setStyleClass("lastname-label");
        WPopupMenu* popup = new WPopupMenu;
        popup->addItem("logout", logout_button);
        popup->setStyleClass("popup-menu");
        popup->triggered().connect([=](const WMenuItem*, NoClass, NoClass, NoClass, NoClass, NoClass)
        {
            fLoggedIn = false;
            root()->removeWidget(fUser_Page);
            root()->addWidget(fHome_Page);
            delete fUser_Page;
            fUser_Page = nullptr;
            setInternalPath("/Home");
            fPrevious_Path = "/Home";
        });
        lastname_label->setMenu(popup);

        fUser_Page->addWidget(new NewsWidget(*fNews_Widget));
        if(fUserPtr->fRole == User::ADMIN)
        {
            WText* admininfo_label = new WText;
            WContainerWidget* admin_widget = new WContainerWidget(fUser_Page);
            admin_widget->setStyleClass("admin-widget");

            fFile_Upload_Widget = new WFileUpload(admin_widget);
            fFile_Upload_Widget->setStyleClass("file-upload-widget");
            fFile_Upload_Widget->setProgressBar(new WProgressBar);
            fFile_Upload_Widget->setFilters("text/*");
            fFile_Upload_Widget->uploaded().connect([=](NoClass)
            {
                thread th(&WebEngine::parseRecord, this);
                th.detach();
                admininfo_label->setText("<b><i>Processing records and committing to database...</i></b>");
                triggerUpdate();
            });
            fFile_Upload_Widget->fileTooLarge().connect([=](const ::int64_t& size, NoClass, NoClass, NoClass, NoClass, NoClass)
            {
                WString message("The file to be uploaded must not exceed {1} bytes which is the maximum allowed"
                                " file size.<br />You tried to upload a file with size of {2} bytes.<br />"
                                "Press the reload/refresh button on your browser to try again.");
                message.arg(maximumRequestSize());
                message.arg(size);
                WMessageBox* warning = new WMessageBox(root());
                (void)warning->addButton(Wt::StandardButton::Ok);
                warning->setWindowTitle("File too large");
                warning->setText(message);
                warning->show();
                warning->buttonClicked().connect([=](const Wt::StandardButton&, NoClass, NoClass, NoClass, NoClass, NoClass)
                {
                    warning->accept();
                    delete warning;
                });
            });

            WPushButton* file_upload_button = new WPushButton("Upload Record", admin_widget);
            file_upload_button->setStyleClass("file-upload-button");
            file_upload_button->setDisabled(true);
            fFile_Upload_Widget->changed().connect([=](NoClass)
            {
                file_upload_button->setEnabled(true);
            });
            file_upload_button->clicked().connect([=](const WMouseEvent&)
            {
                file_upload_button->setDisabled(true);
                fFile_Upload_Widget->upload();
            });

            admin_widget->addWidget(admininfo_label);
            admininfo_label->setTextFormat(Wt::XHTMLText);
            admininfo_label->setStyleClass("admin-label");
            admininfo_label->setWordWrap(true);
            admininfo_label->setText("Use the <b>Upload Record</b> button to upload user records to database.<br />"
                                     "If any error is encountered while parsing the record file, "
                                     "you will be informed on this window.<br />"
                                     "No duplicate mobile number or name is allowed.");
            fParse_Report.connect([=](WString text, NoClass, NoClass, NoClass, NoClass, NoClass)
            {
                UpdateLock lock(this);
                if(lock)
                    admininfo_label->setText(text);
                triggerUpdate();
            });
        }
        else
        {
            WText* userinfo_label = new WText(fUser_Page);
            userinfo_label->setTextFormat(Wt::XHTMLText);
            userinfo_label->setWordWrap(true);
            userinfo_label->setStyleClass("userinfo-label");
            userinfo_label->setText("Name: <i>" + fUserPtr->fFirst_Name + " " + fUserPtr->fLast_Name + "</i><br />"
                                    "Number: <i>" + fUserPtr->fMobile_Number + "</i><br />"
                                    "Network Operator: <i>" + fUserPtr->fNetwork_Operator + "</i><br />"
                                    "Payment Code: <i><b>" + fUserPtr->fPayment_Code + "</b></i><br />");
        }

        WText* footer = new WText(fUser_Page);
        footer->setStyleClass("footer");
        footer->setText("This website was developed by -k0$m@3- Inc."
                        " All Rights Reserved."
                        "<p><i>Powered by Wt.</i></p>");
    });

    return;
}


void WebEngine::parseRecord()
{
    string file_name = fFile_Upload_Widget->spoolFileName();
    if(!fRecord_Queue)
        fRecord_Queue = new queue<int>;
    else
    {
        fParse_Report.emit("<b>Last uploaded record hasn't been fully processed by the server. Try uploading the new record"
                           " later to ensure data persistence on the database and to avoid broken records</b>");
        string command("rm -f " + file_name);
        system(command.c_str());
        return;
    }

    bool error = false;
    string err_str("");
    FILE* file = fopen(file_name.c_str(), "r");

    Auth::BCryptHashFunction hasher(5);
    dbo::Transaction transaction(*fDatabase_Session);
    int database_size = 0;

    try {
        database_size = fDatabase_Session->query<int>("select count(*) from user") - 1;
    }
    catch(const dbo::Exception& ex) {
        log("error") << "webEngine::parseRecord -> dbo::Session.query() -> " << ex.what();
    }

    string default_first_name("wt");
    string default_last_name("name");

    int row =1;
    int column = 1;
    char c = '\0';

    for(; c != EOF; ++database_size)
    {
        string buffer;
        c = fgetc(file);
        bool char_present = false;
        while(c != EOF && c != ',')
        {
            ++column;
            if(c == '\n' || c == '\r')
            {
                ++row;
                column = 1;
            }
            if(!isdigit(c))
                char_present = true;
            buffer.push_back(c);
            c = fgetc(file);
        }

        if(buffer.size() > 11 || buffer.size() < 11 || char_present)
        {
            error = true;
            WString temp("Invalid number with length of {1} from the uploaded record at line {2} column {3}<br />");
            temp.arg(buffer.size());
            temp.arg(row);
            temp.arg(column);
            err_str.append(temp.toUTF8());
            char_present = false;
            continue;
        }

        try
        {
            int count = 0;
            count = fDatabase_Session->query<int>("select count(*) from user where number = ?").bind(buffer);
            if(count)
            {
                error = true;
                WString temp("Tried to duplicate a number on the database from the uploaded record at line {1} column {2}<br />");
                temp.arg(row);
                temp.arg(column);
                err_str.append(temp.toUTF8());
            }
            else
            {
                WString temp("{1}");
                temp.arg(database_size);
                User* user = new User(database_size,
                                      default_first_name + temp.toUTF8() + "." + default_last_name,
                                      hasher.compute(buffer, default_last_name),
                                      default_first_name + temp.toUTF8(),
                                      default_last_name,
                                      buffer,
                                      getNetworkOperatorFromNumber(buffer),
                                      "",
                                      User::VISITOR);
                fDatabase_Session->add<User>(user);
                fRecord_Queue->push(database_size);
            }
        }
        catch(const dbo::Exception& ex) {
            log("error") << "WebEngine::parseRecord -> dbo::Session::query -> " <<ex.what();
        }
    }
    transaction.commit();
    fclose(file);
    string command("rm -f " + file_name);
    system(command.c_str());

    if(!fRecord_Queue->empty())
        generatePaymentCode();
    else
    {
        delete fRecord_Queue;
        fRecord_Queue = nullptr;
    }

    if(error)
        fParse_Report.emit(err_str + "<br />"
                                     "<b>Press the reload/refresh button on your browser to re-upload the modified record</b>");
    else
        fParse_Report.emit("<b>Database update was successful</b><br />"
                           "Press the reload/refresh button on your browser to upload a new record.");

    return;
}


void WebEngine::generatePaymentCode()
{
    if(fPayment_Code_Server == nullptr)
    {
        fCode_Generating_Service = new WIOService;
        fCode_Generating_Service->setThreadCount(3);
        fCode_Generating_Service->start();
        fPayment_Code_Server = new Http::Client(*fCode_Generating_Service);
        fPayment_Code_Server->setTimeout(20);
        fPayment_Code_Server->done().connect(this, &WebEngine::updateDatabaseRecord);
    }

    if(fPayment_Code_Server->get(""))
    {

    }
    else
    {
        log("error") << "";
    }

    return;
}


void WebEngine::updateDatabaseRecord(boost::system::error_code err, Http::Message message)
{
    if(!err)
    {
        if(message.status() == 200)
        {
            try
            {
                dbo::Transaction transaction(*fDatabase_Session);
                dbo::ptr<User> userptr =
                        fDatabase_Session->find<User>().where("serialno = ?").bind(fRecord_Queue->front()).resultValue();
                string code = message.body();
                userptr.modify()->fPayment_Code = code;
                transaction.commit();
                fRecord_Queue->pop();
                if(!fRecord_Queue->empty())
                    generatePaymentCode();
                else
                {
                    delete fRecord_Queue;
                    fRecord_Queue = nullptr;
                    delete fPayment_Code_Server;
                    fPayment_Code_Server = nullptr;
                    fCode_Generating_Service->stop();
                    delete fCode_Generating_Service;
                    fCode_Generating_Service = nullptr;
                }
            }
            catch(dbo::Exception& ex) {
                log("error") << "WebEngine::updateDatabaseRecord -> dbo::Session.find() -> " << ex.what();
            }
        }
        else
        {

        }
    }
    else
        log("error") << "WebEngine::updateDatabaseRecord -> Wt::Http::Client -> " << err.message();

    return;
}


void WebEngine::handleInternalPath(std::string path)
{
  if(path == "/Home" && fPrevious_Path == "/user")
    {
        root()->removeWidget(fUser_Page);
        root()->addWidget(fHome_Page);
        fPrevious_Path = "/Home";
    }
    else if(path == "/user" && fLoggedIn && fPrevious_Path == "/Home")
    {
        root()->removeWidget(fHome_Page);
        root()->addWidget(fUser_Page);
        fPrevious_Path = "/user";
    }

    return;
}


string WebEngine::getNetworkOperatorFromNumber(string number)
{
    string head = number.substr(1, 3);
    string head2 = number.substr(1, 4);

    if(head == "804")
        return "NTEL";
    else if(head == "707")
        return "ZOOMMOBILE";
    else if(head2 == "7027" || head == "709")
        return "MULTI-LINKS";
    else if(head2 == "7028" || head2 == "7029" || head == "819")
        return "STARCOMMS";
    else if(head2 == "7025" || head2 == "7026" || head == "704")
        return "VISAFONE";
    else if(head == "809" || head == "817" || head == "818" || head == "909" || head == "908")
        return "ETISALAT NIGERIA";
    else if(head == "701" || head == "708" || head == "802" || head == "808" || head == "812" || head == "902")
        return "AIRTEL NIGERIA";
    else if(head == "705" || head == "805" || head == "807" || head == "811" || head == "815" || head == "905")
        return "GLOBACOM";
    else if(head == "703" || head == "706" || head == "803" || head == "806" || head == "810" || head == "813" || head == "814" ||
            head == "816" || head == "903")
        return "MTN NIGERIA";

    return "NA";
}
