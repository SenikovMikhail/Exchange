#ifndef ORDER_HPP
#define ORDER_HPP

#include <map>
#include <list>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

class Order{

public:

    Order(int64_t id_, int64_t user_id_, const std::string& pair_, const std::string& type_, double volum_, double price_, 
                                            boost::posix_time::ptime open_time_ = boost::posix_time::second_clock::universal_time())
        : _id(id_)
        , _user_id(user_id_)
        , _pair(pair_)
        , _type(type_)
        , _volum(volum_)
        , _current_volum(_volum)
        , _price(price_)
        , _open_timeUTC( open_time_ )
    {}

    int64_t id() { return _id; }
    int64_t user_id() { return _user_id; }
    std::string& pair() { return _pair; }
    std::string& type() { return _type; }
    double volum() { return _volum; }
    double current_volum() { return _current_volum; }
    double price() { return _price;}
    boost::posix_time::ptime open_timeUTC() { return _open_timeUTC; }
    boost::posix_time::ptime close_timeUTC() { return _close_timeUTC; }

    void set_current_volum(double new_volum) { _current_volum = new_volum; }
    void set_close_time() { _close_timeUTC = boost::posix_time::second_clock::universal_time(); }

    bool operator==(Order& other) const {
        return (_id == other.id());
    }   


private:

    int64_t _id;
    int64_t _user_id;
    std::string _pair;
    std::string _type;
    double _volum;
    double _current_volum;
    double _price;
    boost::posix_time::ptime _open_timeUTC;
    boost::posix_time::ptime _close_timeUTC;

};

#endif