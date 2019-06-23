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

#ifndef __JSON_VALUE_INCLUDED__
#define __JSON_VALUE_INCLUDED_

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <exception>

#if 0 // Debug or not
#include <iostream>
#define NEMIR_JSON_INLINE
#else
#define NEMIR_JSON_DISABLE_LOG
#define NEMIR_JSON_INLINE inline
#endif

namespace nemir
{
    class json_object;
    class json_array;

    class json_value_exception : public std::exception
    {
        std::string _w_str;
    public:
        json_value_exception(std::string const& w) :_w_str(w) {}
        virtual const char* what() const noexcept { return _w_str.c_str(); }
    };

    enum class json_type
    {
        string, // std::string
        integer,// int64_t
        number, // double
        array,  // std::vector<json_value>
        object, // json_node
        boolean,// bool
        null    // std::nullptr_t
    };

    constexpr const char* strs[] = {
        "string",
        "integer",
        "number",
        "array",
        "object",
        "boolean",
        "null"
    };

    constexpr const char* json_type_to_string(json_type t)
    {
        return strs[static_cast<int>(t)];
    }

    class json_value_container
    {
    public:
        virtual json_type type() const = 0;
        virtual void* get(json_type) = 0;
        virtual json_value_container* clone() = 0;
    };

    class json_value
    {
        std::shared_ptr<json_value_container> container;

    protected:
        NEMIR_JSON_INLINE void* get(json_type t) const
        {
            return container->get(t);
        }

    public:
        class json_null;
        class json_object;
        class json_array;

        using json_type_string  = std::string;
        using json_type_integer = int64_t;
        using json_type_number  = double;
        using json_type_array   = json_array;
        using json_type_object  = json_object;
        using json_type_boolean = bool;
        using json_type_null    = json_null;

        template<typename T>
        struct json_value_type
        {
        private:
            json_value_type();
            ~json_value_type();
        };

        template<typename T>
        class t_json_value :
            public json_value_container
        {
            T val;
        public:
            using mytype = t_json_value<T>;

            virtual ~t_json_value();
            NEMIR_JSON_INLINE t_json_value();
            NEMIR_JSON_INLINE t_json_value(T const& v);
            NEMIR_JSON_INLINE t_json_value(T&& v)noexcept;
            NEMIR_JSON_INLINE t_json_value(mytype const& v);
            NEMIR_JSON_INLINE t_json_value(mytype&& v)noexcept;
            virtual json_value_container* clone();
            virtual json_type type() const;
            virtual void* get(json_type t);
        };

        class json_object
        {
            std::map<std::string, json_value> _nodes;

        public:
            using iterator = std::map<std::string, json_value>::iterator;
            NEMIR_JSON_INLINE ~json_object();
            NEMIR_JSON_INLINE  json_object();
            NEMIR_JSON_INLINE  json_object(json_object const& c);
            NEMIR_JSON_INLINE  json_object(json_object&& c) noexcept;

            NEMIR_JSON_INLINE json_object& operator=(json_object const& c);
            NEMIR_JSON_INLINE json_object& operator=(json_object&& c) noexcept;

            NEMIR_JSON_INLINE std::map<std::string, json_value>& nodes();
            NEMIR_JSON_INLINE iterator begin();
            NEMIR_JSON_INLINE iterator end();
            NEMIR_JSON_INLINE iterator find(std::string const& key);
            NEMIR_JSON_INLINE iterator find(const char* key);
            NEMIR_JSON_INLINE iterator erase(iterator at);
            NEMIR_JSON_INLINE iterator erase(iterator first, iterator last);
            NEMIR_JSON_INLINE size_t   erase(std::string const& key);

            NEMIR_JSON_INLINE json_value& operator[](std::string const& key);
        };

        class json_array
        {
            std::vector<json_value> _nodes;

        public:
            using iterator = std::vector<json_value>::iterator;
            NEMIR_JSON_INLINE ~json_array();
            NEMIR_JSON_INLINE  json_array();
            NEMIR_JSON_INLINE  json_array(json_array const& c);
            NEMIR_JSON_INLINE  json_array(json_array&& c) noexcept;

            NEMIR_JSON_INLINE json_array& operator=(json_array const& c);
            NEMIR_JSON_INLINE json_array& operator=(json_array&& c) noexcept;

            NEMIR_JSON_INLINE json_value& operator[](size_t key);
            NEMIR_JSON_INLINE std::vector<json_value>& nodes();
            NEMIR_JSON_INLINE iterator begin();
            NEMIR_JSON_INLINE iterator end();

            NEMIR_JSON_INLINE iterator erase(iterator at);
            NEMIR_JSON_INLINE iterator erase(iterator first, iterator last);
            NEMIR_JSON_INLINE iterator insert(iterator at, json_value const& value);
            NEMIR_JSON_INLINE iterator insert(iterator at, json_value&& value);
            NEMIR_JSON_INLINE void push_back(json_value const& v);
            NEMIR_JSON_INLINE void push_back(json_value&& v);

            template<typename ...Args>
            NEMIR_JSON_INLINE void emplace_back(Args&& ...args);
        };

        class json_null
        {
            std::nullptr_t _val;
        public:
            NEMIR_JSON_INLINE ~json_null();
            NEMIR_JSON_INLINE  json_null();
            NEMIR_JSON_INLINE  json_null(std::nullptr_t c);
            NEMIR_JSON_INLINE  json_null(json_null const& c);
            NEMIR_JSON_INLINE  json_null(json_null&& c) noexcept;

            NEMIR_JSON_INLINE json_null& operator=(std::nullptr_t c);
            NEMIR_JSON_INLINE json_null& operator=(json_null const& c);
            NEMIR_JSON_INLINE json_null& operator=(json_null&& c) noexcept;
        };

        // Default constructor, empty value
        NEMIR_JSON_INLINE json_value();
        // Copy constructor, clone from derived class
        NEMIR_JSON_INLINE json_value(json_value const& c);
        // Move constructor, move pointer
        NEMIR_JSON_INLINE json_value(json_value&& c) noexcept;
        // Copy operator
        NEMIR_JSON_INLINE json_value& operator=(json_value const& c);
        // Move operator
        NEMIR_JSON_INLINE json_value& operator=(json_value&& c) noexcept;

        // Json value constuctor with copy t_json_value
        template<typename T>
        NEMIR_JSON_INLINE json_value(t_json_value<T> const& v);
        // Json value constuctor with move t_json_value
        template<typename T>
        NEMIR_JSON_INLINE json_value(t_json_value<T>&& v) noexcept;

#define JSON_VALUE_CONSTRUCTOR_BY_VALUE_DEF(value_type, json_type) \
NEMIR_JSON_INLINE json_value(value_type const &v);\
NEMIR_JSON_INLINE json_value(value_type &&v);\
NEMIR_JSON_INLINE json_value& operator=(value_type const& v);\
NEMIR_JSON_INLINE json_value& operator=(value_type && v);\
NEMIR_JSON_INLINE operator const value_type & () const;

#define JSON_REFERENCE_OPERATOR_DEF(json_type)\
NEMIR_JSON_INLINE operator json_type& ();

        NEMIR_JSON_INLINE json_value(const wchar_t*) = delete;
        NEMIR_JSON_INLINE json_value& operator=(const wchar_t*) = delete;

        NEMIR_JSON_INLINE json_value(const char* v);
        NEMIR_JSON_INLINE json_value& operator=(const char* v);

        JSON_VALUE_CONSTRUCTOR_BY_VALUE_DEF(json_type_string, json_type_string)
        JSON_VALUE_CONSTRUCTOR_BY_VALUE_DEF(int16_t, json_type_integer)
        JSON_VALUE_CONSTRUCTOR_BY_VALUE_DEF(int32_t, json_type_integer)
        JSON_VALUE_CONSTRUCTOR_BY_VALUE_DEF(json_type_integer, json_type_integer)
        JSON_VALUE_CONSTRUCTOR_BY_VALUE_DEF(json_type_number, json_type_number)
        JSON_VALUE_CONSTRUCTOR_BY_VALUE_DEF(json_type_array, json_type_array)
        JSON_VALUE_CONSTRUCTOR_BY_VALUE_DEF(json_type_object, json_type_object)
        JSON_VALUE_CONSTRUCTOR_BY_VALUE_DEF(json_type_null, json_type_null)
        JSON_VALUE_CONSTRUCTOR_BY_VALUE_DEF(json_type_boolean, json_type_boolean)

        JSON_REFERENCE_OPERATOR_DEF(json_type_string)
        JSON_REFERENCE_OPERATOR_DEF(json_type_integer)
        JSON_REFERENCE_OPERATOR_DEF(json_type_number)
        JSON_REFERENCE_OPERATOR_DEF(json_type_array)
        JSON_REFERENCE_OPERATOR_DEF(json_type_object)
        JSON_REFERENCE_OPERATOR_DEF(json_type_null)
        JSON_REFERENCE_OPERATOR_DEF(json_type_boolean)

        NEMIR_JSON_INLINE json_type type() const { return container->type(); }

        NEMIR_JSON_INLINE json_value& operator[](std::string const& key)
        {
            return static_cast<json_type_object&>(*this)[key];
        }
        NEMIR_JSON_INLINE json_value& operator[](const char* key)
        {
            return static_cast<json_type_object&>(*this)[key];
        }
        NEMIR_JSON_INLINE json_value& operator[](size_t key)
        {
            return static_cast<json_type_array&>(*this)[key];
        }
    };
    // End of class json_value

    // Start specializations of json_value_type<T>
    template<>struct json_value::json_value_type<json_value::json_type_string>
    {
        constexpr static json_type type = json_type::string;
    };
    template<>struct json_value::json_value_type<json_value::json_type_integer>
    {
        constexpr static json_type type = json_type::integer;
    };
    template<>struct json_value::json_value_type<json_value::json_type_number>
    {
        constexpr static json_type type = json_type::number;
    };
    template<>struct json_value::json_value_type<json_value::json_type_array>
    {
        constexpr static json_type type = json_type::array;
    };
    template<>struct json_value::json_value_type<json_value::json_type_object>
    {
        constexpr static json_type type = json_type::object;
    };
    template<>struct json_value::json_value_type<json_value::json_type_boolean>
    {
        constexpr static json_type type = json_type::boolean;
    };
    template<>struct json_value::json_value_type<json_value::json_type_null>
    {
        constexpr static json_type type = json_type::null;
    };
    // end of specializations json_value_type<T>

    // start of t_json_value
#ifndef NEMIR_JSON_DISABLE_LOG
    template<typename T>
    json_value::t_json_value<T>::~t_json_value() { std::cout << "~t_json_value()" << std::endl; }
    template<typename T>
    NEMIR_JSON_INLINE json_value::t_json_value<T>::t_json_value() :val() { std::cout << " t_json_value()" << std::endl; }
    template<typename T>
    NEMIR_JSON_INLINE json_value::t_json_value<T>::t_json_value(T const& v) : val(v) { std::cout << " t_json_value(T const&)" << std::endl; }
    template<typename T>
    NEMIR_JSON_INLINE json_value::t_json_value<T>::t_json_value(T&& v)noexcept :val(std::move(v)) { std::cout << " t_json_value(T&&)" << std::endl; }
    template<typename T>
    NEMIR_JSON_INLINE json_value::t_json_value<T>::t_json_value(mytype const& v) : val(v.val) { std::cout << " t_json_value( mytype const& )" << std::endl; }
    template<typename T>
    NEMIR_JSON_INLINE json_value::t_json_value<T>::t_json_value(mytype&& v)noexcept :val(std::move(v.val)) { std::cout << " t_json_value( mytype&& )" << std::endl; }
    template<typename T>
    json_value_container* json_value::t_json_value<T>::clone()
    {
        std::cout << "clone()" << std::endl;
        return new mytype(*this);
    }
#else
    template<typename T>
    json_value::t_json_value<T>::~t_json_value() {}
    template<typename T>
    NEMIR_JSON_INLINE json_value::t_json_value<T>::t_json_value() :val() {}
    template<typename T>
    NEMIR_JSON_INLINE json_value::t_json_value<T>::t_json_value(T const& v) : val(v) {}
    template<typename T>
    NEMIR_JSON_INLINE json_value::t_json_value<T>::t_json_value(T&& v)noexcept :val(std::move(v)) {}
    template<typename T>
    NEMIR_JSON_INLINE json_value::t_json_value<T>::t_json_value(mytype const& v) : val(v.val) {}
    template<typename T>
    NEMIR_JSON_INLINE json_value::t_json_value<T>::t_json_value(mytype&& v)noexcept :val(std::move(v.val)) {}
    template<typename T>
    json_value_container* json_value::t_json_value<T>::clone() { return new mytype(*this); }
#endif
    template<typename T>
    json_type json_value::t_json_value<T>::type() const { return json_value_type<T>::type; }
    template<typename T>
    void* json_value::t_json_value<T>::get(json_type t)
    {
        if (t != type())
            throw json_value_exception(std::string("Cannot convert json_type ") + json_type_to_string(type()) + " to " + json_type_to_string(t));
        return &val;
    }
    // end of t_json_value
    // start of json_object

#ifndef NEMIR_JSON_DISABLE_LOG
    NEMIR_JSON_INLINE json_value::json_object::~json_object() { std::cout << "~json_object()" << std::endl; }
    NEMIR_JSON_INLINE json_value::json_object::json_object() { std::cout << " json_object()" << std::endl; }
    NEMIR_JSON_INLINE json_value::json_object::json_object(json_object const& c) :_nodes(c._nodes) { std::cout << " json_object(json_object const&)" << std::endl; }
    NEMIR_JSON_INLINE json_value::json_object::json_object(json_object&& c) noexcept :_nodes(std::move(c._nodes)) { std::cout << " json_object(json_object &&)" << std::endl; }
#else
    NEMIR_JSON_INLINE json_value::json_object::~json_object() {}
    NEMIR_JSON_INLINE json_value::json_object::json_object() {}
    NEMIR_JSON_INLINE json_value::json_object::json_object(json_object const& c) : _nodes(c._nodes) {}
    NEMIR_JSON_INLINE json_value::json_object::json_object(json_object&& c) noexcept :_nodes(std::move(c._nodes)) {}
#endif

    NEMIR_JSON_INLINE json_value::json_object& json_value::json_object::operator=(json_object const& c)
    {
        _nodes = c._nodes;
        return *this;
    }

    NEMIR_JSON_INLINE json_value::json_object& json_value::json_object::operator=(json_object&& c) noexcept
    {
        _nodes = std::move(c._nodes);
        return *this;
    }

    NEMIR_JSON_INLINE std::map<std::string, json_value>& json_value::json_object::nodes() { return _nodes; }
    NEMIR_JSON_INLINE json_value::json_object::iterator json_value::json_object::begin() { return _nodes.begin(); }
    NEMIR_JSON_INLINE json_value::json_object::iterator json_value::json_object::end() { return _nodes.end(); }
    NEMIR_JSON_INLINE json_value::json_object::iterator json_value::json_object::find(std::string const& key) { return _nodes.find(key); }
    NEMIR_JSON_INLINE json_value::json_object::iterator json_value::json_object::find(const char* key) { return find(std::string(key)); }
    NEMIR_JSON_INLINE json_value::json_object::iterator json_value::json_object::erase(iterator at) { return _nodes.erase(at); }
    NEMIR_JSON_INLINE json_value::json_object::iterator json_value::json_object::erase(iterator first, iterator last) { return _nodes.erase(first, last); }
    NEMIR_JSON_INLINE size_t json_value::json_object::erase(std::string const& key) { return _nodes.erase(key); }

    NEMIR_JSON_INLINE json_value& json_value::json_object::operator[](std::string const& key) { return _nodes[key]; }

    // end of json_object
    // start of json_array

#ifndef NEMIR_JSON_DISABLE_LOG
    NEMIR_JSON_INLINE json_value::json_object::~json_array() { std::cout << "~json_array()" << std::endl; }
    NEMIR_JSON_INLINE json_value::json_object::json_array() { std::cout << " json_array()" << std::endl; }
    NEMIR_JSON_INLINE json_value::json_object::json_array(json_array const& c) :_nodes(c._nodes) { std::cout << " json_array(json_array const&)" << std::endl; }
    NEMIR_JSON_INLINE json_value::json_object::json_array(json_array&& c) noexcept :_nodes(std::move(c._nodes)) { std::cout << " json_array(json_array &&)" << std::endl; }
#else
    NEMIR_JSON_INLINE json_value::json_array::~json_array() {}
    NEMIR_JSON_INLINE json_value::json_array::json_array() {}
    NEMIR_JSON_INLINE json_value::json_array::json_array(json_array const& c) : _nodes(c._nodes) {}
    NEMIR_JSON_INLINE json_value::json_array::json_array(json_array&& c) noexcept :_nodes(std::move(c._nodes)) {}
#endif

    NEMIR_JSON_INLINE json_value::json_array& json_value::json_array::operator=(json_array const& c)
    {
        _nodes = c._nodes;
        return *this;
    }

    NEMIR_JSON_INLINE json_value::json_array& json_value::json_array::operator=(json_array&& c) noexcept
    {
        _nodes = std::move(c._nodes);
        return *this;
    }

    NEMIR_JSON_INLINE json_value& json_value::json_array::operator[](size_t key) { return _nodes[key]; }
    NEMIR_JSON_INLINE std::vector<json_value>& json_value::json_array::nodes() { return _nodes; }
    NEMIR_JSON_INLINE json_value::json_array::iterator json_value::json_array::begin() { return _nodes.begin(); }
    NEMIR_JSON_INLINE json_value::json_array::iterator json_value::json_array::end() { return _nodes.end(); }

    NEMIR_JSON_INLINE json_value::json_array::iterator json_value::json_array::erase(iterator at) { return _nodes.erase(at); }
    NEMIR_JSON_INLINE json_value::json_array::iterator json_value::json_array::erase(iterator first, iterator last) { return _nodes.erase(first, last); }
    NEMIR_JSON_INLINE json_value::json_array::iterator json_value::json_array::insert(iterator at, json_value const& value) { return _nodes.insert(at, value); }
    NEMIR_JSON_INLINE json_value::json_array::iterator json_value::json_array::insert(iterator at, json_value&& value) { return _nodes.insert(at, std::move(value)); }
    NEMIR_JSON_INLINE void json_value::json_array::push_back(json_value const& v) { _nodes.push_back(v); }
    NEMIR_JSON_INLINE void json_value::json_array::push_back(json_value&& v) { _nodes.push_back(std::move(v)); }

    template<typename ...Args>
    NEMIR_JSON_INLINE void json_value::json_array::emplace_back(Args&& ...args) { _nodes.emplace_back(std::move(args...)); }

    // end of json_array
    // start of json_null
    NEMIR_JSON_INLINE json_value::json_null::~json_null() {}
    NEMIR_JSON_INLINE json_value::json_null::json_null() :_val(nullptr) {}
    NEMIR_JSON_INLINE json_value::json_null::json_null(std::nullptr_t c) : _val(nullptr) {}
    NEMIR_JSON_INLINE json_value::json_null::json_null(json_null const& c) : _val(nullptr) {}
    NEMIR_JSON_INLINE json_value::json_null::json_null(json_null&& c) noexcept :_val(nullptr) {}

    NEMIR_JSON_INLINE json_value::json_null& json_value::json_null::operator=(std::nullptr_t c)
    {
        _val = nullptr;
        return *this;
    }
    NEMIR_JSON_INLINE json_value::json_null& json_value::json_null::operator=(json_null const& c)
    {
        _val = nullptr;
        return *this;
    }
    NEMIR_JSON_INLINE json_value::json_null& json_value::json_null::operator=(json_null&& c) noexcept
    {
        _val = nullptr;
        return *this;
    }
    // end of json_null
    // start of json_value

    // Default constructor, empty value
    NEMIR_JSON_INLINE json_value::json_value() : container(new t_json_value<json_type_null>)
    {}
    // Copy constructor, clone from derived class
    NEMIR_JSON_INLINE json_value::json_value(json_value const& c) : container(c.container->clone())
    {}
    // Move constructor, move pointer
    NEMIR_JSON_INLINE json_value::json_value(json_value&& c) noexcept : container(std::move(c.container))
    {}

    // Copy operator
    NEMIR_JSON_INLINE json_value& json_value::operator=(json_value const& c)
    {
        container.reset(c.container->clone());
        return *this;
    }
    // Move operator
    NEMIR_JSON_INLINE json_value& json_value::operator=(json_value&& c) noexcept
    {
        container.swap(c.container);
        return *this;
    }

    // Json value constuctor with copy t_json_value
    template<typename T>
    NEMIR_JSON_INLINE json_value::json_value(t_json_value<T> const& v) :container(new t_json_value<T>(v))
    {}
    // Json value constuctor with move t_json_value
    template<typename T>
    NEMIR_JSON_INLINE json_value::json_value(t_json_value<T>&& v) noexcept :
        container(new t_json_value<T>(std::move(v)))
    {}


#define JSON_VALUE_CONSTRUCTOR_BY_VALUE(value_type, json_type) \
NEMIR_JSON_INLINE json_value::json_value(value_type const &v) : container(new json_value::t_json_value<json_type>(v)){}\
NEMIR_JSON_INLINE json_value::json_value(value_type &&v): container(new json_value::t_json_value<json_type>(std::move(v))){}\
NEMIR_JSON_INLINE json_value& json_value::operator=(value_type const& v)\
{\
    container.reset(new json_value::t_json_value<json_type>(v));\
    return *this;\
}\
NEMIR_JSON_INLINE json_value& json_value::operator=(value_type && v)\
{\
    container.reset(new json_value::t_json_value<json_type>(std::move(v)));\
    return *this;\
}\
NEMIR_JSON_INLINE json_value::operator const value_type & () const { return *reinterpret_cast<json_type*>(get(json_value::json_value_type<json_type>::type)); }

#define JSON_REFERENCE_OPERATOR(json_type)\
NEMIR_JSON_INLINE json_value::operator json_type& () { return *reinterpret_cast<json_type*>(get(json_value::json_value_type<json_type>::type)); }

    NEMIR_JSON_INLINE json_value::json_value(const char* v) : container(new t_json_value<json_type_string>(v)) {}
    NEMIR_JSON_INLINE json_value& json_value::operator=(const char* v)
    {
        container.reset(new t_json_value<json_type_string>(v));
        return *this;
    }

    JSON_VALUE_CONSTRUCTOR_BY_VALUE(json_value::json_type_string, json_value::json_type_string)
    JSON_VALUE_CONSTRUCTOR_BY_VALUE(int16_t, json_value::json_type_integer)
    JSON_VALUE_CONSTRUCTOR_BY_VALUE(int32_t, json_value::json_type_integer)
    JSON_VALUE_CONSTRUCTOR_BY_VALUE(json_value::json_type_integer, json_value::json_type_integer)
    JSON_VALUE_CONSTRUCTOR_BY_VALUE(json_value::json_type_number, json_value::json_type_number)
    JSON_VALUE_CONSTRUCTOR_BY_VALUE(json_value::json_type_object, json_value::json_type_object)
    JSON_VALUE_CONSTRUCTOR_BY_VALUE(json_value::json_type_array, json_value::json_type_array)
    JSON_VALUE_CONSTRUCTOR_BY_VALUE(json_value::json_type_null, json_value::json_type_null)
    JSON_VALUE_CONSTRUCTOR_BY_VALUE(json_value::json_type_boolean, json_value::json_type_boolean)

    JSON_REFERENCE_OPERATOR(json_value::json_type_string)
    JSON_REFERENCE_OPERATOR(json_value::json_type_integer)
    JSON_REFERENCE_OPERATOR(json_value::json_type_number)
    JSON_REFERENCE_OPERATOR(json_value::json_type_object)
    JSON_REFERENCE_OPERATOR(json_value::json_type_array)
    JSON_REFERENCE_OPERATOR(json_value::json_type_null)
    JSON_REFERENCE_OPERATOR(json_value::json_type_boolean)

#undef JSON_VALUE_CONSTRUCTION_BY_VALUE

    // end of json_value


    using json_value_string  = json_value::t_json_value<nemir::json_value::json_type_string >;
    using json_value_integer = json_value::t_json_value<nemir::json_value::json_type_integer>;
    using json_value_number  = json_value::t_json_value<nemir::json_value::json_type_number >;
    using json_value_array   = json_value::t_json_value<nemir::json_value::json_type_array  >;
    using json_value_object  = json_value::t_json_value<nemir::json_value::json_type_object >;
    using json_value_boolean = json_value::t_json_value<nemir::json_value::json_type_boolean>;
    using json_value_null    = json_value::t_json_value<nemir::json_value::json_type_null   >;
}

#endif//__JSON_VALUE_INCLUDED
