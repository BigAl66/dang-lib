#pragma once

#include "utils.h"

namespace dang::math
{

enum class Axis1 : char {
    None = -1,
    X,
    COUNT
};

enum class Axes1 : unsigned char {
    NONE = 0,
    X = 1 << 0,
    ALL = ~(~0 << 1)
};

enum class Axis2 : char {
    None = -1,
    X,
    Y,
    COUNT
};

enum class Axes2 : unsigned char {
    NONE = 0,
    X = 1 << 0,
    Y = 1 << 1,
    ALL = ~(~0 << 2)
};

enum class Axis3 : char {
    None = -1,
    X,
    Y,
    Z,
    COUNT
};

enum class Axes3 : unsigned char {
    NONE = 0,
    X = 1 << 0,
    Y = 1 << 1,
    Z = 1 << 2,
    ALL = ~(~0 << 3)
};

enum class Corner1 : char {
    None = -1,
    Left,
    Right,
    COUNT
};

enum class Corners1 : unsigned char {
    NONE = 0,
    Left = 1 << 0,
    Right = 1 << 1,
    ALL = ~(~0 << 2)
};

enum class Corner2 : char {
    None = -1,
    LeftBottom,
    RightBottom,
    LeftTop,
    RightTop,
    COUNT
};

enum class Corners2 : unsigned char {
    NONE = 0,
    LeftBottom = 1 << 0,
    RightBottom = 1 << 1,
    LeftTop = 1 << 2,
    RightTop = 1 << 3,
    ALL = ~(~0 << 4)
};

enum class Corner3 : char {
    None = -1,
    LeftBottomBack,
    RightBottomBack,
    LeftTopBack,
    RightTopBack,
    LeftBottomFront,
    RightBottomFront,
    LeftTopFront,
    RightTopFront,
    COUNT
};

enum class Corners3 : unsigned char {
    NONE = 0,
    LeftBottomBack = 1 << 0,
    RightBottomBack = 1 << 1,
    LeftTopBack = 1 << 2,
    RightTopBack = 1 << 3,
    LeftBottomFront = 1 << 4,
    RightBottomFront = 1 << 5,
    LeftTopFront = 1 << 6,
    RightTopFront = 1 << 7,
    ALL = ~(~0 << 8)
};

enum class Edge2 : char {
    None = -1,
    Left,
    Right,
    Bottom,
    Top,
    COUNT
};

enum class Edges2 : unsigned char {
    NONE = 0,
    Left = 1 << 0,
    Right = 1 << 1,
    Bottom = 1 << 2,
    Top = 1 << 3,
    ALL = ~(~0 << 4)
};

enum class Edge3 : char {
    None = -1,
    LeftBottom,
    RightBottom,
    LeftTop,
    RightTop,
    BottomBack,
    TopBack,
    BottomFront,
    TopFront,
    LeftFront,
    RightFront,
    LeftBack,
    RightBack,
    COUNT
};

enum class Edges3 : unsigned short {
    NONE = 0,
    LeftBottom = 1 << 0,
    RightBottom = 1 << 1,
    LeftTop = 1 << 2,
    RightTop = 1 << 3,
    BottomBack = 1 << 4,
    TopBack = 1 << 5,
    BottomFront = 1 << 6,
    TopFront = 1 << 7,
    LeftFront = 1 << 8,
    RightFront = 1 << 9,
    LeftBack = 1 << 10,
    RightBack = 1 << 11,
    ALL = ~(~0 << 12)
};

enum class Facing1 : char {
    None = -1,
    Left,
    Right,
    COUNT
};

enum class Facings1 : unsigned char {
    NONE = 0,
    Left = 1 << 0,
    Right = 1 << 1,
    ALL = ~(~0 << 2)
};

enum class Facing2 : char {
    None = -1,
    Left,
    Right,
    Up,
    Down,
    COUNT
};

enum class Facings2 : unsigned char {
    NONE = 0,
    Left = 1 << 0,
    Right = 1 << 1,
    Up = 1 << 2,
    Down = 1 << 3,
    ALL = ~(~0 << 4)
};

enum class Facing3 : char {
    None = -1,
    Left,
    Right,
    Up,
    Down,
    Back,
    Front,
    COUNT
};

enum class Facings3 : unsigned char {
    NONE = 0,
    Left = 1 << 0,
    Right = 1 << 1,
    Up = 1 << 2,
    Down = 1 << 3,
    Back = 1 << 4,
    Front = 1 << 5,
    ALL = ~(~0 << 6)
};

namespace detail
{

template <std::size_t Dim>
struct AxisSelector {
    using Type = void;
    using SetType = void;
};

template <>
struct AxisSelector<1> {
    using Type = Axis1;
    using SetType = Axes1;
};

template <>
struct AxisSelector<2> {
    using Type = Axis2;
    using SetType = Axes2;
};

template <>
struct AxisSelector<3> {
    using Type = Axis3;
    using SetType = Axes3;
};

template <std::size_t Dim>
struct CornerSelector {
    using Type = void;
    using SetType = void;
};

template <>
struct CornerSelector<1> {
    using Type = Corner1;
    using SetType = Corners1;
};

template <>
struct CornerSelector<2> {
    using Type = Corner2;
    using SetType = Corners2;
};

template <>
struct CornerSelector<3> {
    using Type = Corner3;
    using SetType = Corners3;
};

template <std::size_t Dim>
struct EdgeSelector {
    using Type = void;
    using SetType = void;
};

template <>
struct EdgeSelector<2> {
    using Type = Edge2;
    using SetType = Edges2;
};

template <>
struct EdgeSelector<3> {
    using Type = Edge3;
    using SetType = Edges3;
};

template <std::size_t Dim>
struct FacingSelector {
    using Type = void;
    using SetType = void;
};

template <>
struct FacingSelector<1> {
    using Type = Facing1;
    using SetType = Facings1;
};

template <>
struct FacingSelector<2> {
    using Type = Facing2;
    using SetType = Facings2;
};

template <>
struct FacingSelector<3> {
    using Type = Facing3;
    using SetType = Facings3;
};

}

template <std::size_t Dim>
using Axis = typename detail::AxisSelector<Dim>::Type;

template <std::size_t Dim>
using Axes = typename detail::AxisSelector<Dim>::SetType;

template <std::size_t Dim>
using Corner = typename detail::CornerSelector<Dim>::Type;

template <std::size_t Dim>
using Corners = typename detail::CornerSelector<Dim>::SetType;

template <std::size_t Dim>
using Edge = typename detail::EdgeSelector<Dim>::Type;

template <std::size_t Dim>
using Edges = typename detail::EdgeSelector<Dim>::SetType;

template <std::size_t Dim>
using Facing = typename detail::FacingSelector<Dim>::Type;

template <std::size_t Dim>
using Facings = typename detail::FacingSelector<Dim>::SetType;

}
