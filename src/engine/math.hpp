
#pragma once

#include <type_traits>
#include <cmath>

namespace houseofatmos::engine::math {

    template<int N>
    struct Vec {
        double data[N];

        template<
            typename... Args,
            typename = typename std::enable_if<sizeof...(Args) == N>::type
        >
        Vec(Args... values) {
            double args[] = { static_cast<double>(values)... };
            for(int i = 0; i < N; i += 1) {
                this->data[i] = args[i];
            }
        }

        double x() { return this->data[0]; }
        double y() { return this->data[1]; }
        double z() { return this->data[2]; }
        double w() { return this->data[3]; }

        double r() { return this->data[0]; }
        double g() { return this->data[1]; }
        double b() { return this->data[2]; }
        double a() { return this->data[3]; }

        double min() {
            double min = INFINITY;
            for(int i = 0; i < N; i += 1) {
                double element = this->data[i];
                if(element < min) {
                    min = element;
                }
            }
            return min;
        }

        double max() {
            double max = -INFINITY;
            for(int i = 0; i < N; i += 1) {
                double element = this->data[i];
                if(element > max) {
                    max = element;
                }
            }
            return max;
        }

        double sum() {
            double sum = 0.0;
            for(int i = 0; i < N; i += 1) {
                sum += this->data[i];
            }
            return sum;
        }

        Vec<N> operator+(const Vec<N>& other) {
            Vec<N> sum = *this;
            for(int i = 0; i < N; i += 1) {
                sum.data[i] += other.data[i];
            }
            return sum;
        }

        Vec<N> operator-(const Vec<N>& other) {
            Vec<N> difference = *this;
            for(int i = 0; i < N; i += 1) {
                difference.data[i] -= other.data[i];
            }
            return difference;
        }

        Vec<N> operator*(const Vec<N>& other) {
            Vec<N> product = *this;
            for(int i = 0; i < N; i += 1) {
                product.data[i] *= other.data[i];
            }
            return product;
        }

        Vec<N> operator*(const double factor) {
            Vec<N> scaled = *this;
            for(int i = 0; i < N; i += 1) {
                scaled.data[i] *= factor;
            }
            return scaled;
        }

        Vec<N> operator/(const Vec<N>& other) {
            Vec<N> quotient = *this;
            for(int i = 0; i < N; i += 1) {
                quotient.data[i] /= other.data[i];
            }
            return quotient;
        }

        Vec<N> operator/(const double factor) {
            Vec<N> scaled = *this;
            for(int i = 0; i < N; i += 1) {
                scaled.data[i] /= factor;
            }
            return scaled;
        }

        Vec<N>& operator+=(const Vec<N>& other) {
            *this = *this + other;
            return *this;
        }

        Vec<N>& operator-=(const Vec<N>& other) {
            *this = *this - other;
            return *this;
        }

        Vec<N>& operator*=(const Vec<N>& other) {
            *this = *this * other;
            return *this;
        }

        Vec<N>& operator*=(double factor) {
            *this = *this * factor;
            return *this;
        }

        Vec<N>& operator/=(const Vec<N>& other) {
            *this = *this / other;
            return *this;
        }

        Vec<N>& operator/=(double factor) {
            *this = *this * factor;
            return *this;
        }


        Vec<N> abs() {
            Vec<N> absolute = *this;
            for(int i = 0; i < N; i += 1) {
                if(absolute.data[i] >= 0.0) { continue; }
                absolute.data[i] *= -1.0;
            }
            return absolute;
        }

        double len() {
            return sqrt((this * this).sum());
        }

        Vec<N> normalized() {
            double length = this->len();
            if(length == 0.0) { return *this; }
            return this * (1.0 / length);
        }
    };

}