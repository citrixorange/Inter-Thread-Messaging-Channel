#ifndef __ITMC__
#define __ITMC__

#include <iostream>
#include <memory>
#include <queue>
#include <thread>
#include <functional>
#include <optional>
#include <mutex>

namespace std {

    template <class T, class U>
    struct Callback {
        function<void(T,U)> callback;
    };

    template<class T, class U>
    class Receiver {

        private:
            string label;
            mutex &_mutex;
            shared_ptr<queue<T>> channel;
            optional<shared_ptr<Callback<T,U>>> listener_callback;

        public:
            
            Receiver(string label, shared_ptr<queue<T>> channel, mutex &_mutex) : label(label), _mutex(_mutex), channel(channel), listener_callback(nullopt) {}

            void register_callback(function<void(T,U)> callback) {
                this->listener_callback = make_shared<Callback<T,U>>(Callback<T,U>{callback});
            }
            
            void listen_messages(U param) {

                

                while(true) {


                    cout << "Locking mutex from " << this->label << endl;

                    unique_lock<mutex> lock(this->_mutex);

                    auto queue = this->channel;

                    lock.unlock();

                    cout << "Unlocking mutex from " << this->label << endl;


                    while(!queue->empty()) {
                        T message = queue->front();
                        cout << "Message received at " << this->label << endl;
                        cout << "Calling Callback with argument: " << message << endl;
                        
                        if(this->listener_callback.has_value()) {
                            auto callback = this->listener_callback.value();
                            if (callback) {
                                return callback->callback(message, param);
                            }
                        }

                        queue->pop();
                    }

                    this_thread::sleep_for(chrono::milliseconds(100));
                }

            }
    };

    template<class T>
    class Sender {

        private:

            string label;
            mutex &_mutex;
            shared_ptr<queue<T>> channel;

        public:

            Sender(string label, shared_ptr<queue<T>> channel, mutex &_mutex) : label(label), _mutex(_mutex), channel(channel) {}

            void send_message(T msg) {
                unique_lock<mutex> lock(this->_mutex);
                auto queue = this->channel;
                queue->push(msg);
                lock.unlock();
            }
    };

    template<class T, class U>
    class InterThreadMessageChannel {
        private:
            string label;
            mutex _mutex;
            shared_ptr<queue<T>> channel;
            Receiver<T, U> receiver;
            Sender<T> sender;
            
        public:
            InterThreadMessageChannel(string label) : label(label), channel(make_shared<queue<T>>()), receiver(label, channel, _mutex), sender(label, channel, _mutex) {}
            
            tuple<unique_ptr<Receiver<T,U>>, shared_ptr<Sender<T>>> create_channel() {
                unique_ptr<Receiver<T,U>> receiver = make_unique<Receiver<T,U>>(this->receiver);
                shared_ptr<Sender<T>> sender = make_shared<Sender<T>>(this->sender);
                tuple<unique_ptr<Receiver<T,U>>, shared_ptr<Sender<T>>> channel = make_tuple(move(receiver), sender);
                return channel;
            }

    };

}

#endif