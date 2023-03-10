/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2017 Christian Schoenebeck                              *
 *                      <cuse@users.sourceforge.net>                       *
 *                                                                         *
 *   This library is part of libgig.                                       *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#include <typeinfo>
#include <map>
#include <string>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // for strdup()

#if defined _MSC_VER // Microsoft compiler ...
# define RAW_CPP_TYPENAME(t) t.raw_name()
#else // i.e. especially GCC and clang ...
# define RAW_CPP_TYPENAME(t) t.name()
#endif

#define RAW_CPP_TYPENAME_OF(type) RAW_CPP_TYPENAME(typeid(type))

#define GIG_DECLARE_ENUM(type, ...) \
    enum type { __VA_ARGS__ }; \
    \
    struct type##InfoRegistrator { \
        type##InfoRegistrator() { \
            const char* typeName = RAW_CPP_TYPENAME_OF(type); \
            g_enumsByRawTypeName[typeName] = _parseEnumBody( #__VA_ARGS__ ); \
        } \
    }; \
    \
    static type##InfoRegistrator g_##type##InfoRegistrator

struct EnumDeclaration {
    std::map<size_t, std::string> nameByValue;
    std::map<std::string, size_t> valueByName;
    char** allKeys;

    EnumDeclaration() : allKeys(NULL) {}

    const size_t countKeys() const { return valueByName.size(); }

    void loadAllKeys() {
        const size_t n = valueByName.size();
        allKeys = new char*[n + 1];
        size_t i = 0;
        for (std::map<std::string, size_t>::const_iterator it = valueByName.begin();
             it != valueByName.end(); ++it, ++i)
        {
            allKeys[i] = strdup(it->first.c_str());
        }
        allKeys[n] = NULL;
    }
};

struct EnumKeyVal {
    std::string name;
    size_t value;
    bool isValid() const { return !name.empty(); }
};

static std::map<std::string, EnumDeclaration> g_enumsByRawTypeName;
static std::map<std::string, size_t> g_allEnumValuesByKey;



// *************** Internal functions **************
// *

static inline bool isWhiteSpace(const char& c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static size_t decodeEnumValue(const std::string& encodedValue) {
    return strtoll(encodedValue.c_str(), NULL, 0 /* auto detect base */);
}

static EnumKeyVal _parseEnumKeyVal(const char* start, const char* end, size_t value) {
    EnumKeyVal keyval;

    if (start >= end)
        return keyval; // invalid
    // ignore white space
    for (; isWhiteSpace(*start); ++start)
        if (start >= end)
            return keyval; // invalid
    if (start >= end)
        return keyval; // invalid

    // parse key
    for (const char* p = start; true; ++p) {
        if (p >= end || isWhiteSpace(*p) || *p == '=') {
            const size_t sz = p - start;
            keyval.name.resize(sz);
            memcpy(&keyval.name[0], start, sz);
            keyval.value = value;
            start = p + 1;
            break;
        }
    }

    if (start >= end)
        return keyval; // valid, no explicit value provided
    // seek forward to start of value
    for (; isWhiteSpace(*start) || *start == '='; ++start)
        if (start >= end)
            return keyval; // valid, no explicit value provided
    if (start >= end)
        return keyval; // valid, no explicit value provided

    std::string encodedValue;
    // parse value portion
    for (const char* p = start; true; ++p) {
        if (p >= end || isWhiteSpace(*p)) {
            const size_t sz = p - start;
            encodedValue.resize(sz);
            memcpy(&encodedValue[0], start, sz);
            break;
        }
    }

    if (encodedValue.empty())
        return keyval; // valid, no explicit value provided

    keyval.value = decodeEnumValue(encodedValue);

    return keyval;
}

static EnumDeclaration _parseEnumBody(const char* body) {
    EnumDeclaration decl;
    size_t value = 0;
    for (const char* a = body, *b = body; true; ++b) {
        if (*b == 0 || *b == ',') {
            const EnumKeyVal keyval = _parseEnumKeyVal(a, b, value);
            if (!keyval.isValid()) break;
            decl.nameByValue[keyval.value] = keyval.name;
            decl.valueByName[keyval.name] = keyval.value;
            g_allEnumValuesByKey[keyval.name] = keyval.value;
            value = keyval.value + 1;
            if (*b == 0) break;
            a = b + 1;
        }
    }
    return decl;
}

#include "gig.h"

// *************** gig API functions **************
// *

namespace gig {

    // this has to be defined manually, since leverarge_ctrl_t::type_t is a
    // class internal member type
    static leverage_ctrl_t::type_tInfoRegistrator g_leverageCtrlTTypeT;


    /** @brief Amount of elements in given enum type.
     *
     * Returns the amount of elements of the enum type with raw C++ type name
     * @a typeName. If the requested enum type is unknown, then this function
     * returns @c 0 instead.
     *
     * Note: you @b MUST pass the raw C++ type name, not a demangled human
     * readable C++ type name. On doubt use the overloaded function which takes
     * a @c std::type_info as argument instead.
     *
     * @param typeName - raw C++ type name of enum
     * @returns enum's amount of elements
     */
    size_t enumCount(String typeName) {
        if (!g_enumsByRawTypeName.count(typeName))
            return 0;
        return g_enumsByRawTypeName[typeName].countKeys();
    }

    /** @brief Amount of elements in given enum type.
     *
     * Returns the amount of elements of the enum type given by @a type. If the
     * requested enum type is unknown, then this function returns @c 0 instead.
     *
     * Use the @c typeid() keyword of C++ to get a @c std::type_info object.
     *
     * @param type - enum type of interest
     * @returns enum's amount of elements
     */
    size_t enumCount(const std::type_info& type) {
        return enumCount(RAW_CPP_TYPENAME(type));
    }

    /** @brief Numeric value of enum constant.
     *
     * Returns the numeric value (assigned at library compile time) to the enum
     * constant with given @a name. If the requested enum constant is unknown,
     * then this function returns @c 0 instead.
     *
     * @param key - enum constant name
     * @returns enum constant's numeric value
     */
    size_t enumValue(String key) {
        if (!g_allEnumValuesByKey.count(key))
            return 0;
        return g_allEnumValuesByKey[key];
    }

    /** @brief Check if enum element exists.
     *
     * Checks whether the enum constant with name @a key of enum type with raw
     * C++ enum type name @a typeName exists. If either the requested enum type
     * or enum constant is unknown, then this function returns @c false instead.
     *
     * Note: you @b MUST pass the raw C++ type name, not a demangled human
     * readable C++ type name. On doubt use the overloaded function which takes
     * a @c std::type_info as argument instead.
     *
     * @param typeName - raw C++ type name of enum
     * @param key - name of enum constant
     * @returns @c true if requested enum element exists
     */
    bool enumKey(String typeName, String key) {
        if (!g_enumsByRawTypeName.count(typeName))
            return false;
        return g_enumsByRawTypeName[typeName].valueByName.count(key);
    }

    /** @brief Check if enum element exists.
     *
     * Checks whether the enum constant with name @a key of requested enum
     * @a type exists. If either the requested enum type or enum constant is
     * unknown, then this function returns @c false instead.
     *
     * Use the @c typeid() keyword of C++ to get a @c std::type_info object.
     *
     * @param type - enum type of interest
     * @param key - name of enum constant
     * @returns @c true if requested enum element exists
     */
    bool enumKey(const std::type_info& type, String key) {
        return enumKey(RAW_CPP_TYPENAME(type), key);
    }

    /** @brief Enum constant name of numeric value.
     *
     * Returns the enum constant name (a.k.a. enum element name) for the given
     * numeric @a value and the enum type with raw C++ enum type name
     * @a typeName. If either the requested enum type or enum constant numeric
     * value is unknown, then this function returns @c NULL instead.
     *
     * If the requested enum type contains several enum elements with the
     * requested numeric enum value, then this function will simply return one
     * of them, it is undefined which one it would return exactly in this case.
     *
     * Note: you @b MUST pass the raw C++ type name, not a demangled human
     * readable C++ type name. On doubt use the overloaded function which takes
     * a @c std::type_info as argument instead.
     *
     * @param typeName - raw C++ type name of enum
     * @param value - numeric value of sought enum constant
     * @returns @c true if requested enum element exists
     */
    const char* enumKey(String typeName, size_t value) {
        if (!g_enumsByRawTypeName.count(typeName))
            return NULL;
        if (!g_enumsByRawTypeName[typeName].nameByValue.count(value))
            return NULL;
        return g_enumsByRawTypeName[typeName].nameByValue[value].c_str();
    }

    /** @brief Enum constant name of numeric value.
     *
     * Returns the enum constant name (a.k.a. enum element name) for the given
     * numeric @a value and the given enum @a type. If either the requested
     * enum type or enum constant numeric value is unknown, then this function
     * returns @c NULL instead.
     *
     * If the requested enum type contains several enum elements with the
     * requested numeric enum value, then this function will simply return one
     * of them, it is undefined which one it would return exactly in this case.
     *
     * Use the @c typeid() keyword of C++ to get a @c std::type_info object.
     *
     * @param type - enum type of interest
     * @param value - numeric value of sought enum constant
     * @returns @c true if requested enum element exists
     */
    const char* enumKey(const std::type_info& type, size_t value) {
        return enumKey(RAW_CPP_TYPENAME(type), value);
    }

    /** @brief All element names of enum type.
     *
     * Returns a NULL terminated array of C strings of all enum constant names
     * of the given enum type with raw C++ enum type name @a typeName. If the
     * requested enum type is unknown, then this function returns @c NULL
     * instead.
     *
     * Note: you @b MUST pass the raw C++ type name, not a demangled human
     * readable C++ type name. On doubt use the overloaded function which takes
     * a @c std::type_info as argument instead.
     *
     * @param typeName - raw C++ type name of enum
     * @returns list of all enum element names
     */
    const char** enumKeys(String typeName) {
        if (!g_enumsByRawTypeName.count(typeName))
            return NULL;
        if (!g_enumsByRawTypeName[typeName].allKeys)
            g_enumsByRawTypeName[typeName].loadAllKeys();
        return (const char**) g_enumsByRawTypeName[typeName].allKeys;
    }

    /** @brief All element names of enum type.
     *
     * Returns a NULL terminated array of C strings of all enum constant names
     * of the given enum @a type. If the requested enum type is unknown, then
     * this function returns @c NULL instead.
     *
     * Use the @c typeid() keyword of C++ to get a @c std::type_info object.
     *
     * @param type - enum type of interest
     * @returns list of all enum element names
     */
    const char** enumKeys(const std::type_info& type) {
        return enumKeys(RAW_CPP_TYPENAME(type));
    }

} // namespace gig
