#pragma once
#ifndef CATA_SRC_WEIGHTED_LIST_H
#define CATA_SRC_WEIGHTED_LIST_H

#include "flexbuffer_json.h"
#include "json.h"
#include "rng.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <functional>
#include <sstream>
#include <vector>

template <typename T, typename W> struct weighted_list {
        weighted_list() : total_weight( 0 ) {}

        weighted_list( const weighted_list & ) = default;
        weighted_list( weighted_list && ) noexcept = default;
        weighted_list &operator=( const weighted_list & ) = default;
        weighted_list &operator=( weighted_list && ) noexcept = default;
        virtual ~weighted_list() = default;







        T *add( const T &obj, const W &weight ) {
            if( weight > 0 ) {
                objects.emplace_back( obj, weight );
                total_weight += weight;
                invalidate_precalc();
                return &( objects[objects.size() - 1].first );
            }
            return nullptr;
        }

        T *add( const std::pair<T, W> &obj_and_weight ) {
            return add( obj_and_weight.first, obj_and_weight.second );
        }






        T *try_add( const T &obj, const W &weight ) {
            if( std::find_if( objects.begin(), objects.end(),
            [&obj]( const std::pair<T, W> &element ) {
            return element.first == obj;
        } ) == objects.end() ) {
                return add( obj, weight );
            }
            return nullptr;
        }

        T *try_add( const std::pair<T, W> &obj_and_weight ) {
            return try_add( obj_and_weight.first, obj_and_weight.second );
        }

        void remove( const T &obj ) {
            const auto remove_with_weight = [&obj, this]
            ( typename decltype( objects )::value_type const & itr ) {
                if( itr.first == obj ) {
                    total_weight -= itr.second;
                    return true;
                }
                return false;
            };
            objects.erase( std::remove_if( objects.begin(),
                                           objects.end(), remove_with_weight ), objects.end() );
            invalidate_precalc();
        }









        T *add_or_replace( const T &obj, const W &weight ) {
            if( weight == 0 ) {
                remove( obj );
                return nullptr;
            }
            if( weight > 0 ) {
                invalidate_precalc();
                for( auto &itr : objects ) {
                    if( itr.first == obj ) {
                        total_weight += ( weight - itr.second );
                        itr.second = weight;
                        return &( itr.first );
                    }
                }


                return add( obj, weight );
            }
            return nullptr;
        }





        void apply( std::function<void( const T & )> func ) const {
            for( auto &itr : objects ) {
                func( itr.first );
            }
        }






        void apply( std::function<void( T & )> func ) {
            for( auto &itr : objects ) {
                func( itr.first );
            }
        }






        const T *pick( unsigned int randi ) const {
            if( total_weight > 0 ) {
                return &( objects[pick_ent( randi )].first );
            } else {
                return nullptr;
            }
        }
        const T *pick() const {
            return pick( rng_bits() );
        }







        T *pick( unsigned int randi ) {
            if( total_weight > 0 ) {
                return &( objects[pick_ent( randi )].first );
            } else {
                return nullptr;
            }
        }
        T *pick() {
            return pick( rng_bits() );
        }




        void clear() {
            total_weight = 0;
            objects.clear();
            invalidate_precalc();
        }





        W get_specific_weight( const T &obj ) const {
            for( const std::pair<T, W> &itr : objects ) {
                if( itr.first == obj ) {
                    return itr.second;
                }
            }
            return 0;
        }




        W get_weight() const {
            return total_weight;
        }

        bool is_valid() const {
            return get_weight() > 0;
        }

        typename std::vector<std::pair<T, W> >::iterator begin() {
            return objects.begin();
        }
        typename std::vector<std::pair<T, W> >::iterator end() {
            return objects.end();
        }
        typename std::vector<std::pair<T, W> >::const_iterator begin() const {
            return objects.begin();
        }
        typename std::vector<std::pair<T, W> >::const_iterator end() const {
            return objects.end();
        }
        typename std::vector<std::pair<T, W> >::iterator erase(
            typename std::vector<std::pair<T, W> >::iterator first,
            typename std::vector<std::pair<T, W> >::iterator last ) {
            invalidate_precalc();
            return objects.erase( first, last );
        }
        size_t size() const noexcept {
            return objects.size();
        }
        bool empty() const noexcept {
            return objects.empty();
        }

        std::string to_debug_string() const {
            std::ostringstream os;
            os << "[ ";
            for( const std::pair<T, W> &o : objects ) {
                os << o.first << ":" << o.second << ", ";
            }
            os << "]";
            return os.str();
        }

        friend bool operator==( const weighted_list &l, const weighted_list &r ) {
            return l.objects == r.objects;
        }

        friend bool operator!=( const weighted_list &l, const weighted_list &r ) {
            return !( l == r );
        }

        void deserialize( const JsonValue &jv ) {


            if( jv.test_array() ) {
                for( const JsonValue entry : jv.get_array() ) {
                    if( entry.test_array() ) {
                        std::pair<T, W> p;
                        entry.read( p, true );
                        add( p.first, p.second );
                    } else {
                        T val;
                        entry.read( val );
                        add( val, default_weight );
                    }
                }
            } else {
                jv.throw_error( "weighted list must be in format [[a, b], …]" );
            }
        }

    protected:
        W total_weight = 0;
        W default_weight = 1;
        std::vector<std::pair<T, W> > objects;

        virtual size_t pick_ent( unsigned int ) const = 0;
        virtual void invalidate_precalc() {}
};

template <typename T> struct weighted_int_list : public weighted_list<T, int> {

        weighted_int_list() = default;
        explicit weighted_int_list( int default_weight ) : default_weight( default_weight ) {};

        void precalc() {
            precalc_array.clear();

            precalc_array.reserve( this->total_weight );

            for( size_t i = 0; i < this->objects.size(); i++ ) {
                precalc_array.resize( precalc_array.size() + this->objects[i].second, i );
            }
        }

    protected:

        size_t pick_ent( unsigned int randi ) const override {
            if( this->objects.size() == 1 ) {
                return 0;
            }
            const int picked = ( randi % ( this->total_weight ) ) + 1;
            if( !precalc_array.empty() ) {

                return precalc_array[picked - 1];
            }

            if( cum_weights.size() != this->objects.size() ) {
                cum_weights.clear();
                cum_weights.reserve( this->objects.size() );
                int acc = 0;
                for( const std::pair<T, int> &o : this->objects ) {
                    acc += o.second;
                    cum_weights.push_back( acc );
                }
            }
            return std::distance( cum_weights.begin(),
                                  std::lower_bound( cum_weights.begin(), cum_weights.end(), picked ) );
        }

        void invalidate_precalc() override {
            precalc_array.clear();
            cum_weights.clear();
        }

        int default_weight = 1;
        std::vector<int> precalc_array;
        mutable std::vector<int> cum_weights;
};

static_assert( std::is_nothrow_move_constructible_v<weighted_int_list<int>> );

template <typename T> struct weighted_float_list : public weighted_list<T, double> {


        weighted_float_list() = default;
        explicit weighted_float_list( int default_weight ) : default_weight( default_weight ) {};

    protected:

        size_t pick_ent( unsigned int randi ) const override {
            const double picked = static_cast<double>( randi ) / UINT_MAX * this->total_weight;
            double accumulated_weight = 0;
            size_t i;
            for( i = 0; i < this->objects.size(); i++ ) {
                accumulated_weight += this->objects[i].second;
                if( accumulated_weight >= picked ) {
                    break;
                }
            }
            return i;
        }

        float default_weight = 1;
};

#endif
