#pragma once
#ifndef CATA_TESTS_TEST_STATISTICS_H
#define CATA_TESTS_TEST_STATISTICS_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

#include "cata_catch.h"


constexpr double Z95 = 1.96;
constexpr double Z99 = 2.576;
constexpr double Z99_9 = 3.291;
constexpr double Z99_99 = 3.891;
constexpr double Z99_999 = 4.5;
constexpr double Z99_999_9 = 5.0;



struct epsilon_threshold {
    double midpoint;
    double epsilon;
};


struct upper_lower_threshold {
    double lower_thresh;
    double upper_thresh;
};



constexpr double invalid_err = -1;

template<typename T>
class statistics
{
    private:
        int _types;
        int _n;
        double _sum;
        mutable double _error;
        const double Z_;
        const double Zsq_;
        T _max;
        T _min;
        std::vector< T > samples;
    public:
        explicit statistics( const double Z = Z99_9 ) :
            _types( 0 ), _n( 0 ), _sum( 0 ), _error( invalid_err ),
            Z_( Z ),  Zsq_( Z * Z ), _max( std::numeric_limits<T>::min() ),
            _min( std::numeric_limits<T>::max() ) {}

        void new_type() {
            _types++;
        }
        void add( T new_val ) {
            _error = invalid_err;
            _n++;
            _sum += new_val;
            _max = std::max( _max, new_val );
            _min = std::min( _min, new_val );
            samples.push_back( new_val );
        }













        template<typename U = T>
        std::enable_if_t< std::is_same_v< U, bool >, double >
        margin_of_error() const {
            if( _error != invalid_err ) {
                return _error;
            }

            const double adj_numerator = ( Zsq_ / 2 ) + _sum;
            const double adj_denominator = Zsq_ + _n;
            const double adj_proportion = adj_numerator / adj_denominator;
            const double a = adj_proportion * ( 1.0 - adj_proportion );
            const double b = a / adj_denominator;
            const double c = std::sqrt( b );
            _error = c * Z_;
            return _error;
        }






        template<typename U = T>
        std::enable_if_t < ! std::is_same_v< U, bool >, double >
        margin_of_error() const {
            if( _error != invalid_err ) {
                return _error;
            }
            const double std_err = stddev() / std::sqrt( _n );
            _error = std_err * Z_;
            return _error;
        }






        bool uncertain_about( const epsilon_threshold &t ) const {
            return !test_threshold( t ) &&
                   t.midpoint - t.epsilon < upper() &&
                   t.midpoint + t.epsilon > lower();
        }

        bool test_threshold( const epsilon_threshold &t ) const {
            return ( t.midpoint - t.epsilon ) < lower() &&
                   ( t.midpoint + t.epsilon ) > upper();
        }
        bool test_threshold( const upper_lower_threshold &t ) const {
            return t.lower_thresh < lower() && t.upper_thresh > upper();
        }
        double upper() const {
            double result = avg() + margin_of_error();
            if( std::is_same_v<T, bool> ) {
                result = std::min( result, 1.0 );
            }
            return result;
        }
        double lower() const {
            double result = avg() - margin_of_error();
            if( std::is_same_v<T, bool> ) {
                result = std::max( result, 0.0 );
            }
            return result;
        }


        bool test_confidence_interval( const double v ) const {
            return is_within_epsilon( v, margin_of_error() );
        }

        bool is_within_epsilon( const double v, const double epsilon ) const {
            const double average = avg();
            return( average + epsilon > v ) &&
                  ( average - epsilon < v );
        }





        double variance( const bool sample_variance = true ) const {
            double average = avg();
            double sigma_acc = 0;

            for( const T v : samples ) {
                const double diff = v - average;
                sigma_acc += diff * diff;
            }

            if( sample_variance ) {
                return sigma_acc / static_cast<double>( _n - 1 );
            }
            return sigma_acc / static_cast<double>( _n );
        }




        double stddev( const bool sample_deviation = true ) const {
            return std::sqrt( variance( sample_deviation ) );
        }

        int types() const {
            return _types;
        }
        double sum() const {
            return _sum;
        }
        T max() const {
            return _max;
        }
        T min() const {
            return _min;
        }
        double avg() const {
            return _sum / static_cast<double>( _n );
        }
        int n() const {
            return _n;
        }
        const std::vector<T> &get_samples() const {
            return samples;
        }
};

class BinomialMatcher : public Catch::MatcherBase<int>
{
    public:
        BinomialMatcher( int num_samples, double p, double max_deviation );
        bool match( const int &obs ) const override;
        std::string describe() const override;
    private:
        int num_samples_;
        double p_;
        double max_deviation_;
        double expected_;
        double margin_;
};




inline BinomialMatcher IsBinomialObservation(
    const int num_samples, const double p, const double max_deviation = Z99_999 )
{
    return BinomialMatcher( num_samples, p, max_deviation );
}

#endif
