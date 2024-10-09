
#pragma once

#include <type_traits>
#include <cmath>
#include <iostream>

namespace houseofatmos::engine::math {

    template<int N>
    struct Vec {
        double elements[N];

        Vec() {
            for(int i = 0; i < N; i += 1) {
                this->elements[i] = 0.0;
            }
        }

        template<
            typename... Args,
            typename = typename std::enable_if<sizeof...(Args) == N>::type
        >
        Vec(Args... values) {
            double args[] = { static_cast<double>(values)... };
            for(int i = 0; i < N; i += 1) {
                this->elements[i] = args[i];
            }
        }

        double& operator[](int index) {
            return this->elements[index];
        }
        const double& operator[](int index) const {
            return this->elements[index];
        }

        double& x() { return this->elements[0]; }
        double& y() { return this->elements[1]; }
        double& z() { return this->elements[2]; }
        double& w() { return this->elements[3]; }
        const double& x() const { return this->elements[0]; }
        const double& y() const { return this->elements[1]; }
        const double& z() const { return this->elements[2]; }
        const double& w() const { return this->elements[3]; }

        double& r() { return this->elements[0]; }
        double& g() { return this->elements[1]; }
        double& b() { return this->elements[2]; }
        double& a() { return this->elements[3]; }
        const double& r() const { return this->elements[0]; }
        const double& g() const { return this->elements[1]; }
        const double& b() const { return this->elements[2]; }
        const double& a() const { return this->elements[3]; }

        template<int L>
        Vec<L> swizzle(const char elements[L + 1]) {
            Vec<L> result = Vec<L>();
            for(int i = 0; i < L; i += 1) {
                int index;
                switch(elements[i]) {
                    case 'x': index = 0; break;
                    case 'y': index = 1; break;
                    case 'z': index = 2; break;
                    case 'w': index = 3; break;
                    case 'r': index = 0; break;
                    case 'g': index = 1; break;
                    case 'b': index = 2; break;
                    case 'a': index = 3; break;
                    default:
                        std::cout << "'" << elements[i] 
                            << "' is not a valid element for a " << L
                            << "-dimensional vector!" << std::endl;
                        std::abort();
                }
                result[i] = this->elements[index];
            }
            return result;
        }

        double min() const {
            double min = INFINITY;
            for(int i = 0; i < N; i += 1) {
                double element = this->elements[i];
                if(element < min) {
                    min = element;
                }
            }
            return min;
        }

        double max() const {
            double max = -INFINITY;
            for(int i = 0; i < N; i += 1) {
                double element = this->elements[i];
                if(element > max) {
                    max = element;
                }
            }
            return max;
        }

        double sum() const {
            double sum = 0.0;
            for(int i = 0; i < N; i += 1) {
                sum += this->elements[i];
            }
            return sum;
        }

        Vec<N> operator+(const Vec<N>& other) const {
            Vec<N> sum = *this;
            for(int i = 0; i < N; i += 1) {
                sum.elements[i] += other.elements[i];
            }
            return sum;
        }

        Vec<N> operator-(const Vec<N>& other) const {
            Vec<N> difference = *this;
            for(int i = 0; i < N; i += 1) {
                difference.elements[i] -= other.elements[i];
            }
            return difference;
        }

        Vec<N> operator*(const Vec<N>& other) const {
            Vec<N> product = *this;
            for(int i = 0; i < N; i += 1) {
                product.elements[i] *= other.elements[i];
            }
            return product;
        }

        Vec<N> operator*(const double factor) const {
            Vec<N> scaled = *this;
            for(int i = 0; i < N; i += 1) {
                scaled.elements[i] *= factor;
            }
            return scaled;
        }

        Vec<N> operator/(const Vec<N>& other) const {
            Vec<N> quotient = *this;
            for(int i = 0; i < N; i += 1) {
                quotient.elements[i] /= other.elements[i];
            }
            return quotient;
        }

        Vec<N> operator/(const double factor) const {
            Vec<N> scaled = *this;
            for(int i = 0; i < N; i += 1) {
                scaled.elements[i] /= factor;
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


        Vec<N> abs() const {
            Vec<N> absolute = *this;
            for(int i = 0; i < N; i += 1) {
                if(absolute.elements[i] >= 0.0) { continue; }
                absolute.elements[i] *= -1.0;
            }
            return absolute;
        }

        double len() const {
            return sqrt((this * this).sum());
        }

        Vec<N> normalized() const {
            double length = this->len();
            if(length == 0.0) { return *this; }
            return this * (1.0 / length);
        }

    };

    template<int N>
    std::ostream& operator<<(std::ostream& outs, const Vec<N>& v) {
        outs << "[";
        for(int i = 0; i < N; i += 1) {
            if(i > 0) { outs << ", "; }
            outs << v[i];
        }
        outs << "]";
        return outs;
    }


    template<int R, int C = R>
    struct Mat {
        Vec<R> columns[C];

        Mat() {
            for(int i = 0; i < R && i < C; i += 1) {
                this->columns[i][i] = 1.0;
            }
        }

        template<
            typename... Args,
            typename = typename std::enable_if<sizeof...(Args) == R * C>::type
        >
        Mat(Args... values) {
            double args[] = { static_cast<double>(values)... };
            for(int row_i = 0; row_i < R; row_i += 1) {
                for(int column_i = 0; column_i < C; column_i += 1) {
                    int args_i = row_i * C + column_i;
                    this->columns[column_i][row_i] = args[args_i];
                }
            }
        }

        Vec<C> operator[](int index) const {
            Vec<C> row = Vec<C>();
            for(int column_i = 0; column_i < C; column_i += 1) {
                row[column_i] = this->columns[column_i][index];
            }
            return row;
        }

        double& element(int row, int column) {
            return this->columns[column][row];
        }
        const double& element(int row, int column) const {
            return this->columns[column][row];
        }


        Vec<R> operator*(const Vec<C>& rhs) const {
            Vec<R> transformed = Vec<R>();
            for(int column_i = 0; column_i < C; column_i += 1) {
                transformed += this->columns[column_i] * rhs[column_i];
            }
            return transformed;
        }

        template<int N>
        Mat<R, N> operator*(const Mat<C, N>& rhs) const {
            Mat<R, N> composition = Mat<R, N>();
            for(int column_i = 0; column_i < N; column_i += 1) {
                composition.columns[column_i] = *this * rhs.columns[column_i];
            }
            return composition;
        }

    };

}