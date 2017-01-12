#include "WebEngine.hpp"
#include <Wt/Dbo/FixedSqlConnectionPool>
#include <Wt/Dbo/backend/MySQL>
#include <Wt/WServer>
#include <signal.h>
#include <stdio.h>
using namespace Wt;

#ifdef WTHTTP_CONFIGURATION
#undef WTHTTP_CONFIGURATION
#define WTHTTP_CONFIGURATION "server-config/wthttpd"
#endif

const static dbo::SqlConnectionPool* gSql_Connection_Pool =
        new dbo::FixedSqlConnectionPool(new dbo::backend::MySQL("Nestor", "Nestor", "1316863DAF"), 5);

WApplication* createApplication(const WEnvironment& env)
{
    WebEngine* app = new WebEngine(env, gSql_Connection_Pool);
    app->setupHomePage();
    return app;
}

int main(int argc, char** argv)
{
    try
    {
        WServer server;
        server.setServerConfiguration(argc, argv, WTHTTP_CONFIGURATION);
        server.addEntryPoint(Wt::Application, createApplication, "/");
        if(server.start())
        {
            int signal = WServer::waitForShutdown();
            server.stop();
            if(signal == SIGHUP)
                WServer::restart(argc, argv, environ);
        }
    }
    catch(const WException& ex) {
        fprintf(stderr, "%s\n", ex.what());
    }
    catch(const std::exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
    }

    return 0;
}
