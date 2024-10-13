#ifndef PERSIST_CMS_POLYGON_H
#define PERSIST_CMS_POLYGON_H

#include "../Utility/headers.h"

using namespace std;

namespace PersistCMS {

    class node : public pair<double, double> {
    public:
        using Base = pair<double, double>;
        using Base::Base;

        size_t serialize() const {
            size_t result = 0;
            result += sizeof(this->first);
            result += sizeof(this->second);
            return result;
        }
    };
    class line : public pair<TIME, node> {
    public:
        using Base = pair<TIME, node>;
        using Base::Base;

        size_t serialize() const {
            size_t result = 0;
            result += sizeof(this->first);
            result += this->second.serialize();
            return result;
        }
    };

    // polygon represents (m, b) pair of all lines u = mt + b fitting data ranges (t_k, [alpha_k, omega_k]).
    // in other words, all (m, b) pair subject to
    //     b >= (-t_k)m + alpha_k
    //     b <= (-t_k)m + omega_k
    class polygon {
    protected:
        // store upper vertices from leftmost to rightmost
        list<node> upper{};
        // store lower vertices from leftmost to rightmost
        list<node> lower{};

        // force to generate segment for the first data
        TIME last_time = 0;
        DATA last_value = 0;
        double last_delta = 0;

        // median point between L and R; return {0, 0} if empty
        node median() const {
            if(upper.empty()) [[unlikely]] {
                return {0, 0};
            }
            const node& l = upper.front();
            const node& r = upper.back();
            return {(l.first + r.first) / 2, (l.second + r.second) / 2};
        }

        // initialize polygon
        void initialize_polygon(uint32_t time, int32_t value, double delta) {
            upper.clear();
            lower.clear();

            assert(time - last_time > 0);
            double m_lg = ((value - delta) - (last_value + last_delta)) / (time - last_time);
            double b_lg = (time * (last_value + last_delta) - last_time * (value - delta)) / (time - last_time);
            double m_gg = ((value + delta) - (last_value + last_delta)) / (time - last_time);
            double b_gg = (time * (last_value + last_delta) - last_time * (value + delta)) / (time - last_time);
            double m_gl = ((value + delta) - (last_value - last_delta)) / (time - last_time);
            double b_gl = (time * (last_value - last_delta) - last_time * (value + delta)) / (time - last_time);
            double m_ll = ((value - delta) - (last_value - last_delta)) / (time - last_time);
            double b_ll = (time * (last_value - last_delta) - last_time * (value - delta)) / (time - last_time);

            upper.emplace_back(m_lg, b_lg);
            upper.emplace_back(m_gg, b_gg);
            upper.emplace_back(m_gl, b_gl);

            lower.emplace_back(m_lg, b_lg);
            lower.emplace_back(m_ll, b_ll);
            lower.emplace_back(m_gl, b_gl);
        }

        // compare a vertex against b = (-t)m + c
        //     on the line    ==> return equal
        //     below the line ==> return less
        //     over the line  ==> return greater
        //     incomparable   ==> return unordered (e.g. NaN)
        static constexpr partial_ordering compare(const node v, double t, double c) {
            auto m = v.first;
            auto b = v.second;

            auto lhs = b;
            auto rhs = -t * m + c;

            auto epsilon = (fabs(lhs) + fabs(rhs)) * 1e-16;

            if(fabs(lhs - rhs) < epsilon)
                return partial_ordering::equivalent;
            else
                return lhs <=> rhs;
        }

        // intersect line (begin, ++begin) with b = (-t)m + c
        template<typename I>
        node intersect(I& begin, uint32_t t, double c) const {
            I v1 = begin++;
            I v2 = begin;

            double m1 = v1->first;
            double b1 = v1->second;
            double m2 = v2->first;
            double b2 = v2->second;

            // no intersection on the edge, search next
            if(compare(*v1, t, c) > 0 && compare(*v2, t, c) > 0 ||
               compare(*v1, t, c) < 0 && compare(*v2, t, c) < 0)
                return intersect(begin, t, c);

            // precaution measure; will not occur under current logic:
            assert((b2 - b1) + t * (m2 - m1) != 0);
            double m = (b2 * m1 - b1 * m2 + c * (m2 - m1)) / ((b2 - b1) + t * (m2 - m1));
            double b = (c * (b2 - b1) - t * (b2 * m1 - b1 * m2)) / ((b2 - b1) + t * (m2 - m1));

            return {m, b};
        }

    public:
        polygon() = default;

        void reset() {
            upper.clear();
            lower.clear();
            last_time = 0;
            last_value = 0;
            last_delta = 0;
        }

        // current line; undefined behavior if empty
        line current() const {
            return {last_time, median()};
        }

        // insert (time, [value - delta, value + delta])
        //     if null intersection, return (last_time, (m, b))
        //     else return (0, _)
        line insert(uint32_t time, int32_t value, double delta = 0) {
            line result = {0, {}};
            if(upper.empty()) [[unlikely]] {
                // require initialization
                // set delta to 0 to force recalculation
                initialize_polygon(time, value, 0);
            } else {
                node& l = upper.front();
                node& r = upper.back();
                if(compare(r, time, value - delta) < 0 ||
                   compare(l, time, value + delta) > 0) {
                    // null intersection
                    result = current();
                    initialize_polygon(time, value, delta);
                } else {
                    if(compare(r, time, value + delta) > 0) {
                        // upper half-plane clips through polygon
                        auto dr_it = lower.rbegin();
                        node new_r = intersect(dr_it, time, value + delta);
                        lower.erase(dr_it.base(), lower.end());
                        lower.push_back(new_r);

                        auto ur_it = upper.rbegin();
                        node new_u = intersect(ur_it, time, value + delta);
                        upper.erase(ur_it.base(), upper.end());
                        upper.push_back(new_u);
                        upper.push_back(new_r);
                    }
                    if(compare(l, time, value - delta) < 0) {
                        // lower half-plane clips through polygon
                        auto ul_it = upper.begin();
                        node new_l = intersect(ul_it, time, value - delta);
                        upper.erase(upper.begin(), ul_it);
                        upper.push_front(new_l);

                        auto dl_it = lower.begin();
                        node new_d = intersect(dl_it, time, value - delta);
                        lower.erase(lower.begin(), dl_it);
                        lower.push_front(new_d);
                        lower.push_front(new_l);
                    }
                }
            }

            last_time = time;
            last_value = value;
            last_delta = delta;
            return result;
        }
    };

} // PersistCMS

#endif //PERSIST_CMS_POLYGON_H
