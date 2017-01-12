#ifndef USER_HPP
#define USER_HPP

#include <Wt/Dbo/Dbo>
#include <string>

namespace dbo = Wt::Dbo;

class User : public dbo::Dbo<User>
{
public:
    enum Role
    {
        VISITOR = 0x00,
        ADMIN = 0x11
    };

    User(){}

    User(int serial, std::string username, std::string password, std::string first_name, std::string last_name, std::string mobile_number,
         std::string network_operator, std::string payment_code, Role role) :
        fSerial_No(serial),
        fUsername(username),
        fPassword(password),
        fFirst_Name(first_name),
        fLast_Name(last_name),
        fMobile_Number(mobile_number),
        fNetwork_Operator(network_operator),
        fPayment_Code(payment_code),
        fRole(role) {}

    template<class Action>
    void persist(Action& a)
    {
        dbo::field(a, fSerial_No, "serialno");
        dbo::field(a, fUsername, "username");
        dbo::field(a, fPassword, "password");
        dbo::field(a, fFirst_Name, "firstname");
        dbo::field(a, fLast_Name, "lastname");
        dbo::field(a, fMobile_Number, "number");
        dbo::field(a, fNetwork_Operator, "network");
        dbo::field(a, fPayment_Code, "code");
        dbo::field(a, fRole, "role");
    }

    int fSerial_No;
    std::string fUsername;
    std::string fPassword;
    std::string fFirst_Name;
    std::string fLast_Name;
    std::string fMobile_Number;
    std::string fNetwork_Operator;
    std::string fPayment_Code;
    Role fRole;
};

#endif // USER_HPP
