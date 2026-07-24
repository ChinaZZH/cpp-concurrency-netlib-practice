#pragma once

#include <cstdint>
#include <cmath>
#include <limits>
#include <type_traits>
#include <cassert>


// Q16.48 定点数：64位有符号整数，低40位为小数部分
// 小数部分40为，整数部分23位，最高位为符号位
class Fixed {
public:
    using RawType = int64_t;
    static constexpr int FRACTION_BITS = 40; 
    static constexpr RawType SCALE = (1LL << FRACTION_BITS);
    static constexpr RawType ROUND_HALF = SCALE / 2;

    // 直接利用浮点数在编译期计算，彻底避免手动写魔数
    static constexpr double PI_DOUBLE = 3.14159265358979323846;

    // ---------- 构造函数 ----------
    Fixed() :raw_(0) {} 

    // 乘以SCALE就相当于把数左移FRACTION_BITS位。
    explicit Fixed(int32_t v): raw_(static_cast<RawType>(v) * SCALE) {
        // Debug 模式下检查溢出
        assert(v >= -8388607 && v <= 8388607 && "Integer part overflow in Q24.40");
    }  

    explicit Fixed(float v) {
        // Debug 模式下检查溢出
        assert(v >= -8388607.0f && v <= 8388607.0f && "Integer part overflow in Q24.40");

        float extraAdd = ((v >= 0.0f)? 0.5f : -0.5f);
        raw_ = static_cast<RawType>(v * SCALE + extraAdd);
    } 

    explicit Fixed(double v) {
        // Debug 模式下检查溢出
        assert(v >= -8388607.0f && v <= 8388607.0f && "Integer part overflow in Q24.40");

        double extraAdd = ((v >= 0.0f)? 0.5f : -0.5f);
        raw_ = static_cast<RawType>(v * SCALE + extraAdd);
    } 

    // ---------- 转换 -----------------------------------------------------------
    static Fixed FromRaw(RawType raw)
    {
        Fixed f;
        f.raw_ = raw;
        return f;
    }

    
    RawType Raw() const { return raw_; }
    float ToFloat() const   { return static_cast<float>(raw_)  / SCALE; }
    double ToDouble() const { return static_cast<double>(raw_) / SCALE; }

    // ---------- 常量 ---------（基于 Q24.40：SCALE = 2^40 = 1099511627776）-----
    static Fixed Zero()  { return FromRaw(0); }
    static Fixed One()   { return FromRaw(SCALE); }

    // Pi * 2^40 ≈ 3.141592653589793 * 1099511627776 = 3454188663838.66
    // 四舍五入取整为 3454188663839
    static Fixed Pi() 
    {
        auto raw = static_cast<RawType>(PI_DOUBLE * SCALE + 0.5f); 
        return FromRaw(raw); 
    }

    // PiHalf * 2^40 ≈ 1.5707963267948965 * 1099511627776 = 1727094331919.33
    // 四舍五入取整为 1727094331919
    static Fixed PiHalf() 
    { 
        auto raw = static_cast<RawType>(PI_DOUBLE * ROUND_HALF + 0.5f); 
        return FromRaw(raw); 
    }


    // ---------- 算术运算符 ----------
    Fixed operator-() const { return FromRaw(-raw_); }
    
    Fixed& operator+=(const Fixed& other) { this->raw_ += other.raw_; return *this;}
    Fixed& operator-=(const Fixed& other) { this->raw_ -= other.raw_; return *this;}
    Fixed& operator*=(const Fixed& other) {
        // 关键：使用 __int128 防止溢出
        __int128_t result = static_cast<__int128_t>(this->raw_) * static_cast<__int128_t>(other.raw_);
        // 由于两个数都乘以SCALE， 这个时候相乘就相当 原数A*scale*原数B*scale 多了一个scale所以通过/scale除法来算的最正确的数值。
        result = result / SCALE; 
        this->raw_ = static_cast<RawType>(result);
        return *this;
    }

    Fixed& operator/=(const Fixed& other) {
        assert(other.raw_ != 0 && "Divide by zero in Fixed operator/="); // 调试模式下触发断点，Release下移除
        __int128_t result = (static_cast<__int128_t>(this->raw_) * SCALE) / (other.raw_);
        this->raw_ = static_cast<RawType>(result);
        return *this;
    }

    friend Fixed operator+(Fixed a, const Fixed& b)     { a += b; return a; }
    friend Fixed operator-(Fixed a, const Fixed& b)     { a -= b; return a; }
    friend Fixed operator*(Fixed a, const Fixed& b)     { a *= b; return a; }
    friend Fixed operator/(Fixed a, const Fixed& b)     { a /= b; return a; }

    // ---------- 比较运算符 ----------
    friend bool operator==(const Fixed& a, const Fixed& b)  { return a.raw_ == b.raw_; }
    friend bool operator!=(const Fixed& a, const Fixed& b)  { return a.raw_ != b.raw_; }
    friend bool operator<(const Fixed& a, const Fixed& b)   { return a.raw_ < b.raw_; }
    friend bool operator>(const Fixed& a, const Fixed& b)   { return a.raw_ > b.raw_; }
    friend bool operator<=(const Fixed& a, const Fixed& b)  { return a.raw_ <= b.raw_; }
    friend bool operator>=(const Fixed& a, const Fixed& b)  { return a.raw_ >= b.raw_; }

private:
    RawType raw_;
};