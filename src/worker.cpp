#include <cassert>
#include <compression.h>
#include <helpers.h>
#include <mutex>
#include <optional>
#include <request-code.h>
#include <service.h>

void Service::ping(const Job& job) {

    IF_VERBOSE (
        printf("Ping response\n");
    )

    Header h;
    h.payload_length = 0;
    h.code = static_cast<uint16_t>(Status_Code::OK);
    h.set_net_order();

    Network_Order_Message net_msg(h);
    assert(job.clientfd.get() != -1);
    this->respond(job.clientfd.get(), net_msg);
}

void Service::get_stats(const Job& job) {

    IF_VERBOSE (
        printf("Get_Stats response\n");
    )

    Header h;
    h.payload_length = Service_Constants::GET_STATS_PAYLOAD_SIZE;
    h.code = static_cast<uint16_t>(Status_Code::OK);
    h.set_net_order();

    Network_Order_Message net_msg(h);

    this->stats_lock.lock();
    uint32_t nbo_total_bytes_recieved = htonl(this->total_bytes_recieved);
    uint32_t nbo_total_bytes_sent = htonl(this->total_bytes_sent);
    uint8_t nbo_compression_ratio = this->compression_ratio;
    this->stats_lock.unlock();

    Helpers::add_bytes_to_payload(&nbo_total_bytes_recieved, &net_msg.payload);
    Helpers::add_bytes_to_payload(&nbo_total_bytes_sent, &net_msg.payload);
    Helpers::add_bytes_to_payload(&nbo_compression_ratio, &net_msg.payload);

    this->respond(job.clientfd.get(), net_msg);
}

void Service::reset_stats(const Job& job) {

    IF_VERBOSE (
        printf("Reset_Stats response\n");
    )

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

    IF_VERBOSE (
        printf("Compress response\n");
    )

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

    IF_VERBOSE (
        printf("Worker started\n");
    )

    while (true) {

        std::unique_lock<std::mutex> lock(this->requests_lock);
        this->waiting_workers.wait(lock, [this](){ return this->requests.size(); });

        Job job = std::move(this->requests.front());
        this->requests.pop();
        this->requests_lock.unlock();

        IF_VERBOSE (
            printf("Worker recieved job for client %i\n", job.clientfd.get());
        )

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