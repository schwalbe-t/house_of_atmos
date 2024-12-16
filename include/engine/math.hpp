
#pragma once

#include <type_traits>
#include <cmath>
#include <iostream>
#include <cstring>
#include <span>
#include <vector>
#include <cassert>
#include "nums.hpp"


namespace houseofatmos::engine {

    const f64 pi = 3.141592653589793238463;


    template<size_t N, typename T = f64>
    struct Vec {
        T elements[N];

        Vec() {
            static_assert(N >= 1, "Must at least have one element!");
            for(size_t i = 0; i < N; i += 1) {
                this->elements[i] = 0.0;
            }
        }

        template<
            typename... Args,
            typename = typename std::enable_if<sizeof...(Args) == N>::type
        >
        Vec(Args... values) {
            static_assert(N >= 1, "Must at least have one element!");
            T args[] = { static_cast<T>(values)... };
            for(size_t i = 0; i < N; i += 1) {
                this->elements[i] = args[i];
            }
        }

        Vec(std::span<const T> values) {
            assert(values.size() == N);
            for(size_t i = 0; i < N; i += 1) {
                this->elements[i] = values[i];
            }
        }

        T& operator[](size_t index) {
            return this->elements[index];
        }
        const T& operator[](size_t index) const {
            return this->elements[index];
        }

        T& x() {
            return this->elements[0]; 
        }
        T& y() { 
            static_assert(N >= 2, "Must at least have 2 elements!");
            return this->elements[1]; 
        }
        T& z() { 
            static_assert(N >= 3, "Must at least have 3 elements!");
            return this->elements[2]; 
        }
        T& w() {
            static_assert(N >= 4, "Must at least have 4 elements!");
            return this->elements[3]; 
        }
        const T& x() const { 
            return this->elements[0]; 
        }
        const T& y() const { 
            static_assert(N >= 2, "Must at least have 2 elements!");
            return this->elements[1]; 
        }
        const T& z() const { 
            static_assert(N >= 3, "Must at least have 3 elements!");
            return this->elements[2]; 
        }
        const T& w() const { 
            static_assert(N >= 4, "Must at least have 4 elements!");
            return this->elements[3]; 
        }

        T& r() { 
            return this->elements[0];
        }
        T& g() { 
            static_assert(N >= 2, "Must at least have 2 elements!");
            return this->elements[1];
        }
        T& b() { 
            static_assert(N >= 3, "Must at least have 3 elements!");
            return this->elements[2]; 
        }
        T& a() { 
            static_assert(N >= 4, "Must at least have 4 elements!");
            return this->elements[3]; 
        }
        const T& r() const { 
            return this->elements[0];
        }
        const T& g() const { 
            static_assert(N >= 2, "Must at least have 2 elements!");
            return this->elements[1];
        }
        const T& b() const { 
            static_assert(N >= 3, "Must at least have 3 elements!");
            return this->elements[2]; 
        }
        const T& a() const { 
            static_assert(N >= 4, "Must at least have 4 elements!");
            return this->elements[3]; 
        }

        template<size_t L>
        Vec<L> swizzle(const char elements[L + 1]) {
            static_assert(L >= 1, "Must at least have one element!");
            Vec<L> result = Vec<L>();
            for(size_t i = 0; i < L; i += 1) {
                size_t index;
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

        Vec<N + 1> with(T value) const {
            Vec<N + 1> result = Vec<N + 1>();
            memcpy(result.elements, this->elements, sizeof(T) * N);
            result.elements[N] = value;
            return result;
        }

        T min() const {
            T min = INFINITY;
            for(size_t i = 0; i < N; i += 1) {
                T element = this->elements[i];
                if(element < min) {
                    min = element;
                }
            }
            return min;
        }

        T max() const {
            T max = -INFINITY;
            for(size_t i = 0; i < N; i += 1) {
                T element = this->elements[i];
                if(element > max) {
                    max = element;
                }
            }
            return max;
        }

        T sum() const {
            T sum = 0.0;
            for(size_t i = 0; i < N; i += 1) {
                sum += this->elements[i];
            }
            return sum;
        }

        Vec<N> operator+(const Vec<N>& other) const {
            Vec<N> sum = *this;
            for(size_t i = 0; i < N; i += 1) {
                sum.elements[i] += other.elements[i];
            }
            return sum;
        }

        Vec<N> operator-(const Vec<N>& other) const {
            Vec<N> difference = *this;
            for(size_t i = 0; i < N; i += 1) {
                difference.elements[i] -= other.elements[i];
            }
            return difference;
        }

        Vec<N> operator*(const Vec<N>& other) const {
            Vec<N> product = *this;
            for(size_t i = 0; i < N; i += 1) {
                product.elements[i] *= other.elements[i];
            }
            return product;
        }

        Vec<N> operator*(const T scalar) const {
            Vec<N> scaled = *this;
            for(size_t i = 0; i < N; i += 1) {
                scaled.elements[i] *= scalar;
            }
            return scaled;
        }

        Vec<N> operator/(const Vec<N>& other) const {
            Vec<N> quotient = *this;
            for(size_t i = 0; i < N; i += 1) {
                quotient.elements[i] /= other.elements[i];
            }
            return quotient;
        }

        Vec<N> operator/(const T scalar) const {
            Vec<N> scaled = *this;
            for(size_t i = 0; i < N; i += 1) {
                scaled.elements[i] /= scalar;
            }
            return scaled;
        }

        Vec<N> operator-() const {
            return *this * -1;
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

        Vec<N>& operator*=(T scalar) {
            *this = *this * scalar;
            return *this;
        }

        Vec<N>& operator/=(const Vec<N>& other) {
            *this = *this / other;
            return *this;
        }

        Vec<N>& operator/=(T scalar) {
            *this = *this * scalar;
            return *this;
        }


        Vec<N> abs() const {
            Vec<N> absolute = *this;
            for(size_t i = 0; i < N; i += 1) {
                if(absolute.elements[i] >= 0.0) { continue; }
                absolute.elements[i] *= -1.0;
            }
            return absolute;
        }

        T len() const {
            return sqrt((*this * *this).sum());
        }

        Vec<N> normalized() const {
            T length = this->len();
            if(length == 0.0) { return *this; }
            return *this * (1.0 / length);
        }

        Vec<N> cross(const Vec<N>& rhs) const { 
            static_assert(N == 3, "Both vectors must only have 3 elements!");
            return Vec<3>(
                (this->y() * rhs.z() - this->z() * rhs.y()),
                (this->x() * rhs.z() - this->z() * rhs.x()) * -1,
                (this->x() * rhs.y() - this->y() * rhs.x())
            );
        }

        T dot(const Vec<N>& rhs) const {
            return (*this * rhs).sum();
        }

    };

    template<size_t N>
    using IVec = Vec<N, i64>;

    template<size_t N>
    using UVec = Vec<N, u64>;

    

    template<size_t N>
    std::ostream& operator<<(std::ostream& outs, const Vec<N>& v) {
        outs << "[";
        for(size_t i = 0; i < N; i += 1) {
            if(i > 0) { outs << ", "; }
            outs << v[i];
        }
        outs << "]";
        return outs;
    }


    template<size_t R, size_t C = R>
    struct Mat {
        Vec<R> columns[C];

        Mat() {
            static_assert(R >= 1, "Matrix must have at least 1 row!");
            static_assert(C >= 1, "Matrix must have at least 1 column!");
            for(size_t i = 0; i < R && i < C; i += 1) {
                this->element(i, i) = 1.0;
            }
        }

        template<
            typename... Args,
            typename = typename std::enable_if<sizeof...(Args) == R * C>::type
        >
        Mat(Args... values) {
            static_assert(R >= 1, "Matrix must have at least 1 row!");
            static_assert(C >= 1, "Matrix must have at least 1 column!");
            f64 args[] = { static_cast<f64>(values)... };
            for(size_t row_i = 0; row_i < R; row_i += 1) {
                for(size_t column_i = 0; column_i < C; column_i += 1) {
                    size_t args_i = row_i * C + column_i;
                    this->element(row_i, column_i) = args[args_i];
                }
            }
        }

        template<typename T>
        static Mat<R, C> from_column_major(std::span<const T> values) {
            assert(values.size() == R * C);
            Mat<R, C> result;
            for(size_t column_i = 0; column_i < C; column_i += 1) {
                for(size_t row_i = 0; row_i < R; row_i += 1) {
                    result.element(row_i, column_i)
                        = values[column_i * R + row_i];
                }
            }
            return result;
        }

        template<typename T>
        static Mat<R, C> from_row_major(std::span<const T> values) {
            assert(values.size() == R * C);
            Mat<R, C> result;
            for(size_t row_i = 0; row_i < R; row_i += 1) {
                for(size_t column_i = 0; column_i < C; column_i += 1) {
                    result.element(row_i, column_i)
                        = values[row_i * C + column_i];
                }
            }
            return result;
        }

        template<typename T>
        std::vector<T> as_column_major() const {
            std::vector<T> result;
            result.reserve(R * C);
            for(size_t column_i = 0; column_i < C; column_i += 1) {
                for(size_t row_i = 0; row_i < R; row_i += 1) {
                    result.push_back(this->element(row_i, column_i));
                }
            }
            return result;
        }

        template<typename T>
        std::vector<T> as_row_major() const {
            std::vector<T> result;
            result.reserve(R * C);
            for(size_t row_i = 0; row_i < R; row_i += 1) {
                for(size_t column_i = 0; column_i < C; column_i += 1) {
                    result.push_back(this->element(row_i, column_i));
                }
            }
            return result;
        }

        static Mat<R> rotate_x(f64 angle_rad) {
            static_assert(R == C, "Must be a square matrix!");
            static_assert(R >= 3, "Matrix size must at least be 3!");
            Mat<R> result = Mat<R>();
            result.element(1, 1) =  cos(angle_rad);
            result.element(2, 1) =  sin(angle_rad);
            result.element(1, 2) = -sin(angle_rad);
            result.element(2, 2) =  cos(angle_rad);
            return result;
        }

        static Mat<R> rotate_y(f64 angle_rad) {
            static_assert(R == C, "Must be a square matrix!");
            static_assert(R >= 3, "Matrix size must at least be 3!");
            Mat<R> result = Mat<R>();
            result.element(0, 0) =  cos(angle_rad);
            result.element(2, 0) =  sin(angle_rad);
            result.element(0, 2) = -sin(angle_rad);
            result.element(2, 2) =  cos(angle_rad);
            return result;
        }

        static Mat<R> rotate_z(f64 angle_rad) {
            static_assert(R == C, "Must be a square matrix!");
            static_assert(R >= 2, "Matrix size must at least be 2!");
            Mat<R> result = Mat<R>();
            result.element(0, 0) =  cos(angle_rad);
            result.element(1, 0) =  sin(angle_rad);
            result.element(0, 1) = -sin(angle_rad);
            result.element(1, 1) =  cos(angle_rad);
            return result;
        }

        static Mat<R> quaternion_xyzw(f64 x, f64 y, f64 z, f64 w) {
            static_assert(R >= 3, "Matrix size must at least be 3!");
            Mat<R> result = Mat<R>();
            result.element(0, 0) = 1 - 2 * (y * y + z * z);
            result.element(0, 1) =     2 * (x * y - w * z);
            result.element(0, 2) =     2 * (x * z + w * y);
            result.element(1, 0) =     2 * (x * y + w * z);
            result.element(1, 1) = 1 - 2 * (x * x + z * z);
            result.element(1, 2) =     2 * (y * z - w * x);
            result.element(2, 0) =     2 * (x * z - w * y);
            result.element(2, 1) =     2 * (y * z + w * x);
            result.element(2, 2) = 1 - 2 * (x * x + y * y);
            return result;
        }

        static Mat<R> quaternion_xyzw(const Vec<4>& q) {
            static_assert(R >= 3, "Matrix size must at least be 3!");
            return Mat<R>::quaternion_xyzw(q.x(), q.y(), q.z(), q.w());
        }

        template<size_t N>
        static Mat<R> scale(const Vec<N>& scalars) {
            static_assert(R == C, "Must be a square matrix!");
            static_assert(R >= 1, "Matrix size must at least be 1!");
            static_assert(N <= R, "Scalars must fit inside the Matrix!");
            Mat<R> result = Mat<R>();
            for(size_t i = 0; i < N; i += 1) {
                result.element(i, i) = scalars[i];
            }
            return result;
        }

        template<size_t N>
        static Mat<R> translate(const Vec<N>& offsets) {
            static_assert(R >= 1, "Matrix size must at least be 1!");
            static_assert(N <= R, "Offsets must fit inside the Matrix!");
            Mat<R> result = Mat<R>();
            for(size_t row_i = 0; row_i < N; row_i += 1) {
                result.element(row_i, C - 1) = offsets[row_i];
            }
            return result;
        }

        static Mat<4> look_at(
            const Vec<3>& eye, const Vec<3>& at, const Vec<3>& up
        ) {
            Vec<3> forward = (at - eye).normalized();
            Vec<3> right = up.cross(forward).normalized();
            Vec<3> c_up = forward.cross(right).normalized();
            return Mat<4>(
                   right.x(),    right.y(),    right.z(),   -right.dot(eye),
                    c_up.x(),     c_up.y(),     c_up.z(),    -c_up.dot(eye),
                -forward.x(), -forward.y(), -forward.z(),  forward.dot(eye),
                         0.0,          0.0,          0.0,               1.0        
            );
        }

        static Mat<4> orthographic(
            f64 left, f64 right, f64 top, f64 bottom,
            f64 near, f64 far
        ) {
            f64 m00 = 2.0 / (right - left);
            f64 m11 = 2.0 / (top - bottom);
            f64 m22 = 2.0 / (near - far);
            f64 m03 = (right + left) / (left - right);
            f64 m13 = (top + bottom) / (bottom - top);
            f64 m23 = (far + near) / (near - far);
            return Mat<4>(
                m00, 0.0, 0.0, m03,
                0.0, m11, 0.0, m13,
                0.0, 0.0, m22, m23,
                0.0, 0.0, 0.0, 1.0
            );
        }

        static Mat<4> perspective(
            f64 fov, i64 width, i64 height, f64 near, f64 far
        ) {
            f64 aspect_ratio = (f64) width / height;
            f64 focal_length = 1.0 / tan(fov / 2.0);
            f64 m00 = focal_length / aspect_ratio;
            f64 m11 = focal_length;
            f64 m22 = (far + near) / (near - far);
            f64 m23 = (2.0 * far * near) / (near - far);
            return Mat<4>(
                m00, 0.0,  0.0, 0.0,
                0.0, m11,  0.0, 0.0,
                0.0, 0.0,  m22, m23,
                0.0, 0.0, -1.0, 0.0
            );
        }

        Vec<C> operator[](size_t index) const {
            Vec<C> row = Vec<C>();
            for(size_t column_i = 0; column_i < C; column_i += 1) {
                row[column_i] = this->columns[column_i][index];
            }
            return row;
        }

        f64& element(size_t row, size_t column) {
            return this->columns[column][row];
        }
        const f64& element(size_t row, size_t column) const {
            return this->columns[column][row];
        }


        Vec<R> operator*(const Vec<C>& rhs) const {
            Vec<R> transformed = Vec<R>();
            for(size_t column_i = 0; column_i < C; column_i += 1) {
                transformed += this->columns[column_i] * rhs[column_i];
            }
            return transformed;
        }

        template<size_t N>
        Mat<R, N> operator*(const Mat<C, N>& rhs) const {
            Mat<R, N> composition = Mat<R, N>();
            for(size_t row_i = 0; row_i < R; row_i += 1) {
                for(size_t column_i = 0; column_i < N; column_i += 1) {
                    composition.element(row_i, column_i)
                        = (*this)[row_i].dot(rhs.columns[column_i]);
                }
            }
            return composition;
        }
        
        Mat<R, C> operator*(const f64& rhs) const {
            Mat<R, C> scaled = Mat<R, C>();
            for(size_t row_i = 0; row_i < R; row_i += 1) {
                for(size_t column_i = 0; column_i < C; column_i += 1) {
                    scaled.element(row_i, column_i) 
                        = this->element(row_i, column_i) * rhs;
                }
            }
            return scaled;
        }

        Mat<R, C> operator+(const Mat<R, C>& rhs) const {
            Mat<R, C> sum = Mat<R, C>();
            for(size_t row_i = 0; row_i < R; row_i += 1) {
                for(size_t column_i = 0; column_i < C; column_i += 1) {
                    sum.element(row_i, column_i) = this->element(row_i, column_i)
                        + rhs.element(row_i, column_i);
                }
            }
            return sum;
        }

        Mat<C, R> transposed() const {
            Mat<C, R> result;
            for(size_t row_i = 0; row_i < R; row_i += 1) {
                for(size_t column_i = 0; column_i < C; column_i += 1) {
                    result.element(column_i, row_i)
                        = this->element(row_i, column_i);
                }
            }
            return result;
        }

    };

}