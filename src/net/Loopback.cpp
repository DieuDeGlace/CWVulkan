#include "net/Transport.h"

namespace cwnet {

class LoopbackNetwork::Endpoint : public Transport {
  public:
    Endpoint(LoopbackNetwork &network, PeerId id) : network(network), id(id) {}

    PeerId self() const override { return id; }
    bool valid(PeerId peer) const override { return peer != 0; }

    bool send(PeerId to, Channel channel, const Bytes &packet) override {
        if (to == 0)
            return false;
        std::lock_guard<std::mutex> hold(network.lock);
        network.queues[{to, (int)channel}].push_back({id, packet});
        return true;
    }

    bool receive(Channel channel, Received &out) override {
        std::lock_guard<std::mutex> hold(network.lock);
        auto it = network.queues.find({id, (int)channel});
        if (it == network.queues.end() || it->second.empty())
            return false;
        out = std::move(it->second.front());
        it->second.pop_front();
        return true;
    }

  private:
    LoopbackNetwork &network;
    PeerId id;
};

std::unique_ptr<Transport> LoopbackNetwork::endpoint(PeerId id) { return std::make_unique<Endpoint>(*this, id); }

size_t LoopbackNetwork::waiting(PeerId id, Channel channel) const {
    std::lock_guard<std::mutex> hold(lock);
    auto it = queues.find({id, (int)channel});
    return it == queues.end() ? 0 : it->second.size();
}

} // namespace cwnet
