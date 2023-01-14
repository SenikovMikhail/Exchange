#ifndef USER_HPP
#define USER_HPP


#include <string>
#include <map>
#include <list>

#include "Common.hpp"
#include "Order.hpp"

class User{

public:

    User(int64_t id_, const std::string& name_, const std::string& hash_ = "", double balance_usd_ = 0, double balance_rub_ = 0)
        : _id(id_)
        , _name(name_)
        , _hash( hash_)
    {
        _balance[Currency::USD] = balance_usd_;
        _balance[Currency::RUB] = balance_rub_;
    }

    std::map<std::string, double>& balance() { return _balance ;}
    double balance(const std::string& currency) { return _balance[currency]; }
    void set_balance(const std::string& pair, double new_balance) { _balance[pair] = new_balance; }
    void set_hash(const std::string& hash_) { _hash = hash_; }
    std::string hash() { return _hash; }
    int id() { return _id; }
    std::string& name() { return _name; }

private:
    int64_t _id;
    std::string _name;
    std::string _hash;
    std::map<std::string, double> _balance;

};

#endif