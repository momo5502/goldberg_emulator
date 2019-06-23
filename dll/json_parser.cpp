/* Copyright (C) 2019 Nemirtingas
   This file is part of the Goldberg Emulator

   Those utils are free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   Those utils are distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the Goldberg Emulator; if not, see
   <http://www.gnu.org/licenses/>.
*/

#include "json_value.h"

#include <exception>
#include <algorithm>

// This parser can easely be translated to pure C (if you really have to)

#define OBJECT_START     '{'
#define OBJECT_END       '}'
#define ARRAY_START      '['
#define ARRAY_END        ']'
#define VALUE_END        ','
#define STRING_START     '"'
#define STRING_END       '"'
#define NUMBER_COMMA     '.'
#define KEY_VALUE_SEP    ':'
#define TRUE_START       't'
#define FALSE_START      'f'
#define NULL_START       'n'
#define ESCAPE_CHR       '\\'
#define NEGATIVE_CHAR    '-'

#define OBJECT_START_STR  "{"
#define OBJECT_END_STR    "}"
#define ARRAY_START_STR   "["
#define ARRAY_END_STR     "]"
#define VALUE_END_STR     ","
#define STRING_START_STR  "\""
#define STRING_END_STR    "\""
#define NUMBER_COMMA_STR  "."
#define KEY_VALUE_SEP_STR ":"
#define ESCAPE_CHR_STR    "\\"

using str_it = std::string::const_iterator;

namespace nemir
{

    // s = buffer start, c = buffer current, e = buffer end
    nemir::json_value parse_value(str_it& s, str_it& c, str_it& e);

    class parse_exception : public std::exception
    {
        std::string _w_str;
    public:
        parse_exception(std::string const& w) :_w_str(w) {}
        virtual const char* what() const noexcept { return _w_str.c_str(); }
    };

    void throw_parse_exception(std::string const& w, str_it& s, str_it& c, str_it& e)
    {
        std::string what;
        str_it begin, end;
        if (c - s < 10) // check if we can get 10 chars before current position
            begin = s; // debug string is buffer start, not current pos - 10
        else
            begin = c - 10;

        if (e - c < 10) // check if we can get 10 chars after current position
            end = e; // debug string is buffer end, not current pos + 10;
        else
            end = c + 10;

        std::string str_part(begin, end);

        std::replace_if(str_part.begin(), str_part.end(), iscntrl, ' ');

        throw parse_exception(w + " at pos " + std::to_string(c - s) + " " + str_part);
    }

    // Is the current char an end of value char
    // like a space, an end of array, an end of object or an end of object field
    inline bool is_end_of_value(int32_t c) noexcept
    {
        if (isspace(c) || c == ARRAY_END || c == OBJECT_END || c == VALUE_END)
            return true;

        return false;
    }

    // A template func is better than passing a function pointer
    // No pointer dereference to call the function
    template<int(*func)(int)>
    void buffer_seek(str_it& c, str_it& e, int32_t chr) noexcept
    {
        // if our current char != desired char
        // and we are not at the end of the buffer
        while (*c != chr && c != e)
        {
            // If its not a skippable char, exit
            if (!func(*c))
                break;
            // Go to the next char
            ++c;
        }
    }

    // Skip all whitespaces
    inline void ignore_spaces(str_it& c, str_it& e) noexcept
    {
        while (isspace(*c) && ++c != e);
    }

#define FIND_START(type, start) \
template<int(*func)(int)>\
inline void find_##type ## _start(str_it &s, str_it &c, str_it &e)\
{\
    buffer_seek<func>(c, e, start);\
    if( *c != start )\
        throw_parse_exception("Can't find " #type " start '" start ## _STR "'", s, c, e);\
    ++c;\
}

#define FIND_END(type, end) \
template<int(*func)(int)>\
inline void find_##type ## _stop(str_it &s, str_it &c, str_it &e)\
{\
    buffer_seek<func>(c, e, end);\
    if( *c != end )\
        throw_parse_exception("Can't find " #type " end '" end ## _STR "'", s, c, e);\
}

    FIND_START(object, OBJECT_START)
        FIND_START(array, ARRAY_START)
        FIND_START(string, STRING_START)
        FIND_START(key_value_sep, KEY_VALUE_SEP)

        FIND_END(object, OBJECT_END)
        //FIND_END(array, ARRAY_END)
        //FIND_END(string, STRING_END)

        std::string parse_string(str_it& s, str_it& c, str_it& e)
    {
        std::string res;

        find_string_start<isspace>(s, c, e);

        bool is_escape = false;
        while (c != e)
        {
            if (*c == ESCAPE_CHR && !is_escape)
                is_escape = true;
            else
            {
                if (is_escape)
                {
                    switch (*c)
                    {
                    case '\\': res += '\\'; break;
                    case '"': res += '\"'; break;
                    case 't': res += '\t'; break;
                    case 'r': res += '\r'; break;
                    case 'n': res += '\n'; break;
                    case 'f': res += '\f'; break;
                    case 'b': res += '\b'; break;
                    default: throw_parse_exception(std::string("Can't translate escape char \\") + *c, s, c, e);
                    }
                }
                else if (*c == STRING_END)
                {
                    ++c;
                    break;
                }
                else
                    res += *c;
                is_escape = false;
            }
            ++c;
        }
        if (c == e)
            throw_parse_exception("End of buffer encoutered while parsing string", s, c, e);

        //find_string_stop<isprint>(s, c, e);

        return res;
    }

    bool parse_boolean(str_it& s, str_it& c, str_it& e)
    {
        if (tolower(*c) == TRUE_START)
        {
            constexpr const char v[] = "true";
            bool ok = true;
            for (auto i = std::begin(v); i != (std::end(v) - 1); ++i, ++c)
            {
                if (c == e)
                    throw_parse_exception("End of buffer encoutered while parsing boolean", s, c, e);
                if (tolower(*c) != *i)
                {
                    ok = false;
                    break;
                }
            }
            if (!ok || !is_end_of_value(*c))
                throw_parse_exception("Error while parsing boolean 'true' value", s, c, e);

            return true;
        }
        else if (tolower(*c) == FALSE_START)
        {
            constexpr const char v[] = "false";
            bool ok = true;
            for (auto i = std::begin(v); i != (std::end(v) - 1); ++i, ++c)
            {
                if (c == e)
                    throw_parse_exception("End of buffer encoutered while parsing boolean", s, c, e);
                if (tolower(*c) != *i)
                {
                    ok = false;
                    break;
                }
            }
            if (!ok || !is_end_of_value(*c))
                throw_parse_exception("Error while parsing boolean 'false' value", s, c, e);

            return true;
        }
        else
        {
            throw_parse_exception("Error while parsing boolean value", s, c, e);
        }
        // This is never reached but gcc complains it reaches end of non-void function
        return false;
    }

    std::nullptr_t parse_null(str_it& s, str_it& c, str_it& e)
    {
        if (tolower(*c) != NULL_START)
            throw_parse_exception("Error while parsing null value", s, c, e);

        constexpr const char v[] = "null";
        bool ok = true;
        for (auto i = std::begin(v); i != (std::end(v) - 1); ++i, ++c)
        {
            if (c == e)
                throw_parse_exception("End of buffer encoutered while parsing null", s, c, e);
            if (tolower(*c) != *i)
            {
                ok = false;
                break;
            }
        }
        if (!ok || !is_end_of_value(*c))
            throw_parse_exception("Error while parsing null value", s, c, e);

        return nullptr;
    }

    nemir::json_value::json_array parse_array(str_it& s, str_it& c, str_it& e)
    {
        nemir::json_value::json_array res;

        find_array_start<isspace>(s, c, e);

        while (c != e)
        {
            ignore_spaces(c, e);
            if (*c != ARRAY_END)
            {
                res.emplace_back(std::move(parse_value(s, c, e)));
                ignore_spaces(c, e); // Ignore whitespace
            }
            // End of array
            if (*c == ARRAY_END)
            {
                ++c;
                break;
            }
            // End of array entry, restart to find another entry
            if (*c == VALUE_END)
                ++c;
            else
                throw_parse_exception("No end of value found while parsing array", s, c, e);
        }

        return res;
    }

    nemir::json_value::json_object parse_object(str_it& s, str_it& c, str_it& e)
    {
        nemir::json_value::json_object res;

        find_object_start<isspace>(s, c, e);

        while (c != e)
        {
            ignore_spaces(c, e);

            if (*c != OBJECT_END)
            {
                // Must start with a string (the field key)
                std::string key = parse_string(s, c, e);
                find_key_value_sep_start<isspace>(s, c, e);
                ignore_spaces(c, e);
                res.nodes()[key] = std::move(parse_value(s, c, e));
                ignore_spaces(c, e); // Ignore whitespace
            }

            // End of object
            if (*c == OBJECT_END)
            {
                ++c;
                break;
            }
            // End of object entry, restart to find another entry
            else if (*c == VALUE_END)
                ++c;
            // this node is over but we didn't find its end.
            else
                throw_parse_exception("Error while parsing json_object object/node end '" OBJECT_END_STR "/" VALUE_END_STR "' not found", s, c, e);
        }

        return res;
    }

    nemir::json_value parse_value(str_it& s, str_it& c, str_it& e)
    {
        if (isdigit(*c) || *c == NEGATIVE_CHAR)
        {
            // Number start position
            str_it start = c;
            if (*c == NEGATIVE_CHAR)
                ++c;
            bool number = false;
            while (c != e)
            {
                // If its a digit, continue to parse number
                if (isdigit(*c))
                    ++c;
                // If its a Number comma
                else if (*c == NUMBER_COMMA)
                {
                    // Check if we already encoutered a Number comma
                    if (number)
                        throw_parse_exception("Encoutered multiple commas while parsing value", s, c, e);
                    // Continue to parse number
                    number = true;
                    ++c;
                }
                else if (is_end_of_value(*c))
                {
                    if (number)
                        return nemir::json_value_number(stod(std::string(start, c)));
                    else
                        return nemir::json_value_integer(stoi(std::string(start, c)));
                }
                else
                    throw_parse_exception("End of number value not found", s, c, e);
            }
            throw_parse_exception("End of buffer encountered while parsing number value", s, c, e);
        }

        switch (tolower(*c))
        {
            // Check if value is a string
        case STRING_START: return nemir::json_value_string(parse_string(s, c, e));
            // Check if value is true or false
        case TRUE_START: case FALSE_START: return nemir::json_value_boolean(parse_boolean(s, c, e));
            // Check if value is null
        case NULL_START: return nemir::json_value_null(parse_null(s, c, e));
            // Check if value is an array
        case ARRAY_START: return std::move(nemir::json_value_array(parse_array(s, c, e)));
            // Check if value is an object
        case OBJECT_START: return nemir::json_value_object(parse_object(s, c, e));
        default: throw_parse_exception("Invalid value", s, c, e);
        }

        return nemir::json_value(); // Will never reach, just to make compiler happy
    }

    nemir::json_value parse_json(std::string const& buffer)
    {
        str_it s = buffer.begin(); // buffer start
        str_it c = buffer.begin(); // buffer current
        str_it e = buffer.end();   // buffer end

        ignore_spaces(c, e);

        if (*c == OBJECT_START)
            return nemir::json_value_object(parse_object(s, c, e));

        if (*c == ARRAY_START)
            return nemir::json_value_array(parse_array(s, c, e));

        throw_parse_exception("Error while parsing start of Json, must start with an object (" OBJECT_START_STR ") or array (" ARRAY_START_STR ")", s, c, e);
        return nemir::json_value(); // Will never reach, just to make compiler happy
    }

}