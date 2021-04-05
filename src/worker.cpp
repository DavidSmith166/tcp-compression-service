#include <cassert>
#include <compression.h>
#include <mutex>
#include <optional>
#include <request-code.h>
#include <service.h>

void Service::ping(const Job& job) {
    Header h;
    h.payload_length = 0;
    h.code = static_cast<uint16_t>(Status_Code::OK);
    h.set_net_order();

    Network_Order_Message net_msg(h);
    this->respond(job.clientfd.get(), net_msg);
}

void Service::get_stats(const Job& job) {
    Header h;
    h.payload_length = Service_Constants::GET_STATS_PAYLOAD_SIZE;
    h.code = static_cast<uint16_t>(Status_Code::OK);
    h.set_net_order();

    Network_Order_Message net_msg(h);

    this->stats_lock.lock();
    net_msg.payload.push_back(htonl(this->total_bytes_recieved));
    net_msg.payload.push_back(htonl(this->total_bytes_sent));
    net_msg.payload.push_back(this->compression_ratio);
    this->stats_lock.unlock();

    this->respond(job.clientfd.get(), net_msg);
}

void Service::reset_stats(const Job& job) {
    Header h;
    h.payload_length = 0;
    h.code = static_cast<uint16_t>(Status_Code::OK);
    h.set_net_order();

    this->stats_lock.lock();
    this->total_bytes_recieved = 0;
    this->total_bytes_sent = 0;
    this->compression_ratio = 0;
    this->stats_lock.unlock();

    Network_Order_Message net_msg(h);
    this->respond(job.clientfd.get(), net_msg);
}

void Service::compress(const Job& job) {
    auto payload_opt = Compression::compress(job.msg.payload);  

    if (!payload_opt.has_value()) {
        this->respond_with_error(job.clientfd.get(), Status_Code::UNKNOWN_ERROR);
        return;
    }

    std::vector<char> payload = payload_opt.value();

    this->stats_lock.lock();
    this->compression_ratio = payload.size() / job.msg.payload.size();
    this->stats_lock.unlock();

    Header h;
    h.payload_length = payload.size();
    h.code = static_cast<uint16_t>(Status_Code::OK);
    h.set_net_order();

    Network_Order_Message net_msg(h);
    net_msg.payload = payload;
    this->respond(job.clientfd.get(), net_msg);
}

void Service::process_requests() {
    PRINT("Worker started");

    while (true) {

        std::unique_lock<std::mutex> lock(this->requests_lock);
        this->waiting_workers.wait(lock, [this](){ return this->requests.empty(); });

        Job job = std::move(this->requests.front());
        this->requests.pop();
        this->requests_lock.unlock();

        assert(job.clientfd.get() != -1);

        switch (static_cast<Request_Code>(job.msg.header.code))
        {
            case Request_Code::PING:
                this->ping(job); break;
            case Request_Code::GET_STATS:
                this->get_stats(job); break;
            case Request_Code::RESET_STATS:
                this->reset_stats(job); break;
            case Request_Code::COMPRESS:
                this->compress(job); break;
        }

    }
}