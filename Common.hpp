#ifndef CLIENSERVERECN_COMMON_HPP
#define CLIENSERVERECN_COMMON_HPP

#include "SHA256.hpp"

#include <string>


static short port = 5555;

namespace Requests {

    static std::string Login = "login";
    static std::string Registration = "Reg";
    static std::string Get_balance = "Get_balance";
    static std::string Buy = "Buy";
    static std::string Sell = "Sell";  

}

namespace Currency {

    static std::string USD = "USD";
    static std::string RUB = "RUB";
}

namespace CurrencyPair {

    static std::string USDRUB = "USDRUB";
}

namespace ERROR {
    
    static std::string Registration = "ERROR::USER NAME ALREADY USED";
    static std::string Login = "ERROR::USER NOT FOUND OR PASSWORD IS INCORRECT";
    static std::string Update_pass = "ERROR::UPDATE PASSWORD";
    static std::string Check_hash = "ERROR::Check_hash";

}

namespace SUCCESS {

    static std::string Registration = "SUCCESS::Registration";
    static std::string Login = "SUCCESS::Login";
    static std::string Update_pass = "SUCCESS::Update_pass";
    static std::string Check_hash = "SUCCESS::Check_hash";

}

#endif //CLIENSERVERECN_COMMON_HPP
