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
        const std::u8string name{u8""};
        Example *const      parent{nullptr};
    };

} // namespace

// example struct serialization implementation based on example usage documentation
bJSON_MAKE_SERIALIZABLE(Example)
{
    if (val.name.empty())
    {
        throw std::exception{"Example::name was empty-- cannot serialize to JSON."};
    }
    std::u8string serialized{u8R"""({ "name" : )"""};
    serialized.append(serialize(val.name));
    serialized.append(u8R"""(, "parent" : )""");
    if (val.parent)
    {
        serialized.append(serialize(val.parent->name));
    }
    else
    {
        serialized.append(serialize(JSONValue::LiteralType::null_v));
    }
    serialized.append(u8" }");
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
    bTEST_ASSERT(serialize(JSONValue::LiteralType::null_v) == u8"null");
    bTEST_ASSERT(serialize(JSONValue::LiteralType::true_v) == u8"true");
    bTEST_ASSERT(serialize(JSONValue::LiteralType::false_v) == u8"false");

    bTEST_ASSERT(JSONSerializationInfo<JSONValue::NumberType>::serializable);
    bTEST_ASSERT(serialize<JSONValue::NumberType>(3.14L) == u8"3.14");
    bTEST_ASSERT(serialize<JSONValue::NumberType>(1) == u8"1");
    bTEST_ASSERT(serialize<JSONValue::NumberType>(0u) == u8"0");
    bTEST_ASSERT(serialize<JSONValue::NumberType>(10l) == u8"10");

    bTEST_ASSERT(JSONSerializationInfo<JSONValue::StringType>::serializable);
    bTEST_ASSERT(serialize<JSONValue::StringType>(std::u8string{u8"test string"}) == u8"\"test string\"");

    bTEST_ASSERT(JSONSerializationInfo<JSONValue::ArrayType>::serializable);
    bTEST_ASSERT(
        serialize<JSONValue::ArrayType>(
            {JSONValue{JSONValue::LiteralType::true_v}, JSONValue{3.14}, JSONValue{u8"test"}}) ==
        u8R"""([ true, 3.14, "test" ])""");

    bTEST_ASSERT(JSONSerializationInfo<JSONValue::ObjectType>::serializable);
    bTEST_ASSERT(
        serialize<JSONValue::ObjectType>({
            {{u8"1", JSONValue{JSONValue::LiteralType::true_v}},
             {u8"2", JSONValue{3.14}},
             {u8"3", JSONValue{u8"test"}}}
    }) == u8R"""({ "1" : true, "2" : 3.14, "3" : "test" })""");

    // make sure "undefined" types are not serialized...
    bTEST_ASSERT(serialize<JSONValue>(JSONValue{}) == u8"");
};

/// @brief ensures that strings which contain special characters (backslashes, newline characters, etc.) convert the
/// special characters to their (escaped) JSON equivalent
bTEST_FUNCTION(strings_transform_special_characters, "serialization")
{
    using namespace ben::json;

    // need to make sure the "special characters" are transformed when encoding into JSON

    bTEST_ASSERT(serialize(std::u8string{u8"\\"}) == u8R"""("\\")""");

    // For some reason when using R"""(...)""" with this string, doxygen's preprocessor macro breaks. Weird, but this is
    // the workaround I found! Maybe it has something to do with the "extra" quote?
    bTEST_ASSERT(serialize(std::u8string{u8"\""}) == u8"\"\\\"\"");

    bTEST_ASSERT(serialize(std::u8string{u8"\n"}) == u8R"""("\n")""");
    bTEST_ASSERT(serialize(std::u8string{u8"\r"}) == u8R"""("\r")""");
    bTEST_ASSERT(serialize(std::u8string{u8"\t"}) == u8R"""("\t")""");
    bTEST_ASSERT(serialize(std::u8string{u8"\f"}) == u8R"""("\f")""");
    bTEST_ASSERT(serialize(std::u8string{u8"\b"}) == u8R"""("\b")""");

    // now be sure the "control characters" 0x00 through 0x1F become their "transformed" unicode escape versions...
    bTEST_ASSERT(serialize(std::u8string{u8'\u0000'}) == u8R"""("\u0000")""");
    // ... assmume the rest work!
};

/// @brief ensures that during serialization of an array, empty/undefined values are _not_ serialized (or rather,
/// they're serialized as an empty string)
bTEST_FUNCTION(arrays_ignore_undefined_values, "serialization")
{
    using namespace ben::json;

    bTEST_ASSERT(
        serialize(JSONValue::ArrayType{JSONValue{JSONValue::LiteralType::true_v}, JSONValue{}, JSONValue{u8"test"}}) ==
        u8R"""([ true, "test" ])""");
};

/// @brief ensures that during serialization of an object, keys with empty/undefined values are _not_ serialized (or
/// rather, they're serialized as an empty string)
bTEST_FUNCTION(objects_ignore_undefined_values, "serialization")
{
    using namespace ben::json;

    bTEST_ASSERT(
        serialize(JSONValue::ObjectType{
            {{u8"1", JSONValue{JSONValue::LiteralType::true_v}},
             {u8"2", JSONValue{}},
             {u8"3", JSONValue{u8"test"}}}
    }) == u8R"""({ "1" : true, "3" : "test" })""");
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
    bTEST_ASSERT(serialize(3ul) == serialize<JSONValue::NumberType>(3.0));
    bTEST_ASSERT(serialize(size_t{3}) == serialize<JSONValue::NumberType>(3.0));
    // assume other numeric types work as well...

    // we can serialize char8_t* u8string literals... (this is why everything has been wrapped in a u8 string
    // constructor up to this point)
    bTEST_ASSERT(serialize(u8"test") == serialize(std::u8string(u8"test")));

    // we need a "smarter"/more nuanced way to handle char* and string conversions so we'll save those for a later time

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

    Example e1{.name = u8"top-level"};
    Example e2{.name = u8"child", .parent = &e1};
    Example e3{.name = u8""};

    bTEST_ASSERT(serialize(e1) == u8R"""({ "name" : "top-level", "parent" : null })""");
    bTEST_ASSERT(serialize(e2) == u8R"""({ "name" : "child", "parent" : "top-level" })""");
    bTEST_ASSERT(serialize(e3) == u8"");
};