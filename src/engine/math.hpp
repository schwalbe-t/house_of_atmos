
#pragma once

#include <type_traits>
#include <cmath>
#include <iostream>
#include <cstring>

namespace houseofatmos::engine::math {

    template<int N>
    struct Vec {
        double elements[N];

        Vec() {
            static_assert(N >= 1, "Must at least have one element!");
            for(int i = 0; i < N; i += 1) {
                this->elements[i] = 0.0;
            }
        }

        template<
            typename... Args,
            typename = typename std::enable_if<sizeof...(Args) == N>::type
        >
        Vec(Args... values) {
            static_assert(N >= 1, "Must at least have one element!");
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

        double& x() {
            return this->elements[0]; 
        }
        double& y() { 
            static_assert(N >= 2, "Must at least have 2 elements!");
            return this->elements[1]; 
        }
        double& z() { 
            static_assert(N >= 3, "Must at least have 3 elements!");
            return this->elements[2]; 
        }
        double& w() {
            static_assert(N >= 4, "Must at least have 4 elements!");
            return this->elements[3]; 
        }
        const double& x() const { 
            return this->elements[0]; 
        }
        const double& y() const { 
            static_assert(N >= 2, "Must at least have 2 elements!");
            return this->elements[1]; 
        }
        const double& z() const { 
            static_assert(N >= 3, "Must at least have 3 elements!");
            return this->elements[2]; 
        }
        const double& w() const { 
            static_assert(N >= 4, "Must at least have 4 elements!");
            return this->elements[3]; 
        }

        double& r() { 
            return this->elements[0];
        }
        double& g() { 
            static_assert(N >= 2, "Must at least have 2 elements!");
            return this->elements[1];
        }
        double& b() { 
            static_assert(N >= 3, "Must at least have 3 elements!");
            return this->elements[2]; 
        }
        double& a() { 
            static_assert(N >= 4, "Must at least have 4 elements!");
            return this->elements[3]; 
        }
        const double& r() const { 
            return this->elements[0];
        }
        const double& g() const { 
            static_assert(N >= 2, "Must at least have 2 elements!");
            return this->elements[1];
        }
        const double& b() const { 
            static_assert(N >= 3, "Must at least have 3 elements!");
            return this->elements[2]; 
        }
        const double& a() const { 
            static_assert(N >= 4, "Must at least have 4 elements!");
            return this->elements[3]; 
        }

        template<int L>
        Vec<L> swizzle(const char elements[L + 1]) {
            static_assert(L >= 1, "Must at least have one element!");
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

        Vec<N + 1> with(double value) const {
            Vec<N + 1> result = Vec<N + 1>();
            memcpy(result.elements, this->elements, sizeof(double) * N);
            result.elements[N] = value;
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

        Vec<N> operator*(const double scalar) const {
            Vec<N> scaled = *this;
            for(int i = 0; i < N; i += 1) {
                scaled.elements[i] *= scalar;
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

        Vec<N> operator/(const double scalar) const {
            Vec<N> scaled = *this;
            for(int i = 0; i < N; i += 1) {
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

        Vec<N>& operator*=(double scalar) {
            *this = *this * scalar;
            return *this;
        }

        Vec<N>& operator/=(const Vec<N>& other) {
            *this = *this / other;
            return *this;
        }

        Vec<N>& operator/=(double scalar) {
            *this = *this * scalar;
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
            return sqrt((*this * *this).sum());
        }

        Vec<N> normalized() const {
            double length = this->len();
            if(length == 0.0) { return *this; }
            return *this * (1.0 / length);
        }

        Vec<N> cross(const Vec<N>& rhs) { 
            static_assert(N == 3, "Both vectors must only have 3 elements!");
            return Vec<3>(
                (this->y() * rhs.z() - this->z() * rhs.y()),
                (this->x() * rhs.z() - this->z() * rhs.x()) * -1,
                (this->x() * rhs.y() - this->y() * rhs.x())
            );
        }

        double dot(const Vec<N>& rhs) {
            return (*this * rhs).sum();
        }

        Color as_color() {
            static_assert(N == 3 || N == 4, "Must have 3 or 4 elements!");
            Color result = WHITE;
            result.r = this->r() * 255;
            result.g = this->g() * 255;
            result.b = this->b() * 255;
            if(N == 4) {
                result.a = this->a() * 255;
            }
            return result;
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
            static_assert(R >= 1, "Matrix must have at least 1 row!");
            static_assert(C >= 1, "Matrix must have at least 1 column!");
            for(int i = 0; i < R && i < C; i += 1) {
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
            double args[] = { static_cast<double>(values)... };
            for(int row_i = 0; row_i < R; row_i += 1) {
                for(int column_i = 0; column_i < C; column_i += 1) {
                    int args_i = row_i * C + column_i;
                    this->element(row_i, column_i) = args[args_i];
                }
            }
        }

        static Mat<R> rotate_x(double angle_rad) {
            static_assert(R == C, "Must be a square matrix!");
            static_assert(R >= 3, "Matrix size must at least be 3!");
            Mat<R> result = Mat<R>();
            result.element(1, 1) =  cos(angle_rad);
            result.element(2, 1) =  sin(angle_rad);
            result.element(1, 2) = -sin(angle_rad);
            result.element(2, 2) =  cos(angle_rad);
            return result;
        }

        static Mat<R> rotate_y(double angle_rad) {
            static_assert(R == C, "Must be a square matrix!");
            static_assert(R >= 3, "Matrix size must at least be 3!");
            Mat<R> result = Mat<R>();
            result.element(0, 0) =  cos(angle_rad);
            result.element(2, 0) =  sin(angle_rad);
            result.element(0, 2) = -sin(angle_rad);
            result.element(2, 2) =  cos(angle_rad);
            return result;
        }

        static Mat<R> rotate_z(double angle_rad) {
            static_assert(R == C, "Must be a square matrix!");
            static_assert(R >= 2, "Matrix size must at least be 2!");
            Mat<R> result = Mat<R>();
            result.element(0, 0) =  cos(angle_rad);
            result.element(1, 0) =  sin(angle_rad);
            result.element(0, 1) = -sin(angle_rad);
            result.element(1, 1) =  cos(angle_rad);
            return result;
        }

        template<int N>
        static Mat<R> scale(Vec<N> scalars) {
            static_assert(R == C, "Must be a square matrix!");
            static_assert(R >= 1, "Matrix size must at least be 1!");
            static_assert(N <= R, "Scalars must fit inside the Matrix!");
            Mat<R> result = Mat<R>();
            for(int i = 0; i < N; i += 1) {
                result.element(i, i) = scalars[i];
            }
            return result;
        }

        template<int N>
        static Mat<R> translate(Vec<N> offsets) {
            static_assert(R >= 1, "Matrix size must at least be 1!");
            static_assert(N <= R, "Offsets must fit inside the Matrix!");
            Mat<R> result = Mat<R>();
            for(int row_i = 0; row_i < N; row_i += 1) {
                result.element(row_i, C - 1) = offsets[row_i];
            }
            return result;
        }

        static Mat<4> look_at(Vec<3> eye, Vec<3> at, Vec<3> up) {
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
            double left, double right, double top, double bottom,
            double near, double far
        ) {
            double m00 = 2.0 / (right - left);
            double m11 = 2.0 / (top - bottom);
            double m22 = 2.0 / (near - far);
            double m03 = (right + left) / (left - right);
            double m13 = (top + bottom) / (bottom - top);
            double m23 = (far + near) / (near - far);
            return Mat<4>(
                m00, 0.0, 0.0, m03,
                0.0, m11, 0.0, m13,
                0.0, 0.0, m22, m23,
                0.0, 0.0, 0.0, 1.0
            );
        }

        static Mat<4> perspective(
            double fov, int width, int height, double near, double far
        ) {
            double aspect_ratio = (double) width / height;
            double focal_length = 1.0 / tan(fov / 2.0);
            double m00 = focal_length / aspect_ratio;
            double m11 = focal_length;
            double m22 = (far + near) / (near - far);
            double m23 = (2.0 * far * near) / (near - far);
            return Mat<4>(
                m00, 0.0,  0.0, 0.0,
                0.0, m11,  0.0, 0.0,
                0.0, 0.0,  m22, m23,
                0.0, 0.0, -1.0, 0.0
            );
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
            for(int row_i = 0; row_i < R; row_i += 1) {
                for(int column_i = 0; column_i < N; column_i += 1) {
                    composition.element(row_i, column_i)
                        = (*this)[row_i].dot(rhs.columns[column_i]);
                }
            }
            return composition;
        }

    };

}