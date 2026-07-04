#pragma once

#include "ContainerReader.h"

namespace rav {

class PacketGenerator {
public:
    explicit PacketGenerator(ContainerReader& reader)
        : reader_(reader) {}

    PacketPtr next() {
        return reader_.read_packet();
    }

    bool seek_to_time(double seconds) {
        return reader_.seek_to_time(seconds);
    }

private:
    ContainerReader& reader_;
};

} // namespace rav
