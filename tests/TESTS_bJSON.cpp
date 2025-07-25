/// @file TESTS_bJSON.cpp
/// @brief houses tests for the JSON serialization/parsing capabilities.
///
/// Designed to utilize the bUnitTests framework.

#include "bJSON.h"
#include "bUnitTests.h"

//--"PRIVATE" TEST VALUES-----------------------------------------------------------------------------------------------

namespace
{
    using namespace ben::json;

    /// @brief example struct based on example usage documentation
    struct Example
    {
        const std::string name{""};
        Example *const    parent{nullptr};
    };

} // namespace

// example struct serialization implementation based on example usage documentation
bJSON_MAKE_SERIALIZABLE(Example)
{
    if (val.name.empty())
    {
        throw std::exception{"Example::name was empty-- cannot serialize to JSON."};
    }
    std::string serialized{R"""({ "name" : )"""};
    serialized.append(serialize(val.name));
    serialized.append(R"""(, "parent" : )""");
    if (val.parent)
    {
        serialized.append(serialize(val.parent->name));
    }
    else
    {
        serialized.append(serialize(JSONValue::LiteralType::null_v));
    }
    serialized.append(" }");
    return serialized;
}

//--TESTS---------------------------------------------------------------------------------------------------------------

/// @brief ensures the types which are stored in the std::variant value of JSONValue objects are serializable as we
/// would expect
bTEST_FUNCTION(types_are_serializable, "serialization")
{
    using namespace ben::json;

    // need to specify type until conversions are implemented

    bTEST_ASSERT(JSONSerializationInfo<JSONValue::LiteralType>::serializable);
    bTEST_ASSERT(serialize(JSONValue::LiteralType::null_v) == "null");
    bTEST_ASSERT(serialize(JSONValue::LiteralType::true_v) == "true");
    bTEST_ASSERT(serialize(JSONValue::LiteralType::false_v) == "false");

    bTEST_ASSERT(JSONSerializationInfo<JSONValue::NumberType>::serializable);
    bTEST_ASSERT(serialize<JSONValue::NumberType>(3.14L) == std::format("{}", 3.14));
    bTEST_ASSERT(serialize<JSONValue::NumberType>(1) == std::format("{}", 1));
    bTEST_ASSERT(serialize<JSONValue::NumberType>(0u) == std::format("{}", 0u));
    bTEST_ASSERT(serialize<JSONValue::NumberType>(10l) == std::format("{}", 10l));

    bTEST_ASSERT(JSONSerializationInfo<JSONValue::StringType>::serializable);
    bTEST_ASSERT(serialize<JSONValue::StringType>("test string") == "\"test string\"");

    bTEST_ASSERT(JSONSerializationInfo<JSONValue::ArrayType>::serializable);
    bTEST_ASSERT(
        serialize<JSONValue::ArrayType>(
            {JSONValue{JSONValue::LiteralType::true_v}, JSONValue{3.14}, JSONValue{"test"}}) ==
        R"""([ true, 3.14, "test" ])""");

    bTEST_ASSERT(JSONSerializationInfo<JSONValue::ObjectType>::serializable);
    bTEST_ASSERT(
        serialize<JSONValue::ObjectType>({
            {{"1", JSONValue{JSONValue::LiteralType::true_v}}, {"2", JSONValue{3.14}}, {"3", JSONValue{"test"}}}
    }) ==
        R"""({ "1" : true, "2" : 3.14, "3" : "test" })""");

    // make sure "undefined" types are not serialized...
    bTEST_ASSERT(serialize<JSONValue>(JSONValue{}) == "");
};

/// @brief ensures that strings which contain special characters (backslashes, newline characters, etc.) convert the
/// special characters to their (escaped) JSON equivalent
bTEST_FUNCTION(strings_transform_special_characters, "serialization")
{
    using namespace ben::json;

    // need to make sure the "special characters" are transformed when encoding into JSON

    bTEST_ASSERT(serialize<std::string>("\\") == R"""("\\")""");

    // For some reason when using R"""(...)""" with this string, doxygen's preprocessor macro breaks. Weird, but this is
    // the workaround I found! Maybe it has something to do with the "extra" quote?
    bTEST_ASSERT(serialize<std::string>("\"") == "\"\\\"\"");

    bTEST_ASSERT(serialize<std::string>("\n") == R"""("\n")""");
    bTEST_ASSERT(serialize<std::string>("\r") == R"""("\r")""");
    bTEST_ASSERT(serialize<std::string>("\t") == R"""("\t")""");
    bTEST_ASSERT(serialize<std::string>("\f") == R"""("\f")""");
    bTEST_ASSERT(serialize<std::string>("\b") == R"""("\b")""");
};

/// @brief ensures that during serialization of an array, empty/undefined values are _not_ serialized (or rather,
/// they're serialized as an empty string)
bTEST_FUNCTION(arrays_ignore_undefined_values, "serialization")
{
    using namespace ben::json;

    bTEST_ASSERT(
        serialize(JSONValue::ArrayType{JSONValue{JSONValue::LiteralType::true_v}, JSONValue{}, JSONValue{"test"}}) ==
        R"""([ true, "test" ])""");
};

/// @brief ensures that during serialization of an object, keys with empty/undefined values are _not_ serialized (or
/// rather, they're serialized as an empty string)
bTEST_FUNCTION(objects_ignore_undefined_values, "serialization")
{
    using namespace ben::json;

    bTEST_ASSERT(
        serialize(JSONValue::ObjectType{
            {{"1", JSONValue{JSONValue::LiteralType::true_v}}, {"2", JSONValue{}}, {"3", JSONValue{"test"}}}
    }) ==
        R"""({ "1" : true, "3" : "test" })""");
};

/// @brief ensures that serialization of types which are not directly serializable but which are convertible to JSON
/// values are serializable. For example, JSONValues which hold numbers are constructed with long doubles. Why shouldn't
/// we be able to serialize any number type, though? Just convert it to a long double and use it to construct a
/// JSONValue, then serialize that JSONValue!
bTEST_FUNCTION(conversion_types_work_correctly, "serialization")
{
    using namespace ben::json;

    // bools convert to literals such that:
    //      (bool) true --> (literal) true_v
    //      (bool) false --> (literal) false_v
    bTEST_ASSERT(serialize(false) == serialize(JSONValue::LiteralType::false_v));
    bTEST_ASSERT(serialize(true) == serialize(JSONValue::LiteralType::true_v));

    // we can serialize any numerical type and it serializes as if it were a JSONValue::Number type:
    bTEST_ASSERT(serialize(3.0) == serialize<JSONValue::NumberType>(3.0));
    bTEST_ASSERT(serialize(3u) == serialize<JSONValue::NumberType>(3.0));
    bTEST_ASSERT(serialize(3.f) == serialize<JSONValue::NumberType>(3.0));
    bTEST_ASSERT(serialize(3ul) == serialize<JSONValue::NumberType>(3));
    bTEST_ASSERT(serialize(size_t{3}) == serialize<JSONValue::NumberType>(3));
    // assume other numeric types work as well...

    // we can serialize char* string literals...
    bTEST_ASSERT(serialize("test") == serialize(std::string("test")));

    // will add more conversion tests here as new conversions are added! We hit the "big three" though --
    // bools/literals, numbers, and strings!
};

/// @brief ensures that serialization of a type which has been provided a serialization implementation by the user works
/// as expected
bTEST_FUNCTION(example_type_works_correctly, "serialization")
{
    using namespace ben::json;

    /// asserts that a type which has been given a JSON serialization implementation is known to be JSON serializable
    static_assert(
        is_json_serializable_v<Example>,
        "The serialization helper macro must result in the type being serializable!");

    Example e1{.name = "top-level"};
    Example e2{.name = "child", .parent = &e1};
    Example e3{.name = ""};

    bTEST_ASSERT(serialize(e1) == R"""({ "name" : "top-level", "parent" : null })""");
    bTEST_ASSERT(serialize(e2) == R"""({ "name" : "child", "parent" : "top-level" })""");
    bTEST_ASSERT(serialize(e3) == "");
};

/// @def RELEASE_CONSTEXPR
/// @brief we only want to test constexpr functionality on release right now (due to small string
/// optimization) but honestly, this behavior _SHOULD NOT_ be relied on.

/// @brief ensures that constexpr evaluation is possible for (allowable) constexpr serializable types
/// @note constexpr with std::string is known to not be the greatest decision. See discussion in bJSON.h.
/// @see bJSON.h
bTEST_FUNCTION(constexpr_serializable_types_work, "serialization")
{
#if not defined(DEBUG)
#    define RELEASE_CONSTEXPR constexpr
#else
#    define RELEASE_CONSTEXPR
#endif // !DEBUG

    using namespace ben::json;

    // bools/JSONLiterals are constexpr serializable...
    RELEASE_CONSTEXPR const auto test1{serialize(true)};
    bTEST_ASSERT(test1 == "true");

    RELEASE_CONSTEXPR const auto test2{serialize(JSONValue::JSONLiteralType::null_v)};
    bTEST_ASSERT(test2 == "null");

    // arrays with ONLY bool or JSONLiteral values are constexpr serializable...
    RELEASE_CONSTEXPR const auto test3{
        serialize(JSONValue::ArrayType{JSONValue{false}, JSONValue{JSONValue::LiteralType::true_v}})};
    bTEST_ASSERT(test3 == "[ false, true ]");

    // Note: constexpr does NOT work here because of small string optimization (which is actually why the others _do_
    // work in the first place):

    /*RELEASE_CONSTEXPR*/ const auto test4{serialize(JSONValue::ArrayType{
        JSONValue{false},
        JSONValue{false},
        JSONValue{false},
        JSONValue{false},
        JSONValue{false},
        JSONValue{false},
        JSONValue{false},
        JSONValue{false},
        JSONValue{false},
        JSONValue{false}})};
    bTEST_ASSERT(test4 == "[ false, false, false, false, false, false, false, false, false, false ]");

    // at this point, numbers and strings are not constexpr serializable because they make use of std::format and
    // std::regex respectively. Since JSONObject serialization is dependent on string serialization, it is also not
    // constexpr serializable at this time.

#undef RELEASE_CONSTEXPR
};
