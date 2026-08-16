#pragma once

#include "FixedPoint.h"

// ---------- 超越函数（确定性实现）----------


// 2. Sin/Cos（查表 + 线性插值，确定性最强）
namespace FixedMath 
{
    const Fixed TWO_PI = Fixed(6.283185307f);
    const Fixed PI = Fixed(3.141592654f);
    const Fixed PI_HALF = Fixed(1.570796327f);

     // 1. 先实现角度归一化（修复后的版本）
    inline Fixed NormalizeAngle(Fixed angle) {
        // 取整数倍（向零取整，直接取 raw_ 的整数部分）
        Fixed quotient = angle / TWO_PI;
        int32_t k = quotient.Raw() / Fixed::SCALE; 
        
        // 减去整数倍，压缩到 (-TWO_PI, TWO_PI)
        angle = angle - Fixed(k) * TWO_PI;
        
        // 负数修正，确保最终落在 [0, TWO_PI)
        if (angle < Fixed(0)) {
            angle += TWO_PI;
        }
        return angle;
    }

    // 生成 0~PI/2 的 Sin 表（256 个点）
    // 2. 正弦查找表（在 .cpp 中初始化）
    // 假设我们划分 1024 个点（0 ~ 1024 对应 0 ~ 2PI）
    static constexpr int SIN_TABLE_SIZE = 256;
    static Fixed g_sin_table[SIN_TABLE_SIZE + 1];

    // 初始化表（必须在 main 开始时调用）
    inline void InitSinTable() 
    {
        for(int i = 0; i <= SIN_TABLE_SIZE; ++i)
        {
            double angle = (i / (double)SIN_TABLE_SIZE) * 1.57079632679; // 0 ~ PI/2
            double val = std::sin(angle);
            g_sin_table[i] = Fixed(val);
        }
    }

     // 查表实现 Sin（角度范围：0 ~ 2PI）
    inline Fixed Sin(Fixed angle) {
        // 【关键修改点】这里不再用错误的取模，而是调用修正后的归一化函数
        angle = NormalizeAngle(angle);
        
        // 将 [0, TWO_PI) 映射到 [0, TABLE_SIZE) 的索引范围
        // 使用整数运算避免浮点：index_raw = (angle.raw_ * TABLE_SIZE) / TWO_PI.raw_
        // 但 angle.raw_ * 1024 可能溢出 int32，用 int64 过渡

        // 高效定点映射：等价于 (angle / TWO_PI) * SIN_TABLE_SIZE，但避免了昂贵的 Fixed 除法开销
        int64_t index_raw = (static_cast<int64_t>(angle.Raw()) * SIN_TABLE_SIZE) / TWO_PI.Raw();
        int index = static_cast<int>(index_raw);
        
        // 边界保护（防止 angle == TWO_PI 时的越界）
        if (index >= SIN_TABLE_SIZE) index = SIN_TABLE_SIZE - 1;

        // 查表返回（如果为了极致精度，还可以做相邻两点的线性插值，但为了简便，直接返回值即可）
        return g_sin_table[index];
    }



    inline Fixed Cos(Fixed angle) {
        return Sin(Fixed::PiHalf() - angle);
    }


        // 1. 平方根（牛顿迭代法，整数运算）
    inline Fixed FixedSqrt(Fixed x) {
         if (x < Fixed::Zero()) 
            return Fixed::Zero(); // 负数返回0


            if (x == Fixed::Zero()) 
                return Fixed::Zero();

        // 初始估值：从浮点数借用初始值，但只用整数迭代确保确定性
        // 此处使用整数开方：二分逼近或牛顿迭代
        Fixed::RawType raw = x.Raw();
        Fixed::RawType guess = 1LL << 24; // 初始猜测 1.0 的定点数

        // 牛顿迭代： x_{n+1} = (x_n + raw / x_n) / 2
        // 为保证确定性，固定迭代 20 次
        for (int i = 0; i < 20; ++i) {
            __int128 new_guess = static_cast<__int128>(guess) + static_cast<__int128>(raw) * Fixed::SCALE / guess;
            new_guess /= 2;
            guess = static_cast<Fixed::RawType>(new_guess);
        }

        return Fixed::FromRaw(guess);
    }

}

// namespace FixedMath