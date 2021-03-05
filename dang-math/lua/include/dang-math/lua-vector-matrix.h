#pragma once

#include "dang-lua/State.h"

#include "dang-math/matrix.h"
#include "dang-math/vector.h"

#include "dang-utils/utils.h"

namespace dang::lua {

template <typename T, std::size_t v_dim>
struct ClassInfo<dang::math::Vector<T, v_dim>> {
    using Vector = dang::math::Vector<T, v_dim>;
    using VectorOrScalar = std::variant<Vector, T>;

    using Swizzled = std::variant<T, dang::math::Vector<T, 2>, dang::math::Vector<T, 3>, dang::math::Vector<T, 4>>;
    using Key = std::variant<std::size_t, std::string_view>;

    using MultiplyType = std::conditional_t<std::is_floating_point_v<T>,
                                            std::variant<T,
                                                         Vector,
                                                         dang::math::Matrix<T, 2, v_dim>,
                                                         dang::math::Matrix<T, 3, v_dim>,
                                                         dang::math::Matrix<T, 4, v_dim>>,
                                            std::variant<T, Vector>>;

    using MultiplyResult = std::conditional_t<
        std::is_floating_point_v<T>,
        std::variant<T,
                     dang::math::Vector<T, 2>,
                     dang::math::Vector<T, 3>,
                     dang::math::Vector<T, 4>,
                     dang::math::Matrix<T, 2, v_dim>,
                     dang::math::Matrix<T, 3, v_dim>,
                     dang::math::Matrix<T, 4, v_dim>>,
        std::variant<T, dang::math::Vector<T, 2>, dang::math::Vector<T, 3>, dang::math::Vector<T, 4>>>;

    using DivideType = std::conditional_t<std::is_floating_point_v<T>,
                                          std::variant<T, Vector, dang::math::Matrix<T, v_dim>>,
                                          std::variant<T, Vector>>;

    using DivideResult =
        std::conditional_t<std::is_floating_point_v<T>,
                           std::variant<T, std::optional<Vector>, std::optional<dang::math::Matrix<T, v_dim>>>,
                           std::variant<T, std::optional<Vector>>>;

    inline static const std::string base_class_name = [] {
        using namespace std::literals;
        if constexpr (std::is_same_v<T, float>)
            return "vec"s;
        else if constexpr (std::is_same_v<T, double>)
            return "dvec"s;
        else if constexpr (std::is_same_v<T, int>)
            return "ivec"s;
        else if constexpr (std::is_same_v<T, unsigned>)
            return "uvec"s;
        else if constexpr (std::is_same_v<T, std::size_t>)
            return "svec"s;
        else if constexpr (std::is_same_v<T, bool>)
            return "bvec"s;
        else
            static_assert(dutils::always_false_v<T>, "unsupported vector type");
    }();

    inline static const std::string class_name = base_class_name + std::to_string(v_dim);
    inline static const std::string class_name_ref = class_name + '&';

    static const char* className() { return class_name.c_str(); }
    static const char* classNameRef() { return class_name_ref.c_str(); }

    static constexpr auto table()
    {
        constexpr auto set = +[](Vector& vec, Args<v_dim> values) {
            std::transform(values.begin(), values.end(), vec.begin(), ArgCheck<T>{});
        };

        constexpr auto copy = +[](const Vector& vec) { return vec; };

        constexpr auto unpack = +[](const Vector& vec) { return unpackHelper(vec, std::make_index_sequence<v_dim>{}); };

        std::vector result{reg<set>("set"),
                           reg<copy>("copy"),
                           reg<unpack>("unpack"),
                           reg<&Vector::lessThan>("lessThan"),
                           reg<&Vector::lessThanEqual>("lessThanEqual"),
                           reg<&Vector::greaterThan>("greaterThan"),
                           reg<&Vector::greaterThanEqual>("greaterThanEqual"),
                           reg<&Vector::equal>("equal"),
                           reg<&Vector::notEqual>("notEqual"),
                           reg<&Vector::format>("format")};

        if constexpr (std::is_floating_point_v<T>) {
            result.push_back(reg<&Vector::length>("length"));
            result.push_back(reg<&Vector::normalize>("normalize"));
            result.push_back(reg<&Vector::distanceTo>("distanceTo"));
            result.push_back(reg<&Vector::cosAngleTo>("cosAngleTo"));
            result.push_back(reg<&Vector::radiansTo>("radiansTo"));
            result.push_back(reg<&Vector::degreesTo>("degreesTo"));
            result.push_back(reg<&Vector::radians>("radians"));
            result.push_back(reg<&Vector::degrees>("degrees"));
            result.push_back(reg<&Vector::floor>("floor"));
            result.push_back(reg<&Vector::ceil>("ceil"));

            if constexpr (v_dim == 2) {
                constexpr auto cross = +[](const Vector& vec, const std::optional<Vector>& other) {
                    return other ? vec.cross(*other) : vec.cross();
                };
                result.push_back(reg<cross>("cross"));

                constexpr auto slope = +[](const Vector& vec) { return vec.slope(); };
                result.push_back(reg<slope>("slope"));
            }
        }

        if constexpr (std::is_same_v<T, bool>) {
            result.push_back(reg<&Vector::all>("all"));
            result.push_back(reg<&Vector::any>("any"));
            result.push_back(reg<&Vector::none>("none"));
            result.push_back(reg<&Vector::invert>("invert"));
        }
        else {
            result.push_back(reg<&Vector::sum>("sum"));
            result.push_back(reg<&Vector::product>("product"));
            result.push_back(reg<&Vector::dot>("dot"));
            result.push_back(reg<&Vector::sqrdot>("sqrdot"));
            result.push_back(reg<&Vector::vectorTo>("vectorTo"));
            result.push_back(reg<&Vector::min>("min"));
            result.push_back(reg<&Vector::max>("max"));
            result.push_back(reg<&Vector::clamp>("clamp"));
            result.push_back(reg<&Vector::reflect>("reflect"));

            if constexpr (v_dim == 3) {
                constexpr auto cross = +[](const Vector& lhs, const Vector& rhs) { return lhs.cross(rhs); };
                result.push_back(reg<cross>("cross"));
            }

            if constexpr (std::is_signed_v<T>) {
                result.push_back(reg<&Vector::abs>("abs"));
            }
        }

        return result;
    }

    static constexpr auto metatable()
    {
        constexpr auto len = +[](const Vector&) { return v_dim; };

        constexpr auto eq = +[](const Vector& lhs, const Vector& rhs) { return lhs == rhs; };
        constexpr auto lt = +[](const Vector& lhs, const Vector& rhs) { return lhs < rhs; };
        constexpr auto le = +[](const Vector& lhs, const Vector& rhs) { return lhs <= rhs; };

        constexpr auto index = +[](State& lua, const Vector& vec, Key key) { return std::visit(Index{lua, vec}, key); };

        constexpr auto newindex = +[](State& lua, Vector& vec, Key key, const Swizzled& value) {
            std::visit(NewIndex{lua, vec, value}, key);
        };

        constexpr auto pairs = +[](State& lua, Arg vector) {
            constexpr auto next = +[](State& lua, Arg table, Arg key) {
                auto result = table.next(std::move(key));
                return result ? VarArgs(*result) : VarArgs(lua(nullptr));
            };
            auto metatable = vector.getMetatable();
            return std::tuple{wrap<next>, metatable ? (*metatable)["indextable"] : lua.pushNil()};
        };

        std::vector result{reg<&Vector::format>("__tostring"),
                           reg<len>("__len"),
                           reg<eq>("__eq"),
                           reg<lt>("__lt"),
                           reg<le>("__le"),
                           reg<index>("__index"),
                           reg<newindex>("__newindex"),
                           reg<pairs>("__pairs")};

        if constexpr (!std::is_same_v<T, bool>) {
            constexpr auto add = +[](const VectorOrScalar& lhs, const VectorOrScalar& rhs) {
                return std::visit([](const auto& a, const auto& b) -> VectorOrScalar { return a + b; }, lhs, rhs);
            };
            constexpr auto sub = +[](const VectorOrScalar& lhs, const VectorOrScalar& rhs) {
                return std::visit([](const auto& a, const auto& b) -> VectorOrScalar { return a - b; }, lhs, rhs);
            };
            constexpr auto mul = +[](const VectorOrScalar& lhs, const MultiplyType& rhs) {
                return std::visit([](const auto& a, const auto& b) -> MultiplyResult { return a * b; }, lhs, rhs);
            };
            constexpr auto div = +[](const VectorOrScalar& lhs, const DivideType& rhs) {
                return std::visit([](const auto& a, const auto& b) -> DivideResult { return a / b; }, lhs, rhs);
            };

            result.push_back(reg<add>("__add"));
            result.push_back(reg<sub>("__sub"));
            result.push_back(reg<mul>("__mul"));
            result.push_back(reg<div>("__div"));

            if constexpr (std::is_signed_v<T>) {
                constexpr auto unm = +[](const Vector& vec) { return -vec; };
                result.push_back(reg<unm>("__unm"));
            }
        }

        return result;
    }

    static auto require(State& lua)
    {
        constexpr auto create = +[](State& lua, Arg, VarArgs values) {
            if (values.size() == 0)
                return Vector();
            if (values.size() == 1)
                return Vector(values[0].check<T>());
            if (values.size() == v_dim) {
                Vector result;
                std::transform(values.begin(), values.end(), result.begin(), ArgCheck<T>{});
                return result;
            }

            if constexpr (v_dim == 0)
                lua.error("0 parameters expected, got " + std::to_string(values.size()));
            else if constexpr (v_dim == 1)
                lua.error("0 or 1 parameters expected, got " + std::to_string(values.size()));
            else
                lua.error("0, 1 or " + std::to_string(v_dim) + " parameters expected, got " +
                          std::to_string(values.size()));
        };

        auto result = lua.pushTable();

        if constexpr (v_dim == 2) {
            if constexpr (!std::is_same_v<T, bool>) {
                constexpr auto from_slope = +[](std::optional<T> slope) { return Vector::fromSlope(slope); };
                result.rawSetTable("fromSlope", wrap<from_slope>);
            }

            if constexpr (std::is_floating_point_v<T>) {
                constexpr auto from_angle_rad = +[](T radians) { return Vector::fromRadians(radians); };
                result.rawSetTable("fromRadians", wrap<from_angle_rad>);

                constexpr auto from_angle = +[](T degrees) { return Vector::fromDegrees(degrees); };
                result.rawSetTable("fromDegrees", wrap<from_angle>);
            }
        }

        auto result_mt = lua.pushTable();
        result_mt.rawSetTable("__call", wrap<create>);

        result.setMetatable(std::move(result_mt));
        return result;
    }

private:
    template <std::size_t... v_indices>
    static auto unpackHelper(const Vector& vector, std::index_sequence<v_indices...>)
    {
        return std::tuple{std::get<v_indices>(vector)...};
    }

    static std::optional<int> axisToIndex(char axis)
    {
        if constexpr (v_dim >= 1 && v_dim <= 4)
            if (axis == 'x')
                return 0;
        if constexpr (v_dim >= 2 && v_dim <= 4)
            if (axis == 'y')
                return 1;
        if constexpr (v_dim >= 3 && v_dim <= 4)
            if (axis == 'z')
                return 2;
        if constexpr (v_dim >= 4 && v_dim <= 4)
            if (axis == 'w')
                return 3;
        return std::nullopt;
    }

    struct Index {
        State& lua;
        const Vector& vector;

        template <std::size_t... v_indices, typename... TSwizzles>
        std::optional<Swizzled> accessHelper(std::index_sequence<v_indices...>, TSwizzles... swizzle) const
        {
            auto indices = std::array{axisToIndex(swizzle)...};
            if ((!std::get<v_indices>(indices) || ...))
                return std::nullopt;

            if constexpr (sizeof...(TSwizzles) == 1)
                return vector[*std::get<0>(indices)];
            else
                return dang::math::Vector<T, sizeof...(TSwizzles)>(vector[*std::get<v_indices>(indices)]...);
        }

        template <typename... TSwizzles>
        std::optional<Swizzled> access(TSwizzles... swizzle) const
        {
            return accessHelper(std::index_sequence_for<TSwizzles...>{}, swizzle...);
        }

        std::optional<Swizzled> operator()(std::string_view key) const
        {
            switch (key.size()) {
            case 1:
                return access(key[0]);
            case 2:
                return access(key[0], key[1]);
            case 3:
                return access(key[0], key[1], key[2]);
            case 4:
                return access(key[0], key[1], key[2], key[3]);
            }
            return std::nullopt;
        }

        std::optional<Swizzled> operator()(std::size_t index) const
        {
            if (index >= 1 && index <= v_dim)
                return vector[index - 1];
            return std::nullopt;
        }
    };

    struct NewIndex {
        State& lua;
        Vector& vector;
        const Swizzled& value;

        template <std::size_t... v_indices, typename... TSwizzles>
        void accessHelper(std::index_sequence<v_indices...>, TSwizzles... swizzle) const
        {
            auto indices = std::array{axisToIndex(swizzle)...};
            if ((!std::get<v_indices>(indices) || ...))
                lua.argError(2, "invalid swizzle");

            if (auto opt_value = std::get_if<T>(&value)) {
                ((vector[*std::get<v_indices>(indices)] = *opt_value), ...);
                return;
            }

            if constexpr (sizeof...(TSwizzles) > 1) {
                if (auto opt_values = std::get_if<dang::math::Vector<T, sizeof...(TSwizzles)>>(&value)) {
                    ((vector[*std::get<v_indices>(indices)] = (*opt_values)[v_indices]), ...);
                    return;
                }
            }

            lua.argError(2, "swizzle mismatch");
        }

        template <typename... TSwizzles>
        void access(TSwizzles... swizzle) const
        {
            accessHelper(std::index_sequence_for<TSwizzles...>{}, swizzle...);
        }

        void operator()(std::string_view key)
        {
            switch (key.size()) {
            case 1:
                access(key[0]);
                return;
            case 2:
                access(key[0], key[1]);
                return;
            case 3:
                access(key[0], key[1], key[2]);
                return;
            case 4:
                access(key[0], key[1], key[2], key[3]);
                return;
            }
            lua.argError(2, "invalid swizzle");
        }

        void operator()(std::size_t index)
        {
            if (index < 1 || index > v_dim)
                lua.argError(2, "index out of range");
            if (auto opt_value = std::get_if<T>(&value))
                vector[index - 1] = *opt_value;
            else
                lua.argError(2, "single value expected, got vector");
        }
    };
};

template <typename T, std::size_t v_cols, std::size_t v_rows>
struct ClassInfo<dang::math::Matrix<T, v_cols, v_rows>> {
    using Matrix = dang::math::Matrix<T, v_cols, v_rows>;
    using MatrixOrScalar = std::variant<Matrix, T>;
    using IndexOrPos = std::variant<std::size_t, dang::math::svec2>;
    using IndexResult = std::variant<std::monostate, T, dang::math::Vector<T, v_rows>>;

    using MultiplyType = std::variant<T,
                                      dang::math::Vector<T, v_cols>,
                                      dang::math::Matrix<T, 2, v_cols>,
                                      dang::math::Matrix<T, 3, v_cols>,
                                      dang::math::Matrix<T, 4, v_cols>>;
    using MultiplyResult = std::variant<T,
                                        dang::math::Vector<T, 2>,
                                        dang::math::Vector<T, 3>,
                                        dang::math::Vector<T, 4>,
                                        dang::math::Matrix<T, 2>,
                                        dang::math::Matrix<T, 2, 3>,
                                        dang::math::Matrix<T, 2, 4>,
                                        dang::math::Matrix<T, 3, 2>,
                                        dang::math::Matrix<T, 3>,
                                        dang::math::Matrix<T, 3, 4>,
                                        dang::math::Matrix<T, 4, 2>,
                                        dang::math::Matrix<T, 4, 3>,
                                        dang::math::Matrix<T, 4>>;

    using DivideType = std::variant<T, dang::math::Matrix<T, v_cols>>;
    using DivideResult =
        std::conditional_t<v_cols == v_rows,
                           std::variant<T, std::optional<Matrix>>,
                           std::variant<T, std::optional<Matrix>, std::optional<dang::math::Matrix<T, v_cols>>>>;

    inline static const std::string base_class_name = [] {
        using namespace std::literals;
        if constexpr (std::is_same_v<T, float>)
            return "mat"s;
        else if constexpr (std::is_same_v<T, double>)
            return "dmat"s;
        else
            static_assert(dutils::always_false_v<T>, "unsupported matrix type");
    }();

    inline static const std::string class_name =
        base_class_name + std::to_string(v_cols) + (v_cols != v_rows ? 'x' + std::to_string(v_rows) : "");
    inline static const std::string class_name_ref = class_name + '&';

    static const char* className() { return class_name.c_str(); }
    static const char* classNameRef() { return class_name_ref.c_str(); }

    static constexpr auto table()
    {
        constexpr auto set = +[](Matrix& mat, Args<v_cols> values) {
            std::transform(values.begin(), values.end(), mat.begin(), ArgCheck<dang::math::Vector<T, v_rows>>{});
        };

        constexpr auto copy = +[](const Matrix& mat) { return mat; };

        constexpr auto get_at = +[](const Matrix& mat, std::size_t col, std::size_t row) -> std::optional<T> {
            if (inRange(col, row))
                return mat(col - 1, row - 1);
            return std::nullopt;
        };

        constexpr auto set_at = +[](State& lua, Matrix& mat, std::size_t col, std::size_t row, T value) {
            checkRange(lua, col, row, 2, 3);
            mat(col - 1, row - 1) = value;
        };

        constexpr auto cofactor_at = +[](State& lua, const Matrix& mat, std::size_t col, std::size_t row) {
            checkRange(lua, col, row, 2, 3);
            return mat.cofactor(col - 1, row - 1);
        };

        constexpr auto cofactor = +[](State& lua, const Matrix& mat, dang::math::svec2 pos) {
            checkRange(lua, pos, 2, 2);
            return mat.cofactor(pos);
        };

        std::vector result{reg<set>("set"),
                           reg<copy>("copy"),
                           reg<get_at>("getAt"),
                           reg<set_at>("setAt"),
                           reg<&Matrix::format>("format"),
                           reg<&Matrix::transpose>("transpose"),
                           reg<cofactor_at>("cofactorAt"),
                           reg<cofactor>("cofactor"),
                           reg<&Matrix::cofactorMatrix>("cofactorMatrix"),
                           reg<&Matrix::adjugate>("adjugate"),
                           reg<&Matrix::determinant>("determinant"),
                           reg<&Matrix::solvable>("solvable"),
                           reg<&Matrix::compMul>("compMul"),
                           reg<&Matrix::compDiv>("compDiv")};

        // Matrix parameters for solve functions are not const, to enable in-place calculation
        // (swaps columns around, but reverts to the original at the end)

        if constexpr (v_cols == v_rows + 1) {
            constexpr auto solve_col = +[](State& lua, Matrix& mat, std::size_t col) {
                checkColumn(lua, col, 2);
                return mat.solveCol(col - 1);
            };
            result.push_back(reg<solve_col>("solveCol"));

            constexpr auto solve = +[](Matrix& mat) { return mat.solve(); };
            result.push_back(reg<solve>("solve"));
        }

        if constexpr (v_cols == v_rows) {
            result.push_back(reg<&Matrix::inverse>("inverse"));

            constexpr auto solve_col =
                +[](State& lua, Matrix& mat, std::size_t col, const dang::math::Vector<T, v_cols>& vec) {
                    checkColumn(lua, col, 2);
                    return mat.solveCol(col - 1, vec);
                };
            result.push_back(reg<solve_col>("solveCol"));

            constexpr auto solve =
                +[](Matrix& mat, const dang::math::Vector<T, v_cols>& vec) { return mat.solve(vec); };
            result.push_back(reg<solve>("solve"));
        }
        return result;
    }

    static constexpr auto metatable()
    {
        constexpr auto add = +[](const MatrixOrScalar& lhs, const MatrixOrScalar& rhs) {
            return std::visit([](const auto& a, const auto& b) -> MatrixOrScalar { return a + b; }, lhs, rhs);
        };
        constexpr auto sub = +[](const MatrixOrScalar& lhs, const MatrixOrScalar& rhs) {
            return std::visit([](const auto& a, const auto& b) -> MatrixOrScalar { return a - b; }, lhs, rhs);
        };
        constexpr auto mul = +[](const MatrixOrScalar& lhs, const MultiplyType& rhs) {
            return std::visit([](const auto& a, const auto& b) -> MultiplyResult { return a * b; }, lhs, rhs);
        };
        constexpr auto div = +[](const MatrixOrScalar& lhs, const DivideType& rhs) {
            return std::visit([](const auto& a, const auto& b) -> DivideResult { return a / b; }, lhs, rhs);
        };

        constexpr auto len = +[](const Matrix&) { return v_cols; };

        constexpr auto eq = +[](const Matrix& lhs, const Matrix& rhs) { return lhs == rhs; };
        constexpr auto lt = +[](const Matrix& lhs, const Matrix& rhs) { return lhs < rhs; };
        constexpr auto le = +[](const Matrix& lhs, const Matrix& rhs) { return lhs <= rhs; };

        constexpr auto index = +[](const Matrix& mat, const IndexOrPos& key) { return std::visit(Index{mat}, key); };
        constexpr auto newindex = +[](State& lua, Matrix& mat, const IndexOrPos& key, Arg value) {
            std::visit(NewIndex{lua, mat, value}, key);
        };

        constexpr auto pairs = +[](State& lua, Arg matrix) {
            constexpr auto next = +[](State& lua, Arg table, Arg key) {
                auto result = table.next(std::move(key));
                return result ? VarArgs(*result) : VarArgs(lua(nullptr));
            };
            auto metatable = matrix.getMetatable();
            return std::tuple{wrap<next>, metatable ? (*metatable)["indextable"] : lua.pushNil()};
        };

        std::vector result{reg<&Matrix::format>("__tostring"),
                           reg<add>("__add"),
                           reg<sub>("__sub"),
                           reg<mul>("__mul"),
                           reg<div>("__div"),
                           reg<len>("__len"),
                           reg<eq>("__eq"),
                           reg<lt>("__lt"),
                           reg<le>("__le"),
                           reg<index>("__index"),
                           reg<newindex>("__newindex"),
                           reg<pairs>("__pairs")};

        if constexpr (std::is_signed_v<T>) {
            constexpr auto unm = +[](const Matrix& mat) { return -mat; };
            result.push_back(reg<unm>("__unm"));
        }

        return result;
    }

    static auto require(State& lua)
    {
        constexpr auto create = +[](State& lua, Arg, VarArgs values) {
            using namespace std::literals;
            if (values.size() == 0)
                return Matrix();
            if (values.size() == 1)
                return Matrix(values[0].check<T>());
            if (values.size() == v_cols * v_rows) {
                Matrix result;
                dang::math::sbounds2 bounds{{v_cols, v_rows}};
                for (const auto& [col, row] : bounds)
                    result(col, row) = values[static_cast<int>(col * v_rows + row)].check<T>();
                return result;
            }
            lua.error("0, 1 or "s + std::to_string(v_cols * v_rows) + " arguments expected, got "s +
                      std::to_string(values.size()));
        };

        constexpr auto identity =
            +[](std::optional<T> value) { return value ? Matrix::identity(*value) : Matrix::identity(); };

        auto result = lua.pushTable();
        result.rawSetTable("identity", wrap<identity>);

        auto result_mt = lua.pushTable();
        result_mt.rawSetTable("__call", wrap<create>);

        result.setMetatable(std::move(result_mt));
        return result;
    }

private:
    static bool columnInRange(std::size_t col) { return col >= 1 && col <= v_cols; }
    static bool rowInRange(std::size_t row) { return row >= 1 && row <= v_rows; }
    static bool inRange(std::size_t col, std::size_t row) { return columnInRange(col) && rowInRange(row); }
    static bool inRange(dang::math::svec2 pos) { return inRange(pos.x(), pos.y()); }

    static void checkColumn(State& lua, std::size_t col, int arg)
    {
        if (col < 1 || col > v_cols)
            lua.argError(arg, "column out of range");
    }

    static void checkRow(State& lua, std::size_t row, int arg)
    {
        if (row < 1 || row > v_rows)
            lua.argError(arg, "row out of range");
    }

    static void checkRange(State& lua, std::size_t col, std::size_t row, int col_arg, int row_arg)
    {
        checkColumn(lua, col, col_arg);
        checkRow(lua, row, row_arg);
    }

    static void checkRange(State& lua, dang::math::svec2 pos, int col_arg, int row_arg)
    {
        checkRange(lua, pos.x(), pos.y(), col_arg, row_arg);
    }

    struct Index {
        const Matrix& matrix;

        IndexResult operator()(std::size_t index) const
        {
            if (index >= 1 && index <= v_cols)
                return matrix[index - 1];
            return {};
        }

        IndexResult operator()(dang::math::svec2 pos) const
        {
            if (pos.greaterThanEqual(1).all() && pos.lessThanEqual({v_cols, v_rows}).all())
                return matrix[pos - 1];
            return {};
        }
    };

    struct NewIndex {
        State& lua;
        Matrix& matrix;
        Arg value;

        void operator()(std::size_t col)
        {
            checkColumn(lua, col, 2);
            matrix[col - 1] = value.check<dang::math::Vector<T, v_rows>>();
        }

        void operator()(dang::math::svec2 pos)
        {
            checkRange(lua, pos, 2, 2);
            matrix[pos - 1] = value.check<T>();
        }
    };
};

} // namespace dang::lua
