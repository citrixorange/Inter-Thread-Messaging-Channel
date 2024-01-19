#include "itmc.hpp"

using namespace std;

#define MESSAGE_FROM_ONE_TO_MAIN "MESSAGE_FROM_ONE_TO_MAIN"
#define MESSAGE_FROM_TWO_TO_MAIN "MESSAGE_FROM_TWO_TO_MAIN"
#define MESSAGE_FROM_MAIN_TO_ONE "MESSAGE_FROM_MAIN_TO_ONE"
#define MESSAGE_FROM_MAIN_TO_TWO "MESSAGE_FROM_MAIN_TO_TWO"

void main_service_cb(string message, tuple<shared_ptr<Sender<string>>, shared_ptr<Sender<string>>> handler) {
    auto [main_to_service_one_sender, main_to_service_two_sender] = handler;

    if(message == MESSAGE_FROM_ONE_TO_MAIN) {
        main_to_service_one_sender->send_message(MESSAGE_FROM_MAIN_TO_ONE);
    } else if(message == MESSAGE_FROM_TWO_TO_MAIN) {
        main_to_service_two_sender->send_message(MESSAGE_FROM_MAIN_TO_TWO);
    } else {
        cout << "Not expected message format" << endl;
    }
}

void service_one_cb(string message, shared_ptr<Sender<string>> handler) {

    if(message == MESSAGE_FROM_MAIN_TO_ONE) {
        cout << "Message Received on Service One" << endl;
    }
    else {
        cout << "Not expected message format" << endl;
    }
}

void service_two_cb(string message, shared_ptr<Sender<string>> handler) {

    if(message == MESSAGE_FROM_MAIN_TO_TWO) {
        cout << "Message Received on Service Two" << endl;
    }
    else {
        cout << "Not expected message format" << endl;
    }
}

void mainService(unique_ptr<Receiver<string, tuple<shared_ptr<Sender<string>>, shared_ptr<Sender<string>>>>> receiver, shared_ptr<Sender<string>> main_to_service_one_sender, shared_ptr<Sender<string>> main_to_service_two_sender) {
    this_thread::sleep_for(chrono::milliseconds(100));
    cout << "Lauching Main Service Task..." << endl;
    receiver->register_callback(main_service_cb);
    receiver->listen_messages(make_tuple(main_to_service_one_sender, main_to_service_two_sender));
}

void serviceOne(unique_ptr<Receiver<string, shared_ptr<Sender<string>>>> receiver, shared_ptr<Sender<string>> service_one_to_main_sender) {
    this_thread::sleep_for(chrono::milliseconds(200));
    cout << "Lauching Service One Task..." << endl;
    receiver->register_callback(service_one_cb);
    service_one_to_main_sender->send_message(MESSAGE_FROM_ONE_TO_MAIN);
    receiver->listen_messages(service_one_to_main_sender);
}

void serviceTwo(unique_ptr<Receiver<string, shared_ptr<Sender<string>>>> receiver, shared_ptr<Sender<string>> service_two_to_main_sender) {
    this_thread::sleep_for(chrono::milliseconds(300));
    cout << "Lauching Service Two Task..." << endl;
    receiver->register_callback(service_two_cb);
    service_two_to_main_sender->send_message(MESSAGE_FROM_TWO_TO_MAIN);
    receiver->listen_messages(service_two_to_main_sender);
}

int main() {

    InterThreadMessageChannel<string, tuple<shared_ptr<Sender<string>>, shared_ptr<Sender<string>>>> main_message_channel("Main_Service_Channel");
    tuple<unique_ptr<Receiver<string, tuple<shared_ptr<Sender<string>>, shared_ptr<Sender<string>>>>>, shared_ptr<Sender<string>>> main_channel = main_message_channel.create_channel();

    unique_ptr<Receiver<string, tuple<shared_ptr<Sender<string>>, shared_ptr<Sender<string>>>>> main_receiver = move(get<unique_ptr<Receiver<string, tuple<shared_ptr<Sender<string>>, shared_ptr<Sender<string>>>>>>(main_channel));
    shared_ptr<Sender<string>> service_one_to_main_sender = get<shared_ptr<Sender<string>>>(main_channel);
    shared_ptr<Sender<string>> service_two_to_main_sender = make_shared<Sender<string>>(*service_one_to_main_sender);

    InterThreadMessageChannel<string, shared_ptr<Sender<string>>> service_one_message_channel("Service_One_Channel");
    tuple<unique_ptr<Receiver<string, shared_ptr<Sender<string>>>>, shared_ptr<Sender<string>>> service_one_channel = service_one_message_channel.create_channel();

    unique_ptr<Receiver<string, shared_ptr<Sender<string>>>> service_one_receiver = move(get<unique_ptr<Receiver<string, shared_ptr<Sender<string>>>>>(service_one_channel));
    shared_ptr<Sender<string>> main_to_service_one_sender = get<shared_ptr<Sender<string>>>(service_one_channel);

    InterThreadMessageChannel<string, shared_ptr<Sender<string>>> service_two_message_channel("Service_Two_Channel");
    tuple<unique_ptr<Receiver<string, shared_ptr<Sender<string>>>>, shared_ptr<Sender<string>>> service_two_channel = service_two_message_channel.create_channel();

    unique_ptr<Receiver<string, shared_ptr<Sender<string>>>> service_two_receiver = move(get<unique_ptr<Receiver<string, shared_ptr<Sender<string>>>>>(service_two_channel));
    shared_ptr<Sender<string>> main_to_service_two_sender = get<shared_ptr<Sender<string>>>(service_two_channel);

    jthread main_service(mainService, move(main_receiver), main_to_service_one_sender, main_to_service_two_sender); 
    jthread service_one(serviceOne, move(service_one_receiver), service_one_to_main_sender); 
    jthread service_two(serviceTwo, move(service_two_receiver), service_two_to_main_sender);


    return 0;
}