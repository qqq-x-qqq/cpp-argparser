#include "argparser.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace nargparse
{

struct Value
{
    int int_value;
    float float_value;
    char* string_value;
};

struct Arg
{
    char* short_name;
    char* long_name;
    char* name;
    char* description;

    ArgType type;
    NargsType nargs;

    bool is_flag;
    bool is_positional;
    bool is_set;
    bool default_flag_value;

    void* value_ptr;
    std::size_t string_size;

    IntValidator int_validator;
    FloatValidator float_validator;
    StringValidator string_validator;

    Value* values;
    int value_count;
    int value_capacity;

    Arg* next;
};

struct Parser
{
    char* program_name;
    std::size_t max_arg_len;
    bool has_help;
    Arg* first;
};

static char* copy_text(const char* s)
{
    if (s == nullptr)
    {
        char* out = new char[1];
        out[0] = '\0';
        return out;
    }

    std::size_t len = std::strlen(s);
    char* out = new char[len + 1];
    std::memcpy(out, s, len + 1);
    return out;
}

static void copy_to_buffer(char* dst, std::size_t size, const char* src)
{
    if (dst == nullptr || size == 0)
    {
        return;
    }

    if (src == nullptr)
    {
        dst[0] = '\0';
        return;
    }

    std::size_t len = std::strlen(src);
    if (len >= size)
    {
        len = size - 1;
    }

    std::memcpy(dst, src, len);
    dst[len] = '\0';
}

static void add_arg(ArgumentParser parser, Arg* arg)
{
    if (parser->first == nullptr)
    {
        parser->first = arg;
        return;
    }

    Arg* cur = parser->first;
    while (cur->next != nullptr)
    {
        cur = cur->next;
    }
    cur->next = arg;
}

static Arg* make_arg(ArgType type,
                     const char* short_name,
                     const char* long_name,
                     const char* name,
                     const char* description,
                     bool is_positional,
                     NargsType nargs)
{
    Arg* arg = new Arg;
    arg->short_name = copy_text(short_name);
    arg->long_name = copy_text(long_name);
    arg->name = copy_text(name);
    arg->description = copy_text(description);
    arg->type = type;
    arg->nargs = nargs;
    arg->is_flag = false;
    arg->is_positional = is_positional;
    arg->is_set = false;
    arg->default_flag_value = false;
    arg->value_ptr = nullptr;
    arg->string_size = kMaxArgLen;
    arg->int_validator = nullptr;
    arg->float_validator = nullptr;
    arg->string_validator = nullptr;
    arg->values = nullptr;
    arg->value_count = 0;
    arg->value_capacity = 0;
    arg->next = nullptr;
    return arg;
}

static void clear_arg_values(Arg* arg)
{
    if (arg == nullptr)
    {
        return;
    }

    if (arg->type == TYPE_STRING)
    {
        for (int i = 0; i < arg->value_count; i++)
        {
            delete[] arg->values[i].string_value;
        }
    }

    arg->value_count = 0;
}

static void reset_arg(Arg* arg)
{
    clear_arg_values(arg);
    arg->is_set = false;

    if (arg->value_ptr == nullptr)
    {
        return;
    }

    if (arg->is_flag)
    {
        *(bool*)arg->value_ptr = arg->default_flag_value;
        return;
    }

    if (arg->type == TYPE_INT)
    {
        *(int*)arg->value_ptr = 0;
        return;
    }

    if (arg->type == TYPE_FLOAT)
    {
        *(float*)arg->value_ptr = 0.0f;
        return;
    }

    if (arg->type == TYPE_STRING)
    {
        copy_to_buffer((char*)arg->value_ptr, arg->string_size, "");
    }
}

static void reset_parser(ArgumentParser parser)
{
    for (Arg* cur = parser->first; cur != nullptr; cur = cur->next)
    {
        reset_arg(cur);
    }
}

static bool grow_values(Arg* arg)
{
    if (arg->value_count < arg->value_capacity)
    {
        return true;
    }

    int new_capacity = 4;
    if (arg->value_capacity > 0)
    {
        new_capacity = arg->value_capacity * 2;
    }

    Value* new_values = new Value[new_capacity];
    for (int i = 0; i < arg->value_count; i++)
    {
        new_values[i] = arg->values[i];
        arg->values[i].string_value = nullptr;
    }

    delete[] arg->values;
    arg->values = new_values;
    arg->value_capacity = new_capacity;
    return true;
}

static bool parse_int_value(const char* text, int* value)
{
    if (text == nullptr || value == nullptr || text[0] == '\0')
    {
        return false;
    }

    char* end = nullptr;
    long x = std::strtol(text, &end, 10);
    if (*end != '\0')
    {
        return false;
    }

    *value = (int)x;
    return true;
}

static bool parse_float_value(const char* text, float* value)
{
    if (text == nullptr || value == nullptr || text[0] == '\0')
    {
        return false;
    }

    char* end = nullptr;
    float x = std::strtof(text, &end);
    if (*end != '\0')
    {
        return false;
    }

    *value = x;
    return true;
}

static bool can_take_many(Arg* arg)
{
    return arg->nargs == kNargsZeroOrMore || arg->nargs == kNargsOneOrMore;
}

static bool save_value(Arg* arg, const char* text)
{
    if (arg == nullptr || text == nullptr)
    {
        return false;
    }

    if (!can_take_many(arg) && arg->value_count > 0)
    {
        return false;
    }

    grow_values(arg);

    Value value = {};

    if (arg->type == TYPE_INT)
    {
        int x = 0;
        if (!parse_int_value(text, &x))
        {
            return false;
        }
        if (arg->int_validator != nullptr && !arg->int_validator(x))
        {
            return false;
        }
        value.int_value = x;
    }
    else if (arg->type == TYPE_FLOAT)
    {
        float x = 0.0f;
        if (!parse_float_value(text, &x))
        {
            return false;
        }
        if (arg->float_validator != nullptr && !arg->float_validator(x))
        {
            return false;
        }
        value.float_value = x;
    }
    else if (arg->type == TYPE_STRING)
    {
        if (std::strlen(text) >= arg->string_size)
        {
            return false;
        }
        if (arg->string_validator != nullptr && !arg->string_validator(text))
        {
            return false;
        }
        value.string_value = copy_text(text);
    }
    else
    {
        return false;
    }

    arg->values[arg->value_count] = value;
    arg->value_count++;
    arg->is_set = true;

    if (arg->value_ptr != nullptr && arg->value_count == 1)
    {
        if (arg->type == TYPE_INT)
        {
            *(int*)arg->value_ptr = value.int_value;
        }
        else if (arg->type == TYPE_FLOAT)
        {
            *(float*)arg->value_ptr = value.float_value;
        }
        else if (arg->type == TYPE_STRING)
        {
            copy_to_buffer((char*)arg->value_ptr, arg->string_size, value.string_value);
        }
    }

    return true;
}

static bool is_short_option(const char* text)
{
    return text != nullptr && text[0] == '-' && text[1] != '\0' && text[1] != '-';
}

static bool is_long_option(const char* text)
{
    return text != nullptr && text[0] == '-' && text[1] == '-' && text[2] != '\0';
}

static const char* cut_dashes(const char* text)
{
    if (text == nullptr)
    {
        return "";
    }

    if (text[0] == '-' && text[1] == '-')
    {
        return text + 2;
    }

    if (text[0] == '-')
    {
        return text + 1;
    }

    return text;
}

static Arg* find_named_arg(ArgumentParser parser, const char* text)
{
    if (parser == nullptr || text == nullptr)
    {
        return nullptr;
    }

    const char* equal = std::strchr(text, '=');
    const char* name = cut_dashes(text);
    int len = 0;

    if (equal != nullptr)
    {
        len = (int)(equal - name);
    }
    else
    {
        len = (int)std::strlen(name);
    }

    for (Arg* cur = parser->first; cur != nullptr; cur = cur->next)
    {
        if (cur->is_positional)
        {
            continue;
        }

        const char* short_name = cut_dashes(cur->short_name);
        const char* long_name = cut_dashes(cur->long_name);

        if (cur->short_name[0] != '\0' &&
            (int)std::strlen(short_name) == len &&
            std::strncmp(short_name, name, len) == 0)
        {
            return cur;
        }

        if (cur->long_name[0] != '\0' &&
            (int)std::strlen(long_name) == len &&
            std::strncmp(long_name, name, len) == 0)
        {
            return cur;
        }
    }

    return nullptr;
}

static Arg* find_positional_arg(ArgumentParser parser, int index)
{
    int pos = 0;
    for (Arg* cur = parser->first; cur != nullptr; cur = cur->next)
    {
        if (!cur->is_positional)
        {
            continue;
        }

        if (pos == index)
        {
            return cur;
        }
        pos++;
    }

    return nullptr;
}

static Arg* find_arg_by_name(ArgumentParser parser, const char* name)
{
    for (Arg* cur = parser->first; cur != nullptr; cur = cur->next)
    {
        if (std::strcmp(cur->name, name) == 0)
        {
            return cur;
        }
    }
    return nullptr;
}

ArgumentParser CreateParser(const char* name)
{
    return CreateParser(name, kMaxArgLen);
}

ArgumentParser CreateParser(const char* name, std::size_t max_arg_len)
{
    ArgumentParser parser = new Parser;
    parser->program_name = copy_text(name);
    parser->max_arg_len = max_arg_len == 0 ? kMaxArgLen : max_arg_len;
    parser->has_help = false;
    parser->first = nullptr;
    return parser;
}

void FreeParser(ArgumentParser parser)
{
    if (parser == nullptr)
    {
        return;
    }

    Arg* cur = parser->first;
    while (cur != nullptr)
    {
        Arg* next = cur->next;
        clear_arg_values(cur);
        delete[] cur->values;
        delete[] cur->short_name;
        delete[] cur->long_name;
        delete[] cur->name;
        delete[] cur->description;
        delete cur;
        cur = next;
    }

    delete[] parser->program_name;
    delete parser;
}

void AddHelp(ArgumentParser parser)
{
    if (parser != nullptr)
    {
        parser->has_help = true;
    }
}

void PrintHelp(ArgumentParser parser)
{
    if (parser == nullptr)
    {
        return;
    }

    std::cout << "Usage: " << parser->program_name;

    for (Arg* cur = parser->first; cur != nullptr; cur = cur->next)
    {
        if (cur->is_flag || !cur->is_positional)
        {
            std::cout << " [options]";
            break;
        }
    }

    std::cout << '\n';

    for (Arg* cur = parser->first; cur != nullptr; cur = cur->next)
    {
        if (cur->is_positional)
        {
            std::cout << "  " << cur->name;
        }
        else if (cur->short_name[0] != '\0' && cur->long_name[0] != '\0')
        {
            std::cout << "  " << cur->short_name << ", " << cur->long_name;
        }
        else if (cur->long_name[0] != '\0')
        {
            std::cout << "  " << cur->long_name;
        }
        else
        {
            std::cout << "  " << cur->short_name;
        }

        if (cur->description[0] != '\0')
        {
            std::cout << ": " << cur->description;
        }
        std::cout << '\n';
    }

    if (parser->has_help)
    {
        std::cout << "  -h, --help: Show this help message\n";
    }
}

void AddFlag(ArgumentParser parser,
             const char* short_name,
             const char* long_name,
             bool* flag,
             const char* description,
             bool default_value)
{
    if (parser == nullptr || flag == nullptr)
    {
        return;
    }

    Arg* arg = make_arg(TYPE_BOOL, short_name, long_name, description, description, false, kNargsOptional);
    arg->is_flag = true;
    arg->default_flag_value = default_value;
    arg->value_ptr = flag;
    *flag = default_value;
    add_arg(parser, arg);
}

void AddArgument(ArgumentParser parser,
                 int* value,
                 const char* name,
                 NargsType nargs,
                 IntValidator validator,
                 const char* error_msg)
{
    if (parser == nullptr || value == nullptr || name == nullptr)
    {
        return;
    }

    Arg* arg = make_arg(TYPE_INT, "", "", name, name, true, nargs);
    arg->value_ptr = value;
    arg->int_validator = validator;
    (void)error_msg;
    add_arg(parser, arg);
}

void AddArgument(ArgumentParser parser,
                 float* value,
                 const char* name,
                 NargsType nargs,
                 FloatValidator validator,
                 const char* error_msg)
{
    if (parser == nullptr || value == nullptr || name == nullptr)
    {
        return;
    }

    Arg* arg = make_arg(TYPE_FLOAT, "", "", name, name, true, nargs);
    arg->value_ptr = value;
    arg->float_validator = validator;
    (void)error_msg;
    add_arg(parser, arg);
}

void AddArgument(ArgumentParser parser,
                 char (*value)[kMaxArgLen],
                 const char* name,
                 NargsType nargs,
                 StringValidator validator,
                 const char* error_msg)
{
    if (parser == nullptr || value == nullptr || name == nullptr)
    {
        return;
    }

    Arg* arg = make_arg(TYPE_STRING, "", "", name, name, true, nargs);
    arg->value_ptr = *value;
    arg->string_size = parser->max_arg_len;
    arg->string_validator = validator;
    (void)error_msg;
    add_arg(parser, arg);
}

void AddArgument(ArgumentParser parser,
                 const char* short_name,
                 const char* long_name,
                 int* value,
                 const char* name,
                 NargsType nargs,
                 IntValidator validator,
                 const char* error_msg)
{
    if (parser == nullptr || long_name == nullptr || value == nullptr || name == nullptr)
    {
        return;
    }

    Arg* arg = make_arg(TYPE_INT, short_name, long_name, name, name, false, nargs);
    arg->value_ptr = value;
    arg->int_validator = validator;
    (void)error_msg;
    add_arg(parser, arg);
}

void AddArgument(ArgumentParser parser,
                 const char* short_name,
                 const char* long_name,
                 float* value,
                 const char* name,
                 NargsType nargs,
                 FloatValidator validator,
                 const char* error_msg)
{
    if (parser == nullptr || long_name == nullptr || value == nullptr || name == nullptr)
    {
        return;
    }

    Arg* arg = make_arg(TYPE_FLOAT, short_name, long_name, name, name, false, nargs);
    arg->value_ptr = value;
    arg->float_validator = validator;
    (void)error_msg;
    add_arg(parser, arg);
}

void AddArgument(ArgumentParser parser,
                 const char* short_name,
                 const char* long_name,
                 char (*value)[kMaxArgLen],
                 const char* name,
                 NargsType nargs,
                 StringValidator validator,
                 const char* error_msg)
{
    if (parser == nullptr || long_name == nullptr || value == nullptr || name == nullptr)
    {
        return;
    }

    Arg* arg = make_arg(TYPE_STRING, short_name, long_name, name, name, false, nargs);
    arg->value_ptr = *value;
    arg->string_size = parser->max_arg_len;
    arg->string_validator = validator;
    (void)error_msg;
    add_arg(parser, arg);
}

bool Parse(ArgumentParser parser, int argc, const char* argv[])
{
    if (parser == nullptr || argv == nullptr || argc < 1)
    {
        return false;
    }

    reset_parser(parser);

    for (int i = 1; i < argc; i++)
    {
        if ((std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) && parser->has_help)
        {
            PrintHelp(parser);
            return true;
        }
    }

    int positional_index = 0;

    for (int i = 1; i < argc; i++)
    {
        const char* token = argv[i];

        Arg* cur = nullptr;
        if (is_short_option(token) || is_long_option(token))
        {
            cur = find_named_arg(parser, token);
        }

        if (cur != nullptr)
        {
            if (cur->is_flag)
            {
                *(bool*)cur->value_ptr = true;
                cur->is_set = true;
                continue;
            }

            const char* equal = std::strchr(token, '=');
            const char* value_text = nullptr;

            if (equal != nullptr)
            {
                value_text = equal + 1;
            }
            else
            {
                if (i + 1 >= argc)
                {
                    return false;
                }
                i++;
                value_text = argv[i];
            }

            if (!save_value(cur, value_text))
            {
                return false;
            }
            continue;
        }

        Arg* pos = find_positional_arg(parser, positional_index);
        if (pos == nullptr)
        {
            return false;
        }

        if (!save_value(pos, token))
        {
            return false;
        }

        if (!can_take_many(pos))
        {
            positional_index++;
        }
    }

    for (Arg* cur = parser->first; cur != nullptr; cur = cur->next)
    {
        if (cur->is_flag)
        {
            continue;
        }

        if (cur->nargs == kNargsRequired && cur->value_count != 1)
        {
            return false;
        }

        if (cur->nargs == kNargsOneOrMore && cur->value_count == 0)
        {
            return false;
        }
    }

    return true;
}

bool Parse(ArgumentParser parser, int argc, char* argv[])
{
    return Parse(parser, argc, const_cast<const char**>(argv));
}

int GetRepeatedCount(ArgumentParser parser, const char* name)
{
    Arg* arg = find_arg_by_name(parser, name);
    if (arg == nullptr)
    {
        return 0;
    }
    return arg->value_count;
}

bool GetRepeated(ArgumentParser parser, const char* name, int index, int* value)
{
    Arg* arg = find_arg_by_name(parser, name);
    if (arg == nullptr || arg->type != TYPE_INT || value == nullptr)
    {
        return false;
    }
    if (index < 0 || index >= arg->value_count)
    {
        return false;
    }

    *value = arg->values[index].int_value;
    return true;
}

bool GetRepeated(ArgumentParser parser, const char* name, int index, float* value)
{
    Arg* arg = find_arg_by_name(parser, name);
    if (arg == nullptr || arg->type != TYPE_FLOAT || value == nullptr)
    {
        return false;
    }
    if (index < 0 || index >= arg->value_count)
    {
        return false;
    }

    *value = arg->values[index].float_value;
    return true;
}

bool GetRepeated(ArgumentParser parser, const char* name, int index, const char** value)
{
    Arg* arg = find_arg_by_name(parser, name);
    if (arg == nullptr || arg->type != TYPE_STRING || value == nullptr)
    {
        return false;
    }
    if (index < 0 || index >= arg->value_count)
    {
        return false;
    }

    *value = arg->values[index].string_value;
    return true;
}

}
