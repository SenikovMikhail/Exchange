#include <iostream>
#include <boost/asio.hpp>

#include "User.hpp"
#include "Common.hpp"
#include "json.hpp"

#include <openssl/sha.h>

using boost::asio::ip::tcp;

// Отправка сообщения на сервер по шаблону.
void SendMessage( 
    tcp::socket& aSocket,
    const std::string& aId,
    const std::string& aRequestType,
    nlohmann::json& aMessage)
{
    nlohmann::json req;
    req["UserID"] = aId;
    req["ReqType"] = aRequestType;
    for (nlohmann::json::iterator it = aMessage.begin(); it != aMessage.end(); ++it) {
        req.emplace(it.key(), it.value());
    }

    std::string request = req.dump();
    boost::asio::write(aSocket, boost::asio::buffer(request, request.size()));
}


// Возвращает строку с ответом сервера на последний запрос.
std::string ReadMessage(tcp::socket& aSocket) {

    boost::asio::streambuf b;
    boost::asio::read_until(aSocket, b, "\0");
    std::istream is(&b);
    std::string line(std::istreambuf_iterator<char>(is), {});
    return line;
}

int main() {

    try {

        boost::asio::io_service io_service;

        tcp::resolver resolver(io_service);
        tcp::resolver::query query(tcp::v4(), "127.0.0.1", std::to_string(port));
        tcp::resolver::iterator iterator = resolver.resolve(query);

        tcp::socket s(io_service);
        s.connect(*iterator);

        std::shared_ptr<User> usr;
        short menu_option_num;
        bool start = true;
        do {
            // Тут реализовано "бесконечное" меню.
            std::cout << "\nMenu:\n"
                         "1) Login\n"
                         "2) Register\n"
                         "3) Exit\n"
                         "\n> ";

            std::cin >> menu_option_num;
            std::cout << std::endl;

            switch (menu_option_num) {

                case 1: {

                    std::string name;
                    std::cout << "Hello! Enter your name: ";
                    std::cin >> name;
                    
                    std::string pass;
                    std::cout << "\nEnter password: ";
                    std::cin >> pass;

                    nlohmann::json req;
                    req["name"] = name;
                    
                    SendMessage(s, "-1", Requests::Login, req);
                    std::string ans = ReadMessage(s);

                    if(ans != ERROR::Login) {

                        nlohmann::json login_info = nlohmann::json::parse(ans);
                        req.clear();
                        req["hash"] = sha256(sha256(pass.append(login_info["sult"])).append(login_info["tmp_slut"]));
                        ans.clear();
                        SendMessage(s,  to_string(login_info["user_id"]), Requests::Login, req);
                        ans = ReadMessage(s);
                        if(ans == ERROR::Login){
                            std::cout << std::string(70, '-') << "\n\n\t" << ans << "\n\n" << std::string(70, '-') << "\n" << std::endl; 
                        } else {
                            usr = std::make_shared<User>(login_info["user_id"], name);
                            start = false;
                        }


                    } else {
                        std::cout << std::string(70, '-') << "\n\n\t" << ans << "\n\n" << std::string(70, '-') << "\n" << std::endl; 
                    }


                    break;
                }       

                case 2: {

                    std::string name;
                    std::cout << "Hello! Enter your name: ";
                    std::cin >> name;

                    std::string pass;
                    std::cout << "\nEnter password: ";
                    std::cin >> pass;

                    nlohmann::json req;
                    req["name"] = name;

                    SendMessage(s, "-1", Requests::Registration, req);
                    std::string ans = ReadMessage(s);


                    if(ans == ERROR::Registration){
                        std::cout << std::string(100, '-') << "\n\n\t" << ans << "\n\n" << std::string(100, '-') << "\n" << std::endl; 
                    } else {


                        nlohmann::json user_info = nlohmann::json::parse(ans);
                        req.clear();
                        req["pass"] = sha256(pass.append(user_info["create_time"]));
                        SendMessage(s, user_info["user_id"], Requests::Registration, req);

                        std::string ans2 = ReadMessage(s);
                        if(ans2 == SUCCESS::Update_pass) {
                            std::string user_id = user_info["user_id"];
                            usr = std::make_shared<User>(std::stoi(user_id), name);
                            start = false;
                        } else {
                            std::cout << std::string(160, '-') << "\n\n\t" << ans << "\n\n" << std::string(100, '-') << "\n" << std::endl; 
                        }
                    }
                    
                    break;
                }   

                case 3: {            
                    
                    exit(0);
                    break;
                }

                default:  {
                    std::cout << "Unknown menu option\n" << std::endl;
                    std::cin.clear();
                    while (std::cin.get() != '\n') ;
                }
            }

        } while(start);

        std::cout << std::endl;

        while (true) {
            // Тут реализовано "бесконечное" меню.
            std::cout << " Menu:\n"
                         " 1) User info\n"
                         " 2) Create order\n"
                         " 3) Exit\n"
                         " \n>";

            std::cin >> menu_option_num;
            std::cout << std::endl;

            switch (menu_option_num) {

                case 1: {

                    nlohmann::json req;

                    SendMessage(s, std::to_string(usr->id()), Requests::Get_balance, req);
                    auto msg = ReadMessage(s);
                    auto info =  nlohmann::json::parse(msg);

                    usr->set_balance(Currency::USD, info[Currency::USD]);
                    usr->set_balance(Currency::RUB, info[Currency::RUB]);

                   
                    
                    std::cout << std::string(160, '-') <<"\n id: " << usr->id() << "\n name: " << usr->name() << "\n balance: " << std::endl; 
                    
                    for(auto el : usr->balance())
                        std::cout << "\t" << el.first << ":" << el.second  << "\n" << std::endl;

                    for(auto el = info.begin(); el != --(--info.end()); ++el){
                        
                        std::cout << "order id: " << el.key();
                        std::cout << "\tpair: " << el.value()["pair"];
                        std::cout << "\type: " << el.value()["type"];
                        std::cout << "\tvolum: " << el.value()["volum"];
                        std::cout << "\tprice: " << el.value()["price"];
                        std::cout << "\topen_timeUTC: " << el.value()["open_timeUTC"];
                        std::cout << "\tclose_timeUTC: " << el.value()["close_timeUTC"]
                        << std::endl;                        
                    }
                
                    std::cout << std::string(160, '-') <<"\n" << std::endl;
                    break;
                }
                
                case 2: {

                    bool status = true;

                    std::string pair;
                    std::string type;                    
                         
                    do {

                        short pair_id;
                        std::cout << "Pair:\n"
                         "1) USDRUB\n"
                         "\n>";
                        std::cin >> pair_id;
                        
                        switch(pair_id){

                            case 1:
                                pair = CurrencyPair::USDRUB;
                                status = false;
                                break;

                            default: {
                                std::cout << "Unknown menu option\n" << std::endl;
                                std::cin.clear();
                                while (std::cin.get() != '\n') ;
                            }
                            
                        }
                    } while(status);

                    status = true;

                    do {

                        short type_id;
                        std::cout << "\nType:\n"
                         "1) buy\n"
                         "2) sell\n"
                         "\n>";
                        std::cin >> type_id;
                        
                        switch(type_id){

                            case 1:
                                type = Requests::Buy;
                                status = false;
                                break;

                            case 2:
                                type = Requests::Sell;
                                status = false;
                                break;

                            default: {
                                std::cout << "Unknown menu option\n" << std::endl;
                                std::cin.clear();
                                while (std::cin.get() != '\n') ;
                            }
                            
                        }
                    } while(status);

                    std::cout << "\nEnter volum: ";
                    double volum;
                    std::cin >> volum;
                    
                    std::cout << "\nEnter price: ";
                    double price;
                    std::cin >> price;
                    
                    nlohmann::json req;
                    req["pair"] = pair;
                    req["type"] = type;
                    req["volum"] = volum;
                    req["price"] = price;
                    std::string msg = req.dump();

                    SendMessage(s, std::to_string(usr->id()), type, req);
                    std::cout << "\n" << ReadMessage(s) << "\n" << std::endl;
                    break;
                }

                case 3: {
                    
                    exit(0);
                    break;
                }

                default: {
                    std::cout << "Unknown menu option\n" << std::endl;
                    std::cin.clear();
                    while (std::cin.get() != '\n') ;
                }

            }
        }

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
