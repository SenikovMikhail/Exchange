#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <set>
#include <pqxx/pqxx>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include "User.hpp"
#include "json.hpp"
#include "Common.hpp"

using boost::asio::ip::tcp;


struct compare_by_open_price {
    bool operator()(const std::shared_ptr<Order> first, const std::shared_ptr<Order> second ) const { 
        return first->price() < second->price(); 
    
    };
};  


class Core {

public:

    Core()
    : database("dbname = task user = adm password = adm123 \
      hostaddr = 127.0.0.1 port = 5432")
    {
        pqxx::work  Work(database);
        sql = "Select count(*) from users;";
        pqxx::result Users_count_req(Work.exec(sql));
        Users_count = Users_count_req.begin()[0].as<int64_t>();
        Work.commit();

        pqxx::work  Work2(database);
        sql = "Select count(*) from orders;";
        pqxx::result Orders_count_req(Work2.exec(sql));
        Orders_count = Orders_count_req.begin()[0].as<int64_t>();
        Work2.commit();

        pqxx::work  Work3(database);
        sql = "Select * from orders where close_time='';";
        pqxx::result Orders_count_list(Work3.exec(sql));
        for(auto row : Orders_count_list) {

            active_orders.emplace(row[0].as<int64_t>(), std::make_shared<Order>(row[0].as<int64_t>(), row[1].as<int64_t>(), 
                                                                        row[2].as<std::string>(), row[3].as<std::string>(), 
                                                                        row[4].as<double>(), row[5].as<double>(), 
                                                                        boost::posix_time::time_from_string(row[6].as<std::string>())));

            if(row[3].as<std::string>() == Requests::Buy)
                active_buy_order.insert(active_orders[Orders_count]);
            else if(row[3].as<std::string>() == Requests::Sell)
                active_sell_order.insert(active_orders[Orders_count]);
        }

        Work2.commit();
    }


    double Get_balance(const std::string& user_id, const std::string& currency) {
        return active_users[std::stoi(user_id)]->balance(currency);
    }

    nlohmann::json Get_orders(const std::string& user_id){

        pqxx::work  Work(database);
        sql = "select * from orders where user_id = " + user_id + std::string(";");
        pqxx::result Ans(Work.exec(sql));
        Work.commit();

        nlohmann::json req;

        for(auto row = Ans.begin(); row != Ans.end(); ++row ){

            req[row[0].as<std::string>()].emplace("pair", row[2].as<std::string>());
            req[row[0].as<std::string>()].emplace("type", row[3].as<std::string>());
            req[row[0].as<std::string>()].emplace("volum", row[4].as<double>());
            req[row[0].as<std::string>()].emplace("price", row[5].as<double>());
            req[row[0].as<std::string>()].emplace("open_timeUTC", row[6].as<std::string>());
            req[row[0].as<std::string>()].emplace("close_timeUTC", row[7].as<std::string>());
        }

        return req;
    }


    // "Регистрирует" нового пользователя и возвращает его ID.
    std::string Register_new_user(const std::string& user_name) {

        pqxx::work  Work(database);
        sql = "select * from users where user_name = '" + user_name + std::string("';");
        pqxx::result Sql_ans(Work.exec(sql));
        Work.commit();
        if(Sql_ans.size() != 0 && Sql_ans.begin()[1].as<std::string>() == user_name) {
            return ERROR::Registration;
        } else {

            nlohmann::json usr_info;
            usr_info["user_id"] = std::to_string(Users_count);
            std::string create_time = boost::posix_time::to_simple_string(boost::posix_time::second_clock::universal_time());
            usr_info["create_time"] = create_time;

            pqxx::work Work2(database);
            sql = std::string("insert into users (user_id, user_name, password_hash, create_time, balance_usd, balance_rub) values(")
                    + std::to_string(Users_count) + std::string(", '") + user_name + std::string("', '', '") 
                    + create_time + std::string("', 0, 0);");

            Work2.exec(sql);
            Work2.commit();

            active_users.emplace(Users_count, std::make_shared<User>(Users_count, user_name));
            ++Users_count;

            return usr_info.dump();

        }
    }

    std::string Update_user_pass(const std::string& user_id, const std::string& password){        

        pqxx::work Work2(database);
        sql = "update users set password_hash = '" + password +                    
                    std::string("' where user_id = ") + user_id + std::string(";");;

        Work2.exec(sql);
        Work2.commit();

        return SUCCESS::Update_pass;      

    }


    std::string Login(const std::string& user_name){

        nlohmann::json req;

        pqxx::work  Work(database);
        sql = "select * from users  where user_name = '" + user_name + std::string("';");
        pqxx::result Ans(Work.exec(sql));
        Work.commit();
        if(Ans.size() > 0) {
            req["user_id"] = Ans.begin()[0].as<int64_t>();
            req["sult"] = Ans.begin()[3].as<std::string>();
            req["tmp_slut"] = boost::posix_time::to_simple_string(boost::posix_time::second_clock::universal_time());

            active_users.emplace(Ans.begin()[0].as<int64_t>(), std::make_shared<User>(Ans.begin()[0].as<int64_t>(), user_name, 
            sha256(Ans.begin()[2].as<std::string>().append(req["tmp_slut"]))));

            return req.dump();
        }

        return ERROR::Login;

    }

    std::string Check_hash(const std::string& user_id, const std::string& hash){

        if( hash != active_users[stoi(user_id)]->hash()){
            active_users.erase(std::stoi(user_id));
            return ERROR::Login;
        } else {  
            return SUCCESS::Check_hash;
        }
    }


    void Create_order(int64_t user_id, const std::string& pair, const std::string& order_type, int64_t volum, double price) {
        std::cout << user_id << std::endl;
        active_orders.emplace(Orders_count, std::make_shared<Order>(Orders_count, user_id, pair, order_type, volum, price));
std::cout << active_orders[Orders_count]->id() << std::endl;
        if(order_type == Requests::Buy)
            active_buy_order.insert(active_orders[Orders_count]);
        else if(order_type == Requests::Sell)
            active_sell_order.insert(active_orders[Orders_count]);

        std::cout << user_id << std::endl;

        pqxx::work Work(database);
        sql = "insert into orders (order_id, user_id, pair, order_type, volum, price, open_time, close_time) \
                values("+ std::to_string(Orders_count) + std::string(", ") + std::to_string(user_id) + std::string(", '")  + pair + std::string("', '") + order_type + std::string("', ") + 
                    std::to_string(volum) + std::string(", ") + std::to_string(price) + std::string(", '") +  
                        boost::posix_time::to_simple_string(active_orders[Orders_count]->open_timeUTC()) + std::string("', '');");;
        
        Work.exec(sql);
        Work.commit();

        ++Orders_count;
    }


    void Update_sql_user(std::shared_ptr<Order> order) {

        auto user = active_users[order->user_id()];

        user->set_balance(Currency::USD, 
            (user->balance(Currency::USD) + (order->type() == Requests::Sell ? -1*order->volum() : order->volum() )));

        user->set_balance(Currency::RUB, 
            (user->balance(Currency::RUB) + (order->type() == Requests::Sell ? order->price()*order->volum() : -1*order->price()*order->volum() )));


        pqxx::work Work(database);
        sql = "update users set balance_usd = " + std::to_string(user->balance(Currency::USD)) +
                 std::string(", balance_rub = ") + std::to_string(user->balance(Currency::RUB)) + 
                    std::string(" where user_id = '") + std::to_string(order->user_id())  + std::string("';");;

        Work.exec(sql);
        Work.commit();

    }

    void Close_order(std::shared_ptr<Order> order) {

        order->set_close_time();
        
        pqxx::work  Work(database);
        sql = "update orders set close_time = '" + boost::posix_time::to_simple_string(order->close_timeUTC())
                                                            + std::string("' where order_id=") + std::to_string(order->id());

        Work.exec(sql);
        Work.commit();

        Update_sql_user(order);

        if(order->type() == Requests::Buy)
            active_buy_order.erase(std::find_if(active_buy_order.begin(), active_buy_order.end(), [&](std::shared_ptr<Order> curr) {
                                                        return (*curr == *order);
                                                    }));
        else if(order->type() == Requests::Sell)
            active_sell_order.erase(std::find_if(active_sell_order.begin(), active_sell_order.end(), [&](std::shared_ptr<Order> curr) {
                                                        return (*curr == *order);
                                                    }));

        active_orders.erase(order->id());

    }

    void trade() {
        
        while(!active_buy_order.empty() && !active_sell_order.empty() && (*--active_buy_order.end())->price() >= (*(active_sell_order.begin()))->price()) {
            
            std::multiset< std::shared_ptr<Order>, compare_by_open_price>::iterator buy_order_itr;

            buy_order_itr = --active_buy_order.end();  // max price
            if(active_buy_order.size() != 1) {
                for( ; (*buy_order_itr)->price() == (*--buy_order_itr)->price() && buy_order_itr != active_buy_order.begin(); --buy_order_itr)
                {}
            }

            auto buy_order = *buy_order_itr;
            auto sell_order = (*(active_sell_order.begin()));  // min price

            if( buy_order->price() > sell_order->price()) {

                if(buy_order->current_volum() < sell_order->current_volum()){

                    sell_order->set_current_volum(sell_order->volum() - buy_order->current_volum());             

                    buy_order->set_current_volum(0);

                } else if (buy_order->current_volum() >= sell_order->current_volum()){

                    buy_order->set_current_volum(buy_order->current_volum() - sell_order->current_volum());      

                    sell_order->set_current_volum(0); 
                } 

                if(buy_order->current_volum() == 0)
                    Close_order(buy_order);

                if(sell_order->current_volum() == 0)
                    Close_order(sell_order);

            }  
        }                                                
    }


private:
    // <UserId, UserName>
    int64_t Users_count;
    int64_t Orders_count;

    //user id, obj users 
    std::map<int64_t, std::shared_ptr<User>> active_users;
    //order id, ptr obj Order
    std::map<int64_t, std::shared_ptr<Order>> active_orders;
    // open price, ptr obj Order
    std::multiset< std::shared_ptr<Order>, compare_by_open_price> active_sell_order;
    std::multiset< std::shared_ptr<Order>, compare_by_open_price> active_buy_order;

    pqxx::connection database;
    std::string psql_user = "adm";
    std::string psql_passwd = "adm123";
    std::string pqsl_host_addr = "127.0.0.1";
    std::string psql_port = "5432";
    std::string psql_db_name = "task";
    std::string sql;

};

Core& GetCore() {
    static Core core;
    return core;
}


class session {

public:

    session(boost::asio::io_service& io_service)
        : socket_(io_service)
    {}


    tcp::socket& socket() {

        return socket_;
    }


    void start() {

        socket_.async_read_some(boost::asio::buffer(data_, max_length),
            boost::bind(&session::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));

    }


    // Обработка полученного сообщения.
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
        
        if (!error) {

            data_[bytes_transferred] = '\0';

            // Парсим json, который пришёл нам в сообщении.
            auto j = nlohmann::json::parse(data_);
            auto reqType = j["ReqType"];

            std::string reply = "Error! Unknown request type\n";

            if (reqType == Requests::Login) {

                std::string hash;
                if(j["UserID"] == "-1") {

                    reply = GetCore().Login(j["name"]);

                } else {

                    reply = GetCore().Check_hash(j["UserID"], j["hash"]);                    

                }

            } else if(reqType == Requests::Registration){

                if(j["UserID"] == "-1"){

                    reply = GetCore().Register_new_user(j["name"]);                    

                } else {

                    reply = GetCore().Update_user_pass(j["UserID"], j["pass"]);

                }         

            } else if (reqType == Requests::Get_balance) {

                nlohmann::json req = GetCore().Get_orders(j["UserID"]);
                req.emplace(Currency::USD, GetCore().Get_balance(j["UserID"], Currency::USD));
                req.emplace(Currency::RUB, GetCore().Get_balance(j["UserID"], Currency::RUB));

                reply = req.dump(2);

            } else if (reqType == Requests::Buy || reqType == Requests::Sell) {

                std::cout << j << std::endl;
                
                GetCore().Create_order(stoi(j["UserID"].get<std::string>()), j["pair"].get<std::string>(), 
                                            j["type"].get<std::string>(), j["volum"].get<int64_t>(), j["price"].get<double>());
                
                reply = "Order was created";

                GetCore().trade();

            } 

            boost::asio::async_write(socket_,
                boost::asio::buffer(reply, reply.size()),
                boost::bind(&session::handle_write, this,
                    boost::asio::placeholders::error));
        
        } else {
            delete this;
        }
    }

    void handle_write(const boost::system::error_code& error) {

        if (!error) {

            socket_.async_read_some(boost::asio::buffer(data_, max_length),
                boost::bind(&session::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        
        } else {
            delete this;
        }
    }

private:
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};


class server {

public:

    server(boost::asio::io_service& io_service)
        : io_service_(io_service)
        , acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
    {
        std::cout << "Server started! Listen " << port << " port" << std::endl;

        session* new_session = new session(io_service_);
        acceptor_.async_accept(new_session->socket(),
            boost::bind(&server::handle_accept, this, new_session,
                boost::asio::placeholders::error));
    }

    void handle_accept(session* new_session, const boost::system::error_code& error) {

        if (!error) {

            new_session->start();
            new_session = new session(io_service_);

            acceptor_.async_accept(new_session->socket(),
                boost::bind(&server::handle_accept, this, new_session,
                    boost::asio::placeholders::error));
        
        } else {
            delete new_session;
        }
    }

private:

    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;

};

int main() {

    try {

        boost::asio::io_service io_service;
      
        static Core core;

        server s(io_service);

        io_service.run();
    
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
            std::cout << "Server started! Listen " << port << " port" << std::endl;

    return 0;
}