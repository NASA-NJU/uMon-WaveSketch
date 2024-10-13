#ifndef FIVE_TUPLE_H
#define FIVE_TUPLE_H

#include "murmurhash3.h"

#include <cstdint>
#include <cstdio>
#include <string>

using namespace std;

/* five tuple */
struct five_tuple {

    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t  protocol;

    five_tuple()
    : src_ip(0), dst_ip(0), src_port(0), dst_port(0), protocol(0) {}
    five_tuple(uint32_t id)
    : src_ip(0), dst_ip(id), src_port(0), dst_port(0), protocol(6) {}
    five_tuple(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint8_t proto = 6)
    : src_ip(sip), dst_ip(dip), src_port(sport), dst_port(dport), protocol(proto) {}
    five_tuple(const string& sip, const string& dip, const string& sport, const string& dport, const string& proto = "TCP") {
        src_ip = string_to_ip(sip);
        dst_ip = string_to_ip(dip);
        src_port = stoul(sport);
        dst_port = stoul(dport);
        protocol = proto[0] == 'T' ? 6 : 17;
    }

    static uint32_t string_to_ip(const string& str) {
        if(str.find('.') == string::npos)
            return stoul(str);
        uint32_t a, b, c, d;
        sscanf(str.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d);
        return (a << 24) | (b << 16) | (c << 8) | d;
    }

    static string ip_to_string(uint32_t ip) {
        uint32_t a = (ip >> 24) & 0xFF;
        uint32_t b = (ip >> 16) & 0xFF;
        uint32_t c = (ip >> 8) & 0xFF;
        uint32_t d = ip & 0xFF;
        string result = to_string(a);
        result.append(".");
        result.append(to_string(b));
        result.append(".");
        result.append(to_string(c));
        result.append(".");
        result.append(to_string(d));
        return result;
    }

    size_t hash(uint32_t seed = 0xDEADBEEF) const {
        size_t hash_value = 0;
        simple_hash(this, 13, seed, &hash_value);
        return hash_value;
    }

    friend constexpr strong_ordering operator<=>(const five_tuple& lhs, const five_tuple& rhs) = default;
    friend ostream& operator<<(ostream& os, const five_tuple& t) {
        os << (t.protocol == 6 ? "T" : "U");
        os << t.ip_to_string(t.src_ip) << ":" << t.src_port;
        os << "<>";
        os << t.ip_to_string(t.dst_ip) << ":" << t.dst_port;
        return os;
    }
};

namespace std {
    template<>
    class hash<five_tuple> {
    public:
        size_t operator()(const five_tuple& t) const {
            return t.hash();
        }
    };
}

#endif //FIVE_TUPLE_H
