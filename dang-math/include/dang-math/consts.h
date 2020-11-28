#pragma once

#include "utils.h"

#include "dang-utils/enum.h"

#include "enums.h"
#include "vector.h"

namespace dang::math {

// --- AxisVector

template <std::size_t Dim>
inline constexpr auto AxisVector = nullptr;

template <>
inline constexpr dutils::EnumArray<Axis1, ivec1> AxisVector<1> = {ivec1(1)};

template <>
inline constexpr dutils::EnumArray<Axis2, ivec2> AxisVector<2> = {ivec2(1, 0), ivec2(0, 1)};

template <>
inline constexpr dutils::EnumArray<Axis3, ivec3> AxisVector<3> = {ivec3(1, 0, 0), ivec3(0, 1, 0), ivec3(0, 0, 1)};

inline constexpr auto AxisVector1 = AxisVector<1>;
inline constexpr auto AxisVector2 = AxisVector<2>;
inline constexpr auto AxisVector3 = AxisVector<3>;

// --- FacingVector

template <std::size_t Dim>
inline constexpr auto FacingVector = nullptr;

template <>
inline constexpr dutils::EnumArray<Facing1, ivec1> FacingVector<1> = {ivec1(-1), ivec1(+1)};

template <>
inline constexpr dutils::EnumArray<Facing2, ivec2> FacingVector<2> = {
    ivec2(-1, 0), ivec2(+1, 0), ivec2(0, -1), ivec2(0, +1)};

template <>
inline constexpr dutils::EnumArray<Facing3, ivec3> FacingVector<3> = {
    ivec3(-1, 0, 0), ivec3(+1, 0, 0), ivec3(0, -1, 0), ivec3(0, +1, 0), ivec3(0, 0, -1), ivec3(0, 0, +1)};

inline constexpr auto FacingVector1 = FacingVector<1>;
inline constexpr auto FacingVector2 = FacingVector<2>;
inline constexpr auto FacingVector3 = FacingVector<3>;

// --- CornerVector

template <std::size_t Dim>
inline constexpr auto CornerVector = nullptr;

template <>
inline constexpr dutils::EnumArray<Corner1, ivec1> CornerVector<1> = {ivec1(0), ivec1(1)};

template <>
inline constexpr dutils::EnumArray<Corner2, ivec2> CornerVector<2> = {
    ivec2(0, 0), ivec2(1, 0), ivec2(0, 1), ivec2(1, 1)};

template <>
inline constexpr dutils::EnumArray<Corner3, ivec3> CornerVector<3> = {ivec3(0, 0, 0),
                                                                      ivec3(1, 0, 0),
                                                                      ivec3(0, 1, 0),
                                                                      ivec3(1, 1, 0),
                                                                      ivec3(0, 0, 1),
                                                                      ivec3(1, 0, 1),
                                                                      ivec3(0, 1, 1),
                                                                      ivec3(1, 1, 1)};

inline constexpr auto CornerVector1 = CornerVector<1>;
inline constexpr auto CornerVector2 = CornerVector<2>;
inline constexpr auto CornerVector3 = CornerVector<3>;

// --- AxisFacing

template <std::size_t Dim>
inline constexpr auto AxisFacing = nullptr;

template <>
inline constexpr dutils::EnumArray<Axis1, Facing1> AxisFacing<1> = {Facing1::Right};

template <>
inline constexpr dutils::EnumArray<Axis2, Facing2> AxisFacing<2> = {Facing2::Right, Facing2::Up};

template <>
inline constexpr dutils::EnumArray<Axis3, Facing3> AxisFacing<3> = {Facing3::Right, Facing3::Up, Facing3::Front};

inline constexpr auto AxisFacing1 = AxisFacing<1>;
inline constexpr auto AxisFacing2 = AxisFacing<2>;
inline constexpr auto AxisFacing3 = AxisFacing<3>;

// --- FacingFlipped

template <std::size_t Dim>
inline constexpr auto FacingFlipped = nullptr;

template <>
inline constexpr dutils::EnumArray<Facing1, Facing1> FacingFlipped<1> = {Facing1::Right, Facing1::Left};

template <>
inline constexpr dutils::EnumArray<Facing2, Facing2> FacingFlipped<2> = {
    Facing2::Right, Facing2::Left, Facing2::Up, Facing2::Down};

template <>
inline constexpr dutils::EnumArray<Facing3, Facing3> FacingFlipped<3> = {
    Facing3::Right, Facing3::Left, Facing3::Up, Facing3::Down, Facing3::Front, Facing3::Back};

inline constexpr auto FacingFlipped1 = FacingFlipped<1>;
inline constexpr auto FacingFlipped2 = FacingFlipped<2>;
inline constexpr auto FacingFlipped3 = FacingFlipped<3>;

} // namespace dang::math
