#pragma once

#include "dang-lua/global.h"

#include "dang-utils/utils.h"

namespace dang::lua {

/// @brief Serves as a container for typenames that represent subclasses.
template <typename... TBases>
struct SubClassList {};

/// @brief Can be specialized to provide information about the subclasses of a given class.
template <typename T>
struct SubClasses : SubClassList<> {};

template <typename T>
constexpr SubClasses<T> SubClassesOf{};

struct Property {
    const char* name;
    lua_CFunction get = nullptr;
    lua_CFunction set = nullptr;
};

/// @brief Returns an empty index and metatable and does nothing when required.
/// @remark className() will be used in error messages.
struct DefaultClassInfo {
    // static constexpr const char* className();

    static constexpr auto allow_table_initialization = false;

    static constexpr std::array<luaL_Reg, 0> table() { return {}; }
    static constexpr std::array<luaL_Reg, 0> metatable() { return {}; }
    static constexpr std::array<Property, 0> properties() { return {}; }

    static void require() {}
};

/// @brief Can be specialized to provide an index and metatable of a wrapped class.
template <typename T>
struct ClassInfo {
    static_assert(dutils::always_false_v<T>, "Type has no ClassInfo specialization.");
};

/// @brief Shorthand to access the index table of a wrapped class.
template <typename T>
const auto class_table = ClassInfo<T>::table();

/// @brief Shorthand to access the metatable of a wrapped class.
template <typename T>
const auto class_metatable = ClassInfo<T>::metatable();

/// @brief Shorthand to access the properties of a wrapped class.
template <typename T>
const auto class_properties = ClassInfo<T>::properties();

/// @brief Can be specialized to provide an array of string names for a given enum to convert from and to Lua.
/// @remark The array needs to end with a "null" entry.
template <typename T>
inline constexpr const char* enum_values[1]{};

namespace detail {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-noreturn"
#endif

[[noreturn]] inline void noreturn_lua_error(lua_State* state) { lua_error(state); }

[[noreturn]] inline void noreturn_luaL_error(lua_State* state, const char* message)
{
    lua_pushstring(state, message);
    lua_error(state);
}

[[noreturn]] inline void noreturn_luaL_typeerror(lua_State* state, int arg, const char* type_name)
{
    luaL_typeerror(state, arg, type_name);
}

[[noreturn]] inline void noreturn_luaL_argerror(lua_State* state, int arg, const char* extra_message)
{
    luaL_argerror(state, arg, extra_message);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

/// @brief Somewhat similar to luaL_setfuncs, except it uses any kind of container.
template <typename T>
void setFuncs(lua_State* state, const T& funcs)
{
    for (const auto& func : funcs) {
        lua_pushcfunction(state, func.func);
        lua_setfield(state, -2, func.name);
    }
}

template <typename T>
void setPropertyFuncs(lua_State* state, const T& props, lua_CFunction Property::*accessor)
{
    for (const auto& prop : props) {
        lua_pushcfunction(state, prop.*accessor);
        lua_setfield(state, -2, prop.name);
    }
}

template <typename T>
auto countProperties(const T& properties, lua_CFunction Property::*accessor)
{
    using std::begin, std::end;
    return std::count_if(begin(properties), end(properties), std::mem_fn(accessor));
}

} // namespace detail

/*

--- Convert Protocol ---

static constexpr std::optional<int> push_count = 1;
    -> How many items are pushed by push, usually 1
    -> Can be std::nullopt if the size varies, in which case the getPushCount function must be provided

static constexpr bool allow_nesting = true;
    -> Whether this type can be nested inside of tuples.

static bool isExact(lua_State* state, int pos);
    -> Whether the given stack positions type matches exactly.
    -> lua_type(state, pos) == T

static bool isValid(lua_State* state, int pos);
    -> Whether the given stack position is convertible.
    -> lua_isT(state, pos)

static std::optional<T> at(lua_State* state, int pos);
    -> Tries to convert the given stack position and returns std::nullopt on failure.
    -> lua_toT(state, arg)

static T check(lua_State* state, int arg);
    -> Tries to convert the given argument stack position and raises an argument error on failure.
    -> lua_checkT(state, arg)

static int getPushCount(const &T value);
    -> When push_count is std::nullopt, this function returns the actual count for a given value

static std::string/std::string_view getPushTypename();
    -> Returns the typename of the value
    -> luaL_typename

static void push(lua_State* state, T value);
    -> Pushes the given value onto the stack using push_count values
    -> lua_pushT(state, value)

// A full implementation would look like this:

template <>
struct Convert<T> {
    static constexpr std::optional<int> push_count = 1;
    static constexpr bool allow_nesting = true;

    static bool isExact(lua_State* state, int pos) {}

    static constexpr bool isValid(lua_State* state, int pos) {}

    static std::optional<T> at(lua_State* state, int pos) {}

    static T check(lua_State* state, int arg) {}

    static int getPushCount(const T& value) {}

    static constexpr std::string_view getPushTypename() {}

    static void push(lua_State* state, const T& value) {}
};

*/

/// @brief A Lua class instance can either be its own value or reference an existing instance.
enum class StoreType { None, Value, Reference };

namespace detail {

/// @brief Used to generate unique names for each ClassInfo specialization.
static inline std::size_t class_counter = 1;

/// @brief Provides a unique name and reference name for any given type.
template <typename T>
struct UniqueClassInfo {
    static inline const std::string name = "dang" + std::to_string(class_counter++);
    static inline const std::string name_ref = "dang" + std::to_string(class_counter++);

    static inline int index = LUA_NOREF;
    static inline int newindex = LUA_NOREF;
};

} // namespace detail

/// @brief Converts instances of classes and enums to and from Lua as either value or reference.
template <typename T>
struct Convert {
    static_assert(enum_values<T>[std::size(enum_values<T>) - 1] == nullptr, "enum_values is not null-terminated");
    static_assert(!std::is_enum_v<T> || std::size(enum_values<T>) > 1, "enum_values is empty");

    static constexpr std::optional<int> push_count = 1;
    static constexpr bool allow_nesting = true;

    /// @brief Whether a stack position is a value, reference or neither.
    static StoreType type(lua_State* state, int pos)
    {
        static_assert(std::is_class_v<T>);
        if (luaL_testudata(state, pos, detail::UniqueClassInfo<T>::name.c_str()))
            return StoreType::Value;
        if (luaL_testudata(state, pos, detail::UniqueClassInfo<T>::name_ref.c_str()))
            return StoreType::Reference;
        return type(state, pos, SubClassesOf<T>);
    }

    /// @brief Finds the given string enum value or std::nullopt if not found.
    static std::optional<T> findEnumValue(const char* value)
    {
        static_assert(std::is_enum_v<T>);
        for (std::size_t i = 0; enum_values<T>[i]; i++)
            if (std::strcmp(enum_values<T>[i], value) == 0)
                return static_cast<T>(i);
        return std::nullopt;
    }

    /// @brief Whether the stack position is a valid class value or reference, or an enum.
    static bool isExact(lua_State* state, int pos)
    {
        static_assert(std::is_class_v<T> || std::is_enum_v<T>, "class or enum expected");
        if constexpr (std::is_class_v<T>)
            return type(state, pos) != StoreType::None;
        else if constexpr (std::is_enum_v<T>)
            return at(state, pos);
    }

    /// @brief Whether the stack position is a valid class value or reference, or an enum.
    static bool isValid(lua_State* state, int pos) { return isExact(state, pos); }

    /// @brief Returns a reference to the value at the given stack position or std::nullopt on failure.
    static auto at(lua_State* state, int pos)
        -> std::optional<std::conditional_t<std::is_class_v<T>, std::reference_wrapper<T>, T>>
    {
        static_assert(std::is_class_v<T> || std::is_enum_v<T>, "class or enum expected");
        if constexpr (std::is_class_v<T>) {
            if constexpr (ClassInfo<T>::allow_table_initialization) {
                if (lua_istable(state, pos)) {
                    auto abs_pos = lua_absindex(state, pos);
                    auto& value = push(state, T());

                    lua_pushnil(state);
                    while (lua_next(state, abs_pos)) {
                        // duplicate key and value
                        lua_pushvalue(state, -2);
                        lua_pushvalue(state, -2);
                        // userdata[key] = value
                        lua_settable(state, -5);
                        // pop value, leave key
                        lua_pop(state, 1);
                    }

                    lua_replace(state, pos);
                    return value;
                }
            }

            if (void* value = luaL_testudata(state, pos, detail::UniqueClassInfo<T>::name.c_str()))
                return *static_cast<T*>(value);
            if (void* pointer = luaL_testudata(state, pos, detail::UniqueClassInfo<T>::name_ref.c_str()))
                return **static_cast<T**>(pointer);
            return at(state, pos, SubClassesOf<T>);
        }
        else if constexpr (std::is_enum_v<T>) {
            lua_pushvalue(state, pos);
            auto result = findEnumValue(lua_tostring(state, -1));
            lua_pop(state, 1);
            return result;
        }
    }

    /// @brief Returns a reference to the value at the given argument stack position and raises an argument error on
    /// failure.
    static auto check(lua_State* state, int arg) -> std::conditional_t<std::is_class_v<T>, T&, T>
    {
        static_assert(std::is_class_v<T> || std::is_enum_v<T>, "class or enum expected");
        if constexpr (std::is_class_v<T>) {
            if (auto result = at(state, arg))
                return *result;
            detail::noreturn_luaL_typeerror(state, arg, ClassInfo<T>::className());
        }
        else if constexpr (std::is_enum_v<T>) {
            return static_cast<T>(luaL_checkoption(state, arg, nullptr, enum_values<T>));
        }
    }

    /// @brief Pushes the metatable for a value or reference instance onto the stack.
    template <bool v_reference>
    static void pushMetatable(lua_State* state)
    {
        static_assert(std::is_class_v<T>);
        if (!luaL_newmetatable(
                state, (v_reference ? detail::UniqueClassInfo<T>::name_ref : detail::UniqueClassInfo<T>::name).c_str()))
            return;

        detail::setFuncs(state, class_metatable<T>);

        registerIndex(state);
        registerNewindex(state);
        registerDisplayName(state);

        if constexpr (!v_reference)
            registerCleanup(state);

        protectMetatable(state);
    }

    /// @brief Returns the name of the class or enum.
    static constexpr std::string_view getPushTypename() { return ClassInfo<T>::className(); }

    /// @brief Pushes the in place constructed value or enum string onto the stack.
    template <typename... TArgs>
    static std::conditional_t<std::is_class_v<T>, T&, void> push(lua_State* state, TArgs&&... values)
    {
        static_assert(std::is_class_v<T> || std::is_enum_v<T>);
        static_assert(!std::is_enum_v<T> || sizeof...(TArgs) == 1);
        if constexpr (std::is_class_v<T>) {
            T* userdata = static_cast<T*>(lua_newuserdata(state, sizeof(T)));
            new (userdata) T(std::forward<TArgs>(values)...);
            pushMetatable<false>(state);
            lua_setmetatable(state, -2);
            return *userdata;
        }
        else if constexpr (std::is_enum_v<T>) {
            lua_pushstring(state, enum_values<T>[static_cast<std::size_t>(values)]...);
        }
    }

    /// @brief Pushes a reference to the value on the stack.
    static void push(lua_State* state, std::reference_wrapper<T> value)
    {
        static_assert(std::is_class_v<T>);
        pushRef(state, value.get());
    }

    /// @brief Pushes a reference to the value on the stack.
    static void pushRef(lua_State* state, T& value)
    {
        static_assert(std::is_class_v<T>);
        T** userdata = static_cast<T**>(lua_newuserdata(state, sizeof(T*)));
        *userdata = &value;
        pushMetatable<true>(state);
        lua_setmetatable(state, -2);
    }

private:
    /// @brief Checks whether the type matches any of the supplied sub classes.
    template <typename TFirst, typename... TRest>
    static StoreType type(lua_State* state, int pos, SubClassList<TFirst, TRest...>)
    {
        auto result = Convert<TFirst>::type(state, pos);
        if (result != StoreType::None)
            return result;
        return type<TRest...>(state, pos);
    }

    /// @brief Serves as an exit condition when the list of sub classes is depleted.
    static StoreType type(lua_State* state, int, SubClassList<>) { return StoreType::None; }

    /// @brief Goes through the full list of subclasses to try and convert the value.
    template <typename TFirst, typename... TRest>
    static auto at(lua_State* state, int pos, SubClassList<TFirst, TRest...>)
        -> std::optional<std::reference_wrapper<T>>
    {
        auto result = Convert<TFirst>::at(state, pos);
        return result ? result : at(state, pos, SubClassList<TRest...>{});
    }

    /// @brief Exit condition when the subclass list is depleted.
    static auto at(lua_State*, int, SubClassList<>) -> std::optional<std::reference_wrapper<T>> { return std::nullopt; }

    /// @brief Registers the __index metamethod to the metatable on the top of the stack.
    static void registerIndex(lua_State* state)
    {
        auto& index_ref = detail::UniqueClassInfo<T>::index;
        if (index_ref == LUA_REFNIL)
            return;

        if (index_ref != LUA_NOREF) {
            lua_rawgeti(state, index_ref, LUA_REGISTRYINDEX);
            lua_setfield(state, -2, "__index");
            return;
        }

        int pushed = 0;

        // Push table for properties.
        auto get_count = detail::countProperties(class_properties<T>, &Property::get);
        auto has_properties = get_count > 0;
        if (has_properties) {
            lua_createtable(state, 0, static_cast<int>(get_count));
            pushed++;
            detail::setPropertyFuncs(state, class_properties<T>, &Property::get);
            lua_pushvalue(state, -1);
            lua_setfield(state, -2 - pushed, "get");
        }

        // Push class_table<T>
        auto has_indextable = !class_table<T>.empty();
        if (has_indextable) {
            lua_createtable(state, 0, static_cast<int>(class_table<T>.size()));
            pushed++;
            detail::setFuncs(state, class_table<T>);
            lua_pushvalue(state, -1);
            lua_setfield(state, -2 - pushed, "indextable");
        }

        // Push class_metatable<T>::__index
        auto has_indexfunction = lua_getfield(state, -1 - pushed, "__index") != LUA_TNIL;
        if (has_indexfunction)
            pushed++;
        else
            lua_pop(state, 1);

        if (pushed == 0) {
            index_ref = LUA_REFNIL;
            return;
        }

        if (has_properties) {
            if (has_indextable) {
                if (has_indexfunction)
                    lua_pushcclosure(state, customIndex<1, 2, 3>, 3);
                else
                    lua_pushcclosure(state, customIndex<1, 2, 0>, 2);
            }
            else {
                if (has_indexfunction)
                    lua_pushcclosure(state, customIndex<1, 0, 2>, 2);
                else
                    lua_pushcclosure(state, customIndex<1, 0, 0>, 1);
            }
        }
        else if (has_indextable && has_indexfunction)
            lua_pushcclosure(state, customIndex<0, 1, 2>, 2);
        // else leave singular index table or function on the stack

        lua_pushvalue(state, -2);
        index_ref = luaL_ref(state, -1);
        lua_setfield(state, -2, "__index");
    }

    /// @brief Registers the __newindex metamethod to the metatable on the top of the stack.
    static void registerNewindex(lua_State* state)
    {
        auto& newindex_ref = detail::UniqueClassInfo<T>::newindex;
        if (newindex_ref == LUA_REFNIL)
            return;

        if (newindex_ref != LUA_NOREF) {
            lua_rawgeti(state, newindex_ref, LUA_REGISTRYINDEX);
            lua_setfield(state, -2, "__newindex");
            return;
        }

        int pushed = 0;

        // Push table for properties.
        auto set_count = detail::countProperties(class_properties<T>, &Property::set);
        auto has_properties = set_count > 0;
        if (has_properties) {
            lua_createtable(state, 0, static_cast<int>(set_count));
            pushed++;
            detail::setPropertyFuncs(state, class_properties<T>, &Property::set);
            lua_pushvalue(state, -1);
            lua_setfield(state, -2 - pushed, "set");
        }

        // Push class_metatable<T>::__newindex
        auto has_newindex = lua_getfield(state, -1 - pushed, "__newindex") != LUA_TNIL;
        if (has_newindex)
            pushed++;
        else
            lua_pop(state, 1);

        if (pushed == 0) {
            newindex_ref = LUA_REFNIL;
            return;
        }

        if (has_properties) {
            if (has_newindex)
                lua_pushcclosure(state, customNewindex<1, 2>, 2);
            else
                lua_pushcclosure(state, customNewindex<1, 0>, 1);
        }
        else if (has_newindex)
            lua_pushcclosure(state, customNewindex<0, 1>, 1);

        lua_pushvalue(state, -2);
        newindex_ref = luaL_ref(state, -1);
        lua_setfield(state, -2, "__newindex");
    }

    /// @brief Registers the display name provided by `ClassInfo` to the metatable on the top of the stack.
    static void registerDisplayName(lua_State* state)
    {
        lua_pushstring(state, ClassInfo<T>::className());
        lua_setfield(state, -2, "__name");
    }

    /// @brief Registers the cleanup function on the metatable on the top of the stack.
    static void registerCleanup(lua_State* state)
    {
        lua_pushcfunction(state, cleanup);
        lua_setfield(state, -2, "__gc");
    }

    /// @brief Protects the metatable on the stop of the stack with `false`.
    static void protectMetatable(lua_State* state)
    {
        lua_pushboolean(state, false);
        lua_setfield(state, -2, "__metatable");
    }

    // --- Lua functions ---

    /// @brief __gc, which is used to do cleanup for non-reference values.
    static int cleanup(lua_State* state)
    {
        static_assert(std::is_class_v<T>);
        T* userdata = static_cast<T*>(lua_touserdata(state, 1));
        userdata->~T();
        return 0;
    }

    /// @brief Handles checking properties, the original index table and calling the __index function in this order.
    /// @remark Upvalue indices to use for each style are passed as template parameter and can be 0 to skip entirely.
    template <int v_properties, int v_indextable, int v_indexfunction>
    static int customIndex(lua_State* state)
    {
        static_assert(std::is_class_v<T>);

        if constexpr (v_properties != 0) {
            lua_pushvalue(state, -1);
            if (lua_gettable(state, lua_upvalueindex(v_properties)) != LUA_TNIL) {
                lua_pushvalue(state, 1);
                lua_call(state, 1, 1);
                return 1;
            }
            lua_pop(state, 1);
        }

        if constexpr (v_indextable != 0) {
            lua_pushvalue(state, -1);
            if (lua_gettable(state, lua_upvalueindex(v_indextable)) != LUA_TNIL)
                return 1;
            lua_pop(state, 1);
        }

        if constexpr (v_indexfunction != 0) {
            lua_pushvalue(state, lua_upvalueindex(v_indexfunction));
            lua_insert(state, -3);
            lua_call(state, 2, 1);
            return 1;
        }

        return 0;
    }

    /// @brief Handles properties and calling the original __newindex function in this order.
    /// @remark Upvalue indices to use for each style are passed as template parameter and can be 0 to skip entirely.
    template <int v_properties, int v_indexfunction>
    static int customNewindex(lua_State* state)
    {
        static_assert(std::is_class_v<T>);

        if constexpr (v_properties != 0) {
            lua_pushvalue(state, -2);
            if (lua_gettable(state, lua_upvalueindex(v_properties)) != LUA_TNIL) {
                lua_pushvalue(state, 1);
                lua_pushvalue(state, 3);
                lua_call(state, 2, 0);
                return 0;
            }
            lua_pop(state, 1);
        }

        if constexpr (v_indexfunction != 0) {
            lua_pushvalue(state, lua_upvalueindex(v_indexfunction));
            lua_insert(state, -4);
            lua_call(state, 3, 0);
            return 0;
        }

        std::string name(getPushTypename());
        if (lua_type(state, 2) == LUA_TSTRING) {
            auto prop = lua_tostring(state, 2);
            detail::noreturn_luaL_error(state, ("cannot write property " + name + "." + prop).c_str());
        }
        detail::noreturn_luaL_error(state, ("attempt to index a " + name + " value").c_str());
    }
};

template <typename T>
struct Convert<T&> : Convert<T> {};
template <typename T>
struct Convert<T&&> : Convert<T> {};
template <typename T>
struct Convert<const T> : Convert<T> {};
template <typename T>
struct Convert<std::reference_wrapper<T>> : Convert<T> {};

/// @brief Converts nothing.
template <>
struct Convert<void> {
    static constexpr std::optional<int> push_count = 0;
    static constexpr bool allow_nesting = true;
};

/// @brief Converts nil values.
template <typename TNil>
struct ConvertNil {
    static constexpr std::optional<int> push_count = 1;
    static constexpr bool allow_nesting = true;

    /// @brief Whether the given stack position is nil.
    static bool isExact(lua_State* state, int pos) { return lua_isnil(state, pos); }

    /// @brief Whether the given stack position is nil or none.
    static bool isValid(lua_State* state, int pos) { return lua_isnoneornil(state, pos); }

    /// @brief Returns an instance of TNil for nil and none values, and std::nullopt otherwise.
    static std::optional<TNil> at(lua_State* state, int pos)
    {
        if (lua_isnoneornil(state, pos))
            return TNil();
        return std::nullopt;
    }

    /// @brief Returns an instance of TNil and raises an error if the value is neither nil nor none.
    static TNil check(lua_State* state, int arg)
    {
        if (lua_isnoneornil(state, arg))
            return nullptr;
        detail::noreturn_luaL_argerror(state, arg, "expected a nil value");
    }

    static constexpr std::string_view getPushTypename()
    {
        using namespace std::literals;
        return "nil"sv;
    }

    /// @brief Pushes a nil value on the stack.
    static void push(lua_State* state, TNil = {}) { lua_pushnil(state); }
};

template <>
struct Convert<std::nullptr_t> : ConvertNil<std::nullptr_t> {};
template <>
struct Convert<std::nullopt_t> : ConvertNil<std::nullopt_t> {};
template <>
struct Convert<std::monostate> : ConvertNil<std::monostate> {};

/// @brief Tag struct for Lua's `fail` value.
struct Fail {};

inline constexpr Fail fail;

/// @brief Allows for pushing of the `fail` value, which is currently just `nil`.
template <>
struct Convert<Fail> {
    static constexpr std::optional<int> push_count = 1;
    static constexpr bool allow_nesting = true;

    /// @brief Pushes the `fail` value on the stack.
    static void push(lua_State* state, Fail = {}) { luaL_pushfail(state); }
};

/// @brief Allows for conversion between Lua boolean and C++ bool.
template <>
struct Convert<bool> {
    static constexpr std::optional<int> push_count = 1;
    static constexpr bool allow_nesting = true;

    /// @brief Whether the given stack position contains an actual boolean.
    static bool isExact(lua_State* state, int pos) { return lua_isboolean(state, pos); }

    /// @brief Always returns true, as everything is convertible to boolean.
    static constexpr bool isValid(lua_State*, int) { return true; }

    /// @brief Converts the given stack position and never returns std::nullopt.
    static std::optional<bool> at(lua_State* state, int pos) { return lua_toboolean(state, pos); }

    /// @brief Converts the given stack position and never raises an error.
    static bool check(lua_State* state, int arg) { return lua_toboolean(state, arg); }

    static constexpr std::string_view getPushTypename()
    {
        using namespace std::literals;
        return "boolean"sv;
    }

    /// @brief Pushes the given boolean on the stack.
    static void push(lua_State* state, bool value) { lua_pushboolean(state, value); }
};

/// @brief Allows for conversion between Lua numbers and C++ floating point types.
template <typename T>
struct ConvertFloatingPoint {
    static_assert(std::is_floating_point_v<T>, "T must be floating point");

    static constexpr std::optional<int> push_count = 1;
    static constexpr bool allow_nesting = true;

    /// @brief Whether the stack position contains an actual number.
    static bool isExact(lua_State* state, int pos) { return lua_type(state, pos) == LUA_TNUMBER; }

    /// @brief Whether the stack position contains a number or a string, convertible to a number.
    static bool isValid(lua_State* state, int pos) { return lua_isnumber(state, pos); }

    /// @brief Converts the given argument stack position into a Lua number and returns std::nullopt on failure.
    static std::optional<T> at(lua_State* state, int pos)
    {
        int isnum;
        lua_Number result = lua_tonumberx(state, pos, &isnum);
        if (isnum)
            return static_cast<T>(result);
        return std::nullopt;
    }

    /// @brief Converts the given argument stack position into a floating point type and raises an error on failure.
    static T check(lua_State* state, int arg) { return static_cast<T>(luaL_checknumber(state, arg)); }

    static constexpr std::string_view getPushTypename()
    {
        using namespace std::literals;
        return "number"sv;
    }

    /// @brief Pushes the given number on the stack.
    static void push(lua_State* state, T value) { lua_pushnumber(state, static_cast<lua_Number>(value)); }
};

template <>
struct Convert<float> : ConvertFloatingPoint<float> {};
template <>
struct Convert<double> : ConvertFloatingPoint<double> {};
template <>
struct Convert<long double> : ConvertFloatingPoint<long double> {};

/// @brief Allows for conversion between Lua integers and C++ integral types.
template <typename T>
struct ConvertIntegral {
    static_assert(std::is_integral_v<T>, "T must be integral");

    static constexpr std::optional<int> push_count = 1;
    static constexpr bool allow_nesting = true;

    // TODO: C++20 replace with std::in_range <3
    /// @brief Checks, whether the given Lua integer fits into the range of the C++ integral type.
    static constexpr bool checkRange([[maybe_unused]] lua_Integer value)
    {
        if constexpr (std::is_same_v<T, std::uint64_t>) {
            return value >= 0;
        }
        else {
            if constexpr (std::numeric_limits<T>::max() < std::numeric_limits<lua_Integer>::max()) {
                if (value > std::numeric_limits<T>::max())
                    return false;
            }
            if constexpr (std::numeric_limits<T>::min() > std::numeric_limits<lua_Integer>::min()) {
                if (value < std::numeric_limits<T>::min())
                    return false;
            }
            return true;
        }
    }

    /// @brief Whether the value at the given stack position is an integer and fits the C++ integral type.
    static bool isExact(lua_State* state, int pos)
    {
        if (lua_type(state, pos) == LUA_TNUMBER)
            return false;
        int isnum;
        lua_Number value = lua_tointegerx(state, pos, &isnum);
        return isnum && checkRange(value);
    }

    /// @brief Whether the value at the given stack position is an integer or a string convertible to an integer and
    /// fits the C++ integral type.
    static bool isValid(lua_State* state, int pos)
    {
        int isnum;
        lua_Number value = lua_tointegerx(state, pos, &isnum);
        return isnum && checkRange(value);
    }

    /// @brief Returns an error message for the given number not being in the correct range.
    static std::string getRangeErrorMessage(lua_Integer value)
    {
        return "value " + std::to_string(value) + " must be in range " + std::to_string(std::numeric_limits<T>::min()) +
               " .. " + std::to_string(std::numeric_limits<T>::max());
    }

    /// @brief Converts the given argument stack position into an integral type and returns std::nullopt on failure.
    static std::optional<T> at(lua_State* state, int pos)
    {
        int isnum;
        lua_Integer result = lua_tointegerx(state, pos, &isnum);
        if (isnum && checkRange(result))
            return static_cast<T>(result);
        return std::nullopt;
    }

    /// @brief Converts the given argument stack position into an integral type and raises an error on failure.
    static T check(lua_State* state, int arg)
    {
        lua_Integer result = luaL_checkinteger(state, arg);
        if (!checkRange(result))
            detail::noreturn_luaL_argerror(state, arg, getRangeErrorMessage(result).c_str());
        return static_cast<T>(result);
    }

    static constexpr std::string_view getPushTypename()
    {
        using namespace std::literals;
        return "integer"sv;
    }

    /// @brief Pushes the given integer on the stack.
    static void push(lua_State* state, T value) { lua_pushinteger(state, static_cast<lua_Integer>(value)); }
};

template <>
struct Convert<std::int8_t> : ConvertIntegral<std::int8_t> {};
template <>
struct Convert<std::uint8_t> : ConvertIntegral<std::uint8_t> {};
template <>
struct Convert<std::int16_t> : ConvertIntegral<std::int16_t> {};
template <>
struct Convert<std::uint16_t> : ConvertIntegral<std::uint16_t> {};
template <>
struct Convert<std::int32_t> : ConvertIntegral<std::int32_t> {};
template <>
struct Convert<std::uint32_t> : ConvertIntegral<std::uint32_t> {};
template <>
struct Convert<std::int64_t> : ConvertIntegral<std::int64_t> {};
template <>
struct Convert<std::uint64_t> : ConvertIntegral<std::uint64_t> {};

/// @brief Allows for conversion between Lua strings and std::string.
template <>
struct Convert<std::string> {
    static constexpr std::optional<int> push_count = 1;
    static constexpr bool allow_nesting = true;

    /// @brief Whether the value at the given stack position is a string.
    static bool isExact(lua_State* state, int pos) { return lua_type(state, pos) == LUA_TSTRING; }

    /// @brief Whether the value at the given stack position is a string or a number.
    static bool isValid(lua_State* state, int pos) { return lua_isstring(state, pos); }

    /// @brief Checks, whether the given argument stack position is a string or number and returns std::nullopt on
    /// failure.
    /// @remark Numbers are actually converted to a string in place.
    static std::optional<std::string> at(lua_State* state, int pos)
    {
        std::size_t length;
        const char* string = lua_tolstring(state, pos, &length);
        if (string)
            return std::string(string, length);
        return std::nullopt;
    }

    /// @brief Checks, whether the given argument stack position is a string or number and raises an error on failure.
    /// @remark Numbers are actually converted to a string in place.
    static std::string check(lua_State* state, int arg)
    {
        std::size_t length;
        const char* string = luaL_checklstring(state, arg, &length);
        return std::string(string, length);
    }

    static constexpr std::string_view getPushTypename()
    {
        using namespace std::literals;
        return "string"sv;
    }

    /// @brief Pushes the given string onto the stack.
    static void push(lua_State* state, const std::string& value)
    {
        lua_pushlstring(state, value.c_str(), value.size());
    }
};

/// @brief Allows for conversion between Lua strings and std::string_view.
template <>
struct Convert<std::string_view> {
    static constexpr std::optional<int> push_count = 1;
    static constexpr bool allow_nesting = true;

    /// @brief Whether the value at the given stack position is a string.
    static bool isExact(lua_State* state, int pos) { return lua_type(state, pos) == LUA_TSTRING; }

    /// @brief Whether the value at the given stack position is a string or a number.
    static bool isValid(lua_State* state, int pos) { return lua_isstring(state, pos); }

    /// @brief Checks, whether the given argument stack position is a string or number and returns std::nullopt on
    /// failure.
    /// @remark Numbers are actually converted to a string in place.
    static std::optional<std::string_view> at(lua_State* state, int pos)
    {
        std::size_t length;
        const char* string = lua_tolstring(state, pos, &length);
        if (string)
            return std::string_view(string, length);
        return std::nullopt;
    }

    /// @brief Checks, whether the given argument stack position is a string or number and raises an error on failure.
    /// @remark Numbers are actually converted to a string in place.
    static std::string_view check(lua_State* state, int arg)
    {
        std::size_t length;
        const char* string = luaL_checklstring(state, arg, &length);
        return std::string_view(string, length);
    }

    static constexpr std::string_view getPushTypename()
    {
        using namespace std::literals;
        return "string"sv;
    }

    /// @brief Pushes the given string onto the stack.
    static void push(lua_State* state, std::string_view value) { lua_pushlstring(state, value.data(), value.size()); }
};

/// @brief Allows pushing of char arrays as strings.
template <std::size_t v_count>
struct Convert<char[v_count]> {
    static constexpr std::optional<int> push_count = 1;
    static constexpr bool allow_nesting = true;

    static constexpr std::string_view getPushTypename()
    {
        using namespace std::literals;
        return "string"sv;
    }

    /// @brief Pushes the given string onto the stack, shortening a potential null-termination.
    static void push(lua_State* state, const char (&value)[v_count])
    {
        lua_pushlstring(state, value, value[v_count - 1] ? v_count : v_count - 1);
    }
};

/// @brief Allows pushing of C-Style strings.
template <>
struct Convert<const char*> {
    static constexpr std::optional<int> push_count = 1;
    static constexpr bool allow_nesting = true;

    /// @brief Whether the value at the given stack position is a string.
    static bool isExact(lua_State* state, int pos) { return lua_type(state, pos) == LUA_TSTRING; }

    /// @brief Whether the value at the given stack position is a string or a number.
    static bool isValid(lua_State* state, int pos) { return lua_isstring(state, pos); }

    /// @brief Checks, whether the given argument stack position is a string or number and returns std::nullopt on
    /// failure.
    /// @remark Numbers are actually converted to a string in place.
    static std::optional<const char*> at(lua_State* state, int pos)
    {
        const char* string = lua_tostring(state, pos);
        if (string)
            return string;
        return std::nullopt;
    }

    /// @brief Checks, whether the given argument stack position is a string or number and raises an error on failure.
    /// @remark Numbers are actually converted to a string in place.
    static const char* check(lua_State* state, int arg) { return luaL_checkstring(state, arg); }

    static constexpr std::string_view getPushTypename()
    {
        using namespace std::literals;
        return "string"sv;
    }

    /// @brief Pushes the given null-terminated string onto the stack.
    static void push(lua_State* state, const char* value) { lua_pushstring(state, value); }
};

/// @brief Allows pushing of C-Style strings.
template <>
struct Convert<char*> : Convert<const char*> {};

/// @brief Allows for conversion of C functions.
template <>
struct Convert<lua_CFunction> {
    static constexpr std::optional<int> push_count = 1;
    static constexpr bool allow_nesting = true;

    /// @brief Whether the value at the given stack position is a C function.
    static bool isExact(lua_State* state, int pos) { return lua_iscfunction(state, pos); }

    /// @brief Whether the value at the given stack position is a C function.
    static bool isValid(lua_State* state, int pos) { return isExact(state, pos); }

    /// @brief Checks, whether the given argument stack position is a C function and returns std::nullopt on failure.
    static std::optional<lua_CFunction> at(lua_State* state, int pos)
    {
        if (auto result = lua_tocfunction(state, pos))
            return result;
        return std::nullopt;
    }

    /// @brief Checks, whether the given argument stack position is a C function and raises an error on failure.
    static lua_CFunction check(lua_State* state, int arg)
    {
        if (auto result = lua_tocfunction(state, arg))
            return result;
        detail::noreturn_luaL_argerror(state, arg, "C function expected");
    }

    static constexpr std::string_view getPushTypename()
    {
        using namespace std::literals;
        return "function"sv;
    }

    /// @brief Pushes the given C function onto the stack.
    static void push(lua_State* state, lua_CFunction value) { lua_pushcfunction(state, value); }
};

template <>
struct Convert<int (&)(lua_State*)> : Convert<lua_CFunction> {};
template <>
struct Convert<int(lua_State*)> : Convert<lua_CFunction> {};

/// @brief Allows for conversion for possible nil values using std::optional.
template <typename T>
struct Convert<std::optional<T>> {
    using Base = Convert<T>;

    static_assert(Base::push_count == 1, "Only single values can be optional.");

    static constexpr std::optional<int> push_count = 1;
    static constexpr bool allow_nesting = true;

    /// @brief Whether the value at the given stack position is nil or a valid value.
    static bool isExact(lua_State* state, int pos) { return lua_isnoneornil(state, pos) || Base::isValid(state, pos); }

    /// @brief Whether the value at the given stack position is nil or a valid value.
    static bool isValid(lua_State* state, int pos) { return isExact(state, pos); }

    /// @brief Returns an optional containing a std::nullopt for nil values or a single std::nullopt for invalid values.
    static std::optional<std::optional<T>> at(lua_State* state, int pos)
    {
        if (lua_isnoneornil(state, pos))
            return std::optional<T>();
        auto result = Base::at(state, pos);
        if (result)
            return result;
        return std::nullopt;
    }

    /// @brief Returns std::nullopt for nil values or raises an error for invalid values.
    static std::optional<T> check(lua_State* state, int arg)
    {
        if (lua_isnoneornil(state, arg))
            return std::nullopt;
        return Base::check(state, arg);
    }

    /// @brief Pushes the given value or nil onto the stack.
    static int push(lua_State* state, std::optional<T> value)
    {
        if (value)
            Base::push(state, *value);
        else
            lua_pushnil(state);
        return 1;
    }
};

/// @brief Returns the combined push count of all types or std::nullopt if any push count is not known at compile-time.
template <typename... TValues>
constexpr std::optional<int> CombinedPushCount = [] {
    if constexpr ((Convert<TValues>::push_count && ...))
        return (0 + ... + *Convert<TValues>::push_count);
    else
        return std::nullopt;
}();

/// @brief Returns the combined push count of all values.
template <typename... TValues>
static constexpr int combinedPushCount(const TValues&... values)
{
    return (0 + ... + [&values] {
        if constexpr (Convert<TValues>::push_count)
            return *Convert<TValues>::push_count;
        else
            return Convert<TValues>::getPushCount(values);
    }());
}

/// @brief Allows for conversion of multiple values using tuple like types.
template <typename TTuple, typename... TValues>
struct ConvertTuple {
    static constexpr std::optional<int> push_count = CombinedPushCount<TValues...>;
    static constexpr bool allow_nesting = (Convert<TValues>::allow_nesting && ...);

    static_assert(allow_nesting, "Tuples do not allow nesting of stack indices.");

    /// @brief Whether all stack positions starting at pos are exact.
    static bool isExact(lua_State* state, int pos)
    {
        return isExactHelper(state, pos, std::index_sequence_for<TValues...>());
    }

    /// @brief Whether all stack positions starting at pos are valid.
    static bool isValid(lua_State* state, int pos)
    {
        return isValidHelper(state, pos, std::index_sequence_for<TValues...>());
    }

    /// @brief Converts all stack positions starting at pos or std::nullopt on any failure of any.
    static std::optional<TTuple> at(lua_State* state, int pos)
    {
        return atHelper(state, pos, std::index_sequence_for<TValues...>());
    }

    /// @brief Converts all argument stack positions starting at arg and raises an error on failure of any.
    static TTuple check(lua_State* state, int arg)
    {
        return checkHelper(state, arg, std::index_sequence_for<TValues...>());
    }

    /// @brief Pushes all values in the tuple onto the stack and returns the count.
    static void push(lua_State* state, TTuple&& values)
    {
        pushAll(state, std::index_sequence_for<TValues...>(), std::move(values));
    }

    /// @brief Pushes all values in the tuple onto the stack and returns the count.
    static void push(lua_State* state, const TTuple& values)
    {
        pushAll(state, std::index_sequence_for<TValues...>(), values);
    }

    /// @brief Returns the total push count of all values in the tuple.
    static constexpr int getPushCount(const TTuple& values)
    {
        static_assert(!push_count);
        return std::apply(combinedPushCount, values);
    }

private:
    template <std::size_t... v_indices>
    static bool isExactHelper(lua_State* state, int pos, std::index_sequence<v_indices...>)
    {
        return (Convert<TValues>::isExact(state, pos + v_indices) && ...);
    }

    template <std::size_t... v_indices>
    static bool isValidHelper(lua_State* state, int pos, std::index_sequence<v_indices...>)
    {
        return (Convert<TValues>::isValid(state, pos + v_indices) && ...);
    }

    template <std::size_t... v_indices>
    static std::optional<TTuple> atHelper(lua_State* state, int pos, std::index_sequence<v_indices...>)
    {
        TTuple values{Convert<TValues>::at(state, pos + v_indices)...};
        if ((std::get<v_indices>(values) && ...))
            return TTuple{*std::get<v_indices>(values)...};
        return std::nullopt;
    }

    template <std::size_t... v_indices>
    static TTuple checkHelper(lua_State* state, int arg, std::index_sequence<v_indices...>)
    {
        return {Convert<TValues>::check(state, arg + v_indices)...};
    }

    template <std::size_t... v_indices>
    static void pushAll(lua_State* state, std::index_sequence<v_indices...>, TTuple&& values)
    {
        (Convert<TValues>::push(state, std::move(std::get<v_indices>(values))), ...);
    }

    template <std::size_t... v_indices>
    static void pushAll(lua_State* state, std::index_sequence<v_indices...>, const TTuple& values)
    {
        (Convert<TValues>::push(state, std::get<v_indices>(values)), ...);
    }
};

template <typename... TValues>
struct Convert<std::tuple<TValues...>> : ConvertTuple<std::tuple<TValues...>, TValues...> {};

template <typename TFirst, typename TSecond>
struct Convert<std::pair<TFirst, TSecond>> : ConvertTuple<std::pair<TFirst, TSecond>, TFirst, TSecond> {};

/// @brief Allows for conversion of different values using std::variant.
template <typename... TOptions>
struct Convert<std::variant<TOptions...>> {
    static_assert(((Convert<TOptions>::push_count == 1) && ...), "All variant options must have a push count of one.");

    using Variant = std::variant<TOptions...>;

    static constexpr std::optional<int> push_count = 1;
    static constexpr bool allow_nesting = true;

    /// @brief Whether at least one option matches exactly.
    static bool isExact(lua_State* state, int pos) { return (Convert<TOptions>::isExact(state, pos) || ...); }

    /// @brief Whether at least one option is valid.
    static constexpr bool isValid(lua_State* state, int pos) { return (Convert<TOptions>::isValid(state, pos) || ...); }

    /// @brief Returns the first type that does not return std::nullopt or returns std::nullopt itself if none was
    /// found.
    static std::optional<Variant> at(lua_State* state, int pos)
    {
        return atHelper(state, pos, TypeList<TOptions...>());
    }

    /// @brief Returns the first type that does not return std::nullopt or raises and argument error if none was found.
    static Variant check(lua_State* state, int arg)
    {
        auto value = at(state, arg);
        if (value)
            return *value;

        // Generate a similar message to the official (unfortunately internal) luaL_typeerror() function in the Lua
        // source code: https://www.lua.org/source/5.4/lauxlib.c.html#luaL_typeerror
        std::string error = getPushTypename() + " expected, got ";
        if (luaL_getmetafield(state, arg, "__name") == LUA_TSTRING)
            error += lua_tostring(state, -1);
        else if (lua_type(state, arg) == LUA_TLIGHTUSERDATA)
            error += "light userdata";
        else
            error += luaL_typename(state, arg);
        detail::noreturn_luaL_argerror(state, arg, error.c_str());
    }

    /// @brief Combines all possible options of the variant in the form: "a, b, c or d"
    static std::string getPushTypename() { return getPushTypenameHelper(TypeList<TOptions...>()); }

    /// @brief Pushes the value of the variant.
    static void push(lua_State* state, const Variant& variant)
    {
        std::visit([state](const auto& value) { Convert<decltype(value)>::push(state, (value)); }, variant);
    }

private:
    template <typename... TTypes>
    struct TypeList {};

    template <typename TFirst, typename... TRest>
    static std::optional<Variant> atHelper(lua_State* state, int pos, TypeList<TFirst, TRest...>)
    {
        auto value = Convert<TFirst>::at(state, pos);
        return value ? value : atHelper(state, pos, TypeList<TRest...>());
    }

    static std::optional<Variant> atHelper(lua_State*, int, TypeList<>) { return std::nullopt; }

    template <typename TFirst, typename... TRest>
    static std::string getPushTypenameHelper(TypeList<TFirst, TRest...>)
    {
        std::string result{Convert<TFirst>::getPushTypename()};
        if constexpr (sizeof...(TRest) == 0)
            return result;
        else if constexpr (sizeof...(TRest) == 1)
            return result + " or " + getPushTypenameHelper(TypeList<TRest...>());
        else
            return result + ", " + getPushTypenameHelper(TypeList<TRest...>());
    }
};

} // namespace dang::lua
