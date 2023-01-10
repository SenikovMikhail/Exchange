#ifndef CLIENSERVERECN_COMMON_HPP
#define CLIENSERVERECN_COMMON_HPP

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

}


#endif //CLIENSERVERECN_COMMON_HPP