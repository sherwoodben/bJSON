#pragma once

//--File Info-----------------------------------------------------------------------------------------------------------

/// @file bJSON.h
/// @version 0.1.0
/// @brief single-file "header only" templated JSON library.
///
/// Providing serialization capabilities and helper macros to define the serialization implementation for a desired
/// type. JSON parsing capability is planned in the future.
///
/// @remark For some reason, constexpr (with string results) is only working in release mode... surely that's not a sign
/// that I shouldn't be using constexpr with std::string in the way that I am and this won't come back to bite me in the
/// ass! Actually, seems to be due to small string optimization. So really, I _shouldn't_ be using strings this way!
/// Fails for longer strings (see the array example with 10 "false" values in the tests).
///
/// @todo - investigate how difficult it would be to create constexpr versions of the "native" serialization
/// implementations
/// 
/// @todo - implement JSON parsing

//--Changelog---------------------------------------------------------------------------------------------------------//
/*                                                                                                                    //
//  v0.1.0  -   Initial file contents. Major version 0 until parsing is added.                                        //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------*/

//--Includes------------------------------------------------------------------------------------------------------------

#include <exception>     // for when serialization encounters an error
#include <format>        // for turning numbers into strings
#include <iostream>      // for printing to the console
#include <regex>         // for encoding strings in JSON format
#include <string>        // for strings
#include <type_traits>   // for templated type traits
#include <unordered_map> // for JSONObjects (string keys and JSONValue values)
#include <variant>       // for JSONValues to be able to hold one of multiple types
#include <vector>        // for JSONArrays (list of JSONValues)

//--Macros--------------------------------------------------------------------------------------------------------------

/// @brief we use some macro "hacks" so that when used in this file, the serialization helper macro doesn't include the
/// full namespace
///
/// at the end of bJSON.h bJSON_NAMESPACE() is redefined to have the value ben::json:: and the macro
/// will include the full namespace when used elsewhere in a codebase
#define bJSON_NAMESPACE()

/// @brief "helper macro" which declares a type as JSON serializable, but does not implement a definition
///
/// to provide a definition for the serialization implementation, use of the bJSON_DEFINE_SERIALIZATION() macro is
/// required
///
/// @param T the struct/class to declare serializable
#define bJSON_DECLARE_SERIALIZABLE(T)                                                                                  \
    template <> struct bJSON_NAMESPACE()##JSONSerializationInfo<T>                                                     \
    {                                                                                                                  \
        using SerializationFnType = const std::string (*)(const T &);                                                  \
        static constexpr bool                serializable{true};                                                       \
        static const std::string             serializer_impl(const T &);                                               \
        static constexpr SerializationFnType serializer{&serializer_impl};                                             \
    }

/// @brief "helper macro" which defines the serialization implementation for a type.
/// @param T the struct/class to make serializable
#define bJSON_DEFINE_SERIALIZATION(T)                                                                                  \
    inline const std::string bJSON_NAMESPACE()##JSONSerializationInfo<T>::serializer_impl(const T &val)

/// @brief "helper macro" which registers a type as JSON serializable. Provides a function declaration and expects the
/// user to provide the (inline) definition.
///
/// When providing the serialization implementation, the (public) members of the type are accessible via the variable
/// "val" which is of the desired type (i.e. the single argument for the serialization function is a const T& named
/// "val")
///
/// @param T the struct/class to make serializable
#define bJSON_MAKE_SERIALIZABLE(T)                                                                                     \
    bJSON_DECLARE_SERIALIZABLE(T);                                                                                     \
    bJSON_DEFINE_SERIALIZATION(T)

/// @brief "helper macro" which declares a type as JSON serializable (constexpr version), but does not implement a
/// definition
///
/// to provide a definition for the serialization implementation, use of the bJSON_DEFINE_CONSTEXPR_SERIALIZATION()
/// macro is required
///
/// @see bJSON_DECLARE_SERIALIZABLE(T)
/// @param T the struct/class to declare (constexpr) serializable
#define bJSON_DECLARE_CONSTEXPR_SERIALIZABLE(T)                                                                        \
    template <> struct bJSON_NAMESPACE()##JSONSerializationInfo<T>                                                     \
    {                                                                                                                  \
        using SerializationFnType = const std::string (*)(const T &);                                                  \
        static constexpr bool                serializable{true};                                                       \
        static constexpr const std::string   serializer_impl(const T &);                                               \
        static constexpr SerializationFnType serializer{&serializer_impl};                                             \
    }

/// @brief "helper macro" which defines the (constexpr) serialization implementation for a type.
/// @param T the struct/class to make serializable
#define bJSON_DEFINE_CONSTEXPR_SERIALIZATION(T)                                                                        \
    constexpr const std::string bJSON_NAMESPACE()##JSONSerializationInfo<T>::serializer_impl(const T &val)

/// @brief "helper macro" which registers a type as JSON serializable (constexpr version). Provides a function
/// declaration and expects the user to provide the definition.
///
/// @see bJSON_MAKE_SERIALIZABLE(T)
///
/// @param T the struct/class to make (constexpr) serializable
#define bJSON_MAKE_CONSTEXPR_SERIALIZABLE(T)                                                                           \
    bJSON_DECLARE_CONSTEXPR_SERIALIZABLE(T);                                                                           \
    bJSON_DEFINE_CONSTEXPR_SERIALIZATION(T)

//--JSON Serialization--------------------------------------------------------------------------------------------------

namespace ben
{
    namespace json
    {
        //--Types-------------------------------------------------------------------------------------------------------

        /// @brief a struct containing the information associated with a JSON "value"
        ///
        /// defines an enum JSONLiteralType which can be {null_v, true_v, false_v} which correspond to the
        /// three valid JSON literals
        ///
        /// contains type definitions/aliases for the internally stored values (all of which will be provided
        /// serialization implementations):
        ///     - LiteralType
        ///     - NumberType
        ///     - StringType
        ///     - ArrayType
        ///     - ObjectType
        ///
        /// defines an enum JSONValueType which can be {undefined, literal, number, string, array, object} which is used
        /// to help interpret the variant value. Also, default construction/empty initialization of a JSONValue sets the
        /// stored JSONValueType value to 'undefined', and JSONValues with JSONValueType values of 'undefined' are not
        /// serialized!
        ///
        /// implements the capability to "hold" one of the above types as the "value" member (achieved through the use
        /// of std::variant<LiteralType, NumberType, ...>)
        struct JSONValue
        {
            //--JSONValue Member Types----------------------------------------------------------------------------------

            /// @brief enum corresponding to the three possible JSON literal values
            enum struct JSONLiteralType
            {
                null_v,  ///< the null value
                true_v,  ///< the true value
                false_v, ///< the false value
            };

            /// @brief the LiteralType for JSONValues
            ///
            /// the JSONLiteralType enum encapsulates the three possible JSON literal values, so LiteralType values will
            /// be stored as a JSONLiteralType
            using LiteralType = JSONLiteralType;

            /// @brief the NumberType for JSONValues
            ///
            /// long doubles can store any value from std::numeric_limits<long double>::lowest() to
            /// std::numeric_limits<long double>::max()... that should be enough precision for just about any
            /// application, so NumberType values will be stored as long doubles
            using NumberType = long double;

            /// @brief the StringType for JSONValues
            ///
            /// std::strings can store all unicode characters (depending on encoding, etc. but that shouldn't be an
            /// issue at all), so StringType values will be stored as std::strings
            using StringType = std::string;

            /// @brief the ArrayType for JSONValues
            ///
            /// std::vector<T> stores values of type T such that the elements are accessible via index just like JSON
            /// arrays, so ArrayType will be stored as a std::vector<JSONValue>
            using ArrayType = std::vector<JSONValue>;

            /// @brief the ObjectType for JSONValues
            ///
            /// std::unordered_map<std::string, T> stores values of type T such that the values are accessible via keys
            /// type std::string just like a JSON object, so ObjectType will be stored as a
            /// std::unordered_map<std::string, JSONValue>
            using ObjectType = std::unordered_map<std::string, JSONValue>;

            /// @brief enum corresponding to the five possible/expected states of the variant value's current type (plus
            /// an additional enum value to represent an "undefined"/invalid state)
            enum struct JSONValueType
            {
                undefined, ///< corresponds to a default constructed/empty initialized/moved from/generally
                           ///< invalid JSONValue; when serialized will result in an empty string
                literal,   ///< corresponds to a LiteralType stored as the value for this JSONValue
                number,    ///< corresponds to a NumberType stored as the value for this JSONValue
                string,    ///< corresponds to a StringType stored as the value for this JSONValue
                array,     ///< corresponds to a ArrayType stored as the value for this JSONValue
                object     ///< corresponds to a ObjectType stored as the value for this JSONValue
            };

            //--JSONValue Member Variables------------------------------------------------------------------------------

            /// @brief "track" the current state of the value in the variant defined below
            JSONValueType type{JSONValueType::undefined};

            /// @brief the actual value the JSONValue holds; a variant of one of the five possible types JSONValues can
            /// implement
            std::variant<LiteralType, NumberType, StringType, ArrayType, ObjectType> value{LiteralType::null_v};

            //--Default Ctor and Dtor-----------------------------------------------------------------------------------

            /// @brief leaves the JSONValue in such a state that .type == JSONValueType::undefined (i.e. it will be
            /// skipped when serialized as a "non-existing" value)
            constexpr JSONValue() = default;

            /// @brief default dtor is acceptable since std::variant takes care of the complicated stuff behind the
            /// scenes
            constexpr ~JSONValue() = default;

            //--Other Ctors---------------------------------------------------------------------------------------------

            /// @brief JSONValue::LiteralType ctor
            /// @param val the JSONValue::LiteralType value to store in the JSONValue's variant
            constexpr JSONValue(LiteralType val) : type{JSONValueType::literal}, value{val} {};

            /// @brief boolean ctor (converts to boolean to JSONValue::LiteralType then calls JSONValue::LiteralType
            /// ctor)
            /// @param val the boolean value to convert to a JSONValue::LiteralType (true --> LiteralType::true_v, and
            /// false --> LiteralType::false_v)
            constexpr JSONValue(bool val) : JSONValue{val ? JSONLiteralType::true_v : JSONLiteralType::false_v} {};

            /// @brief JSONValue::NumberType ctor
            /// @param val the JSONValue::NumberType value to store in the JSONValue's variant
            constexpr JSONValue(NumberType val) : type{JSONValueType::number}, value{val} {};

            /// @brief number type ctor
            /// @tparam T the numerical type associated with the value
            /// @tparam enabled boolean value which defaults to true and relies on "enable_if" functionality so it only
            /// compiles if T is an arithmetic type
            /// @param val the numerical value to convert to a JSONValue::NumberType
            template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> enabled = true>
            constexpr JSONValue(T val) : JSONValue{JSONValue::NumberType(val)} {};

            /// @brief const JSONValue::StringType& ctor
            /// @param val the JSONValue::StringType value to store in the JSONValue's variant
            /// @remark copies val into the stored value
            constexpr JSONValue(const StringType &val) : type{JSONValueType::string}, value{val} {};

            /// @brief JSONValue::StringType&& ctor
            /// @param val the JSONValue::StringType value to store in the JSONValue's variant
            /// @remark moves val into the stored value
            constexpr JSONValue(StringType &&val) noexcept : type{JSONValueType::string}, value{std::move(val)} {};

            /// @brief char* string literal ctor
            /// @param val the char* string literal to convert to a string (if the char* is nullptr, converts to an
            /// empty string)
            constexpr JSONValue(const char *const val) : JSONValue(std::string{val ? val : ""}) {};

            /// @brief const JSONValue::ArrayType& ctor
            /// @param val the JSONValue::ArrayType value to store in the JSONValue's variant
            /// @remark copies val into the stored value
            constexpr JSONValue(const ArrayType &val) : type{JSONValueType::array}, value{val} {};

            /// @brief JSONValue::ArrayType&& ctor
            /// @param val the JSONValue::ArrayType value to store in the JSONValue's variant
            /// @remark moves val into the stored value
            constexpr JSONValue(ArrayType &&val) noexcept : type{JSONValueType::array}, value{std::move(val)} {};

            // template <typename T> constexpr JSONValue(const T &val) : JSONValue(JSONValue::ArrayType(val)){};

            /// @brief const JSONValue::ObjectType& ctor
            /// @param val the JSONValue::ObjectType value to store in the JSONValue's variant
            /// @remark copies val into the stored value
            constexpr JSONValue(const ObjectType &val) : type{JSONValueType::object}, value{val} {};

            /// @brief JSONValue::ObjectType&& ctor
            /// @param val the JSONValue::ObjectType value to store in the JSONValue's variant
            /// @remark moves val into the stored value
            constexpr JSONValue(ObjectType &&val) noexcept : type{JSONValueType::object}, value{std::move(val)} {};

            /// @brief copy ctor
            /// @param other the JSONValue to copy the type/value of
            constexpr JSONValue(const JSONValue &other) : type{other.type}, value{other.value} {};

            /// @brief move ctor
            /// @param other the JSONValue to get the type/move the value from
            /// @remark sets other.type to JSONValueType::undefined, leaves other.value in a moved-from (likely invalid)
            /// state
            constexpr JSONValue(JSONValue &&other) noexcept : type{other.type}, value{std::move(other.value)}
            {
                other.type = JSONValueType::undefined;
            };

            //--Assignment Operators------------------------------------------------------------------------------------

            /// @brief JSONValue copy assignment operator
            /// @param other the JSONValue to copy the type/value of
            /// @return a reference to this JSONValue
            constexpr JSONValue &operator=(const JSONValue &other)
            {
                type  = other.type;
                value = other.value;

                return *this;
            };

            /// @brief JSONValue move assignment operator
            /// @param other the JSONValue to get the type/move the value from
            /// @return a reference to this JSONValue
            /// @remark sets other.type to JSONValueType::undefined, leaves other.value in a moved-from (likely invalid)
            /// state
            constexpr JSONValue &operator=(JSONValue &&other) noexcept
            {
                type  = other.type;
                value = std::move(other.value);

                other.type = JSONValueType::undefined;

                return *this;
            };
        };

        //--Templates---------------------------------------------------------------------------------------------------

        /// @brief templated struct which contains information associated with the JSON serialization of a type
        ///
        /// for all types T, the struct contains the following information:
        ///     - JSONSerializationInfo<T>::SerializationFnType; an alias for a pointer to a function accepting a const
        ///     T& as the single argument which returns a (possibly constexpr) const std::string
        ///     - JSONSerializationInfo<T>::serializable; a static constexpr bool which is true if type T is JSON
        ///     serializable and false otherwise
        ///     - JSONSerializationInfo<T>::serializer; a static constexpr function pointer of type
        ///     JSONSerializationInfo<T>::SerializationFnType (depends on T) which points to the serialization
        ///     implementation for type T if it exists or nullptr if it does not exist
        ///
        /// for types T _which have been made serializable via the helper macro_, the struct ALSO contains the
        /// following:
        ///     - JSONSerializationInfo<T>::serializer_impl(const T&); a static (possibly constexpr) member function
        ///     declaration for the implementation of the serialization of type T. Also a feature of the helper macro,
        ///     the JSONSerializationInfo<T>::serializer function pointer is set to the implementation method instead of
        ///     nullptr
        ///
        /// @tparam T the type to get the JSON serialization information of
        template <typename T> struct JSONSerializationInfo
        {
            /// @brief an alias for a pointer to a function matching the type of the serialization implementation for
            /// this type
            using SerializationFnType = const std::string (*)(const T &);

            /// @brief a constexpr boolean which is true if type T is JSON serializable and false otherwise
            static constexpr bool serializable{false};

            /// @brief the default serializer_impl for a generic type returns an empty string; implementations must be
            /// provided for specializations
            /// @param val the value of type T to serialize
            /// @return an empty string (constexpr)
            static constexpr std::string serializer_impl(const T &val) { return std::string{""}; };

            /// @brief a constexpr function pointer to the SerializationFnType for type T which points to the
            /// serialization implementation or nullptr if the type is not JSON serializable
            static constexpr SerializationFnType serializer{nullptr};
        };

        /// @brief helper template which converts to a constexpr bool which is true if
        /// JSONSerializationInfo<T>::serializable is declared and true, false otherwise
        /// @tparam T the type to test for JSON serialization capability
        template <typename T> constexpr bool is_json_serializable_v = JSONSerializationInfo<T>::serializable;

        /// @brief helper template which converts to a constexpr JSONSerializationInfo<T>::SerializationFnType which is
        /// nullptr if T is not JSON serializable, otherwise if T _is_ JSON serializable the value is address of the
        /// serialization implementation for type T
        /// @tparam T the type to get the JSON serialization implementation function pointer for
        template <typename T>
        constexpr JSONSerializationInfo<T>::SerializationFnType json_serializer_v =
            JSONSerializationInfo<T>::serializer;

        /// @brief helper template which converts to a constexpr bool which is true if type T is not JSON serializable
        /// directly but a JSONValue can be constructed from T and false otherwise
        template <typename T>
        constexpr bool converts_to_json_value_v =
            (!is_json_serializable_v<T> && !std::is_same_v<JSONValue, T> && std::is_constructible_v<JSONValue, T>);

        /// @brief templated serialization function which is enabled via the second template parameter which depends on
        /// the type being JSON serializable
        /// @tparam T the type (which is JSON serializable else the instantiation for T isn't compiled) to serialize
        /// @tparam serializer the second template parameter which is of type
        /// JSONSerializationInfo<T>::SerializationFnType which defaults to the json serialization implementation for
        /// type T
        /// @param val the value to serialize (passed by const ref)
        /// @return (constexpr) const string containing the result of the serialization implementation for the value of
        /// type T
        /// @note marked noexcept, which is not to say the serialization implementations themselves are not allowed to
        /// throw; if the implementation throws, the exception (or whatever is thrown) is caught and an empty string is
        /// returned as the result of serialization
        /// @remark to make use of the constexpr functionality at compile time, all serialization implementations which
        /// are called _must_ be constexpr as well. At runtime it does not matter if the serialization implementation
        /// was declared constexpr or not!
        template <
            typename T,
            std::enable_if_t<is_json_serializable_v<T>, typename JSONSerializationInfo<T>::SerializationFnType>
                serializer = json_serializer_v<T>>
        constexpr const std::string serialize(const T &val) noexcept
        {
            std::string serialized{""};

            try
            {
                serialized.append(serializer(val));
            }
            catch (const std::exception &e)
            {
                std::cout << "[ben::json::serialize] Error: " << e.what() << " Returning empty string.\n";
            }
            catch (...)
            {
                std::cout << "[ben::json::serialize] Error: An unknown error has occured. Returning empty string.\n";
            }

            return serialized;
        }

        //--JSONSerializationInfo<JSONValue> Forward Declaration--------------------------------------------------------

        // It is necessary to forward declare the JSONSerializationInfo<JSONValue>::serializer_impl(const JSONValue &)
        // member function which must exist by the time a JSONValue::JSONArray or JSONValue::JSONObject serialization
        // definition is provided AND before the conversion serialization template is instantiated (since the conversion
        // template uses the JSONValue serializer). So, JSONSerializationInfo<JSONValue> is specialized here by hand
        // rather than with the macro.  Serialization implementation definition provided below other / JSONValue variant
        // type serialization implementation macro calls.

        bJSON_DECLARE_CONSTEXPR_SERIALIZABLE(JSONValue);

        //--JSON Value ("default") Types Registration-------------------------------------------------------------------

        bJSON_MAKE_CONSTEXPR_SERIALIZABLE(JSONValue::LiteralType)
        {
            std::string serialized{""};
            switch (val)
            {
            case JSONValue::LiteralType::null_v:
                serialized.append("null");
                break;
            case JSONValue::LiteralType::true_v:
                serialized.append("true");
                break;
            case JSONValue::LiteralType::false_v:
                serialized.append("false");
                break;
            default:
                throw std::exception{"JSONLiteral had invalid value -- was it empty initialized by accident?"};
                break;
            }
            return serialized;
        }

        bJSON_MAKE_SERIALIZABLE(JSONValue::NumberType)
        {
            std::string serialized{std::format("{}", val)};
            return serialized;
        }

        bJSON_MAKE_SERIALIZABLE(JSONValue::StringType)
        {
            std::string serialized{val};

            serialized = std::regex_replace(serialized, std::regex{"\\\\"}, "\\\\");
            serialized = std::regex_replace(serialized, std::regex{"\""}, "\\\"");
            serialized = std::regex_replace(serialized, std::regex{"\n"}, "\\n");
            serialized = std::regex_replace(serialized, std::regex{"\r"}, "\\r");
            serialized = std::regex_replace(serialized, std::regex{"\t"}, "\\t");
            serialized = std::regex_replace(serialized, std::regex{"\f"}, "\\f");
            serialized = std::regex_replace(serialized, std::regex{"\b"}, "\\b");

            serialized.insert(serialized.cbegin(), '"');
            serialized.insert(serialized.cend(), '"');
            return serialized;
        }

        bJSON_MAKE_CONSTEXPR_SERIALIZABLE(JSONValue::ArrayType)
        {
            std::string serialized{"["};
            for (auto first{true}; const auto &element : val)
            {
                if (element.type == JSONValue::JSONValueType::undefined)
                {
                    continue;
                }

                if (!first)
                {
                    serialized.append(",");
                }
                else
                {
                    first = false;
                }

                serialized.append(" ");
                serialized.append(serialize(element));
            }
            serialized.append(" ]");
            return serialized;
        };

        bJSON_MAKE_SERIALIZABLE(JSONValue::ObjectType)
        {
            std::string serialized{"{"};
            for (auto first{true}; const auto &[key, value] : val)
            {
                if (value.type == JSONValue::JSONValueType::undefined)
                {
                    continue;
                }

                if (!first)
                {
                    serialized.append(",");
                }
                else
                {
                    first = false;
                }

                serialized.append(" ");
                serialized.append(serialize(JSONValue{key}));
                serialized.append(" : ");
                serialized.append(serialize(value));
            }
            serialized.append(" }");
            return serialized;
        }

        bJSON_DEFINE_CONSTEXPR_SERIALIZATION(JSONValue)
        {
            std::string serialized{""};
            switch (val.type)
            {
            case JSONValue::JSONValueType::literal:
                serialized.append(serialize(std::get<JSONValue::LiteralType>(val.value)));
                break;
            case JSONValue::JSONValueType::number:
                serialized.append(serialize(std::get<JSONValue::NumberType>(val.value)));
                break;
            case JSONValue::JSONValueType::string:
                serialized.append(serialize(std::get<JSONValue::StringType>(val.value)));
                break;
            case JSONValue::JSONValueType::array:
                serialized.append(serialize(std::get<JSONValue::ArrayType>(val.value)));
                break;
            case JSONValue::JSONValueType::object:
                serialized.append(serialize(std::get<JSONValue::ObjectType>(val.value)));
                break;
            case JSONValue::JSONValueType::undefined:
            default:
                throw std::exception{"JSONValue::type was 'undefined' -- it can not be serialized!"};
                break;
            }
            return serialized;
        }

        //--Converts to JSONValue Type Template-------------------------------------------------------------------------

        /// @brief templated serialization function which is enabled via the second template parameter which depends on
        /// the type not being JSON serializable itself, but serializable via conversion to a JSONValue
        /// @tparam T the type (which is NOT JSON serializable but CAN be used to construct a JSONValue) to serialize
        /// @tparam enabled the second template parameter which is of type bool and defaults to true, only present when
        /// enable_if_t<> evaluates to true which happens when T is not directly serializable but can be converted to a
        /// JSONValue
        /// @param val the value to serialize (passed by forwarding reference)
        /// @return (constexpr) const string containing the result of the serialization implementation for the value of
        /// type T
        /// @see ben::json::serialize<ben::json::JSONValue>(const ben::json::JSONValue & val)
        template <typename T, std::enable_if_t<converts_to_json_value_v<T>, bool> enabled = true>
        constexpr const std::string serialize(T &&val) noexcept
        {
            return serialize(JSONValue{std::forward<T>(val)});
        }

    } // namespace json

} // namespace ben

#undef bJSON_NAMESPACE
#define bJSON_NAMESPACE() ben::json::

//--Doxygen Documentation-----------------------------------------------------------------------------------------------

// clang-format off

//--JSONSerializationInfo<JSONValue::LiteralType> Documentation---------------------------------------------------------

/// @struct ben::json::JSONSerializationInfo<JSONValue::LiteralType>
/// @brief specialization for JSONValue::LiteralType typed JSONSerializationInfo

/// @typedef ben::json::JSONSerializationInfo<JSONValue::LiteralType>::SerializationFnType
/// @brief an alias for a pointer to a function accepting a const JSONValue::LiteralType& as the single argument
/// which returns a const std::string

/// @var constexpr bool ben::json::JSONSerializationInfo<JSONValue::LiteralType>::serializable
/// @brief a constexpr boolean which is true for JSONValue::LiteralTypes

/// @var constexpr ben::json::JSONSerializationInfo<JSONValue::LiteralType>::SerializationFnType ben::json::JSONSerializationInfo<JSONValue::LiteralType>::serializer
/// @brief the serializer function pointer points to the serializer_impl for the JSONValue::LiteralType type

/// @fn constexpr const std::string ben::json::JSONSerializationInfo<JSONValue::LiteralType>::serializer_impl(const JSONValue::LiteralType &val)
/// @brief serializes a JSONValue::LiteralType value
/// @param val the JSONValue::LiteralType to serialize
/// @return a constexpr const std::string containing the serialized JSONValue::LiteralType

//--JSONSerializationInfo<JSONValue::NumberType> Documentation----------------------------------------------------------

/// @struct ben::json::JSONSerializationInfo<JSONValue::NumberType>
/// @brief specialization for JSONValue::NumberType typed JSONSerializationInfo

/// @typedef ben::json::JSONSerializationInfo<JSONValue::NumberType>::SerializationFnType
/// @brief an alias for a pointer to a function accepting a const JSONValue::NumberType& as the single argument
/// which returns a const std::string

/// @var constexpr bool ben::json::JSONSerializationInfo<JSONValue::NumberType>::serializable
/// @brief a constexpr boolean which is true for JSONValue::NumberType

/// @var constexpr ben::json::JSONSerializationInfo<JSONValue::NumberType>::SerializationFnType ben::json::JSONSerializationInfo<JSONValue::NumberType>::serializer
/// @brief the serializer function pointer points to the serializer_impl for the JSONValue::NumberType type

/// @fn const std::string ben::json::JSONSerializationInfo<JSONValue::NumberType>::serializer_impl(const JSONValue::NumberType &val)
/// @brief serializes a JSONValue::NumberType value
/// @param val the JSONValue::NumberType to serialize
/// @return a const std::string containing the serialized JSONValue::NumberType
/// @remark format is _not_ constexpr compatible, so neither is JSONValue::NumberType's serialization

//--JSONSerializationInfo<JSONValue::StringType> Documentation----------------------------------------------------------

/// @struct ben::json::JSONSerializationInfo<JSONValue::StringType>
/// @brief specialization for JSONValue::StringType typed JSONSerializationInfo

/// @typedef ben::json::JSONSerializationInfo<JSONValue::StringType>::SerializationFnType
/// @brief an alias for a pointer to a function accepting a const JSONValue::StringType& as the single argument
/// which returns a const std::string

/// @var constexpr bool ben::json::JSONSerializationInfo<JSONValue::StringType>::serializable
/// @brief a constexpr boolean which is true for JSONValue::StringTypes

/// @var constexpr ben::json::JSONSerializationInfo<JSONValue::StringType>::SerializationFnType ben::json::JSONSerializationInfo<JSONValue::StringType>::serializer
/// @brief the serializer function pointer points to the serializer_impl for the JSONValue::StringType type

/// @fn const std::string ben::json::JSONSerializationInfo<JSONValue::StringType>::serializer_impl(const JSONValue::StringType &val)
/// @brief serializes a JSONValue::StringType value
/// @param val the JSONValue::StringType to serialize
/// @return a const std::string containing the serialized JSONValue::StringType
/// @remark std::regex is _not_ constexpr compatible, so neither is JSONValue::StringType's serialization

//--JSONSerializationInfo<JSONValue::ArrayType> Documentation-----------------------------------------------------------

/// @struct ben::json::JSONSerializationInfo<JSONValue::ArrayType>
/// @brief specialization for JSONValue::ArrayType typed JSONSerializationInfo

/// @typedef ben::json::JSONSerializationInfo<JSONValue::ArrayType>::SerializationFnType
/// @brief an alias for a pointer to a function accepting a const JSONValue::ArrayType& as the single argument
/// which returns a const std::string

/// @var constexpr bool ben::json::JSONSerializationInfo<JSONValue::ArrayType>::serializable
/// @brief a constexpr boolean which is true for JSONValue::ArrayTypes

/// @var constexpr ben::json::JSONSerializationInfo<JSONValue::ArrayType>::SerializationFnType ben::json::JSONSerializationInfo<JSONValue::ArrayType>::serializer
/// @brief the serializer function pointer points to the serializer_impl for the JSONValue::ArrayType type

/// @fn const std::string ben::json::JSONSerializationInfo<JSONValue::ArrayType>::serializer_impl(const JSONValue::ArrayType &val)
/// @brief serializes a JSONValue::ArrayType value
/// @param val the JSONValue::ArrayType to serialize
/// @return a const std::string containing the serialized JSONValue::ArrayType
/// @remark JSONValue is _not_ constexpr compatible (because of JSONValue::NumberType, JSONValue::StringType), so
/// neither is JSONValue::ArrayType's serialization

//--JSONSerializationInfo<JSONValue::ObjectType> Documentation-----------------------------------------------------------

/// @struct ben::json::JSONSerializationInfo<JSONValue::ObjectType>
/// @brief specialization for JSONValue::ObjectType typed JSONSerializationInfo

/// @typedef ben::json::JSONSerializationInfo<JSONValue::ObjectType>::SerializationFnType
/// @brief an alias for a pointer to a function accepting a const JSONValue::ObjectType& as the single argument
/// which returns a const std::string

/// @var constexpr bool ben::json::JSONSerializationInfo<JSONValue::ObjectType>::serializable
/// @brief a constexpr boolean which is true for JSONValue::ObjectTypes

/// @var constexpr ben::json::JSONSerializationInfo<JSONValue::ObjectType>::SerializationFnType ben::json::JSONSerializationInfo<JSONValue::ObjectType>::serializer
/// @brief the serializer function pointer points to the serializer_impl for the JSONValue::ObjectType type

/// @fn const std::string ben::json::JSONSerializationInfo<JSONValue::ObjectType>::serializer_impl(const JSONValue::ObjectType &val)
/// @brief serializes a JSONValue::ObjectType value
/// @param val the JSONValue::ObjectType to serialize
/// @return a const std::string containing the serialized JSONValue::ObjectType
/// @remark JSONValue is _not_ constexpr compatible (because of JSONValue::NumberType, JSONValue::StringType), so
/// neither is JSONValue::ObjectType's serialization

//--JSONSerializationInfo<JSONValue> Documentation----------------------------------------------------------------------

/// @struct ben::json::JSONSerializationInfo<JSONValue>
/// @brief specialization for JSONValue typed JSONSerializationInfo

/// @typedef ben::json::JSONSerializationInfo<JSONValue>::SerializationFnType
/// @brief an alias for a pointer to a function accepting a const JSONValue& as the single argument
/// which returns a const std::string

/// @var constexpr bool ben::json::JSONSerializationInfo<JSONValue>::serializable
/// @brief a constexpr boolean which is true for JSONValues

/// @var constexpr ben::json::JSONSerializationInfo<JSONValue>::SerializationFnType ben::json::JSONSerializationInfo<JSONValue>::serializer
/// @brief the serializer function pointer points to the serializer_impl for the JSONValue type

/// @fn constexpr const std::string ben::json::JSONSerializationInfo<JSONValue>::serializer_impl(const JSONValue &val)
/// @brief serializes a JSONValue value
/// @param val the JSONValue to serialize
/// @return a const std::string containing the serialized JSONValue

// clang-format on

//--Example Usage-----------------------------------------------------------------------------------------------------//
/*                                                                                                                    //
// DO NOT REMOVE THIS SECTION!                                                                                        //
//                                                                                                                    //
// See the above macro usages for examples of how to serialize a type.                                                //
//                                                                                                                    //
// As an additional example, consider the hypothetical struct which has the following definition:                     //
//                                                                                                                    //
//      struct Example                                                                                                //
//      {                                                                                                             //
//          const std::string name{""};                                                                               //
//          Example* const parent{nullptr};                                                                           //
//      };                                                                                                            //
//                                                                                                                    //
// To declare Example as json serializable and to define the serialization implementation, the helper macro is used:  //
//                                                                                                                    //
//      bJSON_MAKE_SERIALIZABLE(Example)                                                                              //
//      {                                                                                                             //
//          if (val.name.empty())                                                                                     //
//          {                                                                                                         //
//              throw std::exception{"Example::name was empty-- cannot serialize to JSON."};                          //
//          }                                                                                                         //
//          std::string serialized{R"""({ "name" : )"""};                                                             //
//          serialized.append(serialize(val.name));                                                                   //
//          serialized.append(R"""(, "parent" : )""");                                                                //
//          if (val.parent)                                                                                           //
//          {                                                                                                         //
//              serialized.append(serialize(val.parent->name));                                                       //
//          }                                                                                                         //
//          else                                                                                                      //
//          {                                                                                                         //
//              serialized.append(serialize(JSONValue::LiteralType::null_v));                                         //
//          }                                                                                                         //
//          serialized.append(" }");                                                                                  //
//          return serialized;                                                                                        //
//      }                                                                                                             //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------*/

//--License-----------------------------------------------------------------------------------------------------------//
/*                                                                                                                    //
// DO NOT REMOVE THIS SECTION!                                                                                        //
//                                                                                                                    //
// MIT License                                                                                                        //
//                                                                                                                    //
// Copyright (c) 2025 sherwoodben                                                                                     //
//                                                                                                                    //
// Permission is hereby granted, free of charge, to any person obtaining a copy                                       //
// of this software and associated documentation files (the "Software"), to deal                                      //
// in the Software without restriction, including without limitation the rights                                       //
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell                                          //
// copies of the Software, and to permit persons to whom the Software is                                              //
// furnished to do so, subject to the following conditions:                                                           //
//                                                                                                                    //
// The above copyright notice and this permission notice shall be included in all                                     //
// copies or substantial portions of the Software.                                                                    //
//                                                                                                                    //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR                                         //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,                                           //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE                                        //
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER                                             //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,                                      //
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE                                      //
// SOFTWARE.                                                                                                          //
//--------------------------------------------------------------------------------------------------------------------*/